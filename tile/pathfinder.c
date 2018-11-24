

#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>


#include "pathfinder.h"

#include "minheap-adapter.h"

#define MARK_MAX 64
#define INIT_PATH_SIZE 64

#define DX(A,B) (A->x - B->x)
#define DZ(A,B) (A->z - B->z)

typedef struct path_node {
	int x;
	int z;
} path_node_t;

typedef struct path {
	int index;
	int size;
	path_node_t* nodes;
	path_node_t init[INIT_PATH_SIZE];
} path_t;

typedef struct node {
	mh_elt_t elt;
	struct node *next;
	struct node *parent;
	int x;
	int z;
	int index;
	int	block;
	float G;
	float H;
	float F;
	int closed;
	int recorded;
} node_t;

typedef struct pathfinder {
	int width;
	int heigh;
	node_t *node;

	int size;
	int* movable;

	char mask[MARK_MAX];

	mh_t openlist;
	node_t* closelist;
} pathfinder_t;

static int DIRECTION[8][2] = {
	{ -1, 0 },
	{ 1, 0 },
	{ 0, -1 },
	{ 0, 1 },
	{ -1, -1 },
	{ -1, 1 },
	{ 1, -1 },
	{ 1, 1 },
};


static inline node_t*
find_node(pathfinder_t* finder, int x, int z) {
	if (x < 0 || x >= finder->width || z < 0 || z >= finder->heigh)
		return NULL;
	return &finder->node[x*finder->heigh + z];
}

static inline int
isblock(pathfinder_t* finder, node_t* node) {
	return node->block != 0;
}

static inline int
movable(pathfinder_t* finder, int x, int z, int ignore) {
	node_t *node = find_node(finder, x, z);
	if (node == NULL)
		return 0;
	if (ignore)
		return !isblock(finder, node);
	return finder->mask[node->block] == 1;
}

static inline int
random_index(int max) {
	float ratio = (rand() % RAND_MAX) / (float)RAND_MAX;
	return ratio * max;
}

node_t*
search_node(pathfinder_t* finder, int x0, int z0, int x1, int z1, finder_dump dump, void* ud) {
	int rx, rz;
	int stopx, stopz;
	finder_mask_reverse(finder);
	finder_raycast(finder, x1, z1, x0, z0, 0, &rx, &rz, &stopx, &stopz, dump, ud);
	finder_mask_reverse(finder);
	return find_node(finder, stopx, stopz);
}

void
is_min_node(pathfinder_t* finder, int cx, int cz, int dx, int dz, int* dt_min, int* mx, int* mz, node_t** list, finder_dump dump, void* ud) {
	int x = cx + dx;
	int z = cz + dz;
	if (movable(finder, x, z, 0)) {
		node_t *node = find_node(finder, x, z);
		if (node->recorded == 0) {
			node->recorded = 1;
			node->next = *list;
			*list = node;

			if (dump) {
				dump(ud, x, z);
			}

			int dt = dx * dx + dz * dz;
			if (*dt_min < 0 || *dt_min > dt) {
				*dt_min = dt;
				*mx = x;
				*mz = z;
			}
		}
	}
}

node_t*
search_node_in_circle(struct pathfinder* finder, int x, int z, int r, finder_dump dump, void* ud) {
	int min_dt = -1;
	int rx, rz;

	node_t* list = NULL;
	int i;
	for (i = 1; i <= r; i++) {
		int tx = 0;
		int tz = i;

		int d = 3 - 2 * r;
		while (tx <= tz) {
			int dir[][2] = { { tx, tz }, { -tx, tz }, { tx, -tz }, { -tx, -tz }, { tz, tx }, { -tz, tx }, { tz, -tx }, { -tz, -tx } };
			int j;
			for (j = 0; j < 8; j++) {
				is_min_node(finder, x, z, dir[j][0], dir[j][1], &min_dt, &rx, &rz, &list, dump, ud);
			}
			if (d < 0) {
				d = d + 4 * tx + 6;
			}
			else {
				d = d + 4 * (tx - tz) + 10;
				tz--;
			}
			tx++;
		}

		if (min_dt != -1) {
			break;
		}
	}

	while (list) {
		node_t* tmp = list;
		list = tmp->next;
		tmp->next = NULL;
		tmp->recorded = 0;
	}

	if (min_dt < 0) {
		return NULL;
	}

	return find_node(finder, rx, rz);
}

