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

#include <stdio.h>
#include "fastqreader.h"
#include "unittest.h"
#include <time.h>
#include "cmdline.h"
#include <sstream>
#include "util.h"
#include "options.h"
#include "processor.h"
#include "memfunc.h"
#include "evaluator.h"

string command;

int main(int argc, char* argv[]){
    // display version info if no argument is given
    if(argc == 1) {
        cout << "defastq: ultra-fast multi-threaded FASTQ demultiplexing" << endl << "version " << VERSION_NUM << endl;
    }
    if (argc == 2 && strcmp(argv[1], "test")==0){
        UnitTest tester;
        tester.run();
        return 0;
    }
    cmdline::parser cmd;
    // input/output
    cmd.add<string>("in1", '1', "input file name for read1", true, "");
    cmd.add<string>("in2", '2', "input file name for read2", false, "");
    cmd.add<string>("barcode_place", 'b', "For MGI it should be read1 or read2, for Illumina, it should be index1/index2/both_index", true, "");
    cmd.add<int>("barcode_start", 's', "If barcode_place is read1 or read2, the barcode starting position should be specified. This is 1-based.", false, 0);
    cmd.add<int>("barcode_length", 'l', "If barcode_place is read1 or read2, the barcode length should be specified", false, 0);
    cmd.add<string>("index", 'i', "a CSV/TSV/FASTA file contains two values (filename, barcode)", true, "");
    cmd.add("reverse_complement", 'r', "specify this if the index barcodes are reverse complement.");
    cmd.add<string>("out_folder", 'o', "output folder, default is current working directory", false, ".");
    cmd.add<string>("undecoded", 'u', "the file name to store undetermined reads, default is 'undecoded'. To discard the undetermined reads, specify discard", false, "undecoded");
    cmd.add<int>("compression", 'z', "compression level for gzip output (1 ~ 12). 0 means no compression, 1 is fastest, 12 is smallest, default is 6. ", false, 6);
    cmd.add<int>("allowed_mismatch", 'a', "allowed mismatch (0~2)", false, 0);
    cmd.add<int>("thread", 'n', "number of threads (at least 4 for SE, 5 for PE), default 0 means one thread per core.", false, 0);
    cmd.add<int>("memory", 'm', "memory limit (GB), 4GB is minimal, default 0 means unlimited.", false, 0);
    cmd.add("debug", 0, "print debug information.");

    cmd.parse_check(argc, argv);
    
    stringstream ss;
    for(int i=0;i<argc;i++){
        ss << argv[i] << " ";
    }
    command = ss.str();

    time_t t1 = time(NULL);

    Options opt;

    opt.in1 = cmd.get<string>("in1");
    opt.in2 = cmd.get<string>("in2");

    Evaluator* e = new Evaluator(&opt);
    e->evaluateSeqLen();

    opt.samplesheet = cmd.get<string>("index");
    opt.outFolder = cmd.get<string>("out_folder");
    opt.undecodedFileName = cmd.get<string>("undecoded");
    if(opt.undecodedFileName == "discard")
        opt.discardUndecoded = true;
    opt.compression = cmd.get<int>("compression");
    opt.threadNum = cmd.get<int>("thread");
    opt.mismatch = cmd.get<int>("allowed_mismatch");
    int mem = cmd.get<int>("memory");
    if(mem>0) {
        if(mem<1)
            error_exit("1GB memory is required, you specified " + to_string(mem) + " GB");
        else if(mem>10000)
            error_exit("memory limit cannot be larger than 10000GB, you specified " + to_string(mem) + " GB");
        else
            opt.memoryLimitBytes = mem * 1024 * 1024 * 1024;
    }

    string barcodePlace = cmd.get<string>("barcode_place");
    if(barcodePlace == "read1") 
    	opt.barcodePlace = BARCODE_AT_READ1;
    else if(barcodePlace == "read2") 
    	opt.barcodePlace = BARCODE_AT_READ2;
    else if(barcodePlace == "index1") 
    	opt.barcodePlace = BARCODE_AT_INDEX1;
    else if(barcodePlace == "index2") 
    	opt.barcodePlace = BARCODE_AT_INDEX2;
    else if(barcodePlace == "both_index") 
    	opt.barcodePlace = BARCODE_AT_BOTH_INDEX;
    else
    	error_exit("Please specify barcode place correctly by -b or --barcode_place. For MGI it should be read1 or read2; for Illumina, it should be index1/index2/both_index");

    if(barcodePlace == "read1" || barcodePlace == "read2") {
    	// minus by one for 1-based to 0-based
	    opt.barcodeStart = cmd.get<int>("barcode_start") - 1;
	    opt.barcodeLength = cmd.get<int>("barcode_length");
	}
    opt.indexReverseComplement = cmd.exist("reverse_complement");
    opt.debug = cmd.exist("debug");

    opt.validate();
        
    Processor p(&opt);
    p.process();

    return 0;
}