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

#include "peprocessor.h"
#include "fastqreader.h"
#include <iostream>
#include <unistd.h>
#include <functional>
#include <thread>
#include <memory.h>
#include "util.h"
#include "memfunc.h"

extern atomic_long globalReadBytesInMem;

PairedEndProcessor::PairedEndProcessor(Options* opt){
    mOptions = opt;
    mProduceFinished = false;
    mDemuxer = new Demuxer(opt);
    mSampleSize = mOptions->samples.size();
    mRead1Loaded = 0;
    mRead2Loaded = 0;
}

PairedEndProcessor::~PairedEndProcessor() {
    delete mDemuxer;
}

bool PairedEndProcessor::process(){

    mRead1InputList = new SingleProducerSingleConsumerList<SimpleRead*>();
    mRead2InputList = new SingleProducerSingleConsumerList<SimpleRead*>();

    // plus two undetermined (R1 and R2)
    mOutputNum = mSampleSize*2;
    if(!mOptions->discardUndecoded)
        mOutputNum += 2;
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
        string suffix;
        bool isRead2 = false;
        if(i%2!=0)
            isRead2 = true;

        if(!isRead2)
            suffix = ".R1";
        else
            suffix = ".R2";
        if(i < mSampleSize*2)
            mConfigs[t] -> addTask(mOptions->samples[i/2].file + suffix, mOutputLists[i], isRead2, false);
        else
            mConfigs[t] -> addTask(mOptions->undecodedFileName + suffix, mOutputLists[i], isRead2, true);
    }


    std::thread reader1(std::bind(&PairedEndProcessor::reader1Task, this));
    std::thread reader2(std::bind(&PairedEndProcessor::reader2Task, this));

    std::thread** writerThreads = new thread*[mWriterThreadNum];
    for(int t=0; t<mWriterThreadNum; t++){
        writerThreads[t] = new std::thread(std::bind(&PairedEndProcessor::writerTask, this, mConfigs[t]));
    }

    std::thread demuxer(std::bind(&PairedEndProcessor::demuxerTask, this));

    reader1.join();
    reader2.join();
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
    delete mRead1InputList;
    delete mRead2InputList;

    return true;
}

bool PairedEndProcessor::processPairedEnd(SimpleRead* r1, SimpleRead* r2){
    int sample = mDemuxer->demux(r1, r2);
    // Undetermined
    if(sample < 0) {
        if(!mOptions->discardUndecoded)
            sample = mSampleSize;
        else {
            delete r1;
            r1 = NULL;
            delete r2;
            r2 = NULL;
            return true;
        }
    }
    mOutputLists[(sample*2)]->produce(r1);
    mOutputLists[(sample*2+1)]->produce(r2);
    return true;
}


void PairedEndProcessor::reader1Task()
{
    long readNum = 0;
    FastqReader reader(mOptions->in1);
    long count=0;
    long sleepTimeMemExceeded = 0;
    long sleepTimeUnbalanced = 0;
    while(true){
        SimpleRead* read = reader.read();
        if(!read){
            break;
        } else {
            mRead1InputList->produce(read);
            mRead1Loaded++;
        }
        count++;
        if((count & 0xFF) == 0xFF) {
            //for every 128 reads, if in memory queue too large, sleep 1s
            if(globalReadBytesInMem > mOptions->readBufferLimitBytes) {
                sleepTimeMemExceeded++;
                mOptions->log(to_string(sleepTimeMemExceeded) + " time reader1 sleeps due to globalReadBytesInMem: "+  to_string(globalReadBytesInMem ));
                sleep(1);
            }
            //unbalanced reading for PE, sleep
            if(mRead1Loaded - mRead2Loaded > mOptions->peReadNumGapLimit) {
                sleepTimeUnbalanced++;
                mOptions->log(to_string(sleepTimeUnbalanced) + " time reader1 sleeps since it loads too fast");
                usleep(100000);
            }
        }
    }
    mRead1InputList->setProducerFinished();
    mOptions->log("reader1 thread exited with sleep time: " + to_string(sleepTimeMemExceeded + sleepTimeUnbalanced));
}

void PairedEndProcessor::reader2Task()
{
    long readNum = 0;
    FastqReader reader(mOptions->in2);
    long count=0;
    long sleepTimeMemExceeded = 0;
    long sleepTimeUnbalanced = 0;
    while(true){
        SimpleRead* read = reader.read();
        if(!read){
            break;
        } else {
            mRead2InputList->produce(read);
            mRead2Loaded++;
        }
        count++;
        if((count & 0xFF) == 0xFF) {
            //for every 128 reads, if memory usage exceeded, or in memory queue too large, sleep
            if(globalReadBytesInMem > mOptions->readBufferLimitBytes) {
                sleepTimeMemExceeded++;
                mOptions->log(to_string(sleepTimeMemExceeded) + " time reader2 sleeps due to globalReadBytesInMem: "+  to_string(globalReadBytesInMem ));
                sleep(1);
            }
            //unbalanced reading for PE, sleep
            if(mRead2Loaded - mRead1Loaded > mOptions->peReadNumGapLimit) {
                sleepTimeUnbalanced++;
                mOptions->log(to_string(sleepTimeUnbalanced) + " time reader2 sleeps since it loads too fast");
                usleep(100000);
            }
        }
    }
    mRead2InputList->setProducerFinished();
    mOptions->log("reader2 thread exited with sleep time: " + to_string(sleepTimeMemExceeded + sleepTimeUnbalanced));
}

void PairedEndProcessor::demuxerTask()
{
    long sleepTime=0;
    while(true) {
        while(mRead1InputList->canBeConsumed() && mRead2InputList->canBeConsumed()) {
            SimpleRead* r1 = mRead1InputList->consume();
            SimpleRead* r2 = mRead2InputList->consume();
            processPairedEnd(r1, r2);
        }
        if(mRead1InputList->isProducerFinished() && !mRead1InputList->canBeConsumed()) {
            break;
        } else if(mRead2InputList->isProducerFinished() && !mRead2InputList->canBeConsumed()) {
            break;
        } else {
            usleep(1);
            sleepTime++;
        }
    }
    mRead1InputList->setConsumerFinished();
    mRead2InputList->setConsumerFinished();
    for(int i=0; i<mOutputNum; i++)
        mOutputLists[i]->setProducerFinished();
    mOptions->log("demuxer thread exited with sleep time: " + to_string(sleepTime));
}

void PairedEndProcessor::writerTask(ThreadConfig* config)
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