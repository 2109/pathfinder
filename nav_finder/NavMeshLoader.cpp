
#include "Intersect.h"
#include "Math.h"
#include "NavMeshFinder.h"
#include "Util.h"

struct TileAux {
	Math::Vector3 center_;
	Math::Vector3 pos_[4];
};

void NavPathFinder::CreateMesh(float** vert, int vertTotal, int** index, int indexTotal) {
	mesh_ = new NavMesh();

	mesh_->vertice_.resize(vertTotal);

	std::vector<std::unordered_map<int, int>> searcher;
	searcher.resize(vertTotal);

	mesh_->lt_.x = mesh_->lt_.y = mesh_->lt_.z = DBL_MAX;
	mesh_->br_.x = mesh_->br_.y = mesh_->br_.z = DBL_MIN;

	for (int i = 0; i < vertTotal; ++i) {
		Math::Vector3& pos = mesh_->vertice_[i];
		pos.x = vert[i][0];
		pos.y = vert[i][1];
		pos.z = vert[i][2];

		mesh_->lt_.x = std::min(mesh_->lt_.x, pos.x);
		mesh_->lt_.z = std::min(mesh_->lt_.z, pos.z);

		mesh_->br_.x = std::max(mesh_->br_.x, pos.x);
		mesh_->br_.z = std::max(mesh_->br_.z, pos.z);
	}

	mesh_->width_ = (uint32_t)(mesh_->br_.x - mesh_->lt_.x);
	mesh_->height_ = (uint32_t)(mesh_->br_.z - mesh_->lt_.z);

	mesh_->node_.resize(indexTotal);

	for (int i = 0; i < indexTotal; ++i) {
		NavNode* node = &mesh_->node_[i];
		node->Init(i, index[i][0]);

		Math::Vector3 tmp;
		for (int j = 1; j <= node->size_; ++j) {
			node->vertice_[j - 1] = index[i][j];
			tmp += mesh_->vertice_[node->vertice_[j - 1]];
		}

		node->center_ = tmp / node->size_;
		node->area_ = CalcNodeArea(node);
		mesh_->area_ += node->area_;

		SortNode(node);

		for (int j = 0; j < node->size_; ++j) {
			int index0 = j;
			int index1 = j + 1 >= node->size_ ? 0 : j + 1;

			int a = node->vertice_[index0];
			int b = node->vertice_[index1];

			NavBorder* border0 = SearchBorder(searcher, a, b);
			BorderLinkNode(border0, node->id_);
			node->border_[j] = border0->id_;

			NavBorder* border1 = SearchBorder(searcher, b, a);
			BorderLinkNode(border1, node->id_);
			border1->opposite_ = border0->id_;
			border0->opposite_ = border1->id_;
		}
	}
}

