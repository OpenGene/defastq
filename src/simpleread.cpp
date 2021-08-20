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

#include "simpleread.h"
#include "util.h"
#include "memfunc.h"

atomic_long globalReadBytesInMem;

SimpleRead::SimpleRead(char* mData, unsigned int dataSize) {
	memset(this, 0, sizeof(SimpleRead));
	setData(mData, dataSize);
	globalReadBytesInMem += (mDataLen + sizeof(SimpleRead));
}

SimpleRead::~SimpleRead() {
	if(mData) {
		tfree(mData);
		mData = NULL;
	}
	globalReadBytesInMem -= (mDataLen + sizeof(SimpleRead));
}

char* SimpleRead::data() {
	return mData;
}

void SimpleRead::setData(char* d, unsigned int len) {
	mData = d;
	mDataLen = len;
	int nextBreak = findNextLineBreak(mData, mDataLen, 1);
	if(nextBreak < 0)
		error_exit("Invalid FASTQ read 1");

	// the name
	mNameLen = nextBreak;
	// fix \r\n
	if(mData[nextBreak-1] == '\r')
		mNameLen--;

	// the seq
	mSeqStart = nextBreak+1;

	nextBreak = findNextLineBreak(mData, mDataLen, nextBreak+1);
	if(nextBreak < 0)
		error_exit("Invalid FASTQ read 2");

	mSeqLen = nextBreak - mSeqStart;
	// fix \r\n
	if(mData[nextBreak-1] == '\r')
		mSeqLen--;

	// the strand
	nextBreak = findNextLineBreak(mData, mDataLen, nextBreak+1);
	if(nextBreak < 0)
		error_exit("Invalid FASTQ read 3");

	mQualStart = nextBreak+1;

	// the qual
	nextBreak = findNextLineBreak(mData, mDataLen, nextBreak+1);
	// the end of file
	if(nextBreak < 0)
		nextBreak = mDataLen+1;
	mQualLen = nextBreak - mQualStart;
	// fix \r\n
	if(mData[nextBreak-1] == '\r')
		mQualLen--;
}

unsigned int SimpleRead::dataLen() {
	return mDataLen;
}

unsigned int SimpleRead::nameLen() {
	return mNameLen;
}

void SimpleRead::setNameLen(unsigned int len) {
	mNameLen = len;
}

unsigned int SimpleRead::seqLen() {
	return mSeqLen;
}

unsigned int SimpleRead::seqStart() {
	return mSeqStart;
}

void SimpleRead::setSeq(unsigned int start, unsigned int len) {
	mSeqStart = start;
	mSeqLen = len;
}

unsigned int SimpleRead::qualLen() {
	return mQualLen;
}

unsigned int SimpleRead::qualStart() {
	return mQualStart;
}

void SimpleRead::setQual(unsigned int start, unsigned int len) {
	mQualStart = start;
	mQualLen = len;
}

bool SimpleRead::getIlluminaIndex1Place(unsigned int &start, unsigned int &len) {
	start = 0;
	len = 0;
	unsigned int end = mSeqStart - 1;
	unsigned int p = end;
	unsigned int plusPos = 0;
	unsigned int lastBase = 0;
	while(p>=0) {
		// find the last colon
		if(mData[p] == ':')
			break;
		if(mData[p] == '+')
			plusPos = p;
		if(lastBase == 0) {
			if(mData[p] == 'A' || mData[p] == 'T' || mData[p] == 'C' || mData[p] == 'G' || mData[p] == 'A')
				lastBase = p;
		}
		p--;
	}

	if(mData[p] != ':')
		return false;
	if(plusPos == 0 && lastBase==0)
		return false;
	start = p+1;
	if(plusPos > start)
		len = plusPos - start;
	else
		len = (lastBase - start + 1);
	return true;
}

bool SimpleRead::getIlluminaIndex2Place(unsigned int &start, unsigned int &len) {
	start = 0;
	len = 0;
	unsigned int end = mSeqStart - 1;
	unsigned int p = end;
	unsigned int plusPos = 0;
	unsigned int lastBase = 0;
	while(p>=0) {
		// find the last colon
		if(mData[p] == ':')
			break;
		if(mData[p] == '+')
			plusPos = p;
		if(lastBase == 0) {
			if(mData[p] == 'A' || mData[p] == 'T' || mData[p] == 'C' || mData[p] == 'G' || mData[p] == 'A')
				lastBase = p;
		}
		p--;
	}

	if(mData[p] != ':')
		return false;
	if(plusPos == 0 || lastBase <= plusPos)
		return false;
	start = plusPos+1;
	len = (lastBase - start + 1);
	return true;
}

bool SimpleRead::getIlluminaBothIndexPlaces(unsigned int &start1, unsigned int &len1, unsigned int &start2, unsigned int &len2) {
	start1 = 0;
	len1 = 0;
	start2 = 0;
	len2 = 0;
	unsigned int end = mSeqStart - 1;
	unsigned int p = end;
	unsigned int plusPos = 0;
	unsigned int lastBase = 0;
	while(p>=0) {
		// find the last colon
		if(mData[p] == ':')
			break;
		if(mData[p] == '+')
			plusPos = p;
		if(lastBase == 0) {
			if(mData[p] == 'A' || mData[p] == 'T' || mData[p] == 'C' || mData[p] == 'G' || mData[p] == 'A')
				lastBase = p;
		}
		p--;
	}

	if(mData[p] != ':')
		return false;
	if(plusPos == 0 || lastBase <= plusPos)
		return false;

	start1 = p+1;
	len1 = plusPos - start1;
	start2 = plusPos+1;
	len2 = (lastBase - start2 + 1);
	return true;
}

void SimpleRead::print() {
	string name(mData, mNameLen);
	string seq(mData + mSeqStart, mSeqLen);
	string strand("+");
	string qual(mData + mQualStart, mQualLen);
	cout<<name<<endl;
	cout<<seq<<endl;
	cout<<strand<<endl;
	cout<<qual<<endl;
}

bool SimpleRead::test() {
	string s("@NB551106:9:H5Y5GBGX2:1:11207:3263:19029 1:N:0:GATCAG+AATACG\rGGCTCACTGCAACCTCTGCCGCCTGGATTCAAGT\r+\rAAAAAEAEEE/A/AAEEE/E/A<EA<EEEAEEEE\r");
	char* mData = new char[s.length()];
	memcpy(mData, s.c_str(), s.length());

	SimpleRead* read = new SimpleRead(mData, strlen(mData));
	read->print();
	unsigned int s1, l1, s2, l2;
	bool hasIndex1 = read->getIlluminaIndex1Place(s1, l1);
	if(hasIndex1)
		cerr << "index1: " << string(read->mData + s1, l1) << endl;
	bool hasIndex2 = read->getIlluminaIndex2Place(s2, l2);
	if(hasIndex2)
		cerr << "index2: " << string(read->mData + s2, l2) << endl;
	bool hasTwoIndexes = read->getIlluminaBothIndexPlaces(s1, l1, s2, l2);
	if(hasTwoIndexes) {
		cerr << "both index: " << string(read->mData + s1, l1) + string(read->mData + s2, l2) << endl;
	}
	delete read;
	return true;
}

void SimpleRead::initCounter() {
	static bool initialized = false;
	if(!initialized) {
		globalReadBytesInMem = 0;
		initialized = true;
	} else {
		error_exit("SimpleRead::initCounter() cannot be called more than one time");
	}
}
