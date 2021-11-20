
#include "intersect.h"
#include "mathex.h"
#include "navmesh_finder.h"
#include "util.h"

struct TileAux {
	Math::Vector3 center_;
	Math::Vector3 pos_[4];
};


void NavPathFinder::CreateMesh(float** vert, int vert_total, int** index, int index_total) {
	mesh_ = new NavMesh();

	mesh_->vertice_.resize(vert_total);

	std::vector<std::unordered_map<int, int>> searcher;
	searcher.resize(vert_total);

	mesh_->lt_.v[0] = mesh_->lt_.v[1] = mesh_->lt_.v[2] = FLT_MAX;
	mesh_->br_.v[0] = mesh_->br_.v[1] = mesh_->br_.v[2] = FLT_MIN;

	for (int i = 0; i < vert_total; ++i) {
		Math::Vector3& pos = mesh_->vertice_[i];
		pos.v[0] = vert[i][0];
		pos.v[1] = vert[i][1];
		pos.v[2] = vert[i][2];

		mesh_->lt_.v[0] = std::min(mesh_->lt_[0], pos[0]);
		mesh_->lt_.v[2] = std::min(mesh_->lt_[2], pos[2]);

		mesh_->br_.v[0] = std::max(mesh_->br_[0], pos[0]);
		mesh_->br_.v[2] = std::max(mesh_->br_[2], pos[2]);
	}

	mesh_->width_ = (uint32_t)(mesh_->br_[0] - mesh_->lt_[0]);
	mesh_->height_ = (uint32_t)(mesh_->br_[2] - mesh_->lt_[2]);

	mesh_->node_.resize(index_total);

	for (int i = 0; i < index_total; ++i) {
		NavNode* node = &mesh_->node_[i];
		node->Init(i, index[i][0]);

		Math::Vector3 tmp;
		for (int j = 1; j <= node->size_; ++j) {
			node->vertice_[j - 1] = index[i][j];
			tmp += mesh_->vertice_[node->vertice_[j - 1]];
		}

		node->center_ = tmp / (float)node->size_;
		node->area_ = CountNodeArea(node);
		mesh_->area_ += node->area_;

		SortNode(node);

		for (int j = 0; j < node->size_; ++j) {
			int index0 = j;
			int index1 = j + 1 >= node->size_ ? 0 : j + 1;

			int a = node->vertice_[index0];
			int b = node->vertice_[index1];

			NavEdge* edge = SearchEdge(searcher, a, b);
			EdgeLinkNode(edge, node->id_);
			node->edge_[j] = edge->id_;

			NavEdge* edge_inverse = SearchEdge(searcher, b, a);
			EdgeLinkNode(edge_inverse, node->id_);
			edge_inverse->inverse_ = edge->id_;
			edge->inverse_ = edge_inverse->id_;
		}
	}

	MakeArea();
}

