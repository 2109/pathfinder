
#include "Util.h"
#include "WpPathFinder.h"

static inline int NodeCmp(min_elt_t* lhs, min_elt_t* rhs) {
	return ((WpPathFinder::WpNode*)lhs)->F > ((WpPathFinder::WpNode*)rhs)->F;
}

static inline float NeighborEstimate(WpPathFinder::WpNode* from, WpPathFinder::WpNode* to) {
	return Math::Distance(from->pos_, to->pos_);
}

static inline float GoalEstimate(WpPathFinder::WpNode* from, WpPathFinder::WpNode* to) {
	return Math::Distance(from->pos_, to->pos_);
}

static inline void ClearHeap(min_elt_t* elt) {
	WpPathFinder::WpNode* node = (WpPathFinder::WpNode*)elt;
	node->Reset();
}

WpPathFinder::WpPathFinder(const char* file) {
	MemoryStream ms;
	Util::ReadFile(file, ms);

	ms >> size_;

	node_ = new WpNode[size_];

	for (uint32_t i = 0; i < size_; ++i) {
		WpNode* node = &node_[i];
		uint32_t x, z;
		ms >> x >> z;
		node->pos_.x = x;
		node->pos_.y = z;
	}

	for (uint32_t i = 0; i < size_; ++i) {
		WpNode* node = &node_[i];
		ms >> node->size_;
		if (node->size_) {
			node->link_ = new uint32_t[node->size_];
			for (uint32_t j = 0; j < node->size_; ++j) {
				ms >> node->link_[j];
			}
		}
	}

	min_heap_ctor_(&openList_, NodeCmp);
	closeList_ = NULL;

	debugFunc_ = NULL;
	debugUserdata_ = NULL;
}

WpPathFinder::~WpPathFinder() {
	for (uint32_t i = 0; i < size_; ++i) {
		WpNode* node = &node_[i];
		if (node->link_) {
			delete[] node->link_;
		}
	}
	min_heap_dtor_(&openList_);

	delete[] node_;
}

void WpPathFinder::SetDebugCallback(DebugFunc func, void* userdata) {
	debugFunc_ = func;
	debugUserdata_ = userdata;
}

WpPathFinder::WpNode* WpPathFinder::SearchNode(const Math::Vector2& pos) {
	double dtMin = -1;
	int result;
	for (uint32_t i = 0; i < size_; i++) {
		WpNode* node = &node_[i];
		double dt = Math::Distance(pos, node->pos_);
		if (dtMin == -1 || dt < dtMin) {
			dtMin = dt;
			result = i;
		}
	}
	return &node_[result];
}

void WpPathFinder::BuildPath(WpNode* node, WpNode* from, std::vector<const Math::Vector2*>& list) {
	list.push_back(&node->pos_);

	WpNode* parent = node->parent_;
	assert(parent != NULL);

	node = parent;
	while (node) {
		if (node != from) {
			list.push_back(&node->pos_);
		} else {
			list.push_back(&node->pos_);
			break;
		}
		node = node->parent_;
	}
	std::reverse(list.begin(), list.end());
}

int WpPathFinder::Find(const Math::Vector2& from, const Math::Vector2& to, std::vector<const Math::Vector2*>& list) {
	WpNode* fromNode = SearchNode(from);
	WpNode* toNode = SearchNode(to);
	if (!fromNode || !toNode || fromNode == toNode) {
		return -1;
	}

	min_heap_push_(&openList_, &fromNode->elt_);

	int status = -1;

	WpNode* node = NULL;
	while ((node = (WpNode*)min_heap_pop_(&openList_)) != NULL) {
		node->next_ = closeList_;
		closeList_ = node;
		node->close_ = true;

		if (node == toNode) {
			BuildPath(node, fromNode, list);
			status = 0;
			break;
		}

		WpNode* link = NULL;
		FindNeighbors(node, &link);
		while (link) {
			WpNode* nei = link;
			if (nei->elt_.index >= 0) {
				int nG = node->G + NeighborEstimate(node, nei);
				if (nG < nei->G) {
					nei->G = nG;
					nei->F = nei->G + nei->H;
					nei->parent_ = node;
					min_heap_adjust_(&openList_, &nei->elt_);
				}
			} else {
				nei->parent_ = node;
				nei->G = node->G + NeighborEstimate(node, nei);
				nei->H = GoalEstimate(nei, toNode);
				nei->F = nei->G + nei->H;
				min_heap_push_(&openList_, &nei->elt_);
				if (debugFunc_) {
					debugFunc_(debugUserdata_, nei->pos_);
				}
			}

			link = nei->next_;
			nei->next_ = NULL;
		}
	}
	Reset();
	return status;
}

int WpPathFinder::Find(int x0, int z0, int x1, int z1, std::vector<const Math::Vector2*>& list) {
	const Math::Vector2 from(x0, z0);
	const Math::Vector2 to(x1, z1);
	return Find(from, to, list);
}

void WpPathFinder::Reset() {
	WpNode* node = closeList_;
	while (node) {
		WpNode* tmp = node;
		node = tmp->next_;
		tmp->Reset();
	}
	closeList_ = NULL;
	min_heap_clear_(&openList_, ClearHeap);
}

void WpPathFinder::FindNeighbors(WpNode* node, WpNode** link) {
	for (uint32_t i = 0; i < node->size_; ++i) {
		WpNode* tmp = &node_[node->link_[i]];
		if (tmp->close_) {
			continue;
		}
		tmp->next_ = *link;
		*link = tmp;
	}
}