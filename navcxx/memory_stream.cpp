#include <algorithm>
#include "memory_stream.h"

const char MemoryStream::kCRLF[] = "\r\n";

MemoryStream::MemoryStream(size_t size) :roffset_(0), woffset_(0) {
	data_ = (char*)malloc(size);
	size_ = size;
}

MemoryStream::MemoryStream(char* buffer, size_t size) {
	roffset_ = 0;
	woffset_ = size;
	data_ = (char*)malloc(size);
	size_ = size;
	memcpy(data_, buffer, size);
}

MemoryStream::~MemoryStream() {
	if (data_) {
		free(data_);
	}
}

void MemoryStream::RetrieveUntil(const char* endc) {
	assert(Begin() <= endc);
	assert(endc <= End());
	roffset_ += endc - Begin();
}

const char* MemoryStream::FindCRLF() {
	const char* crlf = std::search(Begin(), End(), kCRLF, kCRLF + 2);
	return crlf == &data_[woffset_] ? NULL : crlf;
}

const char* MemoryStream::FindCRLF(const char* start) {
	assert(start <= End());
	const char* crlf = std::search((char*)start, End(), kCRLF, kCRLF + 2);
	return crlf == &data_[woffset_] ? NULL : crlf;
}

const char* MemoryStream::FindEOL() {
	const void* eol = memchr(Begin(), '\n', Length());
	return static_cast<const char*>(eol);
}

const char* MemoryStream::FindEOL(const char* start) {
	const void* eol = memchr(start, '\n', Length());
	return static_cast<const char*>(eol);
}
