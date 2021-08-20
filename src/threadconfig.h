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

#ifndef THREAD_CONFIG_H
#define THREAD_CONFIG_H

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include "writer.h"
#include "options.h"
#include "simpleread.h"
#include <atomic>
#include "singleproducersingleconsumerlist.h"

using namespace std;

class ThreadConfig{
public:
    ThreadConfig(Options* opt,  int threadId);
    ~ThreadConfig();

    void addTask(string filename, SingleProducerSingleConsumerList<SimpleRead*>* list, bool isRead2, bool isUndetermined);

    int getThreadId() {return mThreadId;}
    void cleanup();

    bool isInputCompleted();
    void output();
    void setOutputCompleted();

private:
    void deleteWriter();

private:
    Writer* mWriter1;
    Options* mOptions;

    // for spliting output
    int mThreadId;
    bool mInputCompleted;
    vector<SingleProducerSingleConsumerList<SimpleRead*>*> mDataLists;
    vector<Writer*> mWriters;
    vector<string> mFilenames;
    long mSleepTime;

};

#endif