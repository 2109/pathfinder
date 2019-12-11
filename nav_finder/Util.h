#ifndef UTIL_H
#define UTIL_H

#include <string>
#include "MemoryStream.h"
namespace Util {
	int ReadFile(const char* file, std::string& result);

	int ReadFile(const char* file, MemoryStream& result);

};

#endif
