

#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>


#include "pathfinder.h"

typedef struct node {
	struct element elt;
	struct node *next;
	struct node *parent;
	uint32_t *link;
	uint32_t size;
	uint32_t x;
	uint32_t z;
	float G;
	float H;
	float F;
	int closed;
} node_t;

typedef struct pathfinder {
	node_t *node;
	uint32_t size;
	struct minheap* openlist;
	node_t *closelist;
} pathfinder_t;

inline double
dt_dot2dot(int x0,int z0,int x1,int z1) {
	double dx = x0 - x1;
	double dz = z0 - z1;
	return sqrt(dx * dx + dz * dz);
}

static inline node_t*
search_node(pathfinder_t* finder, int x, int z) {
	uint32_t dt_min = -1;
	uint32_t result;
	uint32_t i;
	for ( i = 0; i < finder->size;i++ ) {
		node_t* node = &finder->node[i];
		uint32_t dt = dt_dot2dot(x, z, node->x, node->z);
		if (dt_min == -1 || dt < dt_min) {
			dt_min = dt;
			result = i;
		}
	}
	return &finder->node[result];
}

static inline void
find_neighbors(pathfinder_t * finder, node_t * node, node_t** neighbours) {

	uint32_t i;
	for ( i = 0; i < node->size; i++ ) {
		node_t * nei = &finder->node[node->link[i]];

		if (nei->next) {
			continue;
		}

		nei->next = ( *neighbours );
		( *neighbours ) = nei;
	}
}

static inline double
g_cost(node_t * from, node_t * to) {
	return dt_dot2dot(from->x, from->z, to->x, to->z);
}

static inline double
h_cost(node_t * from, node_t * to) {
	return dt_dot2dot(from->x, from->z, to->x, to->z);
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
	node_t *node = (node_t*)(elt);
	clear_node(node);
}

static inline void
reset(pathfinder_t* finder) {
	node_t * node = finder->closelist;
	while (node) {
		node_t * tmp = node;
		node = tmp->next;
		clear_node(tmp);
	}
	minheap_clear(finder->openlist, heap_clear);
}

static inline int
less(struct element * left, struct element * right) {
	node_t *l = (node_t*)( left );
	node_t *r = (node_t*)( right );
	return l->F < r->F;
}

void
make_path(pathfinder_t *finder, node_t *current, node_t *from, finder_result cb, void* ud) {
	cb(ud, current->x, current->z);

	node_t * parent = current->parent;
	assert(parent != NULL);

	current = parent;
	while ( current ) {
		if ( current != from ) {
			parent = current->parent;
			cb(ud, current->x, current->z);
		}
		else {
			cb(ud, current->x, current->z);
			break;
		}
		current = current->parent;
	}
}

pathfinder_t*
finder_create(const char* file) {
	pathfinder_t *finder = (pathfinder_t*)malloc(sizeof( *finder ));
	memset(finder, 0, sizeof( *finder ));

	FILE* fp = fopen(file, "rb");

	fread((void*)&finder->size, sizeof( uint32_t ), 1, fp);

	finder->node = malloc(sizeof(node_t)* finder->size);

	uint32_t i, j;
	for ( i = 0; i < finder->size; i++ ) {
		node_t* node = &finder->node[i];
		memset(node, 0, sizeof( node_t ));

		fread((void*)&node->x, sizeof( uint32_t ), 1, fp);
		fread((void*)&node->z, sizeof( uint32_t ), 1, fp);
	}

	for ( i = 0; i < finder->size; i++ ) {
		node_t* node = &finder->node[i];

		fread((void*)&node->size, sizeof( uint32_t ), 1, fp);

		if ( node->size > 0 ) {
			node->link = malloc(sizeof(uint32_t)* node->size);
			for ( j = 0; j < node->size; j++ )
				fread((void*)&node->link[j], sizeof( uint32_t ), 1, fp);
		}
	}

	fclose(fp);

	finder->openlist = minheap_create(50 * 50, less);
	finder->closelist = NULL;

	return finder;
}

void
finder_release(pathfinder_t* finder) {
	uint32_t i;
	for ( i = 0; i < finder->size;i++ )
	{
		node_t* node = &finder->node[i];
		if (node->link)
			free(node->link);
	}
	free(finder->node);
	minheap_release(finder->openlist);
	free(finder);
}

int
finder_find(pathfinder_t * finder, int x0, int z0, int x1, int z1, finder_result cb, void* result_ud, finder_dump dump, void* dump_ud) {
	node_t * from = search_node(finder, x0, z0);
	node_t * to = search_node(finder, x1, z1);

	if ( !from || !to || from == to )
		return -1;

	minheap_push(finder->openlist, &from->elt);

	node_t * current = NULL;

	while ( ( current = (node_t*)minheap_pop(finder->openlist) ) != NULL ) {
		current->next = finder->closelist;
		finder->closelist = current;
		current->closed = 1;

		if ( current == to ) {
			make_path(finder, current, from, cb, result_ud);
			reset(finder);
			return 1;
		}

		node_t* neighbors = NULL;

		find_neighbors(finder, current, &neighbors);

		while ( neighbors ) {
			node_t* node = neighbors;
			if ( node->elt.index ) {
				int nG = current->G + g_cost(current, node);
				if ( nG < node->G ) {
					node->G = nG;
					node->F = node->G + node->H;
					node->parent = current;
					minheap_change(finder->openlist, &node->elt);
				}
			}
			else {
				node->parent = current;
				node->G = current->G + g_cost(current, node);
				node->H = h_cost(node, to);
				node->F = node->G + node->H;
				minheap_push(finder->openlist, &node->elt);
				if ( dump != NULL )
					dump(dump_ud, node->x, node->z);
			}

			neighbors = node->next;
			node->next = NULL;
		}
	}
	reset(finder);
	return -1;
}

