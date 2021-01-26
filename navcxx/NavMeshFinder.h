#ifndef NAV_MESH_FINDER_H
#define NAV_MESH_FINDER_H

extern "C" {
#include "minheap-adapter.h"
}
#include "Math.h"
#include "Vector3.h"
struct NavNode {
	mh_elt_t elt_;
	NavNode* next_;
	int id_;
	std::vector<int> vertice_;
	std::vector<int> edge_;
	int size_;

	Math::Vector3 center_;

	int mask_;

	bool record_;

	double area_;

	double G;
	double H;
	double F;

	bool close_;

	NavNode* link_parent_;
	int link_edge_;

	int reserve_;
	Math::Vector3 pos_;

	inline void Init(int id, int size) {
		id_ = id;
		size_ = size;
		vertice_.resize(size_);
		edge_.resize(size_);
		mask_ = 0;
		area_ = 0;
		Reset();
	}

	inline void Reset() {
		mh_init(&elt_);
		link_parent_ = NULL;
		link_edge_ = -1;
		F = G = H = 0;
		next_ = NULL;
		record_ = false;
		close_ = false;
	}
};

struct NavEdge {
	int id_;
	int node_[2];
	int a_;
	int b_;
	int inverse_;
	Math::Vector3 center_;
};

#define NAV_DEBUG_TILE
struct NavTile {
	std::vector<int> node_;
	int center_node_;
	Math::Vector3 center_;
#ifdef NAV_DEBUG_TILE
	Math::Vector3 pos_[4];
#endif
	NavTile() {
		center_node_ = -1;
	}
};

struct NavMesh {
	std::vector<Math::Vector3> vertice_;
	std::vector<NavEdge*> edge_;
	std::vector<NavNode> node_;

	std::vector<NavTile> tile_;
	uint32_t tile_unit_;
	uint32_t tile_width_;
	uint32_t tile_height_;

	double area_;

	Math::Vector3 lt_;
	Math::Vector3 br_;

	uint32_t width_;
	uint32_t height_;

	NavMesh() {
		tile_unit_ = 0;
		tile_width_ = 0;
		tile_height_ = 0;
		area_ = 0;
		width_ = 0;
		height_ = 0;
	}

