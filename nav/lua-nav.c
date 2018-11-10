

#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include <lua.h>
#include <lauxlib.h>

#include "nav.h"

#ifdef WIN32
#define EXPORT  __declspec(dllexport)
#else
#define EXPORT 
#endif

struct scene_context {
	struct nav_mesh_context* ctx;
	int scene;
};

static int
meta_nav_info(struct lua_State* L) {
	struct scene_context* scene_ctx = ( struct scene_context* )lua_touserdata(L, 1);
	struct nav_mesh_context* ctx = scene_ctx->ctx;
	lua_newtable(L);

	lua_newtable(L);
	lua_pushnumber(L, ctx->lt.x);
	lua_setfield(L, -2, "x");
	lua_pushnumber(L, ctx->lt.z);
	lua_setfield(L, -2, "z");
	lua_setfield(L, -2, "lt");

	lua_newtable(L);
	lua_pushnumber(L, ctx->br.x);
	lua_setfield(L, -2, "x");
	lua_pushnumber(L, ctx->br.z);
	lua_setfield(L, -2, "z");
	lua_setfield(L, -2, "br");

	lua_pushinteger(L, ctx->width);
	lua_setfield(L, -2, "width");

	lua_pushinteger(L, ctx->heigh);
	lua_setfield(L, -2, "heigh");

	lua_newtable(L);
	int i;
	for ( i = 0; i < ctx->vertices_size; i++ ) {
		lua_newtable(L);
		lua_pushnumber(L, ctx->vertices[i].x);
		lua_setfield(L, -2, "x");
		lua_pushnumber(L, ctx->vertices[i].z);
		lua_setfield(L, -2, "z");
		lua_rawseti(L, -2, i + 1);
	}
	lua_setfield(L, -2, "vertices");

	lua_newtable(L);
	for ( i = 0; i < ctx->node_size; i++ ) {
		lua_newtable(L);

		struct nav_node* node = &ctx->node[i];
		lua_pushinteger(L, node->id);
		lua_setfield(L, -2, "id");

		lua_newtable(L);
		lua_pushnumber(L, node->center.x);
		lua_setfield(L, -2, "x");
		lua_pushnumber(L, node->center.z);
		lua_setfield(L, -2, "z");
		lua_setfield(L, -2, "center");

		lua_pushinteger(L, node->mask);
		lua_setfield(L, -2, "mask");

		lua_newtable(L);
		int j;
		for ( j = 0; j < node->size; j++ ) {
			lua_pushinteger(L, node->poly[j]);
			lua_rawseti(L, -2, j + 1);
		}
		lua_setfield(L, -2, "poly");

		lua_newtable(L);
		for ( j = 0; j < node->size; j++ ) {
			lua_pushinteger(L, node->border[j]);
			lua_rawseti(L, -2, j + 1);
		}
		lua_setfield(L, -2, "border");

		lua_rawseti(L, -2, i + 1);
	}
	lua_setfield(L, -2, "node");

	lua_newtable(L);
	for ( i = 0; i < ctx->border_offset; i++ ) {
		struct nav_border* border = &ctx->borders[i];
		lua_newtable(L);

		lua_pushinteger(L, i);
		lua_setfield(L, -2, "id");

		lua_pushinteger(L, border->a);
		lua_setfield(L, -2, "a");

		lua_pushinteger(L, border->b);
		lua_setfield(L, -2, "b");

		lua_pushinteger(L, border->opposite);
		lua_setfield(L, -2, "opposite");

		lua_newtable(L);
		lua_pushinteger(L, border->node[0]);
		lua_rawseti(L, -2, 1);
		lua_pushinteger(L, border->node[1]);
		lua_rawseti(L, -2, 2);
		lua_setfield(L, -2, "node");

		lua_newtable(L);
		lua_pushnumber(L, border->center.x);
		lua_setfield(L, -2, "x");
		lua_pushnumber(L, border->center.z);
		lua_setfield(L, -2, "z");
		lua_setfield(L, -2, "center");

		lua_rawseti(L, -2, i + 1);
	}
	lua_setfield(L, -2, "border");

	return 1;
}

