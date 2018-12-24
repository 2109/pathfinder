#include "nav.h"

#ifdef _WIN32
#include <windows.h>
#define USE_THREAD
#endif

//#undef USE_THREAD

struct tile_info
{
	struct vector3 center;
	struct vector3 pos[4];
};

void
add_node(struct nav_tile* tile, int index) {
	if ( tile->size == 0 ) {
		tile->offset = 0;
		tile->size = 1;
		tile->node = (int*)malloc(sizeof(int)* tile->size);
	}
	if ( tile->offset >= tile->size ) {
		tile->size *= 2;
		tile->node = (int*)realloc(tile->node, sizeof(int)* tile->size);
	}
	tile->node[tile->offset] = index;
	tile->offset++;
}

bool
intersect(struct vector3* a, struct vector3* b, struct vector3* c, struct vector3* d) {
	if ( max(a->x, b->x) >= min(c->x, d->x) &&
		max(a->z, b->z) >= min(c->z, d->z) &&
		max(c->x, d->x) >= min(a->x, b->x) &&
		max(c->z, d->z) >= min(a->z, b->z) ) {
		struct vector3 ac, dc, bc, ca, ba, da;
		vector3_sub(a, c, &ac);
		vector3_sub(d, c, &dc);
		vector3_sub(b, c, &bc);
		vector3_sub(c, a, &ca);
		vector3_sub(b, a, &ba);
		vector3_sub(d, a, &da);

		if ( cross_product_direction(&ac, &dc) * cross_product_direction(&dc, &bc) >= 0 ) {
			if ( cross_product_direction(&ca, &ba) * cross_product_direction(&ba, &da) >= 0 )
				return true;
		}
	}

	return false;
}

bool
inside_reactangle(struct tile_info* tileinfo, double x, double z) {
	if ( tileinfo->pos[0].x <= x && x <= tileinfo->pos[2].x )
	{
		if ( tileinfo->pos[0].z <= z && z <= tileinfo->pos[2].z )
		{
			return true;
		}
	}
	return false;
}

void
make_tile(struct nav_mesh_context* ctx, struct tile_info* tileinfo, struct nav_tile* navtile, uint32_t from, uint32_t offset) {
	uint32_t i;
	for ( i = from; i < offset; i++ ) {
		struct tile_info* tile = &tileinfo[i];
		int j;
		for ( j = 0; j < ctx->node_size; j++ ) {
			bool done = false;
			struct nav_node* node = &ctx->node[j];
			int k;
			for ( k = 0; k < 4; k++ ) {
				int l;
				for ( l = 0; l < node->size; l++ ) {
					struct nav_border* border = get_border(ctx, node->border[l]);
					if ( intersect(&tile->pos[k], &tile->pos[( k + 1 ) % 4], &ctx->vertices[border->a], &ctx->vertices[border->b]) ) {
						done = true;
						break;
					}
				}
				if ( done )
					break;
			}

			if ( done )
				add_node(&navtile[i], j);
			else {
				if ( inside_node(ctx, j, tile->center.x, tile->center.y, tile->center.z) ) {
					add_node(&navtile[i], j);
				}
				else {
					if ( inside_reactangle(tile, node->center.x,node->center.z))
						add_node(&navtile[i], j);
				}
			}
		}

		navtile[i].center_node = -1;
		for ( j = 0; j < navtile[i].offset; j++ )
		{
			if ( inside_node(ctx, navtile[i].node[j], navtile[i].center.x, 0, navtile[i].center.z) )
			{
				navtile[i].center_node = navtile[i].node[j];
			}
		}
	}
}

#ifdef USE_THREAD
struct thread_args
{
	struct nav_mesh_context* ctx;
	struct tile_info* tileinfo;
	struct nav_tile* navtile;
	int index;
	int offset;
};

DWORD WINAPI make_thread(LPVOID ctx) {
	struct thread_args* arg = ( struct thread_args* )ctx;
	make_tile(arg->ctx, arg->tileinfo, arg->navtile, arg->index, arg->offset);
	return 0;
}

#endif

struct nav_tile*
create_tile(struct nav_mesh_context* ctx, uint32_t unit) {
	ctx->tile_unit = unit;
	ctx->tile_width = ctx->width / unit + 1;
	ctx->tile_heigh = ctx->heigh / unit + 1;

	uint32_t count = ctx->tile_width * ctx->tile_heigh;
	struct nav_tile* navtile = ( struct nav_tile* )malloc(sizeof( struct nav_tile )*count);
	memset(navtile, 0, sizeof( struct nav_tile )*count);

	struct tile_info* tileinfo = ( struct tile_info* )malloc(sizeof( struct tile_info )*count);
	memset(tileinfo, 0, sizeof( struct tile_info )*count);

	uint32_t z;
	for ( z = 0; z < ctx->tile_heigh; z++ ) {
		uint32_t x;
		for ( x = 0; x < ctx->tile_width; x++ ) {
			uint32_t index = x + z * ctx->tile_width;
			struct tile_info* tile = &tileinfo[index];
			tile->pos[0].x = ctx->lt.x + x * unit;
			tile->pos[0].z = ctx->lt.z + z * unit;
			tile->pos[1].x = ctx->lt.x + ( x + 1 ) * unit;
			tile->pos[1].z = ctx->lt.z + z * unit;
			tile->pos[2].x = ctx->lt.x + ( x + 1 ) * unit;
			tile->pos[2].z = ctx->lt.z + ( z + 1 ) * unit;
			tile->pos[3].x = ctx->lt.x + x * unit;
			tile->pos[3].z = ctx->lt.z + ( z + 1 ) * unit;
			tile->center.x = ctx->lt.x + ( x + 0.5 ) * unit;
			tile->center.z = ctx->lt.z + ( z + 0.5 ) * unit;
			navtile[index].center.x = (uint32_t)tile->center.x;
			navtile[index].center.z = (uint32_t)tile->center.z;
#ifdef _WIN32
			memcpy(&navtile[index].pos, &tile->pos, sizeof( tile->pos ));
#endif

		}
	}

#ifdef USE_THREAD
	SYSTEM_INFO si;
	GetSystemInfo(&si);
	struct thread_args* args = malloc(sizeof( *args ) * si.dwNumberOfProcessors);
	HANDLE* handlers = malloc(sizeof( *handlers ) * si.dwNumberOfProcessors);

	int task_count = count / si.dwNumberOfProcessors;
	for ( int i = 0; i < si.dwNumberOfProcessors; i++ ) {
		struct thread_args* arg = &args[i];
		arg->ctx = ctx;
		arg->tileinfo = tileinfo;
		arg->navtile = navtile;
		arg->index = task_count * i;
		arg->offset = arg->index + task_count;
		if ( arg->offset > count ) {
			arg->offset = count;
		}
		handlers[i] = CreateThread(NULL, 0, make_thread, arg, 0, NULL);
	}

	WaitForMultipleObjects(si.dwNumberOfProcessors, handlers, TRUE, INFINITE);
	for ( int i = 0; i < si.dwNumberOfProcessors; i++ )
		CloseHandle(handlers[i]);

	free(args);
	free(handlers);
#else
	make_tile(ctx, tileinfo, navtile, 0, count);
#endif
	free(tileinfo);
	return navtile;
}


void
release_tile(struct nav_mesh_context* ctx, struct nav_tile* navtile) {
	int count = ctx->tile_width * ctx->tile_heigh;
	int i;
	for ( i = 0; i < count; i++ ) {
		if ( navtile[i].node != NULL )
			free(navtile[i].node);
	}
	free(navtile);
}