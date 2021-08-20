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

#include "memfunc.h"
#include <atomic>
#include "util.h"
#include <unistd.h>
#ifdef __APPLE__
#include <malloc/malloc.h>
#endif
#ifdef __linux__
#include <malloc.h>
#endif 

bool globalMemControlEnabled;
long globalMemLimit;
atomic_long globalUsedMemBytes;

size_t mallocSize(void* ptr) {
#ifdef __APPLE__
	return malloc_size(ptr);
#elif defined(__linux__)
	return malloc_usable_size(ptr);
#endif
	return 0;
}

// record this memory usage
void recordMem(void* ptr, size_t size) {
	if(globalMemControlEnabled) {
		globalUsedMemBytes += size;
	}
}

// release this memory usage record
void releaseMemRecord(void* ptr) {
	if(globalMemControlEnabled) {
		globalUsedMemBytes -= mallocSize(ptr);
	}
}

// traced malloc
void* tmalloc (size_t size) {
	return malloc(size);
	/*
	void* ret = malloc(size);
	if(ret) {
		recordMem(ret, size);
	}
    return ret;*/
}

// traced free
void tfree (void* ptr) {
	//releaseMemRecord(ptr);
    return free(ptr);
}

// traced realloc
void* trealloc (void* ptr, size_t size) {
	return realloc(ptr, size);
	/*
	releaseMemRecord(ptr);
	void* ret = realloc(ptr, size);
	if(ret) {
		recordMem(ret, size);
	}
    return ret;*/
}

// init
void initMemControl(long limitBytes) {
	globalUsedMemBytes = 0;
	globalMemLimit = limitBytes;
	if(limitBytes > 0)
		globalMemControlEnabled = true;
	else
		globalMemControlEnabled = false;
}

bool memExceeded() {
	if(globalMemControlEnabled)
		return globalUsedMemBytes > globalMemLimit;
	else
		return false;
}

void reportMemUsage() {
	cerr << "used: " << globalUsedMemBytes << endl;
}