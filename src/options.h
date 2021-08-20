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

#ifndef OPTIONS_H
#define OPTIONS_H

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>
#include <mutex>
#include "common.h"

using namespace std;

class Sample{
public:
    string index1;
    string index2;
    string file;
};

class Options{
public:
    Options();
    bool validate();
    void adjustWriterBufferSize();
    void log(const string& msg);

public:
    // file name of read1 input
    string in1;
    // file name of read1 input
    string in2;
    // compression level
    int compression;
    // sample sheet CSV file
    string samplesheet;
    // output folder
    string outFolder;
    // undetermined
    string undecodedFileName;
    // suffix of filename
    string suffix1;
    // sample sheet
    vector<Sample> samples;
    // the number of threads, 0 means auto: min(output_file_num, 128)
    int threadNum;
    // is paired-end mode?
    bool pairedEnd;
    // is MGI mode?
    bool mgiMode;
    // allowed mismatch
    int mismatch;
    // the place of barcode
    int barcodePlace;
    // the starting pos of barcode if barcode place is read1/read2
    int barcodeStart;
    // the barcode length of barcode if barcode place is read1/read2
    int barcodeLength;
    // the buffer size for writer
    size_t writerBufferSize;
    // limit of memory
    long memoryLimitBytes;
    // read buffer limit in bytes
    long readBufferLimitBytes;
    // unbalanced read1/read2 number gap limit for paired-end reading
    long peReadNumGapLimit;
    // evalauted read length of read1
    int seqLen1;
    // evalauted read length of read2
    int seqLen2;
    // evalauted read data length of read1
    int seqDataLen1;
    // evalauted read data length of read2
    int seqDataLen2;
    // index reverse complement?
    bool indexReverseComplement;
    // output debug info
    bool debug;
    // mutex for logging
    mutex logmtx;
    // discard the undecoded reads?
    bool discardUndecoded;

private:
    void parseSampleSheet();
    void parseSampleSheetFASTA();

};

#endif