static int
meta_tile_info(struct lua_State* L) {
	struct scene_context* scene_ctx = ( struct scene_context* )lua_touserdata(L, 1);
	struct nav_mesh_context* ctx = scene_ctx->ctx;
	if ( ctx->tile == NULL ) {
		return 0;
	}

	lua_newtable(L);
	uint32_t i;
	for ( i = 0; i < ctx->tile_width * ctx->tile_heigh; i++ ) {
		struct nav_tile* tile = &ctx->tile[i];

		lua_newtable(L);

		lua_newtable(L);
		int j;
		for ( j = 0; j < tile->offset; j++ ) {
			lua_pushinteger(L, tile->node[j]);
			lua_rawseti(L, -2, j + 1);
		}
		lua_setfield(L, -2, "node");

		lua_newtable(L);

		lua_pushnumber(L, tile->center.x);
		lua_rawseti(L, -2, 1);

		lua_pushnumber(L, tile->center.z);
		lua_rawseti(L, -2, 2);

		lua_setfield(L, -2, "center");

		lua_pushinteger(L, tile->center_node);
		lua_setfield(L, -2, "center_node");

		lua_rawseti(L, -2, i + 1);
	}
	return 1;
}

static int
meta_create_tile(struct lua_State* L) {
	struct scene_context* scene_ctx = ( struct scene_context* )lua_touserdata(L, 1);
	uint32_t unit = luaL_optinteger(L, 2, 1);
	struct nav_mesh_context* ctx = scene_ctx->ctx;
	ctx->tile = create_tile(ctx, unit);
	return 0;
}

static int
meta_load_tile(struct lua_State* L) {
	struct scene_context* scene_ctx = ( struct scene_context* )lua_touserdata(L, 1);
	struct nav_mesh_context* ctx = scene_ctx->ctx;

	size_t size;
	const char* file = lua_tolstring(L, 2, &size);
	FILE* fp = fopen(file, "rb");

	if ( !fp ){
		luaL_error(L, "error open file:%s", file);
	}

	uint32_t tile_unit;
	fread(&tile_unit, sizeof( uint32_t ), 1, fp);

	uint32_t count = 0;
	fread(&count, sizeof( uint32_t ), 1, fp);

	ctx->tile = malloc(sizeof( struct nav_tile )*count);
	memset(ctx->tile, 0, sizeof( struct nav_tile )*count);

	uint32_t i;
	for ( i = 0; i < count; i++ ) {

		struct nav_tile* tile = &ctx->tile[i];

		uint32_t size;
		fread(&size, sizeof( uint32_t ), 1, fp);

		tile->offset = tile->size = size;
		tile->node = NULL;
		if ( tile->offset != 0 ) {
			tile->node = malloc(sizeof(int)*tile->offset);
			uint32_t j;
			for ( j = 0; j < size; j++ ) {
				uint32_t val;
				fread(&val, sizeof( uint32_t ), 1, fp);
				tile->node[j] = val;
			}
		}

		float x, z;
		fread(&x, sizeof( float ), 1, fp);
		fread(&z, sizeof( float ), 1, fp);

		tile->center.x = x;
		tile->center.y = 0;
		tile->center.z = z;

		int center_node;
		fread(&center_node, sizeof( int ), 1, fp);
		tile->center_node = center_node;
	}
	fclose(fp);

	ctx->tile_unit = tile_unit;
	ctx->tile_width = ctx->width / tile_unit + 1;
	ctx->tile_heigh = ctx->heigh / tile_unit + 1;
	return 0;
}

static int
meta_release(struct lua_State* L) {
	struct scene_context* scene_ctx = ( struct scene_context* )lua_touserdata(L, 1);
	struct nav_mesh_context* ctx = scene_ctx->ctx;
	release_mesh(ctx);
	return 0;
}

static int
meta_set_mask(struct lua_State* L) {
	struct scene_context* scene_ctx = ( struct scene_context* )lua_touserdata(L, 1);
	struct nav_mesh_context* ctx = scene_ctx->ctx;
	int mask = lua_tointeger(L, 2);
	int enable = lua_tointeger(L, 3);
	set_mask(&ctx->mask_ctx, mask, enable);
	return 0;
}

static int
meta_get_mask(struct lua_State* L) {
	struct scene_context* scene_ctx = ( struct scene_context* )lua_touserdata(L, 1);
	struct nav_mesh_context* ctx = scene_ctx->ctx;
	int index = lua_tointeger(L, 2);
	int mask = get_mask(ctx->mask_ctx, index);
	lua_pushinteger(L, mask);
	return 1;
}

