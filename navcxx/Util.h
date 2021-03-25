#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <vector>
#include "MemoryStream.h"
#include "MathEx.h"
#include "Vector2.h"

namespace Util {
	int ReadFile(const char* file, std::string& result);

	int ReadFile(const char* file, MemoryStream& result);

	void GetCircleRoundIndex(int i, std::vector<Math::Vector2>& result);

	void GetCircleIndex(int i, std::vector<Math::Vector2>& result);
};

#endif