static inline void
find_neighbors(pathfinder_t * finder, struct node * node, node_t **neighbours) {
	int i;
	for (i = 0; i < 8; i++) {
		int x = node->x + DIRECTION[i][0];
		int z = node->z + DIRECTION[i][1];
		node_t * nei = find_node(finder, x, z);
		if (nei && nei->closed == 0) {
			if (!isblock(finder, nei)) {
				nei->next = (*neighbours);
				(*neighbours) = nei;
			}
		}
	}
}

static inline float
neighbor_estimate(node_t * from, node_t * to) {
	int dx = from->x - to->x;
	int dz = from->z - to->z;
	int i;
	for (i = 0; i < 8; ++i) {
		if (DIRECTION[i][0] == dx && DIRECTION[i][1] == dz)
			break;
	}
	if (i < 4)
		return 10.0f;
	return 14.0f;
}

static inline float
goal_estimate(node_t * from, node_t * to, float cost) {
	if (cost < 1) {
		cost = 64;
	}
	return abs(from->x - to->x) * cost + abs(from->z - to->z) * cost;
}

static inline void
clear_node(node_t* node) {
	node->parent = NULL;
	node->F = node->G = node->H = 0;
	node->next = NULL;
	node->closed = 0;
	mh_init(&node->elt);
}

static inline void
heap_clear(mh_elt_t* elt) {
	node_t *node = (node_t*)elt;
	clear_node(node);
}

static inline void
finder_reset(pathfinder_t* finder) {
	node_t * node = finder->closelist;
	while (node) {
		node_t * tmp = node;
		node = tmp->next;
		clear_node(tmp);
	}
	finder->closelist = NULL;
	mh_clear(&finder->openlist, heap_clear);
}

static inline int
less(mh_elt_t * left, mh_elt_t * right) {
	node_t *l = (node_t*)left;
	node_t *r = (node_t*)right;
	return l->F < r->F;
}

static inline int
great(mh_elt_t * left, mh_elt_t * right) {
	node_t *l = (node_t*)left;
	node_t *r = (node_t*)right;
	return l->F > r->F;
}

static inline void
path_init(path_t* path) {
	path->index = 0;
	path->size = INIT_PATH_SIZE;
	path->nodes = path->init;
}

static inline void
path_release(path_t* path) {
	if (path->nodes != path->init) {
		free(path->nodes);
	}
}

static inline void
path_add(path_t* path, int x, int z) {
	if (path->index >= path->size) {
		int nsize = path->size * 2;
		path_node_t* nnodes = malloc(nsize * sizeof(path_node_t));
		memcpy(nnodes, path->nodes, path->size * sizeof(path_node_t));
		path->size = nsize;
		if (path->init != path->nodes) {
			free(path->nodes);
		}
		path->nodes = nnodes;
	}
	path->nodes[path->index].x = x;
	path->nodes[path->index].z = z;
	path->index++;
}

