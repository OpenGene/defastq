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

#include "evaluator.h"
#include "fastqreader.h"
#include <memory.h>

Evaluator::Evaluator(Options* opt){
    mOptions = opt;
}


Evaluator::~Evaluator(){
}

void Evaluator::evaluateSeqLen() {
    if(!mOptions->in1.empty()) {
        mOptions->seqLen1 = computeSeqLen(mOptions->in1, mOptions->seqDataLen1);
        //cerr << mOptions->seqLen1 << ": " << mOptions->seqDataLen1 << endl;
    }
    if(!mOptions->in2.empty()) {
        mOptions->seqLen2 = computeSeqLen(mOptions->in2, mOptions->seqDataLen2);
        //cerr << mOptions->seqLen2 << ": " << mOptions->seqDataLen2 << endl;
    }
}

int Evaluator::computeSeqLen(string filename, int& dataLen) {
    FastqReader reader(filename);

    long records = 0;
    bool reachedEOF = false;

    // get seqlen
    long seqlenTotal=0;
    long datalenTotal=0;
    while(records < 100000) {
        SimpleRead* r = reader.read();
        if(!r) {
            reachedEOF = true;
            break;
        }
        int rlen = r->seqLen();
        int dlen = r->dataLen();
        seqlenTotal += rlen;
        datalenTotal += dlen;
        records ++;
        delete r;
    }

    if(records>0) {
        dataLen = datalenTotal / records;
        return seqlenTotal / records;
    } else {
        dataLen = 0;
        return 0;
    }
}
