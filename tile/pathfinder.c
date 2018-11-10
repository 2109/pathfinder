

#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>


#include "pathfinder.h"


#define MARK_MAX 64
#define INIT_PATH_SIZE 64

#define SQUARE(val) (val*val)
#define GOAL_COST(from,to,cost) (abs(from->x - to->x) * cost + abs(from->z - to->z) * cost)
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
	struct element elt;
	struct node *next;
	struct node *parent;
	int x;
	int z;
	int	block;
	float G;
	float H;
	float F;
	int closed;
} node_t;

typedef struct pathfinder {
	int width;
	int heigh;
	node_t *node;
	char *data;
	char mask[MARK_MAX];

	struct minheap* openlist;
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
	if ( x < 0 || x >= finder->width || z < 0 || z >= finder->heigh )
		return NULL;
	return &finder->node[x*finder->heigh + z];
}

static inline int
isblock(pathfinder_t* finder, node_t* node) {
	return node->block != 0;
}

node_t*
search_node(pathfinder_t* finder, int x, int z, int extend, finder_dump dump, void* ud) {
	int i;
	for ( i = 1; i <= extend; i++ ) {
		int j;
		for ( j = 0; j < 8; j++ ) {
			int tx = x + DIRECTION[j][0] * i;
			int tz = z + DIRECTION[j][1] * i;
			node_t * node = find_node(finder, tx, tz);
			if ( dump ) {
				dump(ud, node->x, node->z);
			}
			
			if ( !isblock(finder, node) ) {
				return node;
			}
		}
	}
	return NULL;
}

static inline int
movable(pathfinder_t* finder, int x, int z, int ignore) {
	node_t *node = find_node(finder, x, z);
	if ( node == NULL )
		return 0;
	if ( ignore )
		return !isblock(finder, node);
	return finder->mask[node->block] != 1;
}

static inline void
find_neighbors(pathfinder_t * finder, struct node * node, node_t **neighbours) {
	int i;
	for ( i = 0; i < 8; i++ ) {
		int x = node->x + DIRECTION[i][0];
		int z = node->z + DIRECTION[i][1];
		node_t * nei = find_node(finder, x, z);
		if ( nei && nei->closed == 0 ) {
			if ( !isblock(finder, nei) ) {
				nei->next = ( *neighbours );
				( *neighbours ) = nei;
			}
		}
	}
}

static inline float
neighbor_cost(node_t * from, node_t * to) {
	int dx = from->x - to->x;
	int dz = from->z - to->z;
	int i;
	for ( i = 0; i < 8; ++i ) {
		if ( DIRECTION[i][0] == dx && DIRECTION[i][1] == dz )
			break;
	}
	if ( i < 4 )
		return 50.0f;
	return 60.0f;
}

static inline void
clear_node(node_t* node) {
	node->parent = NULL;
	node->F = node->G = node->H = 0;
	node->elt.index = 0;
	node->next = NULL;
	node->closed = 0;
}

static inline void
heap_clear(struct element* elt) {
	node_t *node = (node_t*)elt;
	clear_node(node);
}

static inline void
reset(pathfinder_t* finder) {
	node_t * node = finder->closelist;
	while ( node ) {
		node_t * tmp = node;
		node = tmp->next;
		clear_node(tmp);
	}
	finder->closelist = NULL;
	minheap_clear(finder->openlist, heap_clear);
}

static inline int
less(struct element * left, struct element * right) {
	node_t *l = (node_t*)left;
	node_t *r = (node_t*)right;
	return l->F < r->F;
}

static void
path_init(path_t* path) {
	path->index = 0;
	path->size = INIT_PATH_SIZE;
	path->nodes = path->init;
}

static void
path_release(path_t* path) {
	if ( path->nodes != path->init ) {
		free(path->nodes);
	}
}

static void
path_add(path_t* path, int x, int z) {
	if ( path->index >= path->size ) {
		int nsize = path->size * 2;
		path_node_t* nnodes = malloc(nsize * sizeof( path_node_t ));
		memcpy(nnodes, path->nodes, path->size * sizeof( path_node_t ));
		path->size = nsize;
		if ( path->init != path->nodes ) {
			free(path->nodes);
		}
		path->nodes = nnodes;
	}
	path->nodes[path->index].x = x;
	path->nodes[path->index].z = z;
	path->index++;
}

