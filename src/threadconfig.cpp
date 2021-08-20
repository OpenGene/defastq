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

#include "threadconfig.h"
#include "util.h"
#include <memory.h>
#include <unistd.h>

ThreadConfig::ThreadConfig(Options* opt, int threadId){
    mOptions = opt;
    mThreadId = threadId;
    mInputCompleted = false;
    mSleepTime = 0;
}

ThreadConfig::~ThreadConfig() {
    cleanup();
    mOptions->log("writer thread " + to_string(mThreadId) + " destroyed with sleep time: " + to_string(mSleepTime));
}

void ThreadConfig::addTask(string filename, SingleProducerSingleConsumerList<SimpleRead*>* datalist, bool isRead2, bool isUndetermined) {
    mDataLists.push_back(datalist);
    string fullpath = joinpath(mOptions->outFolder, filename)+".fastq";
    if(mOptions->compression > 0)
        fullpath += ".gz";
    mFilenames.push_back(fullpath);
    Writer* writer = new Writer(mOptions, fullpath, mOptions->compression, isRead2, isUndetermined);
    mWriters.push_back(writer);
}

bool ThreadConfig::isInputCompleted() 
{
    return mInputCompleted;
}

void ThreadConfig::output(){
    bool hasData = false;
    bool completed = true;
    for(int i=0; i<mDataLists.size(); i++)
    {
        while(mDataLists[i]->canBeConsumed()) {
            SimpleRead* r = mDataLists[i]->consume();
            mWriters[i]->writeRead(r);
            delete r;
            hasData = true;
        }
        if(completed) {
            if(!mDataLists[i]->isProducerFinished() || mDataLists[i]->canBeConsumed())
                completed = false;
        }
    }
    if(!hasData) {
        usleep(1000);
        mSleepTime++;
        if(mSleepTime%1000 == 0) {
            mOptions->log("writer thread " + to_string(mThreadId) + " has slept for " + to_string(mSleepTime) + " times");
        }
    }
    mInputCompleted = completed;
}

void ThreadConfig::cleanup() {
    deleteWriter();
}

void ThreadConfig::setOutputCompleted() {
    for(int i=0; i<mDataLists.size(); i++)
        mDataLists[i]->setConsumerFinished();
}

void ThreadConfig::deleteWriter() {
    for(int i=0; i<mWriters.size(); i++) {
        delete mWriters[i];
        mWriters[i] = NULL;
    }
}