void NavPathFinder::CreateTile(uint32_t unit) {
	mesh_->tile_unit_ = unit;
	mesh_->tile_width_ = mesh_->width_ / unit + 1;
	mesh_->tile_height_ = mesh_->height_ / unit + 1;

	int total = mesh_->tile_width_ * mesh_->tile_height_;
	mesh_->tile_.resize(total);

	std::vector<TileAux> tileInfo;
	tileInfo.resize(total);

	for (uint32_t z = 0; z < mesh_->tile_height_; ++z) {
		for (uint32_t x = 0; x < mesh_->tile_width_; ++x) {
			uint32_t index = x + z * mesh_->tile_width_;
			TileAux* ti = &tileInfo[index];
			ti->pos_[0].v[0] = mesh_->lt_[0] + x * unit;
			ti->pos_[0].v[2] = mesh_->lt_[2] + z * unit;
			ti->pos_[1].v[0] = mesh_->lt_[0] + (x + 1) * unit;
			ti->pos_[1].v[2] = mesh_->lt_[2] + z * unit;
			ti->pos_[2].v[0] = mesh_->lt_[0] + (x + 1) * unit;
			ti->pos_[2].v[2] = mesh_->lt_[2] + (z + 1) * unit;
			ti->pos_[3].v[0] = mesh_->lt_[0] + x * unit;
			ti->pos_[3].v[2] = mesh_->lt_[2] + (z + 1) * unit;
#ifdef NAV_DEBUG_TILE
			NavTile* tile = &mesh_->tile_[index];
			for (int i = 0; i < 4; ++i) {
				tile->pos_[i] = ti->pos_[i];
			}
#endif
			ti->center_.v[0] = mesh_->lt_[0] + (x + 0.5) * unit;
			ti->center_.v[2] = mesh_->lt_[2] + (z + 0.5) * unit;
			mesh_->tile_[index].center_ = ti->center_;
		}
	}

	for (int i = 0; i < (int)mesh_->node_.size(); ++i) {
		NavNode* node = &mesh_->node_[i];
		Math::Vector3 lt(FLT_MAX, FLT_MAX, FLT_MAX);
		Math::Vector3 br(FLT_MIN, FLT_MIN, FLT_MIN);
		for (int n = 0; n < (int)node->vertice_.size(); ++n) {
			const Math::Vector3& pos = mesh_->vertice_[node->vertice_[n]];
			lt.v[0] = std::min(lt[0], pos[0]);
			lt.v[2] = std::min(lt[2], pos[2]);

			br.v[0] = std::max(br[0], pos[0]);
			br.v[2] = std::max(br[2], pos[2]);
		}

		int xMin = (lt[0] - mesh_->lt_[0]) / unit;
		int zMin = (lt[2] - mesh_->lt_[2]) / unit;
		int xMax = (br[0] - mesh_->lt_[0]) / unit;
		int zMax = (br[2] - mesh_->lt_[2]) / unit;

		for (int z = zMin; z <= zMax; ++z) {
			for (int x = xMin; x <= xMax; ++x) {
				uint32_t index = x + z * mesh_->tile_width_;
				TileAux* ti = &tileInfo[index];
				NavTile* tile = &mesh_->tile_[index];
				bool inside = false;
				for (int j = 0; j < 4; ++j) {
					const Math::Vector3& pos = ti->pos_[j];
					if (InsideNode(node->id_, pos)) {
						tile->node_.push_back(node->id_);
						inside = true;
						break;
					}
				}

				if (!inside) {
					std::vector<Math::Vector3> rectangle = { ti->pos_[3], ti->pos_[2],ti->pos_[1],ti->pos_[0] };
					for (unsigned int j = 0; j < node->vertice_.size(); j++) {
						if (Math::InsidePoly(rectangle, mesh_->vertice_[j])) {
							tile->node_.push_back(node->id_);
							inside = true;
							break;
						}
					}
				}

				if (!inside) {
					bool cross = false;
					for (int j = 0; j < 4; j++) {
						const Math::Vector3& pos = ti->pos_[j];
						for (unsigned int k = 0; k < node->edge_.size(); k++) {
							NavEdge* edge = GetEdge(node->edge_[k]);
							if (Intersect(tile->pos_[j], tile->pos_[(j + 1) % 4], mesh_->vertice_[edge->a_], mesh_->vertice_[edge->b_])) {
								tile->node_.push_back(node->id_);
								cross = true;
								break;
							}
						}
						if (cross) {
							break;
						}
					}
					if (cross) {
						inside = true;
					}
				}

				if (inside) {
					if (InsideNode(node->id_, ti->center_)) {
						assert(tile->center_node_ != node->id_);
						tile->center_node_ = node->id_;
					}
				}
			}
		}
	}
}

NavPathFinder* NavPathFinder::LoadMesh(const char* file) {
	MemoryStream ms;
	if (Util::ReadFile(file, ms) < 0) {
		return NULL;
	}

	NavPathFinder* finder = new NavPathFinder();
	NavMesh* mesh = finder->mesh_;

	int nvert = 0;
	ms >> nvert;
	mesh->vertice_.resize(nvert);
	for (int i = 0; i < nvert; ++i) {
		Math::Vector3& pos = mesh->vertice_[i];
		ms >> pos[0] >> pos[1] >> pos[2];
	}

	int nedge = 0;
	ms >> nedge;
	mesh->edge_.resize(nedge);
	for (int i = 0; i < nedge; ++i) {
		NavEdge* edge = new NavEdge();
		ms >> edge->node_[0] >> edge->node_[1];
		ms >> edge->a_ >> edge->b_ >> edge->inverse_;
		ms >> edge->center_[0] >> edge->center_[1] >> edge->center_[2];
		mesh->edge_.push_back(edge);
	}

	int nnode = 0;
	ms >> nnode;
	mesh->node_.resize(nnode);
	for (int i = 0; i < nnode; ++i) {
		NavNode& node = mesh->node_[i];

		int nvert = 0;
		ms >> nvert;
		node.vertice_.resize(nvert);
		for (int j = 0; j < nvert; ++j) {
			int index = 0;
			ms >> index;
			node.vertice_[j] = index;
		}

		int nedge = 0;
		ms >> nedge;
		node.edge_.resize(nedge);
		for (int j = 0; j < nedge; ++j) {
			int index = 0;
			ms >> index;
			node.edge_[j] = index;
		}
		ms >> node.size_;
		ms >> node.center_[0] >> node.center_[1] >> node.center_[2];
		ms >> node.area_;
	}

	ms >> mesh->area_;
	ms >> mesh->lt_[0] >> mesh->lt_[1] >> mesh->lt_[2];
	ms >> mesh->br_[0] >> mesh->br_[1] >> mesh->br_[2];
	ms >> mesh->width_ >> mesh->height_;

	return finder;
}

