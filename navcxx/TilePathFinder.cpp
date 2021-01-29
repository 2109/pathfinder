
#include "Vector2.h"
#include "Util.h"
#include "TilePathFinder.h"

#define DX(A,B) (A->pos_.x - B->pos_.x)
#define DZ(A,B) (A->pos_.y - B->pos_.y)

static int kDirection[8][2] = { { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 },
{ -1, -1 }, { -1, 1 }, { 1, -1 }, { 1, 1 } };

static inline int NodeCmp(mh_elt_t* lhs, mh_elt_t* rhs) {
	return ((TilePathFinder::PathNode*)lhs)->F > ((TilePathFinder::PathNode*)rhs)->F;
}

static inline float NeighborEstimate(TilePathFinder::PathNode* from, TilePathFinder::PathNode* to) {
	int dx = DX(from, to);
	int dz = DZ(from, to);

	for (int i = 0; i < 8; ++i) {
		if (kDirection[i][0] == dx && kDirection[i][1] == dz) {
			if (i < 4) {
				return 10.0f;
			}
		}
	}
	return 14.0f;
}

static inline float GoalEstimate(TilePathFinder::PathNode* from, TilePathFinder::PathNode* to, float cost) {
	return abs(DX(from, to)) * cost + abs(DZ(from, to)) * cost;
}

static inline void ClearHeap(mh_elt_t* elt) {
	TilePathFinder::PathNode* node = (TilePathFinder::PathNode*)elt;
	node->Reset();
}

