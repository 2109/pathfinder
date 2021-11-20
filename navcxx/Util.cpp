#include <stdint.h>
#include <unordered_map>
#include <unordered_set>
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

	void GetCircleRoundIndex(int i, std::vector<Math::Vector2>& result) {
		std::unordered_map<int, std::unordered_set<int>> record = std::unordered_map<int, std::unordered_set<int>>();

		int tx = 0;
		int tz = i;
		int d = 3 - 2 * i;
		while (tx <= tz) {
			int range[][2] = { { tx, tz }, { -tx, tz }, { tx, -tz }, { -tx, -tz }, { tz, tx }, { -tz, tx }, { tz, -tx }, { -tz, -tx } };
			for (int j = 0; j < 8; ++j) {
				int xOffset = range[j][0];
				int zOffset = range[j][1];
				auto itr = record.find(xOffset);
				if (itr != record.end()) {
					std::unordered_set<int>& set = itr->second;
					auto it = set.find(zOffset);
					if (it == set.end()) {
						set.insert(zOffset);
						result.push_back(Math::Vector2((float)xOffset, (float)zOffset));
					}
				} else {
					record[xOffset] = std::unordered_set<int>();
					record[xOffset].insert(zOffset);
					result.push_back(Math::Vector2((float)xOffset, (float)zOffset));
				}
			}

			if (d < 0) {
				d = d + 4 * tx + 6;
			} else {
				d = d + 4 * (tx - tz) + 10;
				tz--;
			}
			tx++;
		}
	}

	void GetCircleIndex(int i, std::vector<Math::Vector2>& result) {
		std::unordered_map<int, std::unordered_set<int>> record = std::unordered_map<int, std::unordered_set<int>>();

		int tx = 0;
		int tz = i;
		int d = 3 - 2 * i;
		while (tx <= tz) {
			for (int zi = tx; zi <= tz; zi++) {
				int range[][2] = { { tx, zi }, { -tx, zi }, { tx, -zi }, { -tx, -zi }, { zi, tx }, { -zi, tx }, { zi, -tx }, { -zi, -tx } };

				for (int i = 0; i < 8; i++) {
					int xOffset = range[i][0];
					int zOffset = range[i][1];
					auto itr = record.find(xOffset);
					if (itr != record.end()) {
						std::unordered_set<int>& set = itr->second;
						auto it = set.find(zOffset);
						if (it == set.end()) {
							set.insert(zOffset);
							result.push_back(Math::Vector2((float)xOffset, (float)zOffset));
						}
					} else {
						record[xOffset] = std::unordered_set<int>();
						record[xOffset].insert(zOffset);
						result.push_back(Math::Vector2((float)xOffset, (float)zOffset));
					}
				}
			}

			if (d < 0) {
				d = d + 4 * tx + 6;
			} else {
				d = d + 4 * (tx - tz) + 10;
				tz--;
			}
			tx++;
		}
	}
};