NavPathFinder* NavPathFinder::LoadMeshEx(const char* file) {
	MemoryStream ms;
	if (Util::ReadFile(file, ms) < 0) {
		return NULL;
	}

	uint32_t nvert = 0;
	ms >> nvert;
	float** vert_ptr = (float**)malloc(sizeof(*vert_ptr) * nvert);
	for (uint32_t i = 0; i < nvert; ++i) {
		vert_ptr[i] = (float*)malloc(sizeof(float) * 3);
		for (int j = 0; j < 3; ++j) {
			ms >> vert_ptr[i][j];
		}
	}

	uint32_t nindex = 0;
	ms >> nindex;
	int** index_ptr = (int**)malloc(sizeof(*index_ptr) * nindex);
	for (uint32_t i = 0; i < nindex; ++i) {
		uint8_t index = 0;
		ms >> index;
		index_ptr[i] = (int*)malloc(sizeof(int) * (index + 1));
		index_ptr[i][0] = index;
		for (int j = 1; j <= index; ++j) {
			uint16_t val;
			ms >> val;
			index_ptr[i][j] = val;
		}
	}

	NavPathFinder* finder = new NavPathFinder();
	finder->CreateMesh(vert_ptr, nvert, index_ptr, nindex);

	for (uint32_t i = 0; i < nvert; i++) {
		free(vert_ptr[i]);
	}
	free(vert_ptr);

	for (uint32_t i = 0; i < nindex; i++) {
		free(index_ptr[i]);
	}
	free(index_ptr);

	return finder;
}

void NavPathFinder::LoadTile(const char* file) {
	MemoryStream ms;
	if (Util::ReadFile(file, ms) < 0) {
		return;
	}

	ms >> mesh_->tile_unit_ >> mesh_->tile_width_ >> mesh_->tile_height_;

	int ntile = 0;
	ms >> ntile;
	mesh_->tile_.resize(ntile);
	for (int i = 0; i < ntile; ++i) {
		NavTile& tile = mesh_->tile_[i];
		int nnode = 0;
		ms >> nnode;
		tile.node_.resize(nnode);
		for (int j = 0; j < nnode; ++j) {
			int index = 0;
			ms >> index;
			tile.node_[j] = index;
		}
		ms >> tile.center_node_;
		ms >> tile.center_[0] >> tile.center_[1] >> tile.center_[2];
	}
}

void NavPathFinder::LoadTileEx(const char* file) {
	MemoryStream ms;
	if (Util::ReadFile(file, ms) < 0) {
		return;
	}

	uint32_t count = 0;
	ms >> mesh_->tile_unit_ >> count;

	mesh_->tile_.resize(count);

	for (uint32_t i = 0; i < count; ++i) {
		NavTile* tile = &mesh_->tile_[i];
		uint32_t size = 0;
		ms >> size;
		if (size > 0) {
			tile->node_.resize(size);
			for (uint32_t j = 0; j < size; ++j) {
				uint32_t val = 0;
				ms >> val;
				tile->node_[j] = val;
			}
		}
		tile->center_[1] = 0;
		ms >> tile->center_[0] >> tile->center_[2] >> tile->center_node_;
	}
	mesh_->tile_width_ = mesh_->width_ / mesh_->tile_unit_ + 1;
	mesh_->tile_height_ = mesh_->height_ / mesh_->tile_unit_ + 1;
}

