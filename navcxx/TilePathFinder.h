#ifndef TILE_PATH_FINDER_H
#define TILE_PATH_FINDER_H

extern "C" {
#include "minheap-adapter.h"
}
#include "MathEx.h"
#include "Vector2.h"


namespace Math {
	class Vector2;
};

class TilePathFinder {
public:
	typedef void(*DebugFunc)(void* ud, int x, int z);

	static const int kMaskMax = 64;
	static const int kSearchDepth = 8;
	static const int kMaxRandDepth = 16;

	enum Status {
		STATUS_OK = 0,
		STATUS_START_ERROR = -1,
		STATUS_OVER_ERROR = -2,
		STATUS_SAME_POINT = -3,
		STATUS_CANNOT_REACH = -4,
	};

	enum SmoothType {
		None = 0,
		Head = 0x01,
		Tail = 0x02,
	};

	struct PathNode {
		mh_elt_t elt_;
		PathNode* next_;
		PathNode* parent_;
		Math::Vector2 pos_;
		int index_;
		uint8_t block_;
		float G;
		float H;
		float F;
		bool close_;
		bool record_;
		PathNode() {
			index_ = 0;
			block_ = 0;
			Reset();
		}

		void Reset() {
			mh_init(&elt_);
			next_ = parent_ = NULL;
			G = H = F = 0;
			close_ = false;
			record_ = false;
		}
	};

	struct IndexPair {
		int x_;
		int z_;
		IndexPair(int x, int z) {
			x_ = x;
			z_ = z;
		}
	};

public:

	TilePathFinder(int width, int height, int tile, const uint8_t* data);

	virtual ~TilePathFinder();

	static TilePathFinder* LoadFromFile(const char* file);

	void Serialize(const char* file);

	inline int GetWidth() {
		return width_;
	}

	inline int GetHeight() {
		return height_;
	}

	inline uint8_t GetBlock(int x, int z) {
		PathNode* node = FindNode(x, z);
		if (!node) {
			return 0;
		}
		return node->block_;
	}

	inline void SetBlock(int x, int z, uint8_t val) {
		PathNode* node = FindNode(x, z);
		if (!node) {
			return;
		}
		node->block_ = val;
	}

	inline PathNode* FindNode(int x, int z) {
		if (x < 0 || x >= width_ || z < 0 || z >= height_) {
			return NULL;
		}
		return &node_[x * height_ + z];
	}

	inline bool IsBlock(PathNode* node) {
		return node->block_ != 0;
	}

	inline bool IsBlock(int x, int z) {
		PathNode* node = FindNode(x, z);
		if (!node) {
			return false;
		}
		return IsBlock(node);
	}

	inline bool Movable(PathNode* node, bool ignore) {
		if (!node) {
			return false;
		}
		if (ignore) {
			return !IsBlock(node);
		}
		return mask_[node->block_] == 1;
	}

	inline bool Movable(int x, int z, bool ignore) {
		PathNode* node = FindNode(x, z);
		return Movable(node, ignore);
	}

	inline void MaskSet(int index, int mask) {
		if (index < 0 || index >= kMaskMax) {
			return;
		}
		mask_[index] = mask;
	}

	inline void MaskReset() {
		for (int i = 0; i < kMaskMax; ++i) {
			mask_[i] = 0;
		}
	}

	inline void MaskReverse() {
		for (int i = 0; i < kMaskMax; ++i) {
			mask_[i] = !mask_[i];
		}
	}
	inline int ToTile(float val) {
		return (int)(val / tile_);
	}

	inline float ToCoord(int val) {
		return ((float)val + 0.5) * (float)tile_;
	}

	void SetDebugCallback(DebugFunc func, void* userdata);

	PathNode* SearchInCircle(int cx, int cz, int depth);

	PathNode* SearchInReactangle(int cx, int cz, int depth);

	void BuildPath(PathNode* node, PathNode* from, SmoothType smooth, std::vector<const Math::Vector2*>& list);

	int Find(const Math::Vector2& from, const Math::Vector2& to, SmoothType smooth, std::vector<const Math::Vector2*>& list, float estimate = 1.0f);

	int Find(int x0, int z0, int x1, int z1, SmoothType smooth, std::vector<const Math::Vector2*>& list, float estimate = 1.0f);

	int Raycast(const Math::Vector2& from, const Math::Vector2& to, bool ignore, Math::Vector2* result, Math::Vector2* stop, bool use_breshemham);

	int Raycast(int x0, int z0, int x1, int z1, bool ignore, Math::Vector2* result, Math::Vector2* stop, bool use_breshemham);

	void Random(Math::Vector2& result);

	int RandomInCircle(int cx, int cz, int radius, Math::Vector2& result);
protected:
	void Reset();

	bool IsBestNode(int cx, int cz, int dx, int dz, int* dtMin, PathNode** list);

	void FindNeighbors(PathNode* node, PathNode** link);

private:
	int width_;
	int height_;
	int tile_;
	PathNode* node_;
	int* nonblock_;
	int nonblockCount_;
	uint8_t mask_[kMaskMax];
	mh_t openList_;
	PathNode* closeList_;
	std::vector<std::vector<IndexPair>*> circleIndex_;
	std::vector<std::vector<IndexPair>*> rangeIndex_;
	DebugFunc debugFunc_;
	void* debugUserdata_;
};

#endif