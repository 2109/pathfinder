
#include "Intersect.h"
#include "Math.h"
#include "NavMeshFinder.h"

struct SearchAux {
	int centerNode_;
	Math::Vector3* pos_;
};

struct RandomAux {
	NavNode** link_;
	float area_;
};

static bool TileChoose(void* ud, NavPathFinder* finder, NavTile* tile) {
	if (tile->centerNode_ != -1) {
		SearchAux* aux = (SearchAux*)ud;
		aux->centerNode_ = tile->centerNode_;
		aux->pos_ = &tile->center_;
		return false;
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

void NavPathFinder::SearchTile(const Math::Vector3& pos, int depth, SearchFunc func, void *ud) {
	if (mesh_->tile_.size() == 0) {
		return;
	}

	int xIndex = (pos.x - mesh_->lt_.x) / mesh_->tileUnit_;
	int zIndex = (pos.z - mesh_->lt_.z) / mesh_->tileUnit_;

	for (int r = 1; r <= depth; ++r) {
		int xMin = xIndex - r;
		int xMax = xIndex + r;
		int zMin = zIndex - r;
		int zMax = zIndex + r;

		int zRange[2] = { zMin, zMax };

		for (int i = 0; i < 2; i++) {
			int z = zRange[i];

			if (z < 0 || z >= (int)mesh_->tileHeight_) {
				continue;
			}

			for (int x = xMin; x <= xMax; x++) {

				if (x < 0 || x >= (int)mesh_->tileWidth_) {
					continue;
				}

				int index = x + z * mesh_->tileWidth_;
				NavTile* tile = &mesh_->tile_[index];

				if (debugTileFunc_) {
					debugTileFunc_(debugTileUserdata_, index);
				}

				if (!func(ud, this, tile)) {
					return;
				}
			}
		}

		int xRange[2] = { xMin, xMax };

		for (int i = 0; i < 2; i++) {
			int x = xRange[i];
			if (x < 0 || x >= (int)mesh_->tileWidth_) {
				continue;
			}

			for (int z = zMin; z < zMax; z++) {
				if (z < 0 || z >= (int)mesh_->tileHeight_) {
					continue;
				}
				int index = x + z * mesh_->tileWidth_;
				NavTile* tile = &mesh_->tile_[index];

				if (debugTileFunc_) {
					debugTileFunc_(debugTileUserdata_, index);
				}

				if (!func(ud, this, tile)) {
					return;
				}
			}
		}
	}
}

Math::Vector3* NavPathFinder::SearchInRectangle(const Math::Vector3& pos, int depth, int* centerNode) {
	struct SearchAux aux;
	aux.centerNode_ = -1;
	aux.pos_ = NULL;

	SearchTile(pos, depth, TileChoose, &aux);
	if (*centerNode) {
		*centerNode = aux.centerNode_;
	}

	return aux.pos_;
}

Math::Vector3* NavPathFinder::SearchInCircle(const Math::Vector3& pos, int depth, int* centerNode) {
	int xIndex = (pos.x - mesh_->lt_.x) / mesh_->tileUnit_;
	int zIndex = (pos.z - mesh_->lt_.z) / mesh_->tileUnit_;

	for (int i = 1; i <= depth; ++i) {
		if (i <= (int)circleIndex_.size()) {
			std::vector<IndexPair>* range = circleIndex_[i - 1];
			for (size_t i = 0; i < range->size(); i++) {
				const IndexPair& pair = (*range)[i];
				int x = xIndex + pair.x_;
				int z = zIndex + pair.z_;
				NavTile* tile = GetTile(x, z);
				if (tile) {
					if (debugTileFunc_) {
						debugTileFunc_(debugTileUserdata_, x + z * mesh_->tileWidth_);
					}
					if (tile->centerNode_ != -1) {
						if (centerNode) {
							*centerNode = tile->centerNode_;
						}
						return &tile->center_;
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
					int x = xIndex + xOffset;
					int z = zIndex + zOffset;
					NavTile* tile = GetTile(x, z);
					if (tile) {
						if (debugTileFunc_) {
							debugTileFunc_(debugTileUserdata_, x + z * mesh_->tileWidth_);
						}
						if (tile->centerNode_ != -1) {
							if (centerNode) {
								*centerNode = tile->centerNode_;
							}
							return &tile->center_;
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

NavNode* NavPathFinder::SearchNode(const Math::Vector3& pos, int depth) {
	if (mesh_->tile_.size() == 0) {
		for (size_t i = 0; i < mesh_->node_.size(); ++i) {
			if (debugNodeFunc_) {
				debugNodeFunc_(debugNodeUserdata_, i);
			}
			if (InsideNode(i, pos)) {
				return &mesh_->node_[i];
			}
		}
	}

	NavTile* tile = GetTile(pos);
	if (!tile) {
		return NULL;
	}

	for (size_t i = 0; i < tile->node_.size(); ++i) {
		int nodeId = tile->node_[i];
		if (debugNodeFunc_) {
			debugNodeFunc_(debugNodeUserdata_, nodeId);
		}
		if (InsideNode(nodeId, pos)) {
			return &mesh_->node_[nodeId];
		}
	}

	double dtMin = DBL_MAX;
	int nearest = -1;
	for (size_t i = 0; i < tile->node_.size(); ++i) {
		int nodeId = tile->node_[i];
		if (debugNodeFunc_) {
			debugNodeFunc_(debugNodeUserdata_, nodeId);
		}
		double dt = Dot2Node(pos, nodeId);
		if (dtMin < dt) {
			dtMin = dt;
			nearest = nodeId;
		}
	}

	if (nearest != -1) {
		return &mesh_->node_[nearest];
	}

	if (depth > 0) {
		int centerNode = 0;
		if (SearchInCircle(pos, depth, &centerNode)) {
			return &mesh_->node_[centerNode];
		}
	}
	return NULL;
}


bool NavPathFinder::Movable(const Math::Vector3& pos, float fix, float* dtOffset) {
	if (pos.x < mesh_->lt_.x || pos.x > mesh_->br_.x) {
		return false;
	}

	if (pos.z < mesh_->lt_.z || pos.z > mesh_->br_.z) {
		return false;
	}

	if (mesh_->tile_.size() == 0) {
		return false;
	}

	if (dtOffset) {
		*dtOffset = 0;
	}

	NavNode* node = NULL;

	int xIndex = (pos.x - mesh_->lt_.x) / mesh_->tileUnit_;
	int zIndex = (pos.z - mesh_->lt_.z) / mesh_->tileUnit_;
	int index = xIndex + zIndex * mesh_->tileWidth_;

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

	NavNode* searchNode = NULL;
	double dtMin = -1;

	for (int x = xIndex - 1; x <= xIndex + 1; x++) {
		if (x < 0 || x >= (int)mesh_->tileWidth_) {
			continue;
		}
		for (int z = zIndex - 1; z <= zIndex + 1; z++) {
			if (z < 0 || z >= (int)mesh_->tileHeight_) {
				continue;
			}
			int index = x + z * mesh_->tileWidth_;
			tile = &mesh_->tile_[index];

			for (int i = 0; i < (int)tile->node_.size(); i++) {
				int nodeId = tile->node_[i];

				node = GetNode(nodeId);
				if (!node->record_) {
					node->record_ = true;
					node->next_ = searchNode;
					searchNode = node;
					double dt = Dot2Node(pos, nodeId);
					if (dtMin < 0 || dtMin > dt) {
						dtMin = dt;
					}
				}
			}
		}
	}

	while (searchNode) {
		NavNode* current = searchNode;
		assert(current->record_);
		current->record_ = false;
		searchNode = searchNode->next_;
		current->next_ = NULL;
	}

	if (dtOffset) {
		*dtOffset = dtMin;
	}
	if (dtMin > 0 && dtMin <= fix) {
		return true;
	}

	return false;
}

double NavPathFinder::Dot2Node(const Math::Vector3& pos, int nodeId) {
	NavNode* node = &mesh_->node_[nodeId];
	double min = DBL_MAX;
	for (int i = 0; i < node->size_; ++i) {
		const Math::Vector3& pt0 = mesh_->vertice_[node->vertice_[i]];
		const Math::Vector3& pt1 = mesh_->vertice_[node->vertice_[(i + 1) % node->size_]];
		double dt = Math::DistancePointToSegment(pt0, pt1, pos);
		if (dt < min) {
			min = dt;
		}
	}
	return min;
}

Math::Vector3 NavPathFinder::RandomMovable(int nodeId) {
	int randomNode = -1;

	if (nodeId >= 0 && nodeId < (int)mesh_->node_.size()) {
		randomNode = nodeId;
	} else {
		double weight = mesh_->area_ * (Math::Rand(0, 10000) / 10000.0f);
		double tmp = 0.0f;
		for (int i = 0; i < (int)mesh_->node_.size(); i++) {
			tmp += mesh_->node_[i].area_;
			if (weight <= tmp) {
				randomNode = i;
				break;
			}
		}
	}

	NavNode* node = &mesh_->node_[randomNode];

	std::vector<const Math::Vector3*> vertice;
	for (int i = 0; i < (int)node->vertice_.size(); ++i) {
		const Math::Vector3* pos = &mesh_->vertice_[node->vertice_[i]];
		vertice.push_back(pos);
	}

	return Math::RandomInPoly(vertice, node->area_);
}

bool NavPathFinder::RandomInCircle(Math::Vector3& pos, const Math::Vector3& center, int radius) {
	int depth = radius / mesh_->tileUnit_;
	if (depth <= 0) {
		depth = 1;
	}
	NavNode* link = NULL;
	RandomAux aux;
	aux.link_ = &link;
	aux.area_ = 0;

	SearchTile(center, depth, TileLink, &aux);

	int nodeId = -1;
	float randomWeight = Math::Rand(0.0f, aux.area_);
	float weight = 0;
	while (link) {
		weight += link->area_;
		if (debugNodeFunc_) {
			debugNodeFunc_(debugNodeUserdata_, link->id_);
		}
		if (randomWeight <= weight && nodeId == -1) {
			nodeId = link->id_;
		}
		NavNode* ori = link;
		link = link->next_;
		ori->next_ = NULL;
	}

	std::vector<Math::Vector3> rectanble = {
		Math::Vector3(center.x - radius, 0, center.z + radius),
		Math::Vector3(center.x + radius, 0, center.z + radius),
		Math::Vector3(center.x + radius, 0, center.z - radius),
		Math::Vector3(center.x - radius, 0, center.z - radius) };

	std::vector<const Math::Vector3*> out;
	GetOverlapPoly(rectanble, nodeId, out);
	if (out.size() == 0) {
		return false;
	}

	pos = RandomInPoly(out, -1);
	bool status = true;
	if (Math::Distance(pos, center) >= radius) {
		status = false;
	}

	for (int i = 0; i < out.size(); ++i) {
		delete out[i];
	}

	return status;
}

float NavPathFinder::GetHeight(const Math::Vector3& p) {
	NavTile* tile = GetTile(p);
	if (!tile) {
		return -1;
	}

	NavNode* node = NULL;
	for (size_t i = 0; i < tile->node_.size(); ++i) {
		int nodeId = tile->node_[i];
		if (debugNodeFunc_) {
			debugNodeFunc_(debugNodeUserdata_, nodeId);
		}
		if (InsideNode(nodeId, p)) {
			node = &mesh_->node_[nodeId];
			break;
		}
	}

	if (!node) {
		return -1;
	}

	std::vector<int> vert = { 0, 0, 0 };
	int index = -1;
	for (int i = 0; i < node->size_; ++i) {
		vert[0] = node->vertice_[0];
		vert[1] = node->vertice_[i];
		vert[2] = node->vertice_[i + 1];
		if (InsidePoly(vert, p)) {
			index = i;
			break;
		}
	}

	if (index < 0) {
		return -1;
	}

	const float EPS = 1e-6f;

	const Math::Vector3& a = mesh_->vertice_[vert[0]];
	const Math::Vector3& b = mesh_->vertice_[vert[1]];
	const Math::Vector3& c = mesh_->vertice_[vert[2]];

	const Math::Vector3& v0 = c - a;
	const Math::Vector3& v1 = b - a;
	const Math::Vector3& v2 = p - a;

	float denom = v0.x * v1.z - v0.z * v1.x;
	if (fabsf(denom) < EPS) {
		return -1;
	}

	float u = v1.z * v2.x - v1.x * v2.z;
	float v = v0.x * v2.z - v0.z * v2.x;

	if (denom < 0) {
		denom = -denom;
		u = -u;
		v = -v;
	}

	if (u >= 0.0f && v >= 0.0f && (u + v) <= denom) {
		return a.y + (v0.y * u + v1.y * v) / denom;
	}
	return -1;
}

void NavPathFinder::GetOverlapPoly(std::vector<Math::Vector3>& poly, int nodeId, std::vector<const Math::Vector3*>& result) {
	NavNode* node = GetNode(nodeId);

	std::vector<Math::Vector3*> vert;

	bool allInPoly = true;
	for (int i = 0; i < (int)node->vertice_.size(); i++) {
		const Math::Vector3& pos = mesh_->vertice_[node->vertice_[i]];
		if (Math::InsidePoly(poly, pos)) {
			vert.push_back(new Math::Vector3(pos.x, pos.y, pos.z));
		} else {
			allInPoly = false;
		}
	}

	if (allInPoly) {
		for (int i = 0; i < (int)node->vertice_.size(); i++) {
			result.assign(vert.begin(), vert.end());
		}
		return;
	}

	bool allInNode = true;
	for (int i = 0; i < (int)poly.size(); i++) {
		if (InsidePoly(node->vertice_, poly[i])) {
			const Math::Vector3& pos = poly[i];
			vert.push_back(new Math::Vector3(pos.x, pos.y, pos.z));
		} else {
			allInNode = false;
		}
	}

	if (allInNode) {
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
				vert.push_back(new Math::Vector3(cross.x, cross.y, cross.z));
			}
		}
	}

	Math::Vector3 center;
	for (int i = 0; i < (int)vert.size(); ++i) {
		center += *(vert[i]);
	}
	center /= vert.size();

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