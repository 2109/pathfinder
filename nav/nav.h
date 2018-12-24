
#ifndef NAV_H
#define NAV_H
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>

#ifdef _MSC_VER
#define inline __inline
#endif

#define MINHEAP_USE_LIBEVENT

#include "minheap-adapter.h"

#define USE_NAV_TILE

#define GRATE 1
#define HRATE 2

#define get_border(ctx,id) ((id < 0 || id > ctx->border_offset)?NULL:&ctx->borders[id])

#define get_node(ctx,id) ((id < 0 || id >= ctx->node_size) ?NULL:&ctx->node[id])

#define get_mask(ctx,index) (ctx.mask[index])

struct vector3
{
	double x;
	double y;
	double z;
};

struct nav_path
{
	struct vector3* wp;
	int offset;
	int size;
};

struct nav_mesh_mask
{
	int *mask;
	int size;
};

struct nav_node
{
	mh_elt_t elt;
	int id;

	int* poly;
	int* border;
	int size;

	int mask;

	int dt_recorded;

	double area;

	//多边形的中心点
	struct vector3 center;

	double G;
	double H;
	double F;

	struct nav_node* next;
	int closed;

	//缓存A*寻路出来的相邻多边形和与相邻多边形共边的边
	struct nav_node* link_parent;
	int link_border;

	int reserve;
	struct vector3 pos;
};

struct nav_border
{
	int id;
	int node[2];
	int a;
	int b;
	int opposite;
	struct vector3 center;
};

struct nav_tile
{
	int* node;
	int offset;
	int size;
	int center_node;
	struct vector3 center;
#ifdef _WIN32
	struct vector3 pos[4];
#endif
};

struct nav_mesh_context
{
	//顶点
	struct vector3 * vertices;
	int vertices_size;

	//所有边(同一条边有ab和ba两条)
	struct nav_border* borders;
	int border_size;
	int border_offset;

	//多边形节点
	struct nav_node* node;
	int node_size;

	double area;

	//格子信息
	struct nav_tile* tile;
	uint32_t tile_unit;
	uint32_t tile_width;
	uint32_t tile_heigh;

	struct vector3 lt;
	struct vector3 br;

	uint32_t width;
	uint32_t heigh;

	//多边形节点的mask
	struct nav_mesh_mask mask_ctx;

	//寻路结果缓存
	struct nav_path result;

	mh_t openlist;
	struct nav_node* closelist;
};

typedef void(*search_dumper)( void* ud, int index );

struct nav_mesh_context* load_mesh(double** v, int v_cnt, int** p, int p_cnt);
void init_mesh(struct nav_mesh_context* ctx);
void release_mesh(struct nav_mesh_context* ctx);

struct nav_node* search_node(struct nav_mesh_context* mesh_ctx, double x, double y, double z);
struct nav_path* astar_find(struct nav_mesh_context* mesh_ctx, struct vector3* pt_start, struct vector3* pt_over, search_dumper dumper, void* args);
bool raycast(struct nav_mesh_context* ctx, struct vector3* pt_start, struct vector3* pt_over, struct vector3* result, search_dumper dumper, void* userdata);

void set_mask(struct nav_mesh_mask* ctx, int mask, int enable);

struct vector3* around_movable(struct nav_mesh_context*, double x, double z, int range, int* center_node, search_dumper, void*);
bool point_movable(struct nav_mesh_context* ctx, double x, double z, double fix, double* dt_offset);
bool point_height(struct nav_mesh_context* ctx, double x, double z, double* height);
void point_random(struct nav_mesh_context* ctx, struct vector3* result, int poly);

bool intersect(struct vector3* a, struct vector3* b, struct vector3* c, struct vector3* d);
bool inside_node(struct nav_mesh_context* mesh_ctx, int polyId, double x, double y, double z);
double cross_product_direction(struct vector3* vt1, struct vector3* vt2);
void cross_point(struct vector3* a, struct vector3* b, struct vector3* c, struct vector3* d, struct vector3* result);
void vector3_copy(struct vector3* dst, struct vector3* src);
void vector3_sub(struct vector3* a, struct vector3* b, struct vector3* result);

struct nav_tile* create_tile(struct nav_mesh_context* ctx, uint32_t unit);
void release_tile(struct nav_mesh_context* ctx, struct nav_tile* navtile);

#endif