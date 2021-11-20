#include "Intersect.h"
#include "Math.h"
#include "NavMeshFinder.h"

struct SearchAux {
	NavNode** link_;
};

struct RandomAux {
	NavNode** link_;
	float area_;
};

static bool TileChoose(void* ud, NavPathFinder* finder, NavTile* tile) {
	SearchAux* aux = (SearchAux*)ud;
	for (int k = 0; k < (int)tile->node_.size(); k++) {
		NavNode* tmp = finder->GetNode(tile->node_[k]);
		if (!tmp->next_) {
			tmp->next_ = *aux->link_;
			*aux->link_ = tmp;
		}
	}
	return true;
}

static bool TileLink(void* ud, NavPathFinder* finder, NavTile* tile) {
	RandomAux* aux = (RandomAux*)ud;
	for (int k = 0; k < (int)tile->node_.size(); k++) {
		NavNode* tmp = finder->GetNode(tile->node_[k]);
		if (!tmp->next_) {
			tmp->next_ = *aux->link_;
			*aux->link_ = tmp;
			aux->area_ += tmp->area_;
		}
	}
	return true;
}

void NavPathFinder::SearchTile(const Math::Vector3& pos, int depth, SearchFunc func, void* ud) {
	if (mesh_->tile_.size() == 0) {
		return;
	}

	int x_index = (pos[0] - mesh_->lt_[0]) / mesh_->tile_unit_;
	int z_index = (pos[2] - mesh_->lt_[2]) / mesh_->tile_unit_;

	if (depth <= kSearchDepth) {
		int xmin = x_index - depth;
		int zmin = z_index - depth;
		std::vector<IndexPair>* indexes = reactangle_index_[depth - 1];
		for (int i = 0; i < indexes->size(); i++) {
			IndexPair& pair = (*indexes)[i];
			int x = xmin + pair.x_;
			int z = zmin + pair.z_;
			if (x < 0 || x >= (int)mesh_->tile_width_) {
				continue;
			}
			if (z < 0 || z >= (int)mesh_->tile_height_) {
				continue;
			}
			int index = x + z * mesh_->tile_width_;
			NavTile* tile = &mesh_->tile_[index];

			if (debug_tile_func_) {
				debug_tile_func_(debug_tile_userdata_, index);
			}

			if (!func(ud, this, tile)) {
				return;
			}
		}
	} else {

		for (int r = 1; r <= depth; ++r) {
			int x_min = x_index - r;
			int x_max = x_index + r;
			int z_min = z_index - r;
			int z_max = z_index + r;

			int z_range[2] = { z_min, z_max };

			for (int i = 0; i < 2; i++) {
				int z = z_range[i];

				if (z < 0 || z >= (int)mesh_->tile_height_) {
					continue;
				}

				for (int x = x_min; x <= x_max; x++) {

					if (x < 0 || x >= (int)mesh_->tile_width_) {
						continue;
					}

					int index = x + z * mesh_->tile_width_;
					NavTile* tile = &mesh_->tile_[index];

					if (debug_tile_func_) {
						debug_tile_func_(debug_tile_userdata_, index);
					}

					if (!func(ud, this, tile)) {
						return;
					}
				}
			}

			int x_range[2] = { x_min, x_max };

			for (int i = 0; i < 2; i++) {
				int x = x_range[i];
				if (x < 0 || x >= (int)mesh_->tile_width_) {
					continue;
				}

				for (int z = z_min; z < z_max; z++) {
					if (z < 0 || z >= (int)mesh_->tile_height_) {
						continue;
					}
					int index = x + z * mesh_->tile_width_;
					NavTile* tile = &mesh_->tile_[index];

					if (debug_tile_func_) {
						debug_tile_func_(debug_tile_userdata_, index);
					}

					if (!func(ud, this, tile)) {
						return;
					}
				}
			}
		}
	}
}

int NavPathFinder::SearchInRectangle(const Math::Vector3& pos, Math::Vector3* out, int depth, float* out_dt) {
	NavNode* link = NULL;
	SearchAux aux;
	aux.link_ = &link;

	float dt_min = -1;
	int node_id = -1;
	Math::Vector3 p;
	Math::Vector3* ptr = NULL;
	if (out) {
		ptr = &p;
	}
	SearchTile(pos, depth, TileChoose, &aux);
	while (link) {
		NavNode* ori = link;
		double dt = Dot2Node(pos, ori->id_, ptr);
		if (dt_min < 0 || dt_min > dt) {
			dt_min = dt;
			node_id = ori->id_;
			if (out) {
				*out = p;
			}
		}
		if (debug_node_func_) {
			debug_node_func_(debug_node_userdata_, ori->id_);
		}
		link = link->next_;
		ori->next_ = NULL;
	}
	if (out_dt) {
		*out_dt = dt_min;
	}
	return node_id;
}

