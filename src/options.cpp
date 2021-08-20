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

#include "options.h"
#include "util.h"
#include <iostream>
#include <fstream>
#include <sys/stat.h>
#include <string.h>
#include <thread>
#include "sequence.h"
#include "fastareader.h"

Options::Options(){
    in1 = "";
    compression = 6;
    undecodedFileName = "undecoded";
    threadNum = 0;
    pairedEnd = false;
    mgiMode = false;
    mismatch = 0;
    barcodePlace = BARCODE_PLACE_UNKNOWN;
    barcodeStart = -1;
    barcodeLength = 0;
    writerBufferSize = 0x01L<<20; // 1M writer buffer for per output by default
    memoryLimitBytes = 0;
    readBufferLimitBytes = 0x01L<<33; // 8G read buffer limit by default
    peReadNumGapLimit = 0x01L<<23; // 8M reads for default read1/read2 gap limit
    seqLen1 = 0;
    seqLen2 = 0;
    seqDataLen1 = 0;
    seqDataLen2 = 0;
    indexReverseComplement = false;
    debug = false;
    discardUndecoded = false;
}

void Options::log(const string& msg) {
    if(debug) {
        logmtx.lock();
        time_t tt = time(NULL);
        tm* t= localtime(&tt);
        fprintf(stderr, "[%02d:%02d:%02d] ", t->tm_hour, t->tm_min, t->tm_sec);
        cerr << msg << endl;
        logmtx.unlock();
    }
}

void Options::adjustWriterBufferSize() {
    // has user set memory limit?
    if(memoryLimitBytes > 0) {
        // set 1/4 memoryLimitBytes to total buffer
        long writerBufferPool = memoryLimitBytes/4;
        long bufSize = writerBufferPool/(samples.size()+1);
        if(pairedEnd)
            bufSize /= 2;
        // align to 128 bytes
        bufSize = (bufSize/128)*128;
        if(bufSize < 8192) // 8K for MIN
            bufSize = 8192;
        if(bufSize > (0x01L<<22)) // 4M for MAX
            bufSize = (0x01L<<22);

        writerBufferSize = bufSize;

        // set 1/2 memoryLimitBytes to total buffer
        readBufferLimitBytes = memoryLimitBytes/2;
        if( readBufferLimitBytes > (0x01L<<36))
            readBufferLimitBytes = (0x01L<<36); // 64G for MAX
        if( readBufferLimitBytes < (0x01L<<30))
            readBufferLimitBytes = (0x01L<<30); // 1G for MIN
    }

    // set 1/8 readBufferLimitBytes as pe reading gap limit
    if(seqDataLen1 > 0) {
        int avgDataLen = seqDataLen1;
        if(seqDataLen2 > 0)
            avgDataLen = (seqDataLen2 + seqDataLen1) / 2;
        peReadNumGapLimit = readBufferLimitBytes/8;
        peReadNumGapLimit /= avgDataLen;
    }

    log("readBufferLimitBytes: " + to_string(readBufferLimitBytes));
    log("peReadNumGapLimit: " + to_string(peReadNumGapLimit));
    log("writerBufferSize: " + to_string(writerBufferSize));
}

void Options::parseSampleSheetFASTA() {
    log("FASTA format");
    FastaReader reader(samplesheet);
    reader.readAll();

    map<string, string> barcodes = reader.mAllContigs;
    map<string, string>::iterator iter;
    for(iter = barcodes.begin(); iter!=barcodes.end(); iter++) {
        string name = iter->first;
        string barcode = iter->second;

        if(indexReverseComplement){
            Sequence seq(barcode);
            barcode = seq.reverseComplement().mStr;
        }

        Sample s;
        s.file = trim(name);
        s.index1 = trim(barcode);

        log(s.file + ": " + s.index1);

        this->samples.push_back(s);
    }

    adjustWriterBufferSize();

}