void
make_path(pathfinder_t *finder, node_t *current, node_t *from, int smooth, finder_result cb, void* ud) {
	path_t path;
	path_init(&path);

	path_add(&path, current->x, current->z);

	node_t * parent = current->parent;
	assert(parent != NULL);

	int dx0 = DX(current, parent);
	int dz0 = DZ(current, parent);

	current = parent;
	while ( current ) {
		if ( current != from ) {
			parent = current->parent;
			if ( parent != NULL ) {
				int dx1 = DX(current, parent);
				int dz1 = DZ(current, parent);
				if ( dx0 != dx1 || dz0 != dz1 ) {
					path_add(&path, current->x, current->z);
					dx0 = dx1;
					dz0 = dz1;
				}
			}
			else {
				path_add(&path, current->x, current->z);
				break;
			}

		}
		else {
			path_add(&path, current->x, current->z);
			break;
		}
		current = current->parent;
	}

	if (smooth == 0 || path.index == 2) {
		int i;
		for ( i = path.index - 1; i >= 0;i-- ) {
			path_node_t* node = &path.nodes[i];
			cb(ud, node->x, node->z);
		}
	} else {
		path_node_t* node = &path.nodes[path.index - 1];
		cb(ud, node->x, node->z);

		int i, j;
		for ( i = path.index - 1; i >= 2; ) {
			int start = i;
			int last = start - 1;

			for ( j = i - 2; j >= 0;j-- ) {
				path_node_t* start_node = &path.nodes[start];
				path_node_t* check_node = &path.nodes[j];

				int rx, rz;
				finder_raycast(finder, start_node->x, start_node->z, check_node->x, check_node->z, 1, &rx, &rz, NULL, NULL);
				if (rx == check_node->x && rz == check_node->z) {
					last = j;
				} else {
					break;
				}
			}
			node = &path.nodes[last];
			i = last;
			cb(ud, node->x, node->z);
		}

		node = &path.nodes[0];
		cb(ud, node->x, node->z);
	}
	path_release(&path);
}

pathfinder_t*
finder_create(int width, int heigh, char* data) {
	pathfinder_t *finder = (pathfinder_t*)malloc(sizeof( *finder ));
	memset(finder, 0, sizeof( *finder ));

	finder->width = width;
	finder->heigh = heigh;

	finder->node = (node_t*)malloc(width * heigh * sizeof( node_t ));
	memset(finder->node, 0, width * heigh * sizeof( node_t ));

	finder->data = (char*)malloc(width * heigh);
	memset(finder->data, 0, width * heigh);
	memcpy(finder->data, data, width * heigh);

	int i = 0;
	int j = 0;
	for ( ; i < finder->width; ++i ) {
		for ( j = 0; j < finder->heigh; ++j ) {
			int index = i*finder->heigh + j;
			node_t *node = &finder->node[index];
			node->x = i;
			node->z = j;
			node->block = finder->data[index];
		}
	}

	finder->openlist = minheap_create(50 * 50, less);
	finder->closelist = NULL;

	return finder;
}

void
finder_release(pathfinder_t* finder) {
	free(finder->node);
	free(finder->data);
	minheap_release(finder->openlist);
	free(finder);
}