	~NavMesh() {
		for (size_t i = 0; i < edge_.size(); ++i) {
			NavEdge* edge = edge_[i];
			delete edge;
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
		IndexPair() {
			x_ = z_ = 0;
		}
		IndexPair(int x, int z) {
			x_ = x;
			z_ = z;
		}
	};

	struct VertexAux {
		int index_;
		Math::Vector3 pos_;
		Math::Vector3 center_;
	};


public:
	NavPathFinder();

	virtual ~NavPathFinder();

	inline NavMesh* GetMesh() {
		return mesh_;
	}

	inline void SetMask(int index, int enable) {
		if ((size_t)index >= mask_.size()) {
			mask_.resize(mask_.size() * 2);
		}
		mask_[index] = enable;
	}

	inline void SetDebugNodeFunc(DebugFunc func, void* userdata) {
		debug_node_func_ = func;
		debug_node_userdata_ = userdata;
	}

	inline void SetDebugTileFunc(DebugFunc func, void* userdata) {
		debug_tile_func_ = func;
		debug_tile_userdata_ = userdata;
	}

	inline void SetDebugOverlapFunc(DebugOverlapFunc func, void* userdata) {
		debug_overlap_func_ = func;
		debug_overlap_userdata_ = userdata;
	}

	inline NavNode* GetNode(int node_id) {
		if (node_id < 0 || (size_t)node_id >= mesh_->node_.size()) {
			return NULL;
		}
		return &mesh_->node_[node_id];
	}

	inline NavEdge* GetEdge(int edge_id) {
		if (edge_id < 0 || (size_t)edge_id >= mesh_->edge_.size()) {
			return NULL;
		}
		return mesh_->edge_[edge_id];
	}

	inline NavTile* GetTile(int tile_id) {
		if (tile_id < 0 || (size_t)tile_id >= mesh_->tile_.size()) {
			return NULL;
		}
		return &mesh_->tile_[tile_id];
	}

	inline NavTile* GetTile(int x_index, int z_index) {
		if (x_index < 0 || x_index >= (int)mesh_->tile_width_) {
			return NULL;
		}
		if (z_index < 0 || z_index >= (int)mesh_->tile_height_) {
			return NULL;
		}
		int index = x_index + z_index * mesh_->tile_width_;
		return GetTile(index);
	}

	inline NavTile* GetTile(const Math::Vector3& pos) {
		if (pos.x < mesh_->lt_.x || pos.x > mesh_->br_.x) {
			return NULL;
		}
		if (pos.z < mesh_->lt_.z || pos.z > mesh_->br_.z) {
			return NULL;
		}

		int x_index = (pos.x - mesh_->lt_.x) / mesh_->tile_unit_;
		int z_index = (pos.z - mesh_->lt_.z) / mesh_->tile_unit_;

		return GetTile(x_index, z_index);
	}

	inline bool GetMask(int index) {
		if (index < 0 || (size_t)index >= mask_.size()) {
			return false;
		}
		return mask_[index];
	}

	void CreateTile(uint32_t unit);

	void CreateMesh(float** vert, int vert_total, int** index, int index_total);

	static NavPathFinder* LoadMesh(const char* file);

	static NavPathFinder* LoadMeshEx(const char* file);

	void LoadTile(const char* file);

	void LoadTileEx(const char* file);

	void SerializeMesh(const char* file);

	void SerializeTile(const char* file);

	void SearchTile(const Math::Vector3& pos, int depth, SearchFunc func, void* ud);

	NavNode* SearchNode(const Math::Vector3& pos, int depth = 1);

	Math::Vector3* SearchInRectangle(const Math::Vector3& pos, int depth, int* center_node);

	Math::Vector3* SearchInCircle(const Math::Vector3& pos, int depth, int* center_node);

	int Find(const Math::Vector3& src, const Math::Vector3& dst, std::vector<const Math::Vector3*>& list);

	int Raycast(const Math::Vector3& src, const Math::Vector3& dst, Math::Vector3& stop);

	bool Movable(const Math::Vector3& pos, float fix, float* dt_offset);

	Math::Vector3 RandomMovable(int node_id);

	bool RandomInCircle(Math::Vector3& pos, const Math::Vector3& center, int radius);

	float GetHeight(const Math::Vector3& pos);

	void GetOverlapPoly(std::vector<Math::Vector3>& poly, int node_id, std::vector<const Math::Vector3*>& result);

	size_t CountMemory();

	inline void PathAdd(Math::Vector3& pos) {
		if ((size_t)path_index_ >= path_.size()) {
			path_.resize(path_.size() * 2);
		}
		path_[path_index_++] = pos;
	}

	inline void PathCollect(std::vector<const Math::Vector3*>& list) {
		for (int i = path_index_ - 1; i >= 0; --i) {
			list.push_back(&path_[i]);
		}
	}
public:
	inline double CountNodeArea(NavNode* node) {
		std::vector<const Math::Vector3*> vertice;
		for (int i = 0; i < (int)node->vertice_.size(); ++i) {
			const Math::Vector3* pos = &mesh_->vertice_[node->vertice_[i]];
			vertice.push_back(pos);
		}
		return Math::CalcPolyArea(vertice);
	}

	inline double CountTriangleArea(int a, int b, int c) {
		std::vector<const Math::Vector3*> vertice = { &mesh_->vertice_[a], &mesh_->vertice_[b], &mesh_->vertice_[c] };
		return Math::CalcPolyArea(vertice);
	}

	inline void EdgeLinkNode(NavEdge* edge, int id) {
		if (edge->node_[0] == -1) {
			edge->node_[0] = id;
		} else if (edge->node_[1] == -1) {
			edge->node_[1] = id;
		} else {
			assert(0);
		}
	}

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

	static inline void ClearHeap(mh_elt_t* elt) {
		NavNode* node = (NavNode*)elt;
		node->Reset();
	}

	inline void Reset() {
		NavNode* node = close_list_;
		while (node) {
			NavNode* tmp = node;
			node = tmp->next_;
			tmp->Reset();
		}
		close_list_ = NULL;
		mh_clear(&open_list_, ClearHeap);
		path_index_ = 0;
	}

	NavNode* NextEdge(NavNode* node, const Math::Vector3& wp, int& link_edge);

	bool UpdateWp(const Math::Vector3& src, NavNode*& node, NavNode*& parent, Math::Vector3& ptWp, int& link_edge, Math::Vector3& lpt, Math::Vector3& rpt, Math::Vector3& lvt, Math::Vector3& rvt, NavNode*& lnode, NavNode*& rnode);

	void BuildPath(const Math::Vector3& src, const Math::Vector3& dst, NavNode* node, std::vector<const Math::Vector3*>& list);

	void BuildPathUseFunnel(const Math::Vector3& src, const Math::Vector3& dst, NavNode* node, std::vector<const Math::Vector3*>& list);

	void SortNode(NavNode* node);

	NavEdge* SearchEdge(std::vector<std::unordered_map<int, int>>& searcher, int lhs, int rhs);

	NavEdge* AddEdge(int lhs, int rhs);

	double Dot2Node(const Math::Vector3& pos, int node_id);

	bool InsidePoly(std::vector<int>& index, const Math::Vector3& pos);

	bool InsideNode(int node_id, const Math::Vector3& pos);

	void GetLink(NavNode* node, NavNode** link);

public:
	NavMesh*                             mesh_;
	mh_t                                 open_list_;
	NavNode*                             close_list_;
	int                                  path_index_;
	std::vector<Math::Vector3>           path_;
	std::vector<bool>                    mask_;

	std::vector<std::vector<IndexPair>*> circle_index_;
	std::vector<std::vector<IndexPair>*> reactangle_index_;

	DebugFunc                            debug_node_func_;
	void*                                debug_node_userdata_;

	DebugFunc                            debug_tile_func_;
	void*                                debug_tile_userdata_;

	DebugOverlapFunc                     debug_overlap_func_;
	void*                                debug_overlap_userdata_;
};

#endif