void
build_path(pathfinder_t *finder, node_t *node, node_t *from, int smooth, finder_result result_cb, void* result_ud) {
	path_t path;
	path_init(&path);

	path_add(&path, node->x, node->z);

	node_t * parent = node->parent;
	assert(parent != NULL);

	int dx0 = DX(node, parent);
	int dz0 = DZ(node, parent);

	node = parent;
	while (node) {
		if (node != from) {
			parent = node->parent;
			if (parent != NULL) {
				int dx1 = DX(node, parent);
				int dz1 = DZ(node, parent);
				if (dx0 != dx1 || dz0 != dz1) {
					path_add(&path, node->x, node->z);
					dx0 = dx1;
					dz0 = dz1;
				}
			}
			else {
				path_add(&path, node->x, node->z);
				break;
			}

		}
		else {
			path_add(&path, node->x, node->z);
			break;
		}
		node = node->parent;
	}

	if (smooth == 0 || path.index == 2) {
		int i;
		for (i = path.index - 1; i >= 0; i--) {
			path_node_t* node = &path.nodes[i];
			result_cb(result_ud, node->x, node->z);
		}
	}
	else {
		path_node_t* node = &path.nodes[path.index - 1];
		result_cb(result_ud, node->x, node->z);

		int i, j;
		for (i = path.index - 1; i >= 2;) {
			int start = i;
			int last = start - 1;

			for (j = i - 2; j >= 0; j--) {
				path_node_t* start_node = &path.nodes[start];
				path_node_t* check_node = &path.nodes[j];

				int rx, rz;
				finder_raycast(finder, start_node->x, start_node->z, check_node->x, check_node->z, 1, &rx, &rz, NULL, NULL, NULL, NULL);
				if (rx == check_node->x && rz == check_node->z) {
					last = j;
				}
				else {
					break;
				}
			}
			node = &path.nodes[last];
			i = last;
			result_cb(result_ud, node->x, node->z);
		}

		node = &path.nodes[0];
		result_cb(result_ud, node->x, node->z);
	}
	path_release(&path);
}

pathfinder_t*
finder_create(int width, int heigh, char* data) {
	pathfinder_t *finder = (pathfinder_t*)malloc(sizeof(*finder));
	memset(finder, 0, sizeof(*finder));

	finder->width = width;
	finder->heigh = heigh;

	finder->node = (node_t*)malloc(width * heigh * sizeof(node_t));
	memset(finder->node, 0, width * heigh * sizeof(node_t));

	int i = 0;
	int j = 0;
	for (; i < finder->width; ++i) {
		for (j = 0; j < finder->heigh; ++j) {
			int index = i*finder->heigh + j;
			node_t *node = &finder->node[index];
			node->x = i;
			node->z = j;
			node->index = index;
			node->block = data[index];
			mh_init(&node->elt);

			if (!isblock(finder,node)) {
				finder->size++;
			}
		}
	}

#ifdef MINHEAP_USE_LIBEVENT
	mh_ctor(&finder->openlist, great);
#else
	mh_ctor(&finder->openlist, less);
#endif
	finder->closelist = NULL;

	finder_mask_reset(finder);
	finder_mask_set(finder, 0, 1);

	return finder;
}

void
finder_release(pathfinder_t* finder) {
	mh_dtor(&finder->openlist);

	free(finder->node);

	if (finder->movable)
		free(finder->movable);

	free(finder);
}

int
finder_find(pathfinder_t * finder, int x0, int z0, int x1, int z1, int smooth, finder_result result_cb, void* result_ud, finder_dump dump_cb, void* dump_ud, float cost) {
	node_t * from = find_node(finder, x0, z0);
	if (!from) {
		return FINDER_START_ERROR;
	}

	node_t * to = find_node(finder, x1, z1);
	if (!to || isblock(finder, to)) {
		//to = search_node(finder, x0, z0, x1, z1, NULL, NULL);
		to = search_node_in_circle(finder, x1, z1, 10, NULL, NULL);
		if (!to) {
			return FINDER_OVER_ERROR;
		}
	}

	if (from == to) {
		return FINDER_SAME_POINT_ERROR;
	}

	int result = FINDER_CANNOT_REACH;

	mh_push(&finder->openlist, &from->elt);
	node_t * node = NULL;

	while ((node = (node_t*)mh_pop(&finder->openlist)) != NULL) {
		node->next = finder->closelist;
		finder->closelist = node;
		node->closed = 1;

		if (node == to) {
			build_path(finder, node, from, smooth, result_cb, result_ud);
			result = FINDER_OK;
			break;
		}

		node_t* neighbors = NULL;

		find_neighbors(finder, node, &neighbors);
		while (neighbors) {
			node_t* nei = neighbors;

			if (mh_elt_has_init(&nei->elt)) {
				int nG = node->G + neighbor_estimate(node, nei);
				if (nG < nei->G) {
					nei->G = nG;
					nei->F = nei->G + nei->H;
					nei->parent = node;
					mh_adjust(&finder->openlist, &nei->elt);
				}
			}
			else {
				nei->parent = node;
				nei->G = node->G + neighbor_estimate(node, nei);
				nei->H = goal_estimate(nei, to, cost);
				nei->F = nei->G + nei->H;
				mh_push(&finder->openlist, &nei->elt);
				if (dump_cb != NULL) {
					dump_cb(dump_ud, nei->x, nei->z);
				}
			}

			neighbors = nei->next;
			nei->next = NULL;
		}
	}
	finder_reset(finder);
	return result;
}

