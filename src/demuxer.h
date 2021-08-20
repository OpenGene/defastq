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

#ifndef DEMUXER_H
#define DEMUXER_H

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include "options.h"
#include "simpleread.h"
#include <map>

using namespace std;

const int HASH_COLLISION = -2;

class Demuxer{
public:
    Demuxer(Options* opt);
    ~Demuxer();
    int demux(SimpleRead* r);
    int demux(SimpleRead* r1, SimpleRead* r2);
    static bool test();

private:
    void init();
    inline long kmer2key(const char* data, size_t len);
    inline long kmer2keyTwoParts(const char* data1, size_t len1, const char* data2, size_t len2) ;
    inline long kmer2key(string& str);
    inline long base2val(char base);
    inline int hash(long key);
    inline void addKeyToHashTable(long key, int pos);
private:
    Options* mOptions;
    int* mHashTable;
    map<long, int> mIndexSample;
    unsigned int mHashTableLen;
};

#endif