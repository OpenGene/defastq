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

#ifndef PE_PROCESSOR_H
#define PE_PROCESSOR_H

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "read.h"
#include <cstdlib>
#include <condition_variable>
#include <mutex>
#include <thread>
#include "options.h"
#include "threadconfig.h"
#include "demuxer.h"
#include "singleproducersingleconsumerlist.h"

using namespace std;


class PairedEndProcessor{
public:
    PairedEndProcessor(Options* opt);
    ~PairedEndProcessor();
    bool process();

private:
    bool processPairedEnd(SimpleRead* r1, SimpleRead* r2);
    void reader1Task();
    void reader2Task();
    void demuxerTask();
    void writerTask(ThreadConfig* config);

private:
    Options* mOptions;
    bool mProduceFinished;
    ThreadConfig** mConfigs;
    SingleProducerSingleConsumerList<SimpleRead*>* mRead1InputList;
    SingleProducerSingleConsumerList<SimpleRead*>* mRead2InputList;
    SingleProducerSingleConsumerList<SimpleRead*>** mOutputLists;
    Demuxer* mDemuxer;
    int mSampleSize;
    int mWriterThreadNum;
    atomic_long mRead1Loaded;
    atomic_long mRead2Loaded;
    int mOutputNum;
};


#endif