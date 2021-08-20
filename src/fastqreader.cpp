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

#include "fastqreader.h"
#include "util.h"
#include "memfunc.h"
#include <string.h>
#include <cassert>

#define FQ_BUF_SIZE (1<<23)
#define IGZIP_IN_BUF_SIZE (1<<22)
#define GZIP_HEADER_BYTES_REQ (1<<16)

FastqReader::FastqReader(string filename){
	mFilename = filename;
	mZipped = false;
	mFile = NULL;
	mStdinMode = false;
	mFastqBuf = (char* )tmalloc(FQ_BUF_SIZE);
	mBufDataLen = 0;
	mBufUsedLen = 0;
	mHasNoLineBreakAtEnd = false;
	mGzipInputBufferSize = IGZIP_IN_BUF_SIZE;
	mGzipInputBuffer = (unsigned char* )tmalloc(mGzipInputBufferSize);;
	mGzipOutputBufferSize = FQ_BUF_SIZE;
	mGzipOutputBuffer = (unsigned char*)mFastqBuf;
	mCounter = 0;
	init();
}

FastqReader::~FastqReader(){
	close();
	tfree(mFastqBuf);
	tfree(mGzipInputBuffer);
}


bool FastqReader::bufferFinished() {
	if(mZipped) {
		return eof() && mGzipState.avail_in == 0;
	} else {
		return eof();
	}
}

void FastqReader::readToBufIgzip(){
	mBufDataLen = 0;
	while(mBufDataLen == 0) {
		if(eof() && mGzipState.avail_in==0)
			return;
		if (mGzipState.avail_in == 0) {
			mGzipState.next_in = mGzipInputBuffer;
			mGzipState.avail_in = fread(mGzipState.next_in, 1, mGzipInputBufferSize, mFile);
		}
		mGzipState.next_out = mGzipOutputBuffer;
		mGzipState.avail_out = mGzipOutputBufferSize;

		int ret = isal_inflate(&mGzipState);
		if (ret != ISAL_DECOMP_OK) {
			error_exit("igzip: encountered while decompressing file");
		}
		mBufDataLen = mGzipState.next_out - mGzipOutputBuffer;
		if(eof() || mGzipState.avail_in>0)
			break;
	}
	// this block is finished
	if(mGzipState.block_state == ISAL_BLOCK_FINISH) {
		// a new block begins
		if(!eof() || mGzipState.avail_in > 0) {
			if (mGzipState.avail_in == 0) {
				isal_inflate_reset(&mGzipState);
				mGzipState.next_in = mGzipInputBuffer;
				mGzipState.avail_in = fread(mGzipState.next_in, 1, mGzipInputBufferSize, mFile);
			} else if (mGzipState.avail_in >= GZIP_HEADER_BYTES_REQ){
				unsigned char* old_next_in = mGzipState.next_in;
				size_t old_avail_in = mGzipState.avail_in;
				isal_inflate_reset(&mGzipState);
				mGzipState.avail_in = old_avail_in;
				mGzipState.next_in = old_next_in;
			} else {
				size_t old_avail_in = mGzipState.avail_in;
				memmove(mGzipInputBuffer, mGzipState.next_in, mGzipState.avail_in);
				size_t added = 0;
				if(!eof())
					added = fread(mGzipInputBuffer + mGzipState.avail_in, 1, mGzipInputBufferSize - mGzipState.avail_in, mFile);
				isal_inflate_reset(&mGzipState);
				mGzipState.next_in = mGzipInputBuffer;
				mGzipState.avail_in = old_avail_in + added;
			}
			int ret = isal_read_gzip_header(&mGzipState, &mGzipHeader);
			if (ret != ISAL_DECOMP_OK) {
				error_exit("igzip: invalid gzip header found");
			}
		}
	}
}

void FastqReader::readToBuf() {
	mBufDataLen = 0;
	if(mZipped) {
		readToBufIgzip();
	} else {
		if(!eof())
			mBufDataLen = fread(mFastqBuf, 1, FQ_BUF_SIZE, mFile);
	}
	mBufUsedLen = 0;

	if(bufferFinished()) {
		if(mFastqBuf[mBufDataLen-1] != '\n')
			mHasNoLineBreakAtEnd = true;
	}
}

