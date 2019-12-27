
#include "Intersect.h"
#include "Math.h"
#include "NavMeshFinder.h"


static inline int NodeCmp(mh_elt_t* lhs, mh_elt_t* rhs) {
	return ((NavNode*)lhs)->F > ((NavNode*)rhs)->F;
}

static inline float NeighborEstimate(NavNode* from, NavNode* to) {
	double dx = from->pos_.x - to->pos_.x;
	double dy = 0;
	double dz = from->pos_.z - to->pos_.z;
	return sqrt(dx*dx + dy* dy + dz* dz) * NavPathFinder::kGrate;
}

static inline float GoalEstimate(NavNode* from, const Math::Vector3& to) {
	double dx = from->center_.x - to.x;
	double dy = 0;
	double dz = from->center_.z - to.z;
	return sqrt(dx*dx + dy* dy + dz* dz) * NavPathFinder::kHrate;
}

NavPathFinder::NavPathFinder() {
	pathIndex_ = 0;
	path_.resize(kDefaultPath);

	mh_ctor(&openList_, NodeCmp);
	closeList_ = NULL;

	mask_.resize(kMaskMax);
	for (int i = 0; i < kMaskMax; ++i) {
		SetMask(i, 1);
	}
	SetMask(0, 1);

	debugNodeFunc_ = NULL;
	debugNodeUserdata_ = NULL;

	debugTileFunc_ = NULL;
	debugTileUserdata_ = NULL;

	mesh_ = NULL;

	circleIndex_.resize(kSearchDepth);
	for (int i = 1; i <= (int)circleIndex_.size(); ++i) {
		std::vector<IndexPair>* pairInfo = new std::vector<IndexPair>();
		circleIndex_[i - 1] = pairInfo;
		std::unordered_map<int, std::unordered_set<int>> record;

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
}

NavPathFinder::~NavPathFinder() {
	mh_dtor(&openList_);

	for (int i = 0; i < kSearchDepth; ++i) {
		std::vector<IndexPair>* index = circleIndex_[i];
		if (index) {
			delete index;
		}
	}

	if (mesh_) {
		delete mesh_;
	}
}

int NavPathFinder::Find(const Math::Vector3& from, const Math::Vector3& to, std::vector<const Math::Vector3*>& list) {
	NavNode* fromNode = SearchNode(from);
	NavNode* toNode = SearchNode(to);
	if (!fromNode || !toNode) {
		return -1;
	}
	if (fromNode == toNode) {
		PathAdd((Math::Vector3&)to);
		PathAdd((Math::Vector3&)from);
		PathCollect(list);
		return 0;
	}

	fromNode->pos_ = from;
	mh_push(&openList_, &fromNode->elt_);
	NavNode* node = NULL;

	while ((node = (NavNode*)mh_pop(&openList_)) != NULL) {
		node->close_ = true;
		node->next_ = closeList_;
		closeList_ = node;

		if (node == toNode) {
			BuildPath(from, to, node, list);
			Reset();
			return 0;
		}

		NavNode* linkNode = NULL;
		GetLink(node, &linkNode);

		while (linkNode) {
			if (mh_elt_has_init(&linkNode->elt_)) {
				double nG = node->G + NeighborEstimate(node, linkNode);
				if (nG < linkNode->G) {
					linkNode->G = nG;
					linkNode->F = linkNode->G + linkNode->H;
					linkNode->linkParent_ = node;
					linkNode->linkBorder_ = linkNode->reserve_;
					mh_adjust(&openList_, &linkNode->elt_);
				}
			} else {
				linkNode->G = node->G + NeighborEstimate(node, linkNode);
				linkNode->H = GoalEstimate(linkNode, to);
				linkNode->F = linkNode->G + linkNode->H;
				linkNode->linkParent_ = node;
				linkNode->linkBorder_ = linkNode->reserve_;
				mh_push(&openList_, &linkNode->elt_);

				if (debugNodeFunc_) {
					debugNodeFunc_(debugNodeUserdata_, linkNode->id_);
				}
			}
			NavNode* tmp = linkNode;
			linkNode = linkNode->next_;
			tmp->next_ = NULL;
		}
	}
	Reset();
	return 0;
}

int NavPathFinder::Raycast(const Math::Vector3& from, const Math::Vector3& to, Math::Vector3& stop) {
	NavNode* node = SearchNode(from);
	if (!node) {
		return -1;
	}

	Math::Vector3 pt0 = from;
	const Math::Vector3 pt1 = to;

	int index = 0;
	Math::Vector3 vt10 = pt1 - pt0;
	while (node) {
		if (InsideNode(node->id_, to)) {
			stop = to;
			return 0;
		}

		bool cross = false;
		for (int i = 0; i < node->size_; ++i) {
			NavBorder* border = GetBorder(node->border_[i]);
			const Math::Vector3& pt3 = mesh_->vertice_[border->a_];
			const Math::Vector3& pt4 = mesh_->vertice_[border->b_];
			Math::Vector3 vt30 = pt3 - pt0;
			Math::Vector3 vt40 = pt4 - pt0;
			if (Math::InsideVector(vt30, vt40, vt10)) {
				int next = -1;
				if (border->node_[0] != -1) {
					if (border->node_[0] == node->id_) {
						next = border->node_[1];
					} else {
						next = border->node_[0];
					}
				} else {
					assert(border->node_[1] == node->id_);
				}

				if (next == -1) {
					GetIntersectPoint(pt3, pt4, pt1, pt0, stop);
					return 0;
				} else {
					NavNode* nextNode = GetNode(next);
					if (GetMask(nextNode->mask_) == 0) {
						GetIntersectPoint(pt3, pt4, pt1, pt0, stop);
						return 0;
					}

					if (debugNodeFunc_) {
						debugNodeFunc_(debugNodeUserdata_, next);
					}

					cross = true;
					node = nextNode;
					break;
				}
			}
		}

		if (!cross) {
			assert(index == 0);
			pt0 = node->center_;
			vt10 = pt1 - pt0;
		}
	}
	return -1;
}

NavNode* NavPathFinder::NextBorder(NavNode* node, const Math::Vector3& wp, int& linkBorder) {
	Math::Vector3 vt0, vt1;
	linkBorder = node->linkBorder_;
	while (linkBorder != -1) {
		NavBorder* border = GetBorder(linkBorder);
		vt0 = mesh_->vertice_[border->a_] - wp;
		vt1 = mesh_->vertice_[border->b_] - wp;
		if ((vt0.x == 0 && vt0.z == 0) || (vt1.x == 0 && vt1.z == 0)) {
			node = node->linkParent_;
			linkBorder = node->linkBorder_;
		} else {
			break;
		}
	}
	if (linkBorder != -1) {
		return node;
	}

	return NULL;
}

bool NavPathFinder::UpdateWp(const Math::Vector3& from, NavNode*& node, NavNode*& parent, Math::Vector3& ptWp, int& linkBorder, Math::Vector3& lPt, Math::Vector3& rPt, Math::Vector3& lVt, Math::Vector3& rVt, NavNode*& lNode, NavNode*& rNode) {
	PathAdd(ptWp);

	node = NextBorder(node, ptWp, linkBorder);
	if (node == NULL) {
		PathAdd((Math::Vector3&)from);
		return false;
	}

	NavBorder* border = GetBorder(linkBorder);

	lPt = mesh_->vertice_[border->a_];
	rPt = mesh_->vertice_[border->b_];

	lVt = lPt - ptWp;
	rVt = rPt - ptWp;

	parent = node->linkParent_;

	lNode = parent;
	rNode = parent;

	return true;
}

void NavPathFinder::BuildPath(const Math::Vector3& from, const Math::Vector3& to, NavNode* node, std::vector<const Math::Vector3*>& list) {
	PathAdd((Math::Vector3&)to);

	int linkBorder = node->linkBorder_;
	NavBorder* border = GetBorder(linkBorder);

	Math::Vector3 ptWp = to;

	Math::Vector3 lPt = mesh_->vertice_[border->a_];
	Math::Vector3 rPt = mesh_->vertice_[border->b_];

	Math::Vector3 lVt = lPt - ptWp;
	Math::Vector3 rVt = rPt - ptWp;

	NavNode* lNode = node->linkParent_;
	NavNode* rNode = node->linkParent_;

	NavNode* parent = node->linkParent_;
	while (parent) {
		int linkBorder = parent->linkBorder_;
		if (linkBorder == -1) {
			Math::Vector3 target = from - ptWp;

			float lCross = Math::CrossY(lVt, target);
			float rCross = Math::CrossY(rVt, target);

			if (lCross < 0 && rCross > 0) {
				PathAdd((Math::Vector3&)from);
				break;
			} else {
				if (lCross > 0 && rCross > 0) {
					ptWp = lPt;
					if (!UpdateWp(from, lNode, parent, ptWp, linkBorder, lPt, rPt, lVt, rVt, lNode, rNode)) {
						break;
					}
					continue;
				} else if (lCross < 0 && rCross < 0) {
					ptWp = rPt;
					if (!UpdateWp(from, rNode, parent, ptWp, linkBorder, lPt, rPt, lVt, rVt, lNode, rNode)) {
						break;
					}
					continue;
				}
				break;
			}

		}

		border = GetBorder(linkBorder);

		Math::Vector3& lPtTmp = mesh_->vertice_[border->a_];
		Math::Vector3& rPtTmp = mesh_->vertice_[border->b_];

		Math::Vector3 lVtTmp = lPtTmp - ptWp;
		Math::Vector3 rVtTmp = rPtTmp - ptWp;

		float lCrossA = Math::CrossY(lVt, lVtTmp);
		float lCrossB = Math::CrossY(rVt, lVtTmp);

		float rCrossA = Math::CrossY(lVt, rVtTmp);
		float rCrossB = Math::CrossY(rVt, rVtTmp);

		uint8_t mask = 0;
		if (lCrossA < 0 && lCrossB > 0) {
			lNode = parent->linkParent_;
			lPt = lPtTmp;
			lVt = lPt - ptWp;
			mask |= 0x01;
		}

		if (rCrossA < 0 && rCrossB > 0) {
			rNode = parent->linkParent_;
			rPt = rPtTmp;
			rVt = rPt - ptWp;
			mask |= 0x02;
		}

		if (mask == 0x03) {
			continue;
		}

		if (lCrossA > 0 && lCrossB > 0 && rCrossA > 0 && rCrossB > 0) {
			ptWp = lPt;
			if (!UpdateWp(from, lNode, parent, ptWp, linkBorder, lPt, rPt, lVt, rVt, lNode, rNode)) {
				break;
			}
			continue;
		}

		if (lCrossA < 0 && lCrossB < 0 && rCrossA < 0 && rCrossB < 0) {
			ptWp = rPt;
			if (!UpdateWp(from, rNode, parent, ptWp, linkBorder, lPt, rPt, lVt, rVt, lNode, rNode)) {
				break;
			}
			continue;
		}

		parent = parent->linkParent_;
	}

	PathCollect(list);
}

bool NavPathFinder::InsidePoly(std::vector<int>& index, const Math::Vector3& pos) {
	int sign = 0;
	int count = index.size();
	for (int i = 0; i < count; ++i) {
		const Math::Vector3& pt0 = mesh_->vertice_[index[i]];
		const Math::Vector3& pt1 = mesh_->vertice_[index[(i + 1) % count]];
		Math::Vector3 vt0 = pos - pt0;
		Math::Vector3 vt1 = pt1 - pt0;
		float cross = Math::CrossY(vt0, vt1);
		if (cross == 0) {
			continue;
		}
		if (sign == 0) {
			sign = cross > 0 ? 1 : -1;
		} else {
			if (sign == 1 && cross < 0) {
				return false;
			} else if (sign == -1 && cross > 0) {
				return false;
			}
		}
	}
	return true;
}

bool NavPathFinder::InsideNode(int nodeId, const Math::Vector3& pos) {
	NavNode* node = &mesh_->node_[nodeId];
	return InsidePoly(node->vertice_, pos);
}

void NavPathFinder::GetLink(NavNode* node, NavNode** link) {
	for (int i = 0; i < node->size_; i++) {
		int borderId = node->border_[i];
		NavBorder* border = GetBorder(borderId);

		int linked = -1;
		if (border->node_[0] == node->id_) {
			linked = border->node_[1];
		} else {
			linked = border->node_[0];
		}

		if (linked == -1) {
			continue;
		}

		NavNode* tmp = GetNode(linked);
		if (tmp->close_) {
			continue;
		}

		if (GetMask(tmp->mask_)) {
			tmp->next_ = (*link);
			(*link) = tmp;
			tmp->reserve_ = border->opposite_;
			tmp->pos_ = border->center_;
		}
	}
}