void NavPathFinder::CreateTile(uint32_t unit) {
	mesh_->tileUnit_ = unit;
	mesh_->tileWidth_ = mesh_->width_ / unit + 1;
	mesh_->tileHeight_ = mesh_->height_ / unit + 1;

	int total = mesh_->tileWidth_ * mesh_->tileHeight_;
	mesh_->tile_.resize(total);

	std::vector<TileAux> tileInfo;
	tileInfo.resize(total);

	for (uint32_t z = 0; z < mesh_->tileHeight_; ++z) {
		for (uint32_t x = 0; x < mesh_->tileWidth_; ++x) {
			uint32_t index = x + z * mesh_->tileWidth_;
			TileAux* ti = &tileInfo[index];
			ti->pos_[0].x = mesh_->lt_.x + x * unit;
			ti->pos_[0].z = mesh_->lt_.z + z * unit;
			ti->pos_[1].x = mesh_->lt_.x + (x + 1) * unit;
			ti->pos_[1].z = mesh_->lt_.z + z * unit;
			ti->pos_[2].x = mesh_->lt_.x + (x + 1) * unit;
			ti->pos_[2].z = mesh_->lt_.z + (z + 1) * unit;
			ti->pos_[3].x = mesh_->lt_.x + x * unit;
			ti->pos_[3].z = mesh_->lt_.z + (z + 1) * unit;
#ifdef NAV_DEBUG_TILE
			NavTile* tile = &mesh_->tile_[index];
			for (int i = 0; i < 4; ++i) {
				tile->pos_[i] = ti->pos_[i];
			}
#endif
			ti->center_.x = mesh_->lt_.x + (x + 0.5) * unit;
			ti->center_.z = mesh_->lt_.z + (z + 0.5) * unit;
			mesh_->tile_[index].center_ = ti->center_;
		}
	}

	for (int i = 0; i < (int)mesh_->node_.size(); ++i) {
		NavNode* node = &mesh_->node_[i];
		Math::Vector3 lt(DBL_MAX, DBL_MAX, DBL_MAX);
		Math::Vector3 br(DBL_MIN, DBL_MIN, DBL_MIN);
		for (int n = 0; n < (int)node->vertice_.size(); ++n) {
			const Math::Vector3& pos = mesh_->vertice_[node->vertice_[n]];
			lt.x = std::min(lt.x, pos.x);
			lt.z = std::min(lt.z, pos.z);

			br.x = std::max(br.x, pos.x);
			br.z = std::max(br.z, pos.z);
		}

		int xMin = (lt.x - mesh_->lt_.x) / unit;
		int zMin = (lt.z - mesh_->lt_.z) / unit;
		int xMax = (br.x - mesh_->lt_.x) / unit;
		int zMax = (br.z - mesh_->lt_.z) / unit;

		for (int z = zMin; z <= zMax; ++z) {
			for (int x = xMin; x <= xMax; ++x) {
				uint32_t index = x + z * mesh_->tileWidth_;
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

				if (inside) {
					if (InsideNode(node->id_, ti->center_)) {
						assert(tile->centerNode_ != node->id_);
						tile->centerNode_ = node->id_;
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
		ms >> pos.x >> pos.y >> pos.z;
	}

	int nborder = 0;
	ms >> nborder;
	mesh->border_.resize(nborder);
	for (int i = 0; i < nborder; ++i) {
		NavBorder* border = new NavBorder();
		ms >> border->node_[0] >> border->node_[1];
		ms >> border->a_ >> border->b_ >> border->opposite_;
		ms >> border->center_.x >> border->center_.y >> border->center_.z;
		mesh->border_.push_back(border);
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

		int nborder = 0;
		ms >> nborder;
		node.border_.resize(nborder);
		for (int j = 0; j < nborder; ++j) {
			int index = 0;
			ms >> index;
			node.border_[j] = index;
		}
		ms >> node.size_;
		ms >> node.center_.x >> node.center_.y >> node.center_.z;
		ms >> node.area_;
	}

	ms >> mesh->area_;
	ms >> mesh->lt_.x >> mesh->lt_.y >> mesh->lt_.z;
	ms >> mesh->br_.x >> mesh->br_.y >> mesh->br_.z;
	ms >> mesh->width_ >> mesh->height_;

	return finder;
}

NavPathFinder* NavPathFinder::LoadMeshEx(const char* file) {
	MemoryStream ms;
	if (Util::ReadFile(file, ms) < 0) {
		return NULL;
	}

	uint32_t numOfVertice = 0;
	ms >> numOfVertice;
	float** verticePtr = (float**)malloc(sizeof(*verticePtr) * numOfVertice);
	for (uint32_t i = 0; i < numOfVertice; ++i) {
		verticePtr[i] = (float*)malloc(sizeof(float) * 3);
		for (int j = 0; j < 3; ++j) {
			ms >> verticePtr[i][j];
		}
	}

	uint32_t numOfPoly = 0;
	ms >> numOfPoly;
	int** polyPtr = (int**)malloc(sizeof(*polyPtr) * numOfPoly);
	for (uint32_t i = 0; i < numOfPoly; ++i) {
		uint8_t numOfIndex = 0;
		ms >> numOfIndex;
		polyPtr[i] = (int*)malloc(sizeof(int) * (numOfIndex + 1));
		polyPtr[i][0] = numOfIndex;
		for (int j = 1; j <= numOfIndex; ++j) {
			uint16_t val;
			ms >> val;
			polyPtr[i][j] = val;
		}
	}

	NavPathFinder* finder = new NavPathFinder();
	finder->CreateMesh(verticePtr, numOfVertice, polyPtr, numOfPoly);

	for (uint32_t i = 0; i < numOfVertice; i++) {
		free(verticePtr[i]);
	}
	free(verticePtr);

	for (uint32_t i = 0; i < numOfPoly; i++) {
		free(polyPtr[i]);
	}
	free(polyPtr);

	return finder;
}

void NavPathFinder::LoadTile(const char* file) {
	MemoryStream ms;
	if (Util::ReadFile(file, ms) < 0) {
		return;
	}

	ms >> mesh_->tileUnit_ >> mesh_->tileWidth_ >> mesh_->tileHeight_;

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
		ms >> tile.centerNode_;
		ms >> tile.center_.x >> tile.center_.y >> tile.center_.z;
	}
}

void NavPathFinder::LoadTileEx(const char* file) {
	MemoryStream ms;
	if (Util::ReadFile(file, ms) < 0) {
		return;
	}

	uint32_t count = 0;
	ms >> mesh_->tileUnit_ >> count;

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
		tile->center_.y = 0;
		ms >> tile->center_.x >> tile->center_.z >> tile->centerNode_;
	}
	mesh_->tileWidth_ = mesh_->width_ / mesh_->tileUnit_ + 1;
	mesh_->tileHeight_ = mesh_->height_ / mesh_->tileUnit_ + 1;
}

void NavPathFinder::SerializeMesh(const char* file) {
	MemoryStream ms;

	ms << (int)mesh_->vertice_.size();
	for (int i = 0; i < (int)mesh_->vertice_.size(); ++i) {
		const Math::Vector3& pos = mesh_->vertice_[i];
		ms << pos.x << pos.y << pos.z;
	}

	ms << (int)mesh_->border_.size();
	for (int i = 0; i < (int)mesh_->border_.size(); ++i) {
		const NavBorder* border = mesh_->border_[i];
		ms << border->node_[0] << border->node_[1];
		ms << border->a_ << border->b_ << border->opposite_;
		ms << border->center_.x << border->center_.y << border->center_.z;
	}

	ms << (int)mesh_->node_.size();
	for (int i = 0; i < (int)mesh_->node_.size(); ++i) {
		const NavNode& node = mesh_->node_[i];

		ms << (int)node.vertice_.size();
		for (int j = 0; j < (int)node.vertice_.size(); ++j) {
			ms << node.vertice_[j];
		}

		ms << (int)node.border_.size();
		for (int j = 0; j < (int)node.border_.size(); ++j) {
			ms << node.border_[j];
		}
		ms << node.size_;
		ms << node.center_.x << node.center_.y << node.center_.z;
		ms << node.area_;
	}

	ms << mesh_->area_;
	ms << mesh_->lt_.x << mesh_->lt_.y << mesh_->lt_.z;
	ms << mesh_->br_.x << mesh_->br_.y << mesh_->br_.z;
	ms << mesh_->width_ << mesh_->height_;

	FILE* fp = fopen(file, "wb");
	fwrite(ms.Begin(), ms.Length(), 1, fp);
	fclose(fp);
}

void NavPathFinder::SerializeTile(const char* file) {
	MemoryStream ms;
	ms << mesh_->tileUnit_ << mesh_->tileWidth_ << mesh_->tileHeight_;
	ms << (int)mesh_->tile_.size();
	for (int i = 0; i < (int)mesh_->tile_.size(); ++i) {
		const NavTile& tile = mesh_->tile_[i];
		ms << (int)tile.node_.size();
		for (int j = 0; j < (int)tile.node_.size(); ++j) {
			ms << tile.node_[j];
		}
		ms << tile.centerNode_;
		ms << tile.center_.x << tile.center_.y << tile.center_.z;
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

NavBorder* NavPathFinder::SearchBorder(std::vector<std::unordered_map<int, int>>& searcher, int lhs, int rhs) {
	assert(lhs < (int)mesh_->vertice_.size());
	assert(rhs < (int)mesh_->vertice_.size());

	std::unordered_map<int, int>& borderMap = searcher[lhs];
	std::unordered_map<int, int>::iterator itr = borderMap.find(rhs);
	if (borderMap.find(rhs) != borderMap.end()) {
		int index = itr->second;
		return mesh_->border_[index];
	}

	NavBorder* border = AddBorder(lhs, rhs);
	borderMap[rhs] = border->id_;
	return border;
}

NavBorder* NavPathFinder::AddBorder(int lhs, int rhs) {
	NavBorder* border = new NavBorder();
	border->id_ = mesh_->border_.size();
	mesh_->border_.push_back(border);
	border->node_[0] = -1;
	border->node_[1] = -1;
	border->a_ = lhs;
	border->b_ = rhs;
	border->opposite_ = -1;
	border->center_ = mesh_->vertice_[lhs] + mesh_->vertice_[rhs];
	return border;
}