void FastqReader::init(){
	if (ends_with(mFilename, ".gz")){
		mFile = fopen(mFilename.c_str(), "rb");
		if(mFile == NULL) {
			error_exit("Failed to open file: " + mFilename);
		}
		isal_gzip_header_init(&mGzipHeader);
		isal_inflate_init(&mGzipState);
		mGzipState.crc_flag = ISAL_GZIP_NO_HDR_VER;
		mGzipState.next_in = mGzipInputBuffer;
		mGzipState.avail_in = fread(mGzipState.next_in, 1, mGzipInputBufferSize, mFile);
		int ret = isal_read_gzip_header(&mGzipState, &mGzipHeader);
		if (ret != ISAL_DECOMP_OK) {
			error_exit("igzip: Error invalid gzip header found");
		}
		mZipped = true;
	}
	else {
		if(mFilename == "/dev/stdin") {
			mFile = stdin;
		}
		else
			mFile = fopen(mFilename.c_str(), "rb");
		if(mFile == NULL) {
			error_exit("Failed to open file: " + mFilename);
		}
		mZipped = false;
	}
	readToBuf();
}

void FastqReader::clearLineBreaks(char* line) {

	// trim \n, \r or \r\n in the tail
	int readed = strlen(line);
	if(readed >=2 ){
		if(line[readed-1] == '\n' || line[readed-1] == '\r'){
			line[readed-1] = '\0';
			if(line[readed-2] == '\r')
				line[readed-2] = '\0';
		}
	}
}

bool FastqReader::eof() {
	return feof(mFile);//mFile.eof();
}

SimpleRead* FastqReader::read(){
	if(mBufUsedLen >= mBufDataLen && eof()) {
		return NULL;
	}

	int start = mBufUsedLen;
	int end = start+1;

	if(mFastqBuf[start] != '@') {
		/*cerr << "start: " << start << endl;
		int left = max(0, start-100);
		int right = min(mBufDataLen-1, start+100);
		string str1(mFastqBuf+left, start-left);
		cerr<<str1<<endl;
		string str2(mFastqBuf+start, right-start);
		cerr<<str2<<endl;*/
		error_exit("The " + to_string(mCounter) + " read in " +  mFilename +", FASTQ should start with @, not " + string(mFastqBuf[start], 1));
	}

	char lastByte = '\0';
	int lineBreak = 0;

	// find next FASTQ start
	while(end < mBufDataLen) {
		if(mFastqBuf[end] == '\n') {
			end++;
			lineBreak++;
			if(lineBreak == 4) {
				break;
			}
		}
		else {
			end++;
		}
	}

	int len = end - start;
	char* data = (char* )tmalloc(len);
	memcpy(data, mFastqBuf+start, len);

	// this record is well contained in this buf, or this is the last buf
	if(lineBreak == 4 || bufferFinished()) {
		mBufUsedLen = end;
		if(mBufUsedLen == mBufDataLen) 
			readToBuf();
		mCounter++;
		if(lineBreak < 3)
			return NULL;
		else
			return new SimpleRead(data, len);
	}

	while(true) {
		readToBuf();
		start = 0;
		end = 0;
		while(end < mBufDataLen) {
			if(mFastqBuf[end] == '\n') {
				lineBreak++;
				end++;
				if(lineBreak == 4) {
					break;
				}
			}
			else {
				end++;
			}
		}
		int newlen = end - start;
		
		data = (char* )trealloc(data, len + newlen);
		memcpy(data + len, mFastqBuf+start, newlen);
		len += newlen;

		// this record is not well contained in this buf, we need to read new buf
		if(lineBreak == 4 || bufferFinished()) {
			mBufUsedLen = end;
			if(mBufUsedLen == mBufDataLen) {
				readToBuf();
			}
			mCounter++;
			if(lineBreak < 3)
				return NULL;
			else
				return new SimpleRead(data, len);
		}
	}

	return NULL;
}

void FastqReader::close(){
	if (mFile){
		fclose(mFile);//mFile.close();
		mFile = NULL;
	}
}

bool FastqReader::isZipFastq(string filename) {
	if (ends_with(filename, ".fastq.gz"))
		return true;
	else if (ends_with(filename, ".fq.gz"))
		return true;
	else if (ends_with(filename, ".fasta.gz"))
		return true;
	else if (ends_with(filename, ".fa.gz"))
		return true;
	else
		return false;
}

bool FastqReader::isFastq(string filename) {
	if (ends_with(filename, ".fastq"))
		return true;
	else if (ends_with(filename, ".fq"))
		return true;
	else if (ends_with(filename, ".fasta"))
		return true;
	else if (ends_with(filename, ".fa"))
		return true;
	else
		return false;
}

bool FastqReader::isZipped(){
	return mZipped;
}

bool FastqReader::test(){
	FastqReader reader1("testdata/R1.fq");
	FastqReader reader2("testdata/R1.fq.gz");
	SimpleRead* r1 = NULL;
	SimpleRead* r2 = NULL;
	int i=0;
	while(true){
		i++;
		r1=reader1.read();
		r2=reader2.read();
		if(r1 == NULL || r2==NULL)
			break;
		r1->print();
		r2->print();
		delete r1;
		delete r2;
	}
	return true;
}
