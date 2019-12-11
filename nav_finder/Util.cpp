#include <stdint.h>
#include "Util.h"

namespace Util {
	

	int ReadFile(const char* file, std::string& result) {
		result = "";
		FILE* fp = fopen(file, "r");
		if (!fp) {
			return -1;
		}

		char buf[1024];
		while (true) {
			size_t count = fread(buf, 1, sizeof(buf), fp);
			if (count > 0) {
				result.append(buf, count);
			} else {
				break;
			}
		}
		fclose(fp);
		return 0;
	}

	int ReadFile(const char* file, MemoryStream& result) {
		FILE* fp = fopen(file, "rb");
		if (!fp) {
			return -1;
		}

		char buf[1024];
		while (true) {
			size_t count = fread(buf, 1, sizeof(buf), fp);
			if (count > 0) {
				result.Append((uint8_t*)buf, count);
			} else {
				break;
			}
		}
		fclose(fp);
		return 0;
	}
};

