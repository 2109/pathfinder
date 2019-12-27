#ifndef NAV_MESH_FINDER_H
#define NAV_MESH_FINDER_H

extern "C" {
#include "minheap-adapter.h"
}
#include "Math.h"
#include "Vector3.h"

struct VertexAux {
	int index_;
	Math::Vector3 pos_;
	Math::Vector3 center_;
};

static inline bool AngleCompare(const VertexAux& lhs, const VertexAux& rhs) {
	Math::Vector3 v0 = lhs.center_ - lhs.pos_;
	float angle0 = atan2(v0.z, v0.x) * 180 / PI;
	if (angle0 < 0) {
		angle0 += 360;
	}

	Math::Vector3 v1 = rhs.center_ - rhs.pos_;
	float angle1 = atan2(v1.z, v1.x) * 180 / PI;
	if (angle1 < 0) {
		angle1 += 360;
	}

	return angle0 < angle1;
}

struct NavNode {
	mh_elt_t elt_;
	NavNode* next_;
	int id_;
	std::vector<int> vertice_;
	std::vector<int> border_;
	int size_;

	Math::Vector3 center_;

	int mask_;

	bool record_;

	double area_;

	double G;
	double H;
	double F;

	bool close_;

	NavNode* linkParent_;
	int linkBorder_;

	int reserve_;
	Math::Vector3 pos_;

	inline void Init(int id, int size) {
		id_ = id;
		size_ = size;
		vertice_.resize(size_);
		border_.resize(size_);
		mask_ = 0;
		area_ = 0;
		Reset();
	}

	inline void Reset() {
		mh_init(&elt_);
		linkParent_ = NULL;
		linkBorder_ = -1;
		F = G = H = 0;
		next_ = NULL;
		record_ = false;
		close_ = false;
	}
};

struct NavBorder {
	int id_;
	int node_[2];
	int a_;
	int b_;
	int opposite_;
	Math::Vector3 center_;
};

#define NAV_DEBUG_TILE
struct NavTile {
	std::vector<int> node_;
	int centerNode_;
	Math::Vector3 center_;
#ifdef NAV_DEBUG_TILE
	Math::Vector3 pos_[4];
#endif
	NavTile() {
		centerNode_ = -1;
	}
};

struct NavMesh {
	std::vector<Math::Vector3> vertice_;
	std::vector<NavBorder*> border_;
	std::vector<NavNode> node_;

	std::vector<NavTile> tile_;
	uint32_t tileUnit_;
	uint32_t tileWidth_;
	uint32_t tileHeight_;

	double area_;

	Math::Vector3 lt_;
	Math::Vector3 br_;

	uint32_t width_;
	uint32_t height_;

	NavMesh() {
		tileUnit_ = 0;
		tileWidth_ = 0;
		tileHeight_ = 0;
		area_ = 0;
		width_ = 0;
		height_ = 0;
	}

	~NavMesh() {
		for (size_t i = 0; i < border_.size(); ++i) {
			NavBorder* border = border_[i];
			delete border;
		}
	}
};

class NavPathFinder {
public:
	typedef void(*DebugFunc)(void* ud, int id);
	typedef void(*DebugOverlapFunc)(void* ud, std::vector<const Math::Vector3*>& poly);
	typedef bool(*SearchFunc)(void* ud, NavPathFinder* finder, NavTile* tile);

	static const int kMaskMax = 8;
	static const int kDefaultPath = 8;
	static const int kSearchDepth = 8;
	static const int kGrate = 1;
	static const int kHrate = 2;

	struct IndexPair {
		int x_;
		int z_;
		IndexPair(int x, int z) {
			x_ = x;
			z_ = z;
		}
	};

public:
	NavPathFinder();

	virtual ~NavPathFinder();

	inline void SetMask(int index, int enable) {
		if ((size_t)index >= mask_.size()) {
			mask_.resize(mask_.size() * 2);
		}
		mask_[index] = enable;
	}

	inline void SetDebugNodeFunc(DebugFunc func, void* userdata) {
		debugNodeFunc_ = func;
		debugNodeUserdata_ = userdata;
	}

	inline void SetDebugTileFunc(DebugFunc func, void* userdata) {
		debugTileFunc_ = func;
		debugTileUserdata_ = userdata;
	}

	inline void SetDebugOverlapFunc(DebugOverlapFunc func, void* userdata) {
		debugOverlapFunc_ = func;
		debugOverlapUserdata_ = userdata;
	}

	inline NavNode* GetNode(int nodeId) {
		if (nodeId < 0 || (size_t)nodeId >= mesh_->node_.size()) {
			return NULL;
		}
		return &mesh_->node_[nodeId];
	}

	inline NavBorder* GetBorder(int borderId) {
		if (borderId < 0 || (size_t)borderId >= mesh_->border_.size()) {
			return NULL;
		}
		return mesh_->border_[borderId];
	}

	inline NavTile* GetTile(int tileId) {
		if (tileId < 0 || (size_t)tileId >= mesh_->tile_.size()) {
			return NULL;
		}
		return &mesh_->tile_[tileId];
	}

	inline NavTile* GetTile(int xIndex, int zIndex) {
		if (xIndex < 0 || xIndex >= (int)mesh_->tileWidth_) {
			return NULL;
		}
		if (zIndex < 0 || zIndex >= (int)mesh_->tileHeight_) {
			return NULL;
		}
		int index = xIndex + zIndex * mesh_->tileWidth_;
		return GetTile(index);
	}

