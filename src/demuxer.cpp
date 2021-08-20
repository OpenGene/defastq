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

#include "demuxer.h"
#include "util.h"
#include "memfunc.h"

Demuxer::Demuxer(Options* opt){
    mOptions = opt;
    // M table
    mHashTableLen = 1<<26;
    mHashTable = (int*)tmalloc(mHashTableLen*sizeof(int));
    memset(mHashTable, -1, mHashTableLen*sizeof(int));
    init();
}

Demuxer::~Demuxer() {
    if(mHashTable) {
        tfree(mHashTable);
        mHashTable = NULL;
    }
}

void Demuxer::init() {
    if(mOptions == NULL)
        return;

    const char bases[4] = {'A', 'T', 'C', 'G'};

    bool sameLength = true;
    for(int i=0; i<mOptions->samples.size(); i++) {
        Sample s = mOptions->samples[i];
        string barcode = s.index1;

        if(barcode.empty()) {
            error_exit("You should specify barcode for each record");
        }

        if(barcode.length()>30) {
            error_exit("Barcode length should be <= 30bp ");
        }

        long key = kmer2key(barcode);
        if(key < 0)
            error_exit("Barcode can only contain A/T/C/G: " + barcode);

        mIndexSample[key] = i;

        addKeyToHashTable(key, i);

        if(mOptions->mismatch == 1) {
            for(int p=0; p<barcode.length(); p++) {
                char origin = barcode[p];
                for(int b=0; b<4; b++) {
                    if(bases[b] == origin)
                        continue;
                    string mutant = barcode;
                    mutant[p] = bases[b];
                    long key = kmer2key(mutant);
                    mIndexSample[key] = i;
                    addKeyToHashTable(key, i);
                }
            }
        }

        if(mOptions->mismatch == 2) {
            for(int p=0; p<barcode.length(); p++) {
                char originP = barcode[p];
                for(int q=0; q<barcode.length(); q++) {
                    if(p==q)
                        continue;
                    char originQ = barcode[q];
                    for(int b1=0; b1<4; b1++) {
                        if(bases[b1] == originP)
                            continue;
                        for(int b2=0; b2<4; b2++) {
                            if(bases[b2] == originQ)
                                continue;
                            string mutant = barcode;
                            mutant[p] = bases[b1];
                            mutant[q] = bases[b2];
                            long key = kmer2key(mutant);
                            addKeyToHashTable(key, i);
                            mIndexSample[key] = i;
                        }
                    }
                }
            }
        }
    }
}

void Demuxer::addKeyToHashTable(long key, int pos) {
    int hashval = hash(key);
    if(mHashTable[hashval] == -1)
        mHashTable[hashval] = pos;
    else
        mHashTable[hashval] = HASH_COLLISION;
}

int Demuxer::hash(long key) {
    return abs(key * 1713137323L) & (mHashTableLen-1);
}

int Demuxer::demux(SimpleRead* r) {
    long key = 0;
    if(mOptions->barcodePlace == BARCODE_AT_READ1 || mOptions->barcodePlace == BARCODE_AT_READ2) {
        if(mOptions->barcodeStart -1 + mOptions->barcodeLength > r->seqLen())
            return -1;
        key = kmer2key(r->data() + r->seqStart() + mOptions->barcodeStart, mOptions->barcodeLength);
    } else if(mOptions->barcodePlace == BARCODE_AT_INDEX1) {
        unsigned int s1, l1;
        bool hasIndex1 = r->getIlluminaIndex1Place(s1, l1);
        if(!hasIndex1)
            error_exit("Read doesn't have INDEX 1, please confirm that it is Illumina data.");
        key = kmer2key(r->data() + s1, l1);
    } else if(mOptions->barcodePlace == BARCODE_AT_INDEX2) {
        unsigned int s2, l2;
        bool hasIndex2 = r->getIlluminaIndex2Place(s2, l2);
        if(!hasIndex2)
            error_exit("Read doesn't have INDEX 2, please confirm that it is Illumina data.");
        key = kmer2key(r->data() + s2, l2);
    } else if(mOptions->barcodePlace == BARCODE_AT_BOTH_INDEX) {
        unsigned int s1, l1, s2, l2;
        bool hasTwoIndexes = r->getIlluminaBothIndexPlaces(s1, l1, s2, l2);
        if(!hasTwoIndexes)
            error_exit("Read doesn't have dual indexes, please confirm that it is paired-end Illumina data.");
        key = kmer2keyTwoParts(r->data() + s1, l1, r->data() + s2, l2);
    } else 
        return -1;

    if(key < 0)
        return -1;
    int hashval = hash(key);
    if(mHashTable[hashval]>=0)
        return mHashTable[hashval];
    else if(mHashTable[hashval] == HASH_COLLISION)
        return mIndexSample[key];
    else
        return -1;
}

int Demuxer::demux(SimpleRead* r1, SimpleRead* r2) {
    if(mOptions->barcodePlace == BARCODE_AT_READ2)
        return demux(r2);
    else 
        return demux(r1);
}

long Demuxer::kmer2key(string& str) {
    return kmer2key(str.c_str(), str.length());
}

long Demuxer::kmer2key(const char* data, size_t len) {
    if(len > 30)
        error_exit("Index length cannot be longer than 30");
    long kmer = 0;
    for(int k=0; k<len; k++) {
        int val = base2val(data[k]);
        if(val < 0) {
            return -1;
        }
        kmer = (kmer<<2) | val;
    }
    return kmer;
}

long Demuxer::kmer2keyTwoParts(const char* data1, size_t len1, const char* data2, size_t len2) {
    if(len1 + len2 > 30)
        error_exit("Index length cannot be longer than 30");
    long kmer = 0;
    for(int k=0; k<len1; k++) {
        int val = base2val(data1[k]);
        if(val < 0) {
            return -1;
        }
        kmer = (kmer<<2) | val;
    }
    for(int k=0; k<len2; k++) {
        int val = base2val(data2[k]);
        if(val < 0) {
            return -1;
        }
        kmer = (kmer<<2) | val;
    }
    return kmer;
}

long Demuxer::base2val(char base) {
    switch(base){
        case 'A':
            return 0;
        case 'T':
            return 1;
        case 'C':
            return 2;
        case 'G':
            return 3;
        default:
            return -1;
    }
}

bool Demuxer::test(){
    Demuxer d(NULL);
    string s1("AGTCAGAA");
    string s2("ATTCAGAA");
    cout << d.kmer2key(s1) << endl;
    cout << d.kmer2key(s2) << endl;

    return true;
}