/*
MIT License

Copyright (c) 2021 Shifu Chen <chen@haplox.com>

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "seprocessor.h"
#include "fastqreader.h"
#include <iostream>
#include <unistd.h>
#include <functional>
#include <thread>
#include <memory.h>
#include "util.h"
#include "memfunc.h"

extern atomic_long globalReadBytesInMem;

SingleEndProcessor::SingleEndProcessor(Options* opt){
    mOptions = opt;
    mProduceFinished = false;
    mDemuxer = new Demuxer(opt);
    mSampleSize = mOptions->samples.size();
    mWriterThreadNum = 0;
}

SingleEndProcessor::~SingleEndProcessor() {
    delete mDemuxer;
}

bool SingleEndProcessor::process(){

    mInputList = new SingleProducerSingleConsumerList<SimpleRead*>();

    std::thread producer(std::bind(&SingleEndProcessor::readerTask, this));

    // plus one undetermined
    mOutputNum = mSampleSize;
    if(!mOptions->discardUndecoded)
        mOutputNum += 1;
    mWriterThreadNum = mOptions->threadNum - 3;
    if(mWriterThreadNum <= 0)
        mWriterThreadNum = min(128, mOutputNum);

    if(mWriterThreadNum > mOutputNum)
        mWriterThreadNum = mOutputNum;

    mOptions->log("raise " + to_string(mWriterThreadNum) + " writer threads");

    mConfigs = new ThreadConfig*[mWriterThreadNum];
    for(int t=0; t<mWriterThreadNum; t++){
        mConfigs[t] = new ThreadConfig(mOptions, t);
    }

    mOutputLists = new SingleProducerSingleConsumerList<SimpleRead*>*[mOutputNum];
    for(int i=0; i<mOutputNum; i++){
        mOutputLists[i] = new SingleProducerSingleConsumerList<SimpleRead*>();
        // assign the write task to a writer thread
        int t = i % mWriterThreadNum;
        if(i < mSampleSize)
            mConfigs[t] -> addTask(mOptions->samples[i].file, mOutputLists[i], false, false);
        else
            mConfigs[t] -> addTask(mOptions->undecodedFileName, mOutputLists[i], false, true);
    }

    std::thread** writerThreads = new thread*[mWriterThreadNum];
    for(int t=0; t<mWriterThreadNum; t++){
        writerThreads[t] = new std::thread(std::bind(&SingleEndProcessor::writerTask, this, mConfigs[t]));
    }

    std::thread demuxer(std::bind(&SingleEndProcessor::demuxerTask, this));

    producer.join();
    demuxer.join();
    for(int t=0; t<mWriterThreadNum; t++){
        writerThreads[t]->join();
    }

    // clean up
    for(int t=0; t<mWriterThreadNum; t++){
        delete writerThreads[t];
        writerThreads[t] = NULL;
        delete mConfigs[t];
        mConfigs[t] = NULL;
    }

    // clean up lists
    for(int i=0; i<mOutputNum; i++){
        delete mOutputLists[i];
        mOutputLists[i] = NULL;
    }

    delete[] writerThreads;
    delete mConfigs;
    delete mInputList;

    return true;
}

bool SingleEndProcessor::processSingleEnd(SimpleRead* r){
    int sample = mDemuxer->demux(r);
    // Undetermined
    if(sample < 0) {
        if(!mOptions->discardUndecoded)
            sample = mSampleSize;
        else {
            delete r;
            r = NULL;
            return true;
        }
    }
    mOutputLists[sample]->produce(r);
    return true;
}


void SingleEndProcessor::readerTask()
{
    long sleepTime = 0;
    int slept = 0;
    long readNum = 0;
    int sleepTimeMemExceeded = 0;
    FastqReader reader(mOptions->in1);
    int count=0;
    while(true){
        SimpleRead* read = reader.read();
        if(!read){
            break;
        } else {
            mInputList->produce(read);
        }
        count++;
        if((count & 0xFF) == 0xFF) {
            //for every 128 reads, if memory usage exceeded, or in memory queue too large, sleep
            if(globalReadBytesInMem > mOptions->readBufferLimitBytes) {
                sleepTimeMemExceeded++;
                mOptions->log(to_string(sleepTimeMemExceeded) + " time reader sleeps due to globalReadBytesInMem: "+  to_string(globalReadBytesInMem ));
                sleep(1);
                sleepTime++;
            }
        }
    }
    mInputList->setProducerFinished();
    mOptions->log("reader thread exited with sleep time: " + to_string(sleepTime));
}

void SingleEndProcessor::demuxerTask()
{
    long sleepTime = 0;
    while(true) {
        while(mInputList->canBeConsumed()) {
            SimpleRead* r = mInputList->consume();
            processSingleEnd(r);
        }
        if(mInputList->isProducerFinished()) {
            if(!mInputList->canBeConsumed())
                break;
        } else {
            usleep(1);
            sleepTime++;
        }
    }
    mInputList->setConsumerFinished();
    for(int i=0; i<mOutputNum; i++)
        mOutputLists[i]->setProducerFinished();
    mOptions->log("demuxer thread exited with sleep time: " + to_string(sleepTime));
}

void SingleEndProcessor::writerTask(ThreadConfig* config)
{
    while(true) {
        if(config->isInputCompleted()){
            // check one more time to prevent possible loss of data
            config->output();
            break;
        }
        config->output();
    }
    config->setOutputCompleted();
}