void NavPathFinder::SerializeMesh(const char* file) {
	MemoryStream ms;

	ms << (int)mesh_->vertice_.size();
	for (int i = 0; i < (int)mesh_->vertice_.size(); ++i) {
		const Math::Vector3& pos = mesh_->vertice_[i];
		ms << pos[0] << pos[1] << pos[2];
	}

	ms << (int)mesh_->edge_.size();
	for (int i = 0; i < (int)mesh_->edge_.size(); ++i) {
		const NavEdge* edge = mesh_->edge_[i];
		ms << edge->node_[0] << edge->node_[1];
		ms << edge->a_ << edge->b_ << edge->inverse_;
		ms << edge->center_[0] << edge->center_[1] << edge->center_[2];
	}

	ms << (int)mesh_->node_.size();
	for (int i = 0; i < (int)mesh_->node_.size(); ++i) {
		const NavNode& node = mesh_->node_[i];

		ms << (int)node.vertice_.size();
		for (int j = 0; j < (int)node.vertice_.size(); ++j) {
			ms << node.vertice_[j];
		}

		ms << (int)node.edge_.size();
		for (int j = 0; j < (int)node.edge_.size(); ++j) {
			ms << node.edge_[j];
		}
		ms << node.size_;
		ms << node.center_[0] << node.center_[1] << node.center_[2];
		ms << node.area_;
	}

	ms << mesh_->area_;
	ms << mesh_->lt_[0] << mesh_->lt_[1] << mesh_->lt_[2];
	ms << mesh_->br_[0] << mesh_->br_[1] << mesh_->br_[2];
	ms << mesh_->width_ << mesh_->height_;

	FILE* fp = fopen(file, "wb");
	fwrite(ms.Begin(), ms.Length(), 1, fp);
	fclose(fp);
}

void NavPathFinder::SerializeTile(const char* file) {
	MemoryStream ms;
	ms << mesh_->tile_unit_ << mesh_->tile_width_ << mesh_->tile_height_;
	ms << (int)mesh_->tile_.size();
	for (int i = 0; i < (int)mesh_->tile_.size(); ++i) {
		const NavTile& tile = mesh_->tile_[i];
		ms << (int)tile.node_.size();
		for (int j = 0; j < (int)tile.node_.size(); ++j) {
			ms << tile.node_[j];
		}
		ms << tile.center_node_;
		ms << tile.center_[0] << tile.center_[1] << tile.center_[2];
	}
	FILE* fp = fopen(file, "wb");
	fwrite(ms.Begin(), ms.Length(), 1, fp);
	fclose(fp);
}

void NavPathFinder::SortNode(NavNode* node) {
	std::vector<VertexAux> va;
	va.resize(node->size_);
	for (int i = 0; i < node->size_; ++i) {
		va[i].pos_ = mesh_->vertice_[node->vertice_[i]];
		va[i].index_ = node->vertice_[i];
		va[i].center_ = node->center_;
	}

	std::sort(va.begin(), va.end(), AngleCompare);

	for (int i = 0; i < node->size_; ++i) {
		node->vertice_[i] = va[i].index_;
	}
}

NavEdge* NavPathFinder::SearchEdge(std::vector<std::unordered_map<int, int>>& searcher, int lhs, int rhs) {
	assert(lhs < (int)mesh_->vertice_.size());
	assert(rhs < (int)mesh_->vertice_.size());

	std::unordered_map<int, int>& edge_map = searcher[lhs];
	auto itr = edge_map.find(rhs);
	if (edge_map.find(rhs) != edge_map.end()) {
		int index = itr->second;
		return mesh_->edge_[index];
	}

	NavEdge* edge = AddEdge(lhs, rhs);
	edge_map[rhs] = edge->id_;
	return edge;
}

NavEdge* NavPathFinder::AddEdge(int lhs, int rhs) {
	NavEdge* edge = new NavEdge();
	edge->id_ = mesh_->edge_.size();
	mesh_->edge_.push_back(edge);
	edge->node_[0] = -1;
	edge->node_[1] = -1;
	edge->a_ = lhs;
	edge->b_ = rhs;
	edge->inverse_ = -1;
	edge->center_ = mesh_->vertice_[lhs] + mesh_->vertice_[rhs];
	return edge;
}