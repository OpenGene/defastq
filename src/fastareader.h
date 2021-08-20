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

#ifndef FASTA_READER_H
#define FASTA_READER_H

// includes
#include <cctype>
#include <clocale>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <map>

using namespace std;

class FastaReader
{
public:
    FastaReader(string fastaFile, bool forceUpperCase = true);
    ~FastaReader();
    bool hasNext();
    void readNext();
    void readAll();

    inline string currentID()
    {
        return mCurrentID;
    }

    inline string currentDescription()
    {
        return mCurrentDescription;
    }

    inline string currentSequence()
    {
        return mCurrentSequence;
    }

    inline map<string, string>& contigs() {
        return mAllContigs;
    }

    static bool test();


public:
    string mCurrentSequence;
    string mCurrentID ;
    string mCurrentDescription;
    map<string, string> mAllContigs;

private:
    bool readLine();
    bool endOfLine(char c);
    void setFastaSequenceIdDescription();

private:
    string mFastaFile;
    ifstream mFastaFileStream;
    bool mForceUpperCase;
};


#endif

