#include "Intersect.h"
#include "Math.h"
#include "NavMeshFinder.h"


static inline int NodeCmp(mh_elt_t* lhs, mh_elt_t* rhs) {
	return ((NavNode*)lhs)->F > ((NavNode*)rhs)->F;
}

static inline float NeighborEstimate(NavNode* src, NavNode* dst) {
	double dx = src->pos_.x - dst->pos_.x;
	double dy = 0;
	double dz = src->pos_.z - dst->pos_.z;
	return sqrt(dx * dx + dy * dy + dz * dz) * NavPathFinder::kGrate;
}

static inline float GoalEstimate(NavNode* src, const Math::Vector3& dst) {
	double dx = src->center_.x - dst.x;
	double dy = 0;
	double dz = src->center_.z - dst.z;
	return sqrt(dx * dx + dy * dy + dz * dz) * NavPathFinder::kHrate;
}

NavPathFinder::NavPathFinder() {
	path_index_ = 0;
	path_.resize(kDefaultPath);

	mh_ctor(&open_list_, NodeCmp);
	close_list_ = NULL;

	mask_.resize(kMaskMax);
	for (int i = 0; i < kMaskMax; ++i) {
		SetMask(i, 1);
	}
	SetMask(0, 1);

	debug_node_func_ = NULL;
	debug_node_userdata_ = NULL;

	debug_tile_func_ = NULL;
	debug_tile_userdata_ = NULL;

	debug_overlap_func_ = NULL;
	debug_overlap_userdata_ = NULL;

	mesh_ = NULL;

	circle_index_.resize(kSearchDepth);
	for (int i = 1; i <= (int)circle_index_.size(); ++i) {
		std::vector<IndexPair>* pair_info = new std::vector<IndexPair>();
		circle_index_[i - 1] = pair_info;
		std::unordered_map<int, std::unordered_set<int>> record;

		int tx = 0;
		int tz = i;
		int d = 3 - 2 * i;
		while (tx <= tz) {
			int range[][2] = { { tx, tz }, { -tx, tz }, { tx, -tz }, { -tx, -tz },
			{ tz, tx }, { -tz, tx }, { tz, -tx }, { -tz, -tx } };
			for (int j = 0; j < 8; ++j) {
				int x_offset = range[j][0];
				int z_offset = range[j][1];
				auto itr = record.find(x_offset);
				if (itr != record.end()) {
					std::unordered_set<int>& set = itr->second;
					auto it = set.find(z_offset);
					if (it == set.end()) {
						set.insert(z_offset);
						pair_info->push_back(IndexPair(x_offset, z_offset));
					}
				} else {
					record[x_offset] = std::unordered_set<int>();
					record[x_offset].insert(z_offset);
					pair_info->push_back(IndexPair(x_offset, z_offset));
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

	reactangle_index_.resize(kSearchDepth);
	for (int i = 1; i <= kSearchDepth; i++) {
		int size = i * 2 + 1;
		std::vector<IndexPair>* vt = new std::vector<IndexPair>();
		vt->resize(size * size);
		int y = size / 2, x = size / 2;//从中心点开始
		for (int i = 1; i <= size * size; i++) {
			if (x <= size - y - 1 && x >= y) {
				IndexPair& pairs = (*vt)[i - 1];
				pairs.x_ = x;
				pairs.z_ = y;
				x++;
			} else if (x > size - y - 1 && x > y) {
				IndexPair& pairs = (*vt)[i - 1];
				pairs.x_ = x;
				pairs.z_ = y;
				y++;
			} else if (x > size - y - 1 && x <= y) {
				IndexPair& pairs = (*vt)[i - 1];
				pairs.x_ = x;
				pairs.z_ = y;
				x--;
			} else if (x <= size - y - 1 && x < y) {
				IndexPair& pairs = (*vt)[i - 1];
				pairs.x_ = x;
				pairs.z_ = y;
				y--;
			}
		}
		reactangle_index_[i - 1] = vt;
	}
}

NavPathFinder::~NavPathFinder() {
	mh_dtor(&open_list_);

	for (int i = 0; i < kSearchDepth; ++i) {
		std::vector<IndexPair>* index = circle_index_[i];
		if (index) {
			delete index;
		}
	}

	for (int i = 0; i < kSearchDepth; ++i) {
		std::vector<IndexPair>* index = reactangle_index_[i];
		if (index) {
			delete index;
		}
	}

	if (mesh_) {
		delete mesh_;
	}
}

int NavPathFinder::Find(const Math::Vector3& src, const Math::Vector3& dst, std::vector<const Math::Vector3*>& list) {
	NavNode* src_node = SearchNode(src);
	NavNode* dst_node = SearchNode(dst);
	if (!src_node || !dst_node) {
		return -1;
	}
	if (src_node == dst_node) {
		PathAdd((Math::Vector3&)dst);
		PathAdd((Math::Vector3&)src);
		PathCollect(list);
		return 0;
	}

	src_node->pos_ = src;
	mh_push(&open_list_, &src_node->elt_);
	NavNode* node = NULL;

	while ((node = (NavNode*)mh_pop(&open_list_)) != NULL) {
		node->close_ = true;
		node->next_ = close_list_;
		close_list_ = node;

		if (node == dst_node) {
			//BuildPath(src, dst, node, list);
			BuildPathUseFunnel(src, dst, node, list);
			Reset();
			return 0;
		}

		NavNode* link_node = NULL;
		GetLink(node, &link_node);

		while (link_node) {
			if (mh_elt_has_init(&link_node->elt_)) {
				double nG = node->G + NeighborEstimate(node, link_node);
				if (nG < link_node->G) {
					link_node->G = nG;
					link_node->F = link_node->G + link_node->H;
					link_node->link_parent_ = node;
					link_node->link_edge_ = link_node->reserve_;
					mh_adjust(&open_list_, &link_node->elt_);
				}
			} else {
				link_node->G = node->G + NeighborEstimate(node, link_node);
				link_node->H = GoalEstimate(link_node, dst);
				link_node->F = link_node->G + link_node->H;
				link_node->link_parent_ = node;
				link_node->link_edge_ = link_node->reserve_;
				mh_push(&open_list_, &link_node->elt_);

				if (debug_node_func_) {
					debug_node_func_(debug_node_userdata_, link_node->id_);
				}
			}
			NavNode* tmp = link_node;
			link_node = link_node->next_;
			tmp->next_ = NULL;
		}
	}
	Reset();
	return -1;
}

int NavPathFinder::Raycast(const Math::Vector3& src, const Math::Vector3& dst, Math::Vector3& stop) {
	NavNode* node = SearchNode(src);
	if (!node) {
		return -1;
	}

	Math::Vector3 pt0 = src;
	const Math::Vector3 pt1 = dst;

	int index = 0;
	Math::Vector3 vt10 = pt1 - pt0;
	while (node) {
		if (InsideNode(node->id_, dst)) {
			stop = dst;
			return 0;
		}

		bool cross = false;
		for (int i = 0; i < node->size_; ++i) {
			NavEdge* edge = GetEdge(node->edge_[i]);
			const Math::Vector3& pt3 = mesh_->vertice_[edge->a_];
			const Math::Vector3& pt4 = mesh_->vertice_[edge->b_];
			Math::Vector3 vt30 = pt3 - pt0;
			Math::Vector3 vt40 = pt4 - pt0;
			if (Math::InsideVector(vt30, vt40, vt10)) {
				int next = -1;
				if (edge->node_[0] != -1) {
					if (edge->node_[0] == node->id_) {
						next = edge->node_[1];
					} else {
						next = edge->node_[0];
					}
				} else {
					assert(edge->node_[1] == node->id_);
				}

				if (next == -1) {
					GetIntersectPoint(pt3, pt4, pt1, pt0, stop);
					return 0;
				} else {
					NavNode* next_node = GetNode(next);
					if (GetMask(next_node->mask_) == 0) {
						GetIntersectPoint(pt3, pt4, pt1, pt0, stop);
						return 0;
					}

					if (debug_node_func_) {
						debug_node_func_(debug_node_userdata_, next);
					}

					cross = true;
					node = next_node;
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

NavNode* NavPathFinder::NextEdge(NavNode* node, const Math::Vector3& wp, int& link_edge) {
	Math::Vector3 vt0, vt1;
	link_edge = node->link_edge_;
	while (link_edge != -1) {
		NavEdge* edge = GetEdge(link_edge);
		vt0 = mesh_->vertice_[edge->a_] - wp;
		vt1 = mesh_->vertice_[edge->b_] - wp;
		if ((vt0.x == 0 && vt0.z == 0) || (vt1.x == 0 && vt1.z == 0)) {
			node = node->link_parent_;
			link_edge = node->link_edge_;
		} else {
			break;
		}
	}
	if (link_edge != -1) {
		return node;
	}

	return NULL;
}

bool NavPathFinder::UpdateWp(const Math::Vector3& src, NavNode*& node, NavNode*& parent, Math::Vector3& pt_wp, int& link_edge, Math::Vector3& lpt, Math::Vector3& rpt, Math::Vector3& lvt, Math::Vector3& rvt, NavNode*& lnode, NavNode*& rnode) {
	PathAdd(pt_wp);

	node = NextEdge(node, pt_wp, link_edge);
	if (node == NULL) {
		PathAdd((Math::Vector3&)src);
		return false;
	}

	NavEdge* edge = GetEdge(link_edge);

	lpt = mesh_->vertice_[edge->a_];
	rpt = mesh_->vertice_[edge->b_];

	lvt = lpt - pt_wp;
	rvt = rpt - pt_wp;

	parent = node->link_parent_;

	lnode = parent;
	rnode = parent;

	return true;
}

void NavPathFinder::BuildPath(const Math::Vector3& src, const Math::Vector3& dst, NavNode* node, std::vector<const Math::Vector3*>& list) {
	PathAdd((Math::Vector3&)dst);

	int link_edge = node->link_edge_;
	NavEdge* edge = GetEdge(link_edge);

	Math::Vector3 pt_wp = dst;

	Math::Vector3 lpt = mesh_->vertice_[edge->a_];
	Math::Vector3 rpt = mesh_->vertice_[edge->b_];

	Math::Vector3 lvt = lpt - pt_wp;
	Math::Vector3 rvt = rpt - pt_wp;

	NavNode* lnode = node->link_parent_;
	NavNode* rnode = node->link_parent_;

	NavNode* parent = node->link_parent_;
	while (parent) {
		int link_edge = parent->link_edge_;
		if (link_edge == -1) {
			Math::Vector3 target = src - pt_wp;

			float lcross = Math::CrossY(lvt, target);
			float rcross = Math::CrossY(rvt, target);

			if (lcross < 0 && rcross > 0) {
				PathAdd((Math::Vector3&)src);
				break;
			} else {
				if (lcross > 0 && rcross > 0) {
					pt_wp = lpt;
					if (!UpdateWp(src, lnode, parent, pt_wp, link_edge, lpt, rpt, lvt, rvt, lnode, rnode)) {
						break;
					}
					continue;
				} else if (lcross < 0 && rcross < 0) {
					pt_wp = rpt;
					if (!UpdateWp(src, rnode, parent, pt_wp, link_edge, lpt, rpt, lvt, rvt, lnode, rnode)) {
						break;
					}
					continue;
				}
				break;
			}

		}

		edge = GetEdge(link_edge);

		Math::Vector3& lpt_tmp = mesh_->vertice_[edge->a_];
		Math::Vector3& rpt_tmp = mesh_->vertice_[edge->b_];

		Math::Vector3 lvt_tmp = lpt_tmp - pt_wp;
		Math::Vector3 rvt_tmp = rpt_tmp - pt_wp;

		float l_cross_a = Math::CrossY(lvt, lvt_tmp);
		float l_cross_b = Math::CrossY(rvt, lvt_tmp);

		float r_cross_a = Math::CrossY(lvt, rvt_tmp);
		float r_cross_b = Math::CrossY(rvt, rvt_tmp);

		uint8_t mask = 0;
		if (l_cross_a < 0 && l_cross_b > 0) {
			lnode = parent->link_parent_;
			lpt = lpt_tmp;
			lvt = lpt - pt_wp;
			mask |= 0x01;
		}

		if (r_cross_a < 0 && r_cross_b > 0) {
			rnode = parent->link_parent_;
			rpt = rpt_tmp;
			rvt = rpt - pt_wp;
			mask |= 0x02;
		}

		if (mask == 0x03) {
			parent = parent->link_parent_;
			continue;
		}

		if (l_cross_a > 0 && l_cross_b > 0 && r_cross_a > 0 && r_cross_b > 0) {
			pt_wp = lpt;
			if (!UpdateWp(src, lnode, parent, pt_wp, link_edge, lpt, rpt, lvt, rvt, lnode, rnode)) {
				break;
			}
			continue;
		}

		if (l_cross_a < 0 && l_cross_b < 0 && r_cross_a < 0 && r_cross_b < 0) {
			pt_wp = rpt;
			if (!UpdateWp(src, rnode, parent, pt_wp, link_edge, lpt, rpt, lvt, rvt, lnode, rnode)) {
				break;
			}
			continue;
		}
		parent = parent->link_parent_;
	}

	PathCollect(list);
}

struct Funnel {
	enum SIDE {
		eLEFT,
		eSAME,
		eRIGHT
	};
	NavPathFinder* finder_;
	Math::Vector3 pivot_;
	Math::Vector3 lvt_;
	Math::Vector3 rvt_;
	Math::Vector3 lpt_;
	Math::Vector3 rpt_;
	NavNode* lnode_;
	NavNode* rnode_;
	Funnel(NavPathFinder* finder) {
		finder_ = finder;
	}

	void Set(const Math::Vector3& pivot, NavNode* node) {
		lnode_ = node;
		rnode_ = node;
		NavEdge* edge = finder_->GetEdge(node->link_edge_);
		NavMesh* mesh = finder_->GetMesh();
		pivot_ = pivot;
		lpt_ = mesh->vertice_[edge->a_];
		rpt_ = mesh->vertice_[edge->b_];
		lvt_ = lpt_ - pivot_;
		rvt_ = rpt_ - pivot_;
	}

	void SetLeft(NavNode* node, const Math::Vector3& pt) {
		lnode_ = node;
		lpt_ = pt;
		lvt_ = lpt_ - pivot_;
	}

	NavNode* UpdateLeft() {
		int link_edge;
		NavNode* node = finder_->NextEdge(lnode_, pivot_, link_edge);
		if (node == NULL) {
			return NULL;
		}
		finder_->PathAdd((Math::Vector3&)lpt_);
		Set(lpt_, node);
		return node;
	}

	void SetRight(NavNode* node, const Math::Vector3& pt) {
		rnode_ = node;
		rpt_ = pt;
		rvt_ = rpt_ - pivot_;
	}

	NavNode* UpdateRight() {
		int link_edge;
		NavNode* node = finder_->NextEdge(rnode_, pivot_, link_edge);
		if (node == NULL) {
			return NULL;
		}
		finder_->PathAdd((Math::Vector3&)rpt_);
		Set(rpt_, node);
		return node;
	}

	SIDE SideLeft(const Math::Vector3& pt) { 
		Math::Vector3 tmp = pt - pivot_;
		float cross = Math::CrossY(lvt_, tmp);
		if (cross < 0) {
			return eRIGHT;
		} else if (cross > 0) {
			return eLEFT;
		}
		return eSAME;
	}

	SIDE SideRight(const Math::Vector3& pt) {
		Math::Vector3 tmp = pt - pivot_;
		float cross = Math::CrossY(rvt_, tmp);
		if (cross < 0) {
			return eRIGHT;
		} else if (cross > 0) {
			return eLEFT;
		}
		return eSAME;
	}
};

void NavPathFinder::BuildPathUseFunnel(const Math::Vector3& src, const Math::Vector3& dst, NavNode* node, std::vector<const Math::Vector3*>& list) {
	PathAdd((Math::Vector3&)dst);
	Funnel funnel(this);
	funnel.Set(dst, node);

	node = node->link_parent_;
	while (node) {
		int link_edge = node->link_edge_;
		NavEdge* edge = GetEdge(link_edge);
		if (!edge) {
			Funnel::SIDE side_l = funnel.SideLeft(src);
			Funnel::SIDE side_r = funnel.SideRight(src);
			if (side_l == Funnel::eLEFT && side_r == Funnel::eLEFT) {
				node = funnel.UpdateLeft();
				if (node == NULL) {
					PathAdd((Math::Vector3&)src);
					break;
				}
				node = node->link_parent_;
				continue;

			} else if (side_l == Funnel::eRIGHT && side_r == Funnel::eRIGHT) {
				node = funnel.UpdateRight();
				if (node == NULL) {
					PathAdd((Math::Vector3&)src);
					break;
				}
				node = node->link_parent_;
				continue;
			}
			PathAdd((Math::Vector3&)src);
			break;
		}

		Math::Vector3& lpt_tmp = mesh_->vertice_[edge->a_];
		Math::Vector3& rpt_tmp = mesh_->vertice_[edge->b_];

		Funnel::SIDE lvt_side_l = funnel.SideLeft(lpt_tmp);
		Funnel::SIDE lvt_side_r = funnel.SideLeft(rpt_tmp);
		Funnel::SIDE rvt_side_l = funnel.SideRight(lpt_tmp);
		Funnel::SIDE rvt_side_r = funnel.SideRight(rpt_tmp);

		uint8_t mask = 0;
		//下一条边的左点在漏斗里，更新为新的左点,同时记录当前左点的多边形
		if (lvt_side_l != Funnel::eLEFT && rvt_side_l != Funnel::eRIGHT) {
			funnel.SetLeft(node, lpt_tmp);
			mask |= 0x01;
		}
		//下一条边的右点在漏斗里，更新为新的右点,同时记录当前右点的多边形
		if (lvt_side_r != Funnel::eLEFT && rvt_side_r != Funnel::eRIGHT) {
			funnel.SetRight(node, rpt_tmp);
			mask |= 0x02;
		}

		if (mask == 0x03) {
			//如果左右两点同时更新了，直接跳到一个多边形
			node = node->link_parent_;
			continue;
		}
		
		if (lvt_side_l == Funnel::eLEFT && rvt_side_l == Funnel::eLEFT &&
			lvt_side_r == Funnel::eLEFT && rvt_side_r == Funnel::eLEFT) {
			//左右两点都在漏斗左边，更新漏斗，以左点为新的拐点,同时以左点的当时多边形为基础，一直找到一边不共边的多边形
			node = funnel.UpdateLeft();
			if (node == NULL) {
				PathAdd((Math::Vector3&)src);
				break;
			}
			node = node->link_parent_;
			continue;
		}

		if (lvt_side_l == Funnel::eRIGHT && rvt_side_l == Funnel::eRIGHT &&
			lvt_side_r == Funnel::eRIGHT && rvt_side_r == Funnel::eRIGHT) {
			//左右两点都在漏斗右边，更新漏斗，以右点为新的拐点,同时以右点的当时多边形为基础，一直找到一边不共边的多边形
			node = funnel.UpdateRight();
			if (node == NULL) {
				PathAdd((Math::Vector3&)src);
				break;
			}
			node = node->link_parent_;
			continue;
		}
		node = node->link_parent_;
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

bool NavPathFinder::InsideNode(int node_id, const Math::Vector3& pos) {
	NavNode* node = &mesh_->node_[node_id];
	return InsidePoly(node->vertice_, pos);
}

void NavPathFinder::GetLink(NavNode* node, NavNode** link) {
	for (int i = 0; i < node->size_; i++) {
		int edge_id = node->edge_[i];
		NavEdge* edge = GetEdge(edge_id);

		int linked = -1;
		if (edge->node_[0] == node->id_) {
			linked = edge->node_[1];
		} else {
			linked = edge->node_[0];
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
			tmp->reserve_ = edge->inverse_;
			tmp->pos_ = edge->center_;
		}
	}
}
