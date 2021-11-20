#include "intersect.h"
#include "mathex.h"
#include "navmesh_finder.h"


static inline int NodeCmp(min_elt_t* lhs, min_elt_t* rhs) {
	return ((NavNode*)lhs)->F > ((NavNode*)rhs)->F;
}

static inline float NeighborEstimate(NavNode* src, NavNode* dst) {
	double dx = src->pos_.x() - dst->pos_.x();
	double dy = 0;
	double dz = src->pos_.z() - dst->pos_.z();
	return sqrt(dx * dx + dy * dy + dz * dz) * NavPathFinder::kGrate;
}

static inline float GoalEstimate(NavNode* src, const Math::Vector3& dst) {
	double dx = src->center_.x() - dst.x();
	double dy = 0;
	double dz = src->center_.z() - dst.z();
	return sqrt(dx * dx + dy * dy + dz * dz) * NavPathFinder::kHrate;
}

NavPathFinder::NavPathFinder() {
	path_index_ = 0;
	path_.resize(kDefaultPath);

	min_heap_ctor_(&open_list_, NodeCmp);

	close_list_.prev_ = &close_list_;
	close_list_.next_ = &close_list_;

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
		int y = size / 2, x = size / 2;//´ÓÖÐÐÄµã¿ªÊ¼
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
	min_heap_dtor_(&open_list_);

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

	if (src_node->area_id_ != dst_node->area_id_) {
		return -1;
	}

	src_node->pos_ = src;
	min_heap_push_(&open_list_, &src_node->elt_);
	NavNode* node = NULL;

	while ((node = (NavNode*)min_heap_pop_(&open_list_)) != NULL) {
		node->Insert(&close_list_);

		if (node == dst_node) {
			BuildPathUseFunnel(src, dst, node, list);
			Reset();
			return 0;
		}

		NavNode* link_node = NULL;
		GetLink(node, &link_node, false);

		while (link_node) {
			double nG = node->G + NeighborEstimate(node, link_node);
			if (link_node->next_) {
				if (nG < link_node->G) {
					link_node->Remove();
					link_node->UpdateParent(node, nG, GoalEstimate(link_node, dst));
					min_heap_push_(&open_list_, &link_node->elt_);
					if (debug_node_func_) {
						debug_node_func_(debug_node_userdata_, link_node->id_);
					}
				}
			} else {
				if (link_node->elt_.index >= 0) {
					if (nG < link_node->G) {
						link_node->UpdateParent(node, nG, link_node->H);
						min_heap_adjust_(&open_list_, &link_node->elt_);
					}
				} else {
					link_node->UpdateParent(node, nG, GoalEstimate(link_node, dst));
					min_heap_push_(&open_list_, &link_node->elt_);
					if (debug_node_func_) {
						debug_node_func_(debug_node_userdata_, link_node->id_);
					}
				}
			}
		
			NavNode* tmp = link_node;
			link_node = link_node->link_;
			tmp->link_ = NULL;
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
		if ((vt0.x() == 0 && vt0.z() == 0) || (vt1.x() == 0 && vt1.z() == 0)) {
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
		float cross = Math::Cross_Y(lvt_, tmp);
		if (cross < 0) {
			return eRIGHT;
		} else if (cross > 0) {
			return eLEFT;
		}
		return eSAME;
	}

	SIDE SideRight(const Math::Vector3& pt) {
		Math::Vector3 tmp = pt - pivot_;
		float cross = Math::Cross_Y(rvt_, tmp);
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
		//ÏÂÒ»Ìõ±ßµÄ×óµãÔÚÂ©¶·Àï£¬¸üÐÂÎªÐÂµÄ×óµã,Í¬Ê±¼ÇÂ¼µ±Ç°×óµãµÄ¶à±ßÐÎ
		if (lvt_side_l != Funnel::eLEFT && rvt_side_l != Funnel::eRIGHT) {
			funnel.SetLeft(node, lpt_tmp);
			mask |= 0x01;
		}
		//ÏÂÒ»Ìõ±ßµÄÓÒµãÔÚÂ©¶·Àï£¬¸üÐÂÎªÐÂµÄÓÒµã,Í¬Ê±¼ÇÂ¼µ±Ç°ÓÒµãµÄ¶à±ßÐÎ
		if (lvt_side_r != Funnel::eLEFT && rvt_side_r != Funnel::eRIGHT) {
			funnel.SetRight(node, rpt_tmp);
			mask |= 0x02;
		}

		if (mask == 0x03) {
			//Èç¹û×óÓÒÁ½µãÍ¬Ê±¸üÐÂÁË£¬Ö±½ÓÌøµ½Ò»¸ö¶à±ßÐÎ
			node = node->link_parent_;
			continue;
		}

		if (lvt_side_l == Funnel::eLEFT && rvt_side_l == Funnel::eLEFT &&
			lvt_side_r == Funnel::eLEFT && rvt_side_r == Funnel::eLEFT) {
			//×óÓÒÁ½µã¶¼ÔÚÂ©¶·×ó±ß£¬¸üÐÂÂ©¶·£¬ÒÔ×óµãÎªÐÂµÄ¹Õµã,Í¬Ê±ÒÔ×óµãµÄµ±Ê±¶à±ßÐÎÎª»ù´¡£¬Ò»Ö±ÕÒµ½Ò»±ß²»¹²±ßµÄ¶à±ßÐÎ
			node = funnel.UpdateLeft();
			if (node == NULL) {
				PathAdd((Math::Vector3&)src);
				break;
			}
		} else if (lvt_side_l == Funnel::eRIGHT && rvt_side_l == Funnel::eRIGHT &&
			lvt_side_r == Funnel::eRIGHT && rvt_side_r == Funnel::eRIGHT) {
			//×óÓÒÁ½µã¶¼ÔÚÂ©¶·ÓÒ±ß£¬¸üÐÂÂ©¶·£¬ÒÔÓÒµãÎªÐÂµÄ¹Õµã,Í¬Ê±ÒÔÓÒµãµÄµ±Ê±¶à±ßÐÎÎª»ù´¡£¬Ò»Ö±ÕÒµ½Ò»±ß²»¹²±ßµÄ¶à±ßÐÎ
			node = funnel.UpdateRight();
			if (node == NULL) {
				PathAdd((Math::Vector3&)src);
				break;
			}
		}
		node = node->link_parent_;
	}

	PathCollect(list);
}

bool NavPathFinder::InsideTriangle(int a, int b, int c, const Math::Vector3& pos) {
	int vert[3] = { a, b, c };
	int sign = 0;
	for (int i = 0; i < 3; ++i) {
		const Math::Vector3& pt0 = mesh_->vertice_[vert[i]];
		const Math::Vector3& pt1 = mesh_->vertice_[vert[(i + 1) % 3]];
		Math::Vector3 vt0 = pos - pt0;
		Math::Vector3 vt1 = pt1 - pt0;
		float cross = Math::Cross_Y(vt0, vt1);
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

bool NavPathFinder::InsidePoly(std::vector<int>& index, const Math::Vector3& pos) {
	int sign = 0;
	int count = index.size();
	for (int i = 0; i < count; ++i) {
		const Math::Vector3& pt0 = mesh_->vertice_[index[i]];
		const Math::Vector3& pt1 = mesh_->vertice_[index[(i + 1) % count]];
		Math::Vector3 vt0 = pos - pt0;
		Math::Vector3 vt1 = pt1 - pt0;
		float cross = Math::Cross_Y(vt0, vt1);
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

void NavPathFinder::GetLink(NavNode* node, NavNode** link, bool check_close) {
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
		if (!check_close && tmp->next_) {
			continue;
		}

		if (GetMask(tmp->mask_)) {
			tmp->link_ = (*link);
			(*link) = tmp;
			tmp->reserve_ = edge->inverse_;
			tmp->pos_ = edge->center_;
		}
	}
}

size_t NavPathFinder::CountMemory() {
	size_t total = 0;
	total += sizeof(*mesh_);
	total += sizeof(Math::Vector3) * mesh_->vertice_.size();
	total += sizeof(NavEdge) * mesh_->edge_.size();
	total += sizeof(NavNode) * mesh_->node_.size();
	for (size_t i = 0; i < mesh_->node_.size(); ++i) {
		NavNode& node = mesh_->node_[i];
		total += sizeof(int) * node.vertice_.size();
		total += sizeof(int) * node.edge_.size();
	}
	total += sizeof(NavTile) * mesh_->tile_.size();
	return total;
}

void NavPathFinder::MakeArea() {
	std::queue<NavNode*> queue;

	std::vector<uint8_t> visited;
	visited.resize(mesh_->node_.size());

	int area_id = 0;
	for (unsigned int i = 0; i < mesh_->node_.size(); i++) {
		if (visited[i] == 0) {
			BFS(area_id++, i, queue, visited);
		}
	}
}

void NavPathFinder::BFS(int area_id, int v, std::queue<NavNode*>& queue, std::vector<uint8_t>& visited) {
	visited[v] = 1;
	queue.push(&mesh_->node_[v]);
	while (!queue.empty()) {
		NavNode* node = queue.front();
		node->area_id_ = area_id;
		queue.pop();

		NavNode* link_node = NULL;
		GetLink(node, &link_node,  true);

		while (link_node) {
			if (link_node->mask_ == node->mask_) {
				if (visited[link_node->id_] == 0) {
					visited[link_node->id_] = 1;
					queue.push(link_node);
				}
			}
			NavNode* tmp = link_node;
			link_node = link_node->link_;
			tmp->link_ = NULL;
		}
	}
}