static int
meta_find(struct lua_State* L) {
	struct scene_context* scene_ctx = ( struct scene_context* )lua_touserdata(L, 1);
	struct nav_mesh_context* ctx = scene_ctx->ctx;

	struct vector3 start, over;
	start.x = lua_tonumber(L, 2);
	start.z = lua_tonumber(L, 3);
	over.x = lua_tonumber(L, 4);
	over.z = lua_tonumber(L, 5);

	struct nav_path* path = astar_find(ctx, &start, &over, NULL, NULL);
	if ( !path )
		return 0;

	lua_createtable(L, path->offset, 0);
	int i;
	for ( i = 0; i < path->offset; i++ ) {
		lua_pushinteger(L, i + 1);
		lua_createtable(L, 0, 2);
		lua_pushnumber(L, path->wp[i].x);
		lua_setfield(L, -2, "x");
		lua_pushnumber(L, path->wp[i].z);
		lua_setfield(L, -2, "z");
		lua_settable(L, -3);
	}

	return 1;
}

static int
meta_raycast(struct lua_State* L) {
	struct scene_context* scene_ctx = ( struct scene_context* )lua_touserdata(L, 1);
	struct nav_mesh_context* ctx = scene_ctx->ctx;

	struct vector3 start, over, result;
	start.x = lua_tonumber(L, 2);
	start.z = lua_tonumber(L, 3);
	over.x = lua_tonumber(L, 4);
	over.z = lua_tonumber(L, 5);

	bool ok = raycast(ctx, &start, &over, &result, NULL, NULL);
	if ( !ok ) {
		return 0;
	}

	lua_pushnumber(L, result.x);
	lua_pushnumber(L, result.z);
	return 2;
}

static int
meta_around_movable(struct lua_State* L) {
	struct scene_context* scene_ctx = ( struct scene_context* )lua_touserdata(L, 1);
	struct nav_mesh_context* ctx = scene_ctx->ctx;

	double x = lua_tonumber(L, 2);
	double z = lua_tonumber(L, 3);

	int val = point_movable(ctx, x, z, 0, NULL);
	if ( val ) {
		lua_pushnumber(L, x);
		lua_pushnumber(L, z);
		return 2;
	}

	int r = luaL_optinteger(L, 4, 3);

	struct vector3* pos = around_movable(ctx, x, z, r, NULL, NULL, NULL);
	if ( pos == NULL )
		return 0;

	struct vector3 over, result;
	over.x = x;
	over.z = z;

	bool ok = raycast(ctx, pos, &over, &result, NULL, NULL);
	if ( !ok ) {
		return 0;
	}

	lua_pushnumber(L, result.x);
	lua_pushnumber(L, result.z);
	return 2;
}

static int
meta_movable(struct lua_State* L) {
	struct scene_context* scene_ctx = ( struct scene_context* )lua_touserdata(L, 1);
	struct nav_mesh_context* ctx = scene_ctx->ctx;
	double x = lua_tonumber(L, 2);
	double z = lua_tonumber(L, 3);
	double fix = luaL_optnumber(L, 4, 50);

	double offset;
	bool val = point_movable(ctx, x, z, fix, &offset);
	lua_pushboolean(L, val);
	lua_pushnumber(L, offset);
	return 2;
}

static int
meta_nav_height(struct lua_State* L) {
	struct scene_context* scene_ctx = ( struct scene_context* )lua_touserdata(L, 1);
	struct nav_mesh_context* ctx = scene_ctx->ctx;
	double x = lua_tonumber(L, 2);
	double z = lua_tonumber(L, 3);

	double height;
	if ( point_height(ctx, x, z, &height) ) {
		lua_pushnumber(L, height);
		return 1;
	}
	return 0;
}

static int
meta_random_point(struct lua_State* L) {
	struct scene_context* scene_ctx = ( struct scene_context* )lua_touserdata(L, 1);
	struct nav_mesh_context* ctx = scene_ctx->ctx;

	struct vector3 pt;

	point_random(ctx, &pt, -1);

	lua_pushnumber(L, pt.x);
	lua_pushnumber(L, pt.z);
	return 2;
}

static int 
meta_nav_size(struct lua_State* L) {
	struct scene_context* scene_ctx = ( struct scene_context* )lua_touserdata(L, 1);
	struct nav_mesh_context* ctx = scene_ctx->ctx;
	lua_pushnumber(L, ctx->lt.x);
	lua_pushnumber(L, ctx->lt.z);
	lua_pushnumber(L, ctx->width);
	lua_pushnumber(L, ctx->heigh);
	return 4;
}