void
raycast(pathfinder_t* finder, int x0, int z0, int x1, int z1, int ignore, int* resultx, int* resultz, int* stopx, int* stopz, finder_dump dump, void* ud) {
	float fx0 = x0 + 0.5f;
	float fz0 = z0 + 0.5f;
	float fx1 = x1 + 0.5f;
	float fz1 = z1 + 0.5f;
	float rx = fx0;
	float rz = fz0;
	int founded = 0;

	if (fx0 == fx1) {
		float z = z0;
		for (; z0 < z1 ? z <= z1 : z >= z1; z0 < z1 ? z++ : z--) {
			if (dump != NULL)
				dump(ud, x0, z);
			if (movable(finder, x0, z, ignore) == 0) {
				if (stopx) {
					*stopx = x0;
				}
				if (stopz) {
					*stopz = z;
				}
				founded = 1;
				break;
			}
			else {
				rx = x0;
				rz = z;
			}
		}
	}
	else {
		float slope = (fz1 - fz0) / (fx1 - fx0);
		if (abs(slope) < 1) {
			float inc = fx1 >= fx0 ? 1 : -1;
			float x = fx0;
			founded = 0;
			for (; fx1 >= fx0 ? x <= fx1 : x >= fx1; x += inc) {
				float z = slope * (x - fx0) + fz0;
				if (dump != NULL)
					dump(ud, x, z);
				if (movable(finder, x, z, ignore) == 0) {
					if (stopx) {
						*stopx = x;
					}
					if (stopz) {
						*stopz = z;
					}
					founded = 1;
					break;
				}
				else {
					rx = x;
					rz = z;
				}
			}
		}
		else {
			float inc = fz1 >= fz0 ? 1 : -1;
			float z = fz0;
			founded = 0;
			for (; fz1 >= fz0 ? z <= fz1 : z >= fz1; z += inc) {
				float x = (z - fz0) / slope + fx0;
				if (dump != NULL)
					dump(ud, x, z);
				if (movable(finder, x, z, ignore) == 0) {
					if (stopx) {
						*stopx = x;
					}
					if (stopz) {
						*stopz = z;
					}
					founded = 1;
					break;
				}
				else {
					rx = x;
					rz = z;
				}
			}
		}
	}

	*resultx = (int)rx;
	*resultz = (int)rz;
}

static inline void
swap(int* a, int *b) {
	int tmp = *a;
	*a = *b;
	*b = tmp;
}

//breshenhamÖ±ÏßËã·¨
void
raycast_breshenham(pathfinder_t* finder, int x0, int z0, int x1, int z1, int ignore, int* rx, int* rz, int* stopx, int* stopz, finder_dump dump, void* ud) {
	int dx = abs(x1 - x0);
	int dz = abs(z1 - z0);

	int steep = dz > dx ? 1 : 0;
	if (steep) {
		swap(&x0, &z0);
		swap(&x1, &z1);
		swap(&dx, &dz);
	}

	int xstep = x0 < x1 ? 1 : -1;
	int zstep = z0 < z1 ? 1 : -1;

	int x, z;

	int dt;
	for (dt = dz - dx; xstep == 1 ? x0 <= x1 : x1 <= x0;) {
		if (steep) {
			x = z0;
			z = x0;
		}
		else {
			x = x0;
			z = z0;
		}


		if (movable(finder, x, z, ignore) == 0) {
			if (stopx) {
				*stopx = x;
			}
			if (stopz) {
				*stopz = z;
			}
			return;
		}
		*rx = x;
		*rz = z;

		if (dump != NULL) {
			dump(ud, x, z);
		}

		if (dt >= 0) {
			z0 += zstep;
			dt -= dx;
		}
		x0 += xstep;
		dt += dz;
	}
}

