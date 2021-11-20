#ifndef MEMORYSTREAM_H
#define MEMORYSTREAM_H
#include <string>
#include <stdint.h>
#include <assert.h>
class MemoryStream {
public:
	static const size_t kDEFAULT_SIZE = 0x100;
	static const size_t kTHREHOLD = 1024 * 1024;
	static const char kCRLF[];

	MemoryStream(size_t size = kDEFAULT_SIZE);
	MemoryStream(char* buffer, size_t size);
	virtual ~MemoryStream();

	inline char* Data() {
		return data_;
	}

	inline const char* Data() const {
		return data_;
	}

	inline char* Begin() {
		if (roffset_ >= woffset_)
			return NULL;
		return &data_[roffset_];
	}

	inline const char* Begin() const {
		if (roffset_ >= woffset_)
			return NULL;
		return &data_[roffset_];
	}

	inline char* End() {
		return &data_[woffset_];
	}

	inline const char* End() const {
		return &data_[woffset_];
	}

	inline int ReadOffset() {
		return roffset_;
	}

	inline void ReadOffset(int offset) {
		if (offset < 0) {
			offset = 0;
		}
		roffset_ = offset;
	}

	inline int WriteOffset() {
		return woffset_;
	}

	inline void WriteOffset(int offset) {
		if (offset < 0) {
			offset = 0;
		}
		woffset_ = offset;
	}

	inline size_t Length() {
		return roffset_ >= woffset_ ? 0 : woffset_ - roffset_;
	}

	inline const size_t Length() const {
		return roffset_ >= woffset_ ? 0 : woffset_ - roffset_;
	}

	inline size_t Size() {
		return size_;
	}

	inline const size_t Size() const {
		return size_;
	}

	inline void Reserve(size_t space) {
		if (space + woffset_ <= size_) {
			return;
		}
		size_t size = size_ * 2;
		if (size < space + woffset_) {
			size = space + woffset_;
		}

		if (size >= kTHREHOLD) {

		}

		data_ = (char*)realloc(data_, size);
		size_ = size;
	}

	inline void Reset() {
		roffset_ = woffset_ = 0;
	}

	inline char* Steal(int* size = NULL) {
		char* result = data_;
		if (size) {
			*size = woffset_;
		}
		data_ = NULL;
		size_ = 0;
		Reset();
		return result;
	}

	inline char* Peek(size_t size) {
		if (size > Length()) {
			return NULL;
		}
		return &data_[roffset_];
	}

	inline uint8_t operator[](size_t pos) const {
		assert(pos >= 0 && pos < woffset_);
		return data_[pos];
	}

	template<typename T>
	inline void Append(T value) {
		Append((const uint8_t *)&value, sizeof(value));
	}

	inline void Append(const char* str, size_t cnt) {
		Append((const uint8_t*)str, cnt);
		Append<uint8_t>(0);
	}

	inline void Append(const std::string& str) {
		Append(str.c_str(), str.size());
	}

	inline void Append(const uint8_t* val, size_t cnt) {
		if (cnt == 0) {
			return;
		}
		Reserve(cnt);
		memcpy((void*)&data_[woffset_], val, cnt);
		woffset_ += cnt;
	}

	template<typename T>
	inline T Read() {
		assert(sizeof(T) <= Length());
		T val = *((T*)&data_[roffset_]);
		roffset_ += sizeof(T);
		return val;
	}

	inline MemoryStream& operator<<(bool value) {
		Append<bool>(value);
		return *this;
	}

	inline MemoryStream& operator<<(uint8_t value) {
		Append<uint8_t>(value);
		return *this;
	}

	inline MemoryStream& operator<<(uint16_t value) {
		Append<uint16_t>(value);
		return *this;
	}

	inline MemoryStream& operator<<(uint32_t value) {
		Append<uint32_t>(value);
		return *this;
	}

	inline MemoryStream& operator<<(uint64_t value) {
		Append<uint64_t>(value);
		return *this;
	}