TilePathFinder::TilePathFinder(int width, int height, int tile, const uint8_t* data) {
	width_ = width;
	height_ = height;
	tile_ = tile;

	nonblockCount_ = 0;

	node_ = new PathNode[width_ * height_];
	for (int x = 0; x < width; ++x) {
		for (int z = 0; z < height; ++z) {
			PathNode* node = FindNode(x, z);
			node->index_ = x * height_ + z;
			node->pos_.x = x;
			node->pos_.y = z;
			node->block_ = data[node->index_];
			mh_init(&node->elt_);
			if (!IsBlock(node)) {
				nonblockCount_++;
			}
		}
	}

	nonblock_ = NULL;
	mh_ctor(&openList_, NodeCmp);
	closeList_.prev_ = &closeList_;
	closeList_.next_ = &closeList_;

	MaskReset();
	MaskSet(0, 1);

	debugFunc_ = NULL;
	debugUserdata_ = NULL;

	rangeIndex_.resize(kSearchDepth);
	for (int i = 1; i <= (int)rangeIndex_.size(); ++i) {
		std::vector<IndexPair>* pairInfo = new std::vector<IndexPair>();
		rangeIndex_[i - 1] = pairInfo;
		std::unordered_map<int, std::unordered_set<int>> record = std::unordered_map<int, std::unordered_set<int>>();

		int tx = 0;
		int tz = i;
		int d = 3 - 2 * i;
		while (tx <= tz) {
			int range[][2] = { { tx, tz }, { -tx, tz }, { tx, -tz }, { -tx, -tz },
			{ tz, tx }, { -tz, tx }, { tz, -tx }, { -tz, -tx } };
			for (int j = 0; j < 8; ++j) {
				int xOffset = range[j][0];
				int zOffset = range[j][1];
				std::unordered_map<int, std::unordered_set<int>>::iterator itr = record.find(xOffset);
				if (itr != record.end()) {
					std::unordered_set<int>& set = itr->second;
					std::unordered_set<int>::iterator it = set.find(zOffset);
					if (it == set.end()) {
						set.insert(zOffset);
						pairInfo->push_back(IndexPair(xOffset, zOffset));
					}
				} else {
					record[xOffset] = std::unordered_set<int>();
					record[xOffset].insert(zOffset);
					pairInfo->push_back(IndexPair(xOffset, zOffset));
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

	circleIndex_.resize(kMaxRandDepth);
	for (int i = 1; i <= (int)circleIndex_.size(); ++i) {
		std::vector<IndexPair>* pairInfo = new std::vector<IndexPair>();
		circleIndex_[i - 1] = pairInfo;
		std::unordered_map<int, std::unordered_set<int>> record = std::unordered_map<int, std::unordered_set<int>>();

		int tx = 0;
		int tz = i;
		int d = 3 - 2 * i;
		while (tx <= tz) {
			for (int zi = tx; zi <= tz; zi++) {
				int range[][2] = { { tx, zi }, { -tx, zi }, { tx, -zi }, { -tx, -zi },
				{ zi, tx }, { -zi, tx }, { zi, -tx }, { -zi, -tx } };

				for (int i = 0; i < 8; i++) {
					int xOffset = range[i][0];
					int zOffset = range[i][1];
					std::unordered_map<int, std::unordered_set<int>>::iterator itr = record.find(xOffset);
					if (itr != record.end()) {
						std::unordered_set<int>& set = itr->second;
						std::unordered_set<int>::iterator it = set.find(zOffset);
						if (it == set.end()) {
							set.insert(zOffset);
							pairInfo->push_back(IndexPair(xOffset, zOffset));
						}
					} else {
						record[xOffset] = std::unordered_set<int>();
						record[xOffset].insert(zOffset);
						pairInfo->push_back(IndexPair(xOffset, zOffset));
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
}

TilePathFinder::~TilePathFinder() {
	mh_dtor(&openList_);

	delete[] node_;

	if (nonblock_) {
		delete[] nonblock_;
	}

	for (int i = 0; i < kSearchDepth; ++i) {
		std::vector<IndexPair>* index = rangeIndex_[i];
		if (index) {
			delete index;
		}
	}

	for (int i = 0; i < kMaxRandDepth; ++i) {
		std::vector<IndexPair>* index = circleIndex_[i];
		if (index) {
			delete index;
		}
	}
}

TilePathFinder* TilePathFinder::LoadFromFile(const char* file) {
	MemoryStream ms;
	if (Util::ReadFile(file, ms) < 0) {
		return NULL;
	}

	int version, width, height, tile;
	ms >> version >> width >> height >> tile;

	TilePathFinder* finder = new TilePathFinder(width, height, tile, (const uint8_t*)ms.Begin());

	return finder;
}

void TilePathFinder::Serialize(const char* file) {
	MemoryStream ms;
	ms << (int32_t)0 << width_ << height_ << tile_;
	for (int i = 0; i < width_ * height_; i++) {
		ms << node_[i].block_;
	}

	FILE* fp = fopen(file, "wb");
	fwrite(ms.Begin(), ms.Length(), 1, fp);
	fclose(fp);
}

void TilePathFinder::SetDebugCallback(DebugFunc func, void* userdata) {
	debugFunc_ = func;
	debugUserdata_ = userdata;
}

TilePathFinder::PathNode* TilePathFinder::SearchInCircle(int cx, int cz, int depth) {
	for (int i = 1; i <= depth; ++i) {
		if (i <= (int)rangeIndex_.size()) {
			std::vector<IndexPair>* range = rangeIndex_[i - 1];
			for (size_t i = 0; i < range->size(); i++) {
				const IndexPair& pair = (*range)[i];
				PathNode* node = FindNode(cx + pair.x_, cz + pair.z_);
				if (node) {
					if (debugFunc_) {
						debugFunc_(debugUserdata_, node->pos_.x, node->pos_.y);
					}
					if (Movable(node, false)) {
						return node;
					}
				}
			}
		} else {
			int tx = 0;
			int tz = i;
			int d = 3 - 2 * i;
			while (tx <= tz) {
				int range[][2] = { { tx, tz }, { -tx, tz }, { tx, -tz }, { -tx, -tz },
				{ tz, tx }, { -tz, tx }, { tz, -tx }, { -tz, -tx } };
				for (int j = 0; j < 8; ++j) {
					int xOffset = range[j][0];
					int zOffset = range[j][1];
					PathNode* node = FindNode(cx + xOffset, cz + zOffset);
					if (node) {
						if (debugFunc_) {
							debugFunc_(debugUserdata_, node->pos_.x, node->pos_.y);
						}
						if (Movable(node, false)) {
							return node;
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
	}
	return NULL;
}

TilePathFinder::PathNode* TilePathFinder::SearchInReactangle(int cx, int cz, int depth) {
	for (int r = 1; r <= depth; ++r) {
		int xMin = cx - r;
		int xMax = cx + r;
		int zMin = cz - r;
		int zMax = cz + r;

		int zRange[2] = { zMin, zMax };

		for (int i = 0; i < 2; i++) {
			int z = zRange[i];

			if (z < 0 || z >= height_) {
				continue;
			}

			for (int x = xMin; x <= xMax; x++) {

				if (x < 0 || x >= width_) {
					continue;
				}

				PathNode* node = FindNode(x, z);
				if (node) {
					if (debugFunc_) {
						debugFunc_(debugUserdata_, node->pos_.x, node->pos_.y);
					}
					if (Movable(node, false)) {
						return node;
					}
				}
			}
		}

		int xRange[2] = { xMin, xMax };

		for (int i = 0; i < 2; i++) {
			int x = xRange[i];
			if (x < 0 || x >= width_) {
				continue;
			}

			for (int z = zMin; z < zMax; z++) {
				if (z < 0 || z >= height_) {
					continue;
				}
				PathNode* node = FindNode(x, z);
				if (node) {
					if (debugFunc_) {
						debugFunc_(debugUserdata_, node->pos_.x, node->pos_.y);
					}
					if (Movable(node, false)) {
						return node;
					}
				}
			}
		}
	}

	return NULL;
}

void TilePathFinder::BuildPath(PathNode* node, PathNode* from, SmoothType smooth, std::vector<const Math::Vector2*>& list) {
	std::vector<const Math::Vector2*> path;

	PathNode* parent = node->parent_;
	assert(parent);

	path.push_back(&node->pos_);

	int dx0 = DX(node, parent);
	int dz0 = DZ(node, parent);

	node = parent;
	while (node) {
		if (node != from) {
			parent = node->parent_;
			if (parent) {
				int dx1 = DX(node, parent);
				int dz1 = DZ(node, parent);
				if (dx0 != dx1 || dz0 != dz1) {
					path.push_back(&node->pos_);
					dx0 = dx1;
					dz0 = dz1;
				}
			} else {
				path.push_back(&node->pos_);
				break;
			}
		} else {
			path.push_back(&node->pos_);
		}
		node = node->parent_;
	}

	if (smooth == SmoothType::None || path.size() == 2) {
		list.resize(path.size());
		std::reverse_copy(path.begin(), path.end(), list.begin());
		return;
	}

	if (smooth & SmoothType::Head) {
		int tail = path.size() - 1;
		const Math::Vector2* node = path[tail];
		list.push_back(node);
		for (int i = tail; i >= 2;) {
			int start = i;
			int last = start - 1;
			for (int j = i - 2; j >= 0; j--) {
				const Math::Vector2* startNode = path[start];
				const Math::Vector2* checkNode = path[j];

				Math::Vector2 result;
				Raycast(*startNode, *checkNode, true, &result, NULL, false);
				if ((int)result.x == (int)checkNode->x &&
					(int)result.y == (int)checkNode->y) {
					last = j;
				} else {
					break;
				}
			}

			node = path[last];
			list.push_back(node);
			i = last;
		}

		node = path[0];
		list.push_back(node);
	} else {
		std::swap(list, path);
		std::reverse(list.begin(), list.end());
	}

	if (smooth & SmoothType::Tail) {
		std::vector<const Math::Vector2*> smoothPath;
		for (int head = 0; head < list.size() - 1; head++) {
			smoothPath.push_back(list[head]);
			for (int tail = list.size() - 1; tail > head + 1; tail--) {
				const Math::Vector2* headNode = list[head];
				const Math::Vector2* tailNode = list[tail];
				Math::Vector2 result;
				Raycast(*headNode, *tailNode, true, &result, NULL, false);
				if ((int)result.x == (int)tailNode->x && (int)result.y == (int)tailNode->y) {
					head = tail - 1;
					break;
				}
			}
		}
		smoothPath.push_back(list[list.size() - 1]);
		std::swap(list, smoothPath);
	}
}

//#define TILE_NEW_VERSION
int TilePathFinder::Find(const Math::Vector2& from, const Math::Vector2& to, SmoothType smooth, std::vector<const Math::Vector2*>& list, bool check_close, float estimate) {
	PathNode* fromNode = FindNode((int)from.x, (int)from.y);
	if (!fromNode || IsBlock(fromNode)) {
		fromNode = SearchInCircle((int)from.x, (int)from.y, kSearchDepth);
		if (!fromNode) {
			return STATUS_START_ERROR;
		}
	}

	PathNode* toNode = FindNode((int)to.x, (int)to.y);
	if (!toNode || IsBlock(toNode)) {
		toNode = SearchInCircle((int)to.x, (int)to.y, kSearchDepth);
		if (!toNode) {
			return STATUS_OVER_ERROR;
		}
	}

	if (fromNode == toNode) {
		return STATUS_SAME_POINT;
	}

	if (Raycast(from, to, false, NULL, NULL, false) == 0) {
		list.push_back(&fromNode->pos_);
		list.push_back(&toNode->pos_);
		return STATUS_OK;
	}

	Status status = STATUS_CANNOT_REACH;

	mh_push(&openList_, &fromNode->elt_);
	PathNode* node = NULL;
	while ((node = (PathNode*)mh_pop(&openList_)) != NULL) {
		PathNode* next = closeList_.next_;
		next->prev_ = node;
		node->next_ = next;
		node->prev_ = &closeList_;
		closeList_.next_ = node;
		if (node == toNode) {
			BuildPath(node, fromNode, smooth, list);
			status = STATUS_OK;
			break;
		}

		PathNode* link = NULL;
		FindNeighbors(node, &link, check_close);
		while (link) {
			PathNode* nei = link;
			int nG = node->G + NeighborEstimate(node, nei);
			if (nei->next_) {
				if (nG < nei->G) {
					PathNode* prev = nei->prev_;
					PathNode* next = nei->next_;
					prev->next_ = next;
					next->prev_ = prev;
					nei->prev_ = nei->next_ = NULL;
					nei->parent_ = node;
					nei->G = nG;
					nei->H = GoalEstimate(nei, toNode, estimate);
					nei->F = nei->G + nei->H;
					mh_push(&openList_, &nei->elt_);
					if (debugFunc_) {
						debugFunc_(debugUserdata_, nei->pos_.x, nei->pos_.y);
					}
				}
			} else {
				if (mh_elt_has_init(&nei->elt_)) {
					if (nG < nei->G) {
						nei->G = nG;
						nei->F = nei->G + nei->H;
						nei->parent_ = node;
						mh_adjust(&openList_, &nei->elt_);
					}
				} else {
					nei->parent_ = node;
					nei->G = nG;
					nei->H = GoalEstimate(nei, toNode, estimate);
					nei->F = nei->G + nei->H;
					mh_push(&openList_, &nei->elt_);
					if (debugFunc_) {
						debugFunc_(debugUserdata_, nei->pos_.x, nei->pos_.y);
					}
				}
			}

			link = nei->nei_;
			nei->nei_ = NULL;
		}
	}
	Reset();
	return status;
}

int TilePathFinder::Find(int x0, int z0, int x1, int z1, SmoothType smooth, std::vector<const Math::Vector2*>& list, float estimate) {
	const Math::Vector2 from(x0, z0);
	const Math::Vector2 to(x1, z1);
	return Find(from, to, smooth, list, estimate);
}

int TilePathFinder::Raycast(const Math::Vector2& from, const Math::Vector2& to, bool ignore, Math::Vector2* result, Math::Vector2* stop, bool use_breshemham) {
	return Raycast((int)from.x, (int)from.y, (int)to.x, (int)to.y, ignore, result, stop, use_breshemham);
}

int TilePathFinder::Raycast(int x0, int z0, int x1, int z1, bool ignore, Math::Vector2* result, Math::Vector2* stop, bool use_breshemham) {
	if (use_breshemham) {
		int rx = x0;
		int rz = z0;

		int dx = abs(x1 - x0);
		int dz = abs(z1 - z0);

		bool steep = dz > dx ? 1 : 0;
		if (steep) {
			std::swap(x0, z0);
			std::swap(x1, z1);
			std::swap(dx, dz);
		}

		int xstep = x0 < x1 ? 1 : -1;
		int zstep = z0 < z1 ? 1 : -1;

		for (int dt = dz - dx; xstep == 1 ? x0 <= x1 : x1 <= x0;) {
			int x, z;
			if (steep) {
				x = z0;
				z = x0;
			} else {
				x = x0;
				z = z0;
			}

			if (!Movable(x, z, ignore)) {
				stop->x = x;
				stop->y = z;
				break;
			}
			rx = x;
			rz = z;

			if (debugFunc_) {
				debugFunc_(debugUserdata_, x, z);
			}

			if (dt >= 0) {
				z0 += zstep;
				dt -= dx;
			}
			x0 += xstep;
			dt += dz;
		}

		if (result) {
			result->x = floor(rx);
			result->y = floor(rz);
		}

		if (stop) {
			stop->x = floor(stop->x);
			stop->y = floor(stop->y);
		}

		if (rx == x1 && rz == z1) {
			return 0;
		}
		return -1;
	} else {
		float fx0 = x0 + 0.5f;
		float fz0 = z0 + 0.5f;
		float fx1 = x1 + 0.5f;
		float fz1 = z1 + 0.5f;
		float rx = fx0;
		float rz = fz0;

		if (fx0 == fx1) {
			for (float z = z0; z0 < z1 ? z <= z1 : z >= z1; z0 < z1 ? z++ : z--) {
				if (debugFunc_) {
					debugFunc_(debugUserdata_, x0, z);
				}

				if (!Movable(x0, z, ignore)) {
					if (stop) {
						stop->x = x0;
						stop->y = z;
					}
					break;
				} else {
					rx = x0;
					rz = z;
				}
			}
		} else {
			float slope = (fz1 - fz0) / (fx1 - fx0);
			if (fabs(slope) < 1) {
				float inc = fx1 >= fx0 ? 1 : -1;
				for (float x = fx0; fx1 >= fx0 ? x <= fx1 : x >= fx1; x += inc) {
					float z = slope * (x - fx0) + fz0;

					if (debugFunc_) {
						debugFunc_(debugUserdata_, x, z);
					}

					if (!Movable(x, z, ignore)) {
						if (stop) {
							stop->x = x;
							stop->y = z;
						}
						break;
					} else {
						rx = x;
						rz = z;
					}
				}
			} else {
				float inc = fz1 >= fz0 ? 1 : -1;
				for (float z = fz0; fz1 >= fz0 ? z <= fz1 : z >= fz1; z += inc) {
					float x = (z - fz0) / slope + fx0;

					if (debugFunc_) {
						debugFunc_(debugUserdata_, x, z);
					}

					if (!Movable(x, z, ignore)) {
						if (stop) {
							stop->x = x;
							stop->y = z;
						}
						break;
					} else {
						rx = x;
						rz = z;
					}
				}
			}
		}

		if (result) {
			result->x = floor(rx);
			result->y = floor(rz);
		}

		if (stop) {
			stop->x = floor(stop->x);
			stop->y = floor(stop->y);
		}

		if ((int)rx == x1 && (int)rz == z1) {
			return 0;
		}
		return -1;

	}
}

void TilePathFinder::Random(Math::Vector2& result) {
	if (!nonblock_) {
		nonblock_ = new int[nonblockCount_];
		int index = 0;
		for (int i = 0; i < width_ * height_; ++i) {
			PathNode* node = &node_[i];
			if (!IsBlock(node)) {
				nonblock_[index++] = i;
			}
		}
	}

	PathNode* node = &node_[nonblock_[Math::Rand(0, nonblockCount_ - 1)]];
	result = node->pos_;
}

int TilePathFinder::RandomInCircle(int cx, int cz, int radius, Math::Vector2& result) {
	if (radius < 1) {
		radius = 1;
	}

	int index = 0;
	PathNode* link = NULL;

	if (radius <= (int)circleIndex_.size() && circleIndex_[radius - 1]) {
		std::vector<IndexPair>* range = circleIndex_[radius - 1];
		for (size_t i = 0; i < range->size(); i++) {
			const IndexPair& pair = (*range)[i];
			PathNode* node = FindNode(cx + pair.x_, cz + pair.z_);
			if (node && !IsBlock(node)) {
				if (debugFunc_) {
					debugFunc_(debugUserdata_, node->pos_.x, node->pos_.y);
				}
				node->nei_ = link;
				link = node;
				index++;
			}
		}
	} else {
		int tx = 0;
		int tz = radius;
		int d = 3 - 2 * radius;
		while (tx <= tz) {
			for (int zi = tx; zi <= tz; zi++) {
				int range[][2] = { { tx, zi }, { -tx, zi }, { tx, -zi }, { -tx, -zi },
				{ zi, tx }, { -zi, tx }, { zi, -tx }, { -zi, -tx } };

				for (int i = 0; i < 8; i++) {
					int xOffset = range[i][0];
					int zOffset = range[i][1];
					PathNode* node = FindNode(cx + xOffset, cz + zOffset);
					if (node) {
						if (!IsBlock(node)) {
							if (!node->nei_) {
								node->nei_ = link;
								link = node;
								index++;
								if (debugFunc_) {
									debugFunc_(debugUserdata_, node->pos_.x, node->pos_.y);
								}
							}
						}
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

	if (index == 0) {
		return -1;
	}

	int target = Math::Rand(1, index);

	int i = 0;
	while (link) {
		PathNode* tmp = link;
		link = tmp->nei_;
		tmp->nei_ = NULL;
		i++;
		if (target == i) {
			result = tmp->pos_;
		}
	}
	return 0;
}

void TilePathFinder::Reset() {
	PathNode* node = closeList_.next_;
	while (node != &closeList_) {
		PathNode* tmp = node;
		node = tmp->next_;
		tmp->Reset();
	}
	closeList_.prev_ = &closeList_;
	closeList_.next_ = &closeList_;
	mh_clear(&openList_, ClearHeap);
}

void TilePathFinder::FindNeighbors(PathNode* node, PathNode** link, bool check_close) {
	for (int i = 0; i < 8; ++i) {
		int x = (int)node->pos_.x + kDirection[i][0];
		int z = (int)node->pos_.y + kDirection[i][1];
		PathNode* nei = FindNode(x, z);

		if (nei) {
			if (check_close) {
				if (!IsBlock(nei)) {
					nei->nei_ = (*link);
					(*link) = nei;
				}
			} else if (!nei->next_) {
				if (!IsBlock(nei)) {
					nei->nei_ = (*link);
					(*link) = nei;
				}
			}
		}
	}
}
