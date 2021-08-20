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


#include "fastareader.h"
#include "util.h"
#include <sstream>

FastaReader::FastaReader(string faFile, bool forceUpperCase)
{
    // Set locale and disable stdio synchronization to improve iostream performance
    // http://www.drdobbs.com/the-standard-librarian-iostreams-and-std/184401305
    // http://stackoverflow.com/questions/5166263/how-to-get-iostream-to-perform-better
    setlocale(LC_ALL,"C");
    ios_base::sync_with_stdio(false);

    mFastaFile = faFile;
    mForceUpperCase = forceUpperCase;
    if (is_directory(mFastaFile)) {
        string error_msg = "There is a problem with the provided fasta file: \'";
        error_msg.append(mFastaFile);
        error_msg.append("\' is a directory NOT a file...\n");
        throw invalid_argument(error_msg);
    }
    mFastaFileStream.open( mFastaFile.c_str(),ios::in);
    // verify that the file can be read
    if (!mFastaFileStream.is_open()) {
        string msg = "There is a problem with the provided fasta file: could NOT read ";
        msg.append(mFastaFile.c_str());
        msg.append("...\n");
        throw invalid_argument(msg);
    }

    char c;
    // seek to first contig
    while (mFastaFileStream.get(c) && c != '>') {
        if (mFastaFileStream.eof()) {
            break;
        }
    }
}

FastaReader::~FastaReader()
{
    if (mFastaFileStream.is_open()) {
        mFastaFileStream.close();
    }
}

void FastaReader::readNext()
{
    mCurrentID = "";
    mCurrentDescription = "";
    mCurrentSequence = "";
    bool foundHeader = false;
    
    char c;
    stringstream ssSeq;
    stringstream ssHeader;
    while(true){
        mFastaFileStream.get(c);
        if(c == '>' || mFastaFileStream.eof())
            break;
        else {
            if (foundHeader){
                if(mForceUpperCase && c>='a' && c<='z') {
                    c -= ('a' - 'A');
                }
                ssSeq << c;
            }
            else
                ssHeader << c;
        }

        string line = "";
        getline(mFastaFileStream,line,'\n');


        if(foundHeader == false) {
            ssHeader << line;
            foundHeader = true;
        }
        else {
            str_keep_valid_sequence(line, mForceUpperCase);
            ssSeq << line;
        }
    }
    mCurrentSequence = ssSeq.str();
    string header = ssHeader.str();

    mCurrentID = header;
}

bool FastaReader::hasNext() {
    return !mFastaFileStream.eof();
}

void FastaReader::readAll() {
    while(!mFastaFileStream.eof()){
        readNext();
        mAllContigs[mCurrentID] = mCurrentSequence;
    }
}

bool FastaReader::test(){
    FastaReader reader("testdata/tinyref.fa");
    reader.readAll();

    string contig1 = "GATCACAGGTCTATCACCCTATTAATTGGTATTTTCGTCTGGGGGGTGTGGAGCCGGAGCACCCTATGTCGCAGT";
    string contig2 = "GTCTGCACAGCCGCTTTCCACACAGAACCCCCCCCTCCCCCCGCTTCTGGCAAACCCCAAAAACAAAGAACCCTA";

    if(reader.mAllContigs.count("contig1") == 0 || reader.mAllContigs.count("contig2") == 0 )
        return false;

    if(reader.mAllContigs["contig1"] != contig1 || reader.mAllContigs["contig2"] != contig2 )
        return false;

    return true;

}