int
finder_find(pathfinder_t * finder, int x0, int z0, int x1, int z1, int smooth, finder_result cb, void* result_ud, finder_dump dump, void* dump_ud, float cost) {
	node_t * from = find_node(finder, x0, z0);
	if (!from) {
		return ERROR_START_POINT;
	}

	node_t * to = find_node(finder, x1, z1);
	if ( !to || isblock(finder, to) ) {
		to = search_node(finder, x1, z1, 5, NULL, NULL);
		if (!to) {
			return ERROR_OVER_POINT;
		}
	}

	if ( from == to ) {
		return ERROR_SAME_POINT;
	}

	minheap_push(finder->openlist, &from->elt);

	node_t * current = NULL;

	while ( ( current = (node_t*)minheap_pop(finder->openlist) ) != NULL ) {
		current->next = finder->closelist;
		finder->closelist = current;
		current->closed = 1;

		if ( current == to ) {
			make_path(finder, current, from, smooth, cb, result_ud);
			reset(finder);
			return 0;
		}

		node_t* neighbors = NULL;

		find_neighbors(finder, current, &neighbors);
		while ( neighbors ) {
			node_t* node = neighbors;

			if ( node->elt.index ) {
				int nG = current->G + neighbor_cost(current, node);
				if ( nG < node->G ) {
					node->G = nG;
					node->F = node->G + node->H;
					node->parent = current;
					minheap_change(finder->openlist, &node->elt);
				}
			}
			else {
				node->parent = current;
				node->G = current->G + neighbor_cost(current, node);
				node->H = GOAL_COST(node, to, cost);
				node->F = node->G + node->H;
				minheap_push(finder->openlist, &node->elt);
				if ( dump != NULL ) {
					dump(dump_ud, node->x, node->z);
				}	
			}

			neighbors = node->next;
			node->next = NULL;
		}
	}
	reset(finder);
	return ERROR_CANNOT_REACH;
}

void
raycast(pathfinder_t* finder, int x0, int z0, int x1, int z1, int ignore, int* resultx, int* resultz, finder_dump dump, void* ud) {
	float fx0 = x0 + 0.5f;
	float fz0 = z0 + 0.5f;
	float fx1 = x1 + 0.5f;
	float fz1 = z1 + 0.5f;
	float rx = fx0;
	float rz = fz0;
	int founded = 0;

	if ( fx0 == fx1 ) {
		float z = z0;
		for ( ; z0 < z1 ? z <= z1 : z >= z1; z0 < z1 ? z++ :z--) {
			if ( dump != NULL )
				dump(ud, x0, z);
			if ( movable(finder, x0, z, ignore) == 0 ) {
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
		float slope = ( fz1 - fz0 ) / ( fx1 - fx0 );
		if ( abs(slope) < 1 ) {
			float inc = fx1 >= fx0 ? 1 : -1;
			float x = fx0;
			founded = 0;
			for ( ; fx1 >= fx0 ? x <= fx1 : x >= fx1; x += inc ) {
				float z = slope * ( x - fx0 ) + fz0;
				if ( dump != NULL )
					dump(ud, x, z);
				if ( movable(finder, x, z, ignore) == 0 ) {
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
			for ( ; fz1 >= fz0 ? z <= fz1 : z >= fz1; z += inc ) {
				float x = ( z - fz0 ) / slope + fx0;
				if ( dump != NULL )
					dump(ud, x, z);
				if ( movable(finder, x, z, ignore) == 0 ) {
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

	if ( founded == 0 && movable(finder, (int)fx1, (int)fz1, ignore) == 1 ) {
		rx = (float)x1;
		rz = (float)z1;
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
raycast_breshenham(pathfinder_t* finder, int x0, int z0, int x1, int z1, int ignore, int* rx, int* rz, finder_dump dump, void* ud) {
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
	for ( dt = dz - dx; xstep == 1 ? x0 <= x1 : x1 <= x0; ) {
		if (steep) {
			x = z0;
			z = x0;
		} else {
			x = x0;
			z = z0;
		}
		
		
		if ( movable(finder, x, z, ignore) == 0 ) {
			return;
		}
		*rx = x;
		*rz = z;

		if ( dump != NULL ) {
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
finder_raycast(struct pathfinder* finder, int x0, int z0, int x1, int z1, int ignore, int* resultx, int* resultz, finder_dump dump, void* ud) {
#ifdef RAYCAST_BRESHENHAM
	raycast_breshenham(finder,x0,z0,x1,z1,ignore,resultx,resultz,dump,ud);
#else
	raycast(finder, x0, z0, x1, z1, ignore, resultx, resultz, dump, ud);
#endif
}

int
finder_movable(pathfinder_t * finder, int x, int z, int ignore) {
	return movable(finder, x, z, ignore);
}

void
finder_mask_set(pathfinder_t * finder, int index, int enable) {
	if ( index < 0 || index >= MARK_MAX ) {
		return;
	}
	finder->mask[index] = enable;
}

void
finder_mask_reset(pathfinder_t * finder) {
	int i = 0;
	for ( ; i < MARK_MAX; i++ )
		finder->mask[i] = 0;
}