int
init_meta(struct lua_State* L, int scene, struct nav_mesh_context* ctx) {
	struct scene_context* scene_ctx = ( struct scene_context* )lua_newuserdata(L, sizeof( struct scene_context ));
	scene_ctx->ctx = ctx;
	scene_ctx->scene = scene;

	lua_newtable(L);

	lua_pushcfunction(L, meta_release);
	lua_setfield(L, -2, "__gc");

	luaL_Reg l[] = {
		{ "nav_info", meta_nav_info },
		{ "tile_info", meta_tile_info },
		{ "create_tile", meta_create_tile },
		{ "load_tile", meta_load_tile },
		{ "find", meta_find },
		{ "raycast", meta_raycast },
		{ "set_mask", meta_set_mask },
		{ "get_mask", meta_get_mask },
		{ "movable", meta_movable },
		{ "nav_height", meta_nav_height },
		{ "random_point", meta_random_point },
		{ "nav_size", meta_nav_size },
		{ "around_movable", meta_around_movable },
		{ NULL, NULL },
	};
	luaL_newlib(L, l);

	lua_setfield(L, -2, "__index");

	lua_setmetatable(L, -2);
	return 1;
}

int
_create_nav(struct lua_State* L) {
	size_t size;
	const char* file = lua_tolstring(L, 1, &size);
	FILE* fp = fopen(file, "rb");
	if ( !fp ) {
		luaL_error(L, "error open file:%s", file);
	}
		
	uint32_t i, j;
	uint32_t vsize;
	fread(&vsize, sizeof( uint32_t ), 1, fp);

	double** vptr = (double**)malloc(sizeof( *vptr ) * vsize);
	for ( i = 0; i < vsize; i++ )
	{
		vptr[i] = (double*)malloc(sizeof(double)* 3);
		for ( j = 0; j < 3; j++ )
		{
			float val;
			fread(&val, sizeof( float ), 1, fp);
			vptr[i][j] = val;
		}
	}

	uint32_t psize;
	fread(&psize, sizeof( uint32_t ), 1, fp);

	int** pptr = (int**)malloc(sizeof( *pptr ) * psize);
	for ( i = 0; i < psize; i++ )
	{
		uint8_t index_count;
		fread(&index_count, sizeof( uint8_t ), 1, fp);

		pptr[i] = (int*)malloc(sizeof(int)*( index_count + 1 ));
		pptr[i][0] = index_count;
		for ( j = 1; j <= index_count; j++ )
		{
			uint16_t val;
			fread(&val, sizeof( uint16_t ), 1, fp);
			pptr[i][j] = val;
		}
	}

	struct nav_mesh_context* ctx = load_mesh(vptr, vsize, pptr, psize);

	for ( i = 0; i < vsize; i++ ) {
		free(vptr[i]);
	}
	free(vptr);
	for ( i = 0; i < psize; i++ ) {
		free(pptr[i]);
	}
	free(pptr);

	return init_meta(L, 0, ctx);
}

static int
_read_nav(lua_State* L) {
	size_t size;
	const char* file = lua_tolstring(L, 1, &size);
	FILE* fp = fopen(file, "rb");
	if ( !fp )
		luaL_error(L, "error open file:%s", file);


	lua_createtable(L, 0, 0);

	uint32_t vertex_count;
	fread(&vertex_count, sizeof( uint32_t ), 1, fp);
	lua_createtable(L, vertex_count, 0);

	uint32_t i;
	for ( i = 0; i < vertex_count; i++ )
	{
		lua_createtable(L, 3, 0);

		uint32_t j;
		for ( j = 1; j <= 3; j++ )
		{
			float val;
			fread(&val, sizeof( float ), 1, fp);

			lua_pushnumber(L, val);
			lua_rawseti(L, -2, j);
		}

		lua_rawseti(L, -2, i + 1);
	}
	lua_setfield(L, -2, "v");

	uint32_t poly_count;
	fread(&poly_count, sizeof( uint32_t ), 1, fp);
	lua_createtable(L, poly_count, 0);

	for ( i = 1; i <= poly_count; i++ )
	{
		uint8_t index_count;
		fread(&index_count, sizeof( uint8_t ), 1, fp);
		lua_createtable(L, index_count, 0);
		uint8_t j;
		for ( j = 1; j <= index_count; j++ )
		{
			uint16_t val;
			fread(&val, sizeof( uint16_t ), 1, fp);
			lua_pushinteger(L, val);
			lua_rawseti(L, -2, j);
		}
		lua_rawseti(L, -2, i);
	}

	lua_setfield(L, -2, "p");
	return 1;
}

