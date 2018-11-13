#ifndef PATHFINDER_H
#define PATHFINDER_H

#ifndef _MSC_VER
#include <stdbool.h>
#else
#define inline __inline
#endif

#include "minheap.h"
#include "minheap-internal.h"

#define MINHEAP_USE_LIBEVENT

#include "minheap-adapter.h"

#define ERROR_CANNOT_REACH	-1
#define ERROR_START_POINT	-2
#define ERROR_OVER_POINT	-3
#define ERROR_SAME_POINT	-4


struct pathfinder;
struct node;

typedef void(*finder_result)(void *ud, int x, int z);
typedef void(*finder_dump)(void* ud, int x, int z);

struct pathfinder* finder_create(int width, int heigh, char* data);
void finder_release(struct pathfinder* finder);
void finder_raycast(struct pathfinder* finder, int x0, int z0, int x1, int z1, int ignore, int* resultx, int* resultz, finder_dump dump, void* ud);
int finder_find(struct pathfinder * finder, int x0, int z0, int x1, int z1, int smooth, finder_result cb, void* result_ud, finder_dump dump, void* dump_ud,float cost);
int finder_movable(struct pathfinder * finder, int x, int z, int ignore);
void finder_mask_set(struct pathfinder * finder, int mask_index, int enable);
void finder_mask_reset(struct pathfinder * finder);
struct node* search_node(struct pathfinder* finder, int x, int z, int extend, finder_dump dump, void* ud );

#endif