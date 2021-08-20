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

#ifndef SIMPLEREAD_H
#define SIMPLEREAD_H

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <iostream>
#include <fstream>
#include <atomic>

using namespace std;

class SimpleRead{
public:
    SimpleRead(char* data, unsigned int dataSize);
    ~SimpleRead();
    char* data();
    unsigned int dataLen();
    unsigned int nameLen();
    unsigned int seqLen();
    unsigned int seqStart();
    unsigned int qualLen();
    unsigned int qualStart();
    void setData(char* d, unsigned int len);
    void setNameLen(unsigned int len);
    void setSeq(unsigned int start, unsigned int len);
    void setQual(unsigned int start, unsigned int len);
    bool getIlluminaIndex1Place(unsigned int &start, unsigned int &len);
    bool getIlluminaIndex2Place(unsigned int &start, unsigned int &len);
    bool getIlluminaBothIndexPlaces(unsigned int &start1, unsigned int &len1, unsigned int &start2, unsigned int &len2);

public:
    static bool test();
    void print();
    static void initCounter();

private:


private:
	char* mData;
    unsigned int mDataLen;
    unsigned int mNameLen;
    unsigned int mSeqLen;
    unsigned int mSeqStart;
    unsigned int mQualLen;
    unsigned int mQualStart;
};

#endif