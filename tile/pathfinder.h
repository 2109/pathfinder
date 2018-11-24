#ifndef PATHFINDER_H
#define PATHFINDER_H


#ifdef _MSC_VER
#define inline __inline
#endif

#define MINHEAP_USE_LIBEVENT

#define FINDER_OK				 0
#define FINDER_CANNOT_REACH		-1
#define FINDER_START_ERROR		-2
#define FINDER_OVER_ERROR		-3
#define FINDER_SAME_POINT_ERROR	-4


struct pathfinder;
struct node;

typedef void(*finder_result)(void *ud, int x, int z);
typedef void(*finder_dump)(void* ud, int x, int z);

struct pathfinder* finder_create(int width, int heigh, char* data);
void finder_release(struct pathfinder* finder);

int finder_find(struct pathfinder * finder, int x0, int z0, int x1, int z1, int smooth, finder_result cb, void* result_ud, finder_dump dump, void* dump_ud,float cost);
void finder_raycast(struct pathfinder* finder, int x0, int z0, int x1, int z1, int ignore, int* resultx, int* resultz, int* stopx, int* stopz, finder_dump dump, void* ud);

void finder_bound(struct pathfinder* finder, int* width, int* heigh);
int finder_movable(struct pathfinder * finder, int x, int z, int ignore);
void finder_mask_set(struct pathfinder * finder, int mask_index, int enable);
void finder_mask_reset(struct pathfinder * finder);
void finder_mask_reverse(struct pathfinder * finder);


#endif