int NavPathFinder::SearchInCircle(const Math::Vector3& pos, Math::Vector3* out, int depth, float* out_dt) {
	NavNode* link = NULL;

	int x_index = (pos[0] - mesh_->lt_[0]) / mesh_->tile_unit_;
	int z_index = (pos[2] - mesh_->lt_[2]) / mesh_->tile_unit_;

	for (int i = 1; i <= depth; ++i) {
		if (i <= (int)circle_index_.size()) {
			std::vector<IndexPair>* range = circle_index_[i - 1];
			for (size_t i = 0; i < range->size(); i++) {
				const IndexPair& pair = (*range)[i];
				int x = x_index + pair.x_;
				int z = z_index + pair.z_;
				NavTile* tile = GetTile(x, z);
				if (tile) {
					if (debug_tile_func_) {
						debug_tile_func_(debug_tile_userdata_, x + z * mesh_->tile_width_);
					}
					for (int k = 0; k < (int)tile->node_.size(); k++) {
						NavNode* tmp = GetNode(tile->node_[k]);
						if (!tmp->next_) {
							tmp->next_ = link;
							link = tmp;
						}
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
					int x_offset = range[j][0];
					int z_offset = range[j][1];
					int x = x_index + x_offset;
					int z = z_index + z_offset;
					NavTile* tile = GetTile(x, z);
					if (tile) {
						if (debug_tile_func_) {
							debug_tile_func_(debug_tile_userdata_, x + z * mesh_->tile_width_);
						}
						for (int k = 0; k < (int)tile->node_.size(); k++) {
							NavNode* tmp = GetNode(tile->node_[k]);
							if (!tmp->next_) {
								tmp->next_ = link;
								link = tmp;
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
	}

	float dt_min = -1;
	int node_id = -1;
	Math::Vector3 p;
	Math::Vector3* ptr = NULL;
	if (out) {
		ptr = &p;
	}
	while (link) {
		NavNode* ori = link;
		double dt = Dot2Node(pos, ori->id_, ptr);
		if (dt_min < 0 || dt_min > dt) {
			dt_min = dt;
			node_id = ori->id_;
			if (out) {
				*out = p;
			}
		}
		if (debug_node_func_) {
			debug_node_func_(debug_node_userdata_, ori->id_);
		}
		link = link->next_;
		ori->next_ = NULL;
	}
	if (out_dt) {
		*out_dt = dt_min;
	}
	return node_id;
}

NavNode* NavPathFinder::SearchNode(const Math::Vector3& pos, int depth) {
	NavTile* tile = GetTile(pos);
	if (!tile) {
		return NULL;
	}

	int node_id = -1;
	float height = GetHeight(pos, &node_id);
	if (height >= 0) {
		return &mesh_->node_[node_id];
	}

	double dt_min = DBL_MAX;
	int nearest = -1;
	for (size_t i = 0; i < tile->node_.size(); ++i) {
		int node_id = tile->node_[i];
		if (debug_node_func_) {
			debug_node_func_(debug_node_userdata_, node_id);
		}
		double dt = Dot2Node(pos, node_id);
		if (dt_min > dt) {
			dt_min = dt;
			nearest = node_id;
		}
	}

	if (nearest != -1) {
		return &mesh_->node_[nearest];
	}

	if (depth > 0) {
		int node_id = SearchInRectangle(pos, NULL, depth, NULL);
		if (node_id >= 0) {
			return &mesh_->node_[node_id];
		}
	}
	return NULL;
}

bool NavPathFinder::Movable(const Math::Vector3& pos, float fix, float* dt_offset) {
	if (dt_offset) {
		*dt_offset = 0;
	}

	if (pos[0] < mesh_->lt_[0] || pos[0] > mesh_->br_[0] || pos[2] < mesh_->lt_[2] || pos[2] > mesh_->br_[2]) {
		return false;
	}

	NavNode* node = NULL;

	int x_index = (pos[0] - mesh_->lt_[0]) / mesh_->tile_unit_;
	int z_index = (pos[2] - mesh_->lt_[2]) / mesh_->tile_unit_;
	int index = x_index + z_index * mesh_->tile_width_;

	NavTile* tile = &mesh_->tile_[index];
	for (int i = 0; i < (int)tile->node_.size(); i++) {
		if (InsideNode(tile->node_[i], pos)) {
			node = &mesh_->node_[tile->node_[i]];
			break;
		}
	}

	if (node) {
		if (GetMask(node->mask_) == 1) {
			return true;
		}
		return false;
	}

	if (fix <= 0.1f) {
		return false;
	}
	float dt_min = -1;
	SearchInRectangle(pos, NULL, 1, &dt_min);

	if (dt_offset) {
		*dt_offset = dt_min;
	}
	if (dt_min > 0 && dt_min <= fix) {
		return true;
	}

	return false;
}

double NavPathFinder::Dot2Node(const Math::Vector3& pos, int node_id, Math::Vector3* p) {
	NavNode* node = &mesh_->node_[node_id];
	double min = DBL_MAX;
	Math::Vector3 point;
	Math::Vector3* ptr = NULL;
	if (p) {
		ptr = &point;
	}
	for (int i = 0; i < node->size_; ++i) {
		const Math::Vector3& pt0 = mesh_->vertice_[node->vertice_[i]];
		const Math::Vector3& pt1 = mesh_->vertice_[node->vertice_[(i + 1) % node->size_]];

		double dt = Math::DistancePointToSegment(pt0, pt1, pos, ptr);
		if (dt < min) {
			min = dt;
			if (p) {
				*p = point;
			}
		}
	}
	return min;
}

Math::Vector3 NavPathFinder::RandomMovable(int node_id) {
	int random_node = -1;

	if (node_id >= 0 && node_id < (int)mesh_->node_.size()) {
		random_node = node_id;
	} else {
		double weight = mesh_->area_ * (Math::Rand(0, 10000) / 10000.0f);
		double tmp = 0.0f;
		for (int i = 0; i < (int)mesh_->node_.size(); i++) {
			tmp += mesh_->node_[i].area_;
			if (weight <= tmp) {
				random_node = i;
				break;
			}
		}
	}

	NavNode* node = &mesh_->node_[random_node];

	std::vector<const Math::Vector3*> vertice;
	for (int i = 0; i < (int)node->vertice_.size(); ++i) {
		const Math::Vector3* pos = &mesh_->vertice_[node->vertice_[i]];
		vertice.push_back(pos);
	}

	return Math::RandomInPoly(vertice, node->area_);
}

bool NavPathFinder::RandomInCircle(Math::Vector3& pos, const Math::Vector3& center, int radius) {
	int depth = radius / mesh_->tile_unit_;
	if (depth <= 0) {
		depth = 1;
	}
	NavNode* link = NULL;
	RandomAux aux;
	aux.link_ = &link;
	aux.area_ = 0;

	SearchTile(center, depth, TileLink, &aux);

	int node_id = -1;
	float random_weight = Math::Rand(0.0f, aux.area_);
	float weight = 0;
	while (link) {
		weight += link->area_;
		if (random_weight <= weight && node_id == -1) {
			node_id = link->id_;
		}
		NavNode* ori = link;
		link = link->next_;
		ori->next_ = NULL;
	}

	std::vector<Math::Vector3> rectangle = {
		Math::Vector3(center[0] - radius, 0.0f, center[2] + radius),
		Math::Vector3(center[0] + radius, 0.0f, center[2] + radius),
		Math::Vector3(center[0] + radius, 0.0f, center[2] - radius),
		Math::Vector3(center[0] - radius, 0.0f, center[2] - radius) };

	std::vector<const Math::Vector3*> out;
	GetOverlapPoly(rectangle, node_id, out);
	if (out.size() == 0) {
		return false;
	}

	if (debug_overlap_func_) {
		debug_overlap_func_(debug_overlap_userdata_, out);
	}

	pos = RandomInPoly(out, -1);
	bool status = true;
	if (Distance(pos, center) >= (float)radius) {
		status = false;
	}

	for (std::vector<const Math::Vector3*>::size_type i = 0; i < out.size(); ++i) {
		delete out[i];
	}

	return status;
}

float NavPathFinder::GetHeight(const Math::Vector3& p, int* result_node) {
	NavTile* tile = GetTile(p);
	if (!tile) {
		return -1;
	}

	float height = -1;
	for (size_t i = 0; i < tile->node_.size(); ++i) {
		int node_id = tile->node_[i];
		if (debug_node_func_) {
			debug_node_func_(debug_node_userdata_, node_id);
		}
		if (InsideNode(node_id, p)) {
			NavNode* node = &mesh_->node_[node_id];

			int index = 0;
			for (index = 0; index < node->size_; ++index) {
				if (InsideTriangle(node->vertice_[0], node->vertice_[index], node->vertice_[index + 1], p)) {
					break;
				}
			}

			const Math::Vector3& a = mesh_->vertice_[node->vertice_[0]];
			const Math::Vector3& b = mesh_->vertice_[node->vertice_[index]];
			const Math::Vector3& c = mesh_->vertice_[node->vertice_[index + 1]];

			const Math::Vector3 v0 = c - a;
			const Math::Vector3 v1 = b - a;
			const Math::Vector3 v2 = p - a;

			float denom = v0[0] * v1[2] - v0[2] * v1[0];
			if (fabsf(denom) < 1e-6f) {
				continue;
			}

			float u = v1[2] * v2[0] - v1[0] * v2[2];
			float v = v0[0] * v2[2] - v0[2] * v2[0];

			if (denom < 0) {
				denom = -denom;
				u = -u;
				v = -v;
			}

			if (u >= 0.0f && v >= 0.0f && (u + v) <= denom) {
				float tmp = a[1] + (v0[1] * u + v1[1] * v) / denom;
				if (tmp > height) {
					height = tmp;
					if (result_node) {
						*result_node = node_id;
					}
				}
			}
		}
	}

	return height;
}

float NavPathFinder::GetHeightNew(const Math::Vector3& p, int* result_node) {
	NavTile* tile = GetTile(p);
	if (!tile) {
		return -1;
	}

	float height = -1;
	for (size_t i = 0; i < tile->node_.size(); ++i) {
		int node_id = tile->node_[i];
		if (debug_node_func_) {
			debug_node_func_(debug_node_userdata_, node_id);
		}
		if (InsideNode(node_id, p)) {
			NavNode* node = &mesh_->node_[node_id];

			int index = 0;
			for (index = 0; index < node->size_; ++index) {
				if (InsideTriangle(node->vertice_[0], node->vertice_[index], node->vertice_[index + 1], p)) {
					break;
				}
			}
			const Math::Vector3& a = mesh_->vertice_[node->vertice_[0]];
			const Math::Vector3& b = mesh_->vertice_[node->vertice_[index]];
			const Math::Vector3& c = mesh_->vertice_[node->vertice_[index + 1]];

			Math::Plane plane;
			plane.Set(a, b, c);
			Math::Vector3 result;
			plane.LineHit(p, Math::Vector3(p[0], 0.0f, p[2]), result);
			if (result[1] > height) {
				height = result[1];
				if (result_node) {
					*result_node = node_id;
				}
			}
		}
	}

	return height;
}

void NavPathFinder::GetOverlapPoly(std::vector<Math::Vector3>& poly, int node_id, std::vector<const Math::Vector3*>& result) {
	NavNode* node = GetNode(node_id);

	std::vector<Math::Vector3*> vert;

	bool all_in_poly = true;
	for (int i = 0; i < (int)node->vertice_.size(); i++) {
		const Math::Vector3& pos = mesh_->vertice_[node->vertice_[i]];
		if (Math::InsidePoly(poly, pos)) {
			vert.push_back(new Math::Vector3(pos[0], pos[1], pos[2]));
		} else {
			all_in_poly = false;
		}
	}

	if (all_in_poly) {
		for (int i = 0; i < (int)node->vertice_.size(); i++) {
			result.assign(vert.begin(), vert.end());
		}
		return;
	}

	bool all_in_node = true;
	for (int i = 0; i < (int)poly.size(); i++) {
		if (InsidePoly(node->vertice_, poly[i])) {
			const Math::Vector3& pos = poly[i];
			vert.push_back(new Math::Vector3(pos[0], pos[1], pos[2]));
		} else {
			all_in_node = false;
		}
	}

	if (all_in_node) {
		for (int i = 0; i < (int)poly.size(); i++) {
			result.assign(vert.begin(), vert.end());
		}
		return;
	}

	for (int i = 0; i < (int)poly.size(); i++) {
		Math::Vector3& p1 = poly[i];
		Math::Vector3& p2 = poly[(i + 1) % poly.size()];
		for (int j = 0; j < (int)node->vertice_.size(); j++) {
			const Math::Vector3& q1 = mesh_->vertice_[node->vertice_[j]];
			const Math::Vector3& q2 = mesh_->vertice_[node->vertice_[(j + 1) % node->vertice_.size()]];
			if (Math::Intersect(p1, p2, q1, q2)) {
				Math::Vector3 cross;
				Math::GetIntersectPoint(p1, p2, q1, q2, cross);
				vert.push_back(new Math::Vector3(cross[0], cross[1], cross[2]));
			}
		}
	}

	Math::Vector3 center;
	for (int i = 0; i < (int)vert.size(); ++i) {
		center += *(vert[i]);
	}
	center /= (float)vert.size();

	std::vector<VertexAux> va;
	va.resize(vert.size());
	for (int i = 0; i < (int)vert.size(); ++i) {
		va[i].pos_ = *(vert[i]);
		va[i].index_ = i;
		va[i].center_ = center;
	}

	std::sort(va.begin(), va.end(), AngleCompare);

	for (int i = 0; i < (int)va.size(); ++i) {
		Math::Vector3* p = vert[va[i].index_];
		result.push_back(p);
	}
}