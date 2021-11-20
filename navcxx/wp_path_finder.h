#ifndef WP_PATH_FINDER_H
#define WP_PATH_FINDER_H
extern "C" {
#include "minheap-internal.h"
}
#include "mathex.h"
// #include "Vector2.h"


class WpPathFinder {
public:
	typedef void(*DebugFunc)(void* ud, const Math::Vector2& pos);

	struct WpNode {
		min_elt_t elt_;
		WpNode* next_;
		WpNode* parent_;
		Math::Vector2 pos_;
		uint32_t* link_;
		uint32_t size_;
		float G;
		float H;
		float F;
		bool close_;
		WpNode() {
			link_ = NULL;
			size_ = 0;
			Reset();
		}

		void Reset() {
			min_heap_elem_init_(&elt_);
			next_ = parent_ = NULL;
			G = H = F = 0;
			close_ = false;
		}
	};

public:
	WpPathFinder(const char* file);

	virtual ~WpPathFinder();

	void SetDebugCallback(DebugFunc func, void* userdata);

	WpNode* SearchNode(const Math::Vector2& pos);

	void BuildPath(WpNode* node, WpNode* from, std::vector<const Math::Vector2*>& list);

	int Find(const Math::Vector2& from, const Math::Vector2& to, std::vector<const Math::Vector2*>& list);

	int Find(int x0, int z0, int x1, int z1, std::vector<const Math::Vector2*>& list);

protected:
	void Reset();

	void FindNeighbors(WpNode* node, WpNode** link);

public:
	WpNode* node_;
	uint32_t size_;
	min_heap_t openList_;
	WpNode* closeList_;

	DebugFunc debugFunc_;
	void* debugUserdata_;
};

#endif