void
finder_raycast(struct pathfinder* finder, int x0, int z0, int x1, int z1, int ignore, int* resultx, int* resultz, int* stopx, int* stopz, finder_dump dump, void* ud) {
#ifdef RAYCAST_BRESHENHAM
	raycast_breshenham(finder, x0, z0, x1, z1, ignore, resultx, resultz, stopx, stopz, dump, ud);
#else
	raycast(finder, x0, z0, x1, z1, ignore, resultx, resultz, stopx, stopz, dump, ud);
#endif
}

void
finder_bound(pathfinder_t * finder, int* width, int* heigh) {
	*width = finder->width;
	*heigh = finder->heigh;
}

void
finder_mask_set(pathfinder_t * finder, int index, int enable) {
	if (index < 0 || index >= MARK_MAX) {
		return;
	}
	finder->mask[index] = enable;
}

void
finder_mask_reset(pathfinder_t * finder) {
	int i = 0;
	for (; i < MARK_MAX; i++) {
		finder->mask[i] = 0;
	}
}

void
finder_mask_reverse(pathfinder_t * finder) {
	int i = 0;
	for (; i < MARK_MAX; i++) {
		finder->mask[i] = !finder->mask[i];
	}
}

void finder_random(struct pathfinder * finder, int* x, int* z) {
	if (finder->movable == NULL) {
		finder->movable = malloc(sizeof(int)* finder->size);

		int index = 0;
		int i;
		for (i = 0; i < finder->width * finder->heigh;i++) {
			node_t* node = &finder->node[i];
			if (!isblock(finder,node)) {
				finder->movable[index++] = i;
			}
		}
	}

	node_t* node = &finder->node[finder->movable[random_index(finder->size)]];

	*x = node->x;
	*z = node->z;
}

int 
finder_random_in_circle(struct pathfinder * finder, int cx, int cz, int r, int* x, int* z) {
	int tx = 0;
	int tz = r;

	int* node_index = malloc((2 * r)*(2 * r) * sizeof(int));
	int index = 0;

	int d = 3 - 2 * r;
	while (tx <= tz) {
		int zi;
		for (zi = tx; zi <= tz;zi++) {
			int dir[][2] = { { tx, zi }, { -tx, zi }, { tx, -zi }, { -tx, -zi }, { zi, tx }, { -zi, tx }, { zi, -tx }, { -zi, -tx } };
			int j;
			for (j = 0; j < 8; j++) {
				node_t* node = find_node(finder, cx + dir[j][0], cz + dir[j][1]);
				if (node && node->recorded == 0) {
					node->recorded = 1;
					
					if (!isblock(finder, node)) {
						node_index[index++] = node->index;
					}
				}
			}
		}
		
		if (d < 0) {
			d = d + 4 * tx + 6;
		}
		else {
			d = d + 4 * (tx - tz) + 10;
			tz--;
		}
		tx++;
	}

	if (index == 0) {
		return -1;
	}

	node_t* node = &finder->node[node_index[random_index(index)]];

	*x = node->x;
	*z = node->z;

	int i;
	for (i = 0; i < index;i++) {
		node_t* node = &finder->node[node_index[i]];
		node->recorded = 0;
	}
	free(node_index);
	return 0;
}

int
finder_movable(pathfinder_t * finder, int x, int z, int ignore) {
	return movable(finder, x, z, ignore);
}