	inline MemoryStream& operator<<(int8_t value) {
		Append<int8_t>(value);
		return *this;
	}

	inline MemoryStream& operator<<(int16_t value) {
		Append<int16_t>(value);
		return *this;
	}

	inline MemoryStream& operator<<(int32_t value) {
		Append<int32_t>(value);
		return *this;
	}

	inline MemoryStream& operator<<(int64_t value) {
		Append<int64_t>(value);
		return *this;
	}

	inline MemoryStream& operator<<(float value) {
		Append<float>(value);
		return *this;
	}

	inline MemoryStream& operator<<(double value) {
		Append<double>(value);
		return *this;
	}

	inline MemoryStream& operator<<(const std::string& value) {
		Append(value.c_str(), value.length());
		return *this;
	}

	inline MemoryStream& operator<<(const char *str) {
		Append(str, str ? strlen(str) : 0);
		return *this;
	}

	inline MemoryStream& operator<<(MemoryStream& rhs) {
		Append<uint32_t>(rhs.Length());
		Append((const uint8_t*)rhs.Begin(), rhs.Length());
		return *this;
	}

	inline MemoryStream& operator>>(bool &value) {
		value = Read<char>() > 0 ? true : false;
		return *this;
	}

	inline MemoryStream& operator>>(uint8_t &value) {
		value = Read<uint8_t>();
		return *this;
	}

	inline MemoryStream& operator>>(uint16_t &value) {
		value = Read<uint16_t>();
		return *this;
	}

	inline MemoryStream& operator>>(uint32_t &value) {
		value = Read<uint32_t>();
		return *this;
	}

	inline MemoryStream& operator>>(uint64_t &value) {
		value = Read<uint64_t>();
		return *this;
	}

	inline MemoryStream& operator>>(int8_t &value) {
		value = Read<int8_t>();
		return *this;
	}

	inline MemoryStream& operator>>(int16_t &value) {
		value = Read<int16_t>();
		return *this;
	}

	inline MemoryStream& operator>>(int32_t &value) {
		value = Read<int32_t>();
		return *this;
	}

	inline MemoryStream& operator>>(int64_t &value) {
		value = Read<int64_t>();
		return *this;
	}

	inline MemoryStream& operator>>(float &value) {
		value = Read<float>();
		return *this;
	}

	inline MemoryStream& operator>>(double &value) {
		value = Read<double>();
		return *this;
	}

	inline MemoryStream& operator>>(std::string& value) {
		size_t length = 0;
		for (size_t i = roffset_; i < size_; i++) {
			if (data_[i] == 0) {
				break;
			}
			length++;
		}

		if (length != 0) {
			value.append(&data_[roffset_], length);
			roffset_ += length + 1;
		} else {
			roffset_ += 1;
		}

		return *this;
	}

	MemoryStream& operator>>(char *value) {
		size_t length = 0;
		for (size_t i = roffset_; i < size_; i++) {
			if (data_[i] == 0) {
				break;
			}
			length++;
		}

		if (length != 0) {
			memcpy(value, &data_[roffset_], length);
			roffset_ += length + 1;
			value[length] = '\0';
		}

		return *this;
	}

	inline MemoryStream& operator>>(MemoryStream& rhs) {
		uint32_t length = Read<uint32_t>();
		rhs.Reserve(length);
		memcpy((void*)&rhs.data_[rhs.woffset_], (void*)&data_[roffset_], length);
		rhs.woffset_ += length;
		roffset_ += length;
		return *this;
	}

	void RetrieveUntil(const char* endc);

	const char* FindCRLF();
	const char* FindCRLF(const char* start);

	const char* FindEOL();
	const char* FindEOL(const char* start);

private:
	MemoryStream(const MemoryStream&);
	MemoryStream& operator=(const MemoryStream&);

private:
	size_t roffset_;
	size_t woffset_;
	size_t size_;
	char* data_;
};

#endif
