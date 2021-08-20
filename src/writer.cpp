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

#include "writer.h"
#include "util.h"
#include "fastqreader.h"
#include <string.h>
#include "memfunc.h"

Writer::Writer(Options* opt, string filename, int compression, bool isRead2, bool isUndetermined){
	mCompression = compression;
	mFilename = filename;
	mCompressor = NULL;
	mZipped = false;
	haveToClose = true;
	mBuffer = NULL;
	mBufDataLen = 0;
	mOptions = opt;
	mBufSize = mOptions->writerBufferSize;
	mIsUndetermined = isUndetermined;
	mIsRead2 = isRead2;
	init();
}

Writer::~Writer(){
	flush();
	if(haveToClose) {
		close();
	}
}

void Writer::flush() {
	if(mBufDataLen > 0) {
		write(mBuffer, mBufDataLen);
		mBufDataLen = 0;
	}
}

string Writer::filename(){
	return mFilename;
}

void Writer::init(){
	mBuffer = (char*) tmalloc(mBufSize);
	if(mBuffer == NULL) {
		error_exit("Failed to allocate write buffer with size: " + to_string(mBufSize));
	}
	if (ends_with(mFilename, ".gz")){
		mCompressor = libdeflate_alloc_compressor(mCompression);
		if(mCompressor == NULL) {
			error_exit("Failed to alloc libdeflate_alloc_compressor, please check the libdeflate library.");
		}
		mZipped = true;
		mFP = fopen(mFilename.c_str(), "wb");
		if(mFP == NULL) {
			error_exit("Failed to write: " + mFilename);
		}
	} else {
		mFP = fopen(mFilename.c_str(), "wb");
		if(mFP == NULL) {
			error_exit("Failed to write: " + mFilename);
		}
		//mOutStream = new ofstream();
		//mOutStream->open(mFilename.c_str(), ifstream::out);
	}
}

bool Writer::writeRead(SimpleRead* r) {
	char* d = r->data();
	if(r->dataLen() + mBufDataLen > mBufSize)
		flush();
	if(r->dataLen() > mBufSize)
		write(d, r->dataLen());
	else {
		if((mOptions->barcodePlace == BARCODE_AT_READ1 && !mIsRead2) || (mOptions->barcodePlace == BARCODE_AT_READ2 && mIsRead2)) {
			// remove the barcode from read
			int cutLen = min(mOptions->barcodeLength, (int)r->seqLen() - mOptions->barcodeStart);
			if(cutLen <= 0 || mIsUndetermined) {
				memcpy(mBuffer + mBufDataLen, d, r->dataLen());
				mBufDataLen += r->dataLen();
			} else {
				// first part
				memcpy(mBuffer + mBufDataLen, d, r->seqStart() + mOptions->barcodeStart);
				mBufDataLen += r->seqStart() + mOptions->barcodeStart;
				// second part
				int secondPartStart = r->seqStart() + mOptions->barcodeStart + cutLen;
				int secondPartLen =  r->qualStart() + mOptions->barcodeStart - secondPartStart;
				memcpy(mBuffer + mBufDataLen, d + secondPartStart, secondPartLen);
				mBufDataLen += secondPartLen;
				// second part
				int thirdPartStart = r->qualStart() + mOptions->barcodeStart + cutLen;
				int thirdPartLen =  r->dataLen() - thirdPartStart;
				if(thirdPartLen > 0) {
					memcpy(mBuffer + mBufDataLen, d + thirdPartStart, thirdPartLen);
					mBufDataLen += thirdPartLen;
				}
			}
		} else {
			memcpy(mBuffer + mBufDataLen, d, r->dataLen());
			mBufDataLen += r->dataLen();
		}
	}
	return true;
}

bool Writer::write(char* strdata, size_t size) {
	size_t written;
	bool status;
	
	if(mZipped){
		size_t bound = libdeflate_gzip_compress_bound(mCompressor, size);
		void* out = tmalloc(bound);
		size_t outsize = libdeflate_gzip_compress(mCompressor, strdata, size, out, bound);
		if(outsize == 0)
			status = false;
		else {
			size_t ret = fwrite(out, 1, outsize, mFP );
			status = ret>0;
			//mOutStream->write((char*)out, outsize);
			//status = !mOutStream->fail();
		}
		tfree(out);
	}
	else{
		size_t ret = fwrite(strdata, 1, size, mFP );
		status = ret>0;
		//mOutStream->write(strdata, size);
		//status = !mOutStream->fail();
	}
	return status;
}

void Writer::close(){
	if (mZipped){
		if (mCompressor){
			libdeflate_free_compressor(mCompressor);
			mCompressor = NULL;
		}
	}
	/*if(mOutStream) {
		if (mOutStream->is_open()){
			mOutStream->flush();
			mOutStream = NULL;
		}
	}*/
	if(mBuffer) {
		tfree(mBuffer);
		mBuffer = NULL;
	}
	if(mFP) {
		fclose(mFP);
		mFP = NULL;
	}
}

bool Writer::isZipped(){
	return mZipped;
}