static int
_write_nav(lua_State* L) {
	size_t size;
	const char* file = lua_tolstring(L, 1, &size);
	
	FILE* fp = fopen(file, "wb");
	if ( !fp )
		luaL_error(L, "error open file:%s", file);

	luaL_checktype(L, 2, LUA_TTABLE);
	lua_getfield(L, 2, "v");
	luaL_checktype(L, -1, LUA_TTABLE);

	uint32_t vertex_count = lua_rawlen(L, -1);
	fwrite(&vertex_count, sizeof( uint32_t ), 1, fp);

	uint32_t i;
	for ( i = 1; i <= vertex_count; i++ )
	{
		lua_rawgeti(L, -1, i);

		uint32_t j;
		for ( j = 1; j <= 3; j++ )
		{
			lua_rawgeti(L, -1, j);
			float val = lua_tonumber(L, -1);
			fwrite(&val, sizeof( float ), 1, fp);
			lua_pop(L, 1);
		}

		lua_pop(L, 1);
	}

	lua_pop(L, 1);

	lua_getfield(L, 2, "p");
	luaL_checktype(L, -1, LUA_TTABLE);

	uint32_t poly_count = lua_rawlen(L, -1);
	fwrite(&poly_count, sizeof( uint32_t ), 1, fp);
	for ( i = 1; i <= poly_count; i++ )
	{
		lua_rawgeti(L, -1, i);
		uint8_t index_count = lua_rawlen(L, -1);
		fwrite(&index_count, sizeof( uint8_t ), 1, fp);
		uint8_t j;
		for ( j = 1; j <= index_count; j++ )
		{
			lua_rawgeti(L, -1, j);
			uint16_t val = lua_tointeger(L, -1);
			fwrite(&val, sizeof( uint16_t ), 1, fp);
			lua_pop(L, 1);
		}
		lua_pop(L, 1);
	}
	fclose(fp);
	return 0;
}

static int
_write_tile(lua_State* L) {
	size_t size;
	const char* file = lua_tolstring(L, 1, &size);
	uint32_t tile_unit = 0;

	FILE* fp = fopen(file, "wb");
	if ( !fp )
		luaL_error(L, "error open file:%s", file);

	tile_unit = lua_tointeger(L, 2);

	luaL_checktype(L, 3, LUA_TTABLE);

	fwrite(&tile_unit, sizeof( uint32_t ), 1, fp);

	uint32_t tile_count = lua_rawlen(L, -1);
	fwrite(&tile_count, sizeof( uint32_t ), 1, fp);

	uint32_t i, j;
	for ( i = 1; i <= tile_count; i++ )
	{
		lua_rawgeti(L, -1, i);

		lua_getfield(L, -1, "node");
		uint32_t node_count = lua_rawlen(L, -1);
		fwrite(&node_count, sizeof( uint32_t ), 1, fp);
		for ( j = 1; j <= node_count; j++ )
		{
			lua_rawgeti(L, -1, j);
			uint32_t val = lua_tointeger(L, -1);
			fwrite(&val, sizeof( uint32_t ), 1, fp);
			lua_pop(L, 1);
		}
		lua_pop(L, 1);

		lua_getfield(L, -1, "center");
		for ( j = 1; j <= 2; j++ )
		{
			lua_rawgeti(L, -1, j);
			float val = lua_tonumber(L, -1);
			fwrite(&val, sizeof( float ), 1, fp);
			lua_pop(L, 1);
		}
		lua_pop(L, 1);

		lua_getfield(L, -1, "center_node");
		int center_node = lua_tointeger(L, -1);
		fwrite(&center_node, sizeof( int ), 1, fp);
		lua_pop(L, 1);

		lua_pop(L, 1);

	}
	fclose(fp);
	return 0;
}

int
luaopen_nav_core(lua_State *L) {
	luaL_Reg l[] = {
		{ "create", _create_nav },
		{ "read_nav", _read_nav },
		{ "write_nav", _write_nav },
		{ "write_tile", _write_tile },
		{ NULL, NULL }
	};

	lua_createtable(L, 0, ( sizeof( l ) ) / sizeof(luaL_Reg)-1);
	luaL_setfuncs(L, l, 0);
	return 1;
}