	inline NavTile* GetTile(const Math::Vector3& pos) {
		if (pos.x < mesh_->lt_.x || pos.x > mesh_->br_.x) {
			return NULL;
		}
		if (pos.z < mesh_->lt_.z || pos.z > mesh_->br_.z) {
			return NULL;
		}

		int xIndex = (pos.x - mesh_->lt_.x) / mesh_->tileUnit_;
		int zIndex = (pos.z - mesh_->lt_.z) / mesh_->tileUnit_;

		return GetTile(xIndex, zIndex);
	}

	inline bool GetMask(int index) {
		if (index < 0 || (size_t)index >= mask_.size()) {
			return NULL;
		}
		return mask_[index];
	}

	void CreateTile(uint32_t unit);

	void CreateMesh(float** vert, int vertTotal, int** index, int indexTotal);

	static NavPathFinder* LoadMesh(const char* file);

	static NavPathFinder* LoadMeshEx(const char* file);

	void LoadTile(const char* file);

	void LoadTileEx(const char* file);

	void SerializeMesh(const char* file);

	void SerializeTile(const char* file);

	void SearchTile(const Math::Vector3& pos, int depth, SearchFunc func, void* ud);

	NavNode* SearchNode(const Math::Vector3& pos, int depth = 1);

	Math::Vector3* SearchInRectangle(const Math::Vector3& pos, int depth, int* centerNode);

	Math::Vector3* SearchInCircle(const Math::Vector3& pos, int depth, int* centerNode);

	int Find(const Math::Vector3& from, const Math::Vector3& to, std::vector<const Math::Vector3*>& list);

	int Raycast(const Math::Vector3& from, const Math::Vector3& to, Math::Vector3& stop);

	bool Movable(const Math::Vector3& pos, float fix, float* dtOffset);

	Math::Vector3 RandomMovable(int nodeId);

	bool RandomInCircle(Math::Vector3& pos, const Math::Vector3& center, int radius);

	float GetHeight(const Math::Vector3& pos);

	void GetOverlapPoly(std::vector<Math::Vector3>& poly, int nodeId, std::vector<const Math::Vector3*>& result);

public:
	NavNode* NextBorder(NavNode* node, const Math::Vector3& wp, int& linkBorder);

	bool UpdateWp(const Math::Vector3& from, NavNode*& node, NavNode*& parent, Math::Vector3& ptWp, int& linkBorder, Math::Vector3& lhsPt, Math::Vector3& rhsPt, Math::Vector3& lhsVt, Math::Vector3& rhsVt, NavNode*& lhsNode, NavNode*& rhsNode);

	void BuildPath(const Math::Vector3& from, const Math::Vector3& to, NavNode* node, std::vector<const Math::Vector3*>& list);

	inline double CalcNodeArea(NavNode* node) {
		std::vector<const Math::Vector3*> vertice;
		for (int i = 0; i < (int)node->vertice_.size(); ++i) {
			const Math::Vector3* pos = &mesh_->vertice_[node->vertice_[i]];
			vertice.push_back(pos);
		}
		return Math::CalcPolyArea(vertice);
	}

	inline double CalcTriangleArea(int a, int b, int c) {
		std::vector<const Math::Vector3*> vertice = { &mesh_->vertice_[a], &mesh_->vertice_[b], &mesh_->vertice_[c] };
		return Math::CalcPolyArea(vertice);
	}

	void SortNode(NavNode* node);

	NavBorder* SearchBorder(std::vector<std::unordered_map<int, int>>& searcher, int lhs, int rhs);

	NavBorder* AddBorder(int lhs, int rhs);

	inline void BorderLinkNode(NavBorder* border, int id) {
		if (border->node_[0] == -1) {
			border->node_[0] = id;
		} else if (border->node_[1] == -1) {
			border->node_[1] = id;
		} else {
			assert(0);
		}
	}

	double Dot2Node(const Math::Vector3& pos, int nodeId);

	bool InsidePoly(std::vector<int>& index, const Math::Vector3& pos);

	bool InsideNode(int nodeId, const Math::Vector3& pos);

	void GetLink(NavNode* node, NavNode** link);

	static inline void ClearHeap(mh_elt_t* elt) {
		NavNode* node = (NavNode*)elt;
		node->Reset();
	}

	inline void Reset() {
		NavNode* node = closeList_;
		while (node) {
			NavNode* tmp = node;
			node = tmp->next_;
			tmp->Reset();
		}
		closeList_ = NULL;
		mh_clear(&openList_, ClearHeap);
		pathIndex_ = 0;
	}

	inline void PathAdd(Math::Vector3& pos) {
		if ((size_t)pathIndex_ >= path_.size()) {
			path_.resize(path_.size() * 2);
		}
		path_[pathIndex_++] = pos;
	}

	inline void PathCollect(std::vector<const Math::Vector3*>& list) {
		for (int i = pathIndex_ - 1; i >= 0; --i) {
			list.push_back(&path_[i]);
		}
	}

public:
	NavMesh* mesh_;
	mh_t openList_;
	NavNode* closeList_;
	int pathIndex_;
	std::vector<Math::Vector3> path_;
	std::vector<bool> mask_;

	std::vector<std::vector<IndexPair>*> circleIndex_;

	DebugFunc debugNodeFunc_;
	void* debugNodeUserdata_;

	DebugFunc debugTileFunc_;
	void* debugTileUserdata_;

	DebugOverlapFunc debugOverlapFunc_;
	void* debugOverlapUserdata_;
};

#endif