void Options::parseSampleSheet() {
    if(samplesheet.empty()) {
        error_exit("sample sheet CSV file should be specified by -s or --sample_sheet");
    } else {
        check_file_valid(samplesheet);
    }

    log("parsing index file: " + samplesheet);

    if(ends_with(samplesheet, ".fasta") or ends_with(samplesheet, ".fa")) {
        return parseSampleSheetFASTA();
    }

    ifstream file;
    file.open(samplesheet.c_str(), ifstream::in);
    const int maxLine = 1000;
    char line[maxLine];

    string sep;

    while(file.getline(line, maxLine)){
        // trim \n, \r or \r\n in the tail
        int readed = strlen(line);
        if(readed >=2 ){
            if(line[readed-1] == '\n' || line[readed-1] == '\r'){
                line[readed-1] = '\0';
                if(line[readed-2] == '\r')
                    line[readed-2] = '\0';
            }
        }
        if(sep.empty()) {
            for(int i=0;i<readed;i++) {
                if(line[i] == ',') {
                    sep = ",";
                }
                else if(line[i] == '\t') {
                    sep = "\t";
                }
            }
        }

        string linestr(line);

        // comment or header line
        if(line[0] == '#')
            continue;
        if(sep.empty())
            continue;

        vector<string> splitted;
        split(linestr, splitted, sep);
        // a valid line need 4 columns: name, left, center, right
        if(splitted.size()<2)
            continue;

        Sample s;
        s.file = trim(splitted[0]);
        s.index1 = trim(splitted[1]);
        if(indexReverseComplement){
            Sequence seq(s.index1);
            s.index1 = seq.reverseComplement().mStr;
        }
        if(splitted.size()>=3) {
            s.index2 = trim(splitted[2]);
            if(indexReverseComplement){
                Sequence seq(s.index2);
                s.index2 = seq.reverseComplement().mStr;
            }
        }
        log(s.file + ": " + s.index1);

        this->samples.push_back(s);
    }

    adjustWriterBufferSize();

}

bool Options::validate() {
    if(in1.empty()) {
        if(!in2.empty())
            error_exit("read2 input is specified by <in2>, but read1 input is not specified by <in1>");
        else
            error_exit("read1 input should be specified by --in1, or enable --stdin if you want to read STDIN");
    } else {
        check_file_valid(in1);
    }

    if(!in2.empty()) {
        check_file_valid(in2);
        pairedEnd = true;
    }

    if(!file_exists(outFolder)) {
        mkdir(outFolder.c_str(), 0777);
    }

    if(file_exists(outFolder) && !is_directory(outFolder)) {
        error_exit(outFolder + " is a file, not a directory");
    }

    if(!file_exists(outFolder) || !is_directory(outFolder)) {
        error_exit(outFolder + " is not a directory, or cannot be created");
    }

    parseSampleSheet();

    if(samples.size() == 0)
        error_exit("no sample found, did you provide a valid index CSV file by -s or --index?");

    if(threadNum > 0) {
        if(pairedEnd && threadNum<5)
            error_exit("at least 5 threads must be set for PE mode");
        else if(threadNum<4)
            error_exit("at least 4 threads must be set");
    } else {
        // if auto threading specified, use as much as cores
        threadNum = max((unsigned int)5, std::thread::hardware_concurrency());
    }

    if(mismatch<0 || mismatch>2)
        error_exit("allowed mismatch should be 0 ~ 2");

    if(compression<0 || compression>12)
        error_exit("compression setting should be 0 ~ 12");

    if(barcodePlace == BARCODE_AT_READ1 || barcodePlace == BARCODE_AT_READ2) {
        if(barcodeStart < 0)
            error_exit("If barcode_place is read1 or read2, the barcode starting position should be specified by -s or --barcode_start");
        if(barcodeLength == 0)
            error_exit("If barcode_place is read1 or read2, the barcode length should be specified by -l or --barcode_length");
    }

    if(barcodePlace == BARCODE_AT_READ2) {
        if(in2.empty())
            error_exit("If barcode_place is read2, the read2 input file should be specified by -2 or --in2");
    }

    return true;
}
