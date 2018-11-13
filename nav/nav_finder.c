#include "nav.h"
#include <float.h>

inline double
cross_product_direction(struct vector3* vt1, struct vector3* vt2) {
	return vt1->z * vt2->x - vt1->x * vt2->z;
}

inline void
cross_product(struct vector3* vt1, struct vector3* vt2, struct vector3* result) {
	result->x = vt1->y * vt2->z - vt1->z * vt2->y;
	result->y = vt1->z * vt2->x - vt1->x * vt2->z;
	result->z = vt1->x * vt2->y - vt1->y * vt2->x;
}

inline double
cross_dot(struct vector3* vt1, struct vector3* vt2) {
	return vt1->x * vt2->x + vt1->y * vt2->y + vt1->z * vt2->z;
}


inline void
cross_point(struct vector3* a, struct vector3* b, struct vector3* c, struct vector3* d, struct vector3* result) {
	result->x = ( ( b->x - a->x ) * ( c->x - d->x ) * ( c->z - a->z ) - c->x * ( b->x - a->x ) * ( c->z - d->z ) + a->x * ( b->z - a->z ) * ( c->x - d->x ) ) / ( ( b->z - a->z )*( c->x - d->x ) - ( b->x - a->x ) * ( c->z - d->z ) );
	result->z = ( ( b->z - a->z ) * ( c->z - d->z ) * ( c->x - a->x ) - c->z * ( b->z - a->z ) * ( c->x - d->x ) + a->z * ( b->x - a->x ) * ( c->z - d->z ) ) / ( ( b->x - a->x )*( c->z - d->z ) - ( b->z - a->z ) * ( c->x - d->x ) );
}

inline void
vector3_copy(struct vector3* dst, struct vector3* src) {
	dst->x = src->x;
	dst->y = src->y;
	dst->z = src->z;
}

inline void
vector3_sub(struct vector3* a, struct vector3* b, struct vector3* result) {
	result->x = a->x - b->x;
	result->y = a->y - b->y;
	result->z = a->z - b->z;
}

void
set_mask(struct nav_mesh_mask* ctx, int mask, int enable) {
	if ( mask >= ctx->size ) {
		ctx->size *= 2;
		ctx->mask = realloc(ctx->mask, sizeof(int)* ctx->size);
	}
	ctx->mask[mask] = enable;
}

inline double
dot2dot(struct vector3* a, struct vector3* b) {
	double dx = a->x - b->x;
	double dz = a->z - b->z;
	return sqrt(dx * dx + dz * dz);
}

static inline double
dot2line(struct vector3* pt, struct vector3* start, struct vector3* over) {
	double a, b, c, s;

	a = dot2dot(pt, over);
	if ( a <= 0.00001 ) {
		return 0.0f;
	}

	b = dot2dot(pt, start);
	if ( b <= 0.00001 ) {
		return 0.0f;
	}

	c = dot2dot(start, over);
	if ( c <= 0.00001 ) {
		return 0.0f;
	}

	if ( a * a >= b * b + c * c ) {
		return b;
	}

	if ( b * b >= a * a + c * c ) {
		return a;
	}

	s = ( a + b + c ) / 2;
	s = sqrt(s * ( s - a ) * ( s - b ) * ( s - c ));

	return  2 * s / c;
}

static inline double
dot2poly(struct nav_mesh_context* mesh_ctx, int poly_id, struct vector3* pt) {
	struct nav_node* nav_node = &mesh_ctx->node[poly_id];
	int i;
	double min = DBL_MAX;
	for ( i = 0; i < nav_node->size; i++ ) {
		struct vector3* vt1 = &mesh_ctx->vertices[nav_node->poly[i]];
		struct vector3* vt2 = &mesh_ctx->vertices[nav_node->poly[( i + 1 ) % nav_node->size]];
		double dist = dot2line(pt, vt1, vt2);
		if ( dist < min )
			min = dist;
	}
	return min;
}

static inline bool
vt_inside_vt(struct vector3* vt, struct vector3* vtl, struct vector3* vtr) {
	double sign_a = cross_product_direction(vtl, vt);
	double sign_b = cross_product_direction(vtr, vt);

	if ( ( sign_a < 0 && sign_b > 0 ) || ( sign_a == 0 && sign_b > 0 ) || ( sign_a < 0 && sign_b == 0 ) ) {
		return true;
	}
	return false;
}

bool
inside_poly(struct nav_mesh_context* mesh_ctx, int* poly, int size, struct vector3* vt3) {
	int sign = 0;
	int i;
	for ( i = 0; i < size; i++ ) {
		struct vector3* vt1 = &mesh_ctx->vertices[poly[i]];
		struct vector3* vt2 = &mesh_ctx->vertices[poly[( i + 1 ) % size]];

		struct vector3 vt21 = { vt2->x - vt1->x, 0, vt2->z - vt1->z };
		struct vector3 vt31 = { vt3->x - vt1->x, 0, vt3->z - vt1->z };

		double dot = cross_product_direction(&vt21, &vt31);
		if ( dot == 0 )
			continue;

		if ( sign == 0 )
			sign = dot > 0 ? 1 : -1;
		else {
			if ( sign == 1 && dot < 0 )
				return false;
			else if ( sign == -1 && dot > 0 )
				return false;
		}
	}
	return true;
}

inline bool
inside_node(struct nav_mesh_context* mesh_ctx, int polyId, double x, double y, double z) {
	struct nav_node* nav_node = &mesh_ctx->node[polyId];
	struct vector3 vt = { x, y, z };
	return inside_poly(mesh_ctx, nav_node->poly, nav_node->size, &vt);
}

struct nav_node*
search_node(struct nav_mesh_context* ctx, double x, double y, double z) {
		if ( x < ctx->lt.x || x > ctx->br.x )
			return NULL;
		if ( z < ctx->lt.z || z > ctx->br.z )
			return NULL;

		if ( ctx->tile == NULL ) {
			int i;
			for ( i = 0; i < ctx->node_size; i++ ) {
				if ( inside_node(ctx, i, x, y, z) )
					return &ctx->node[i];
			}
			return NULL;
		}

		int x_index = ( x - ctx->lt.x ) / ctx->tile_unit;
		int z_index = ( z - ctx->lt.z ) / ctx->tile_unit;

		int index = x_index + z_index * ctx->tile_width;

		struct nav_tile* tile = &ctx->tile[index];

		int i;
		for ( i = 0; i < tile->offset; i++ ) {
			if ( inside_node(ctx, tile->node[i], x, y, z) ) {
				return &ctx->node[tile->node[i]];
			}
		}

		struct vector3 pt = { x, 0, z };

		double min_dt = DBL_MAX;
		int nearest = -1;
		for ( i = 0; i < tile->offset; i++ ) {
			double dt = dot2poly(ctx, tile->node[i], &pt);
			if ( min_dt > dt ) {
				min_dt = dt;
				nearest = tile->node[i];
			}
		}

		if ( nearest != -1 ) {
			return &ctx->node[nearest];
		}

		int center_node = 0;
		if ( around_movable(ctx, x, z, 1, &center_node, NULL, NULL) ) {
			return &ctx->node[center_node];
		}

		return NULL;
}

void
get_link(struct nav_mesh_context* mesh_ctx, struct nav_node* node, struct nav_node** linked_node) {
	int i;
	for ( i = 0; i < node->size; i++ ) {
		int border_index = node->border[i];
		struct nav_border* border = get_border(mesh_ctx, border_index);

		int linked = -1;
		if ( border->node[0] == node->id )
			linked = border->node[1];
		else
			linked = border->node[0];

		if ( linked == -1 )
			continue;

		struct nav_node* tmp = get_node(mesh_ctx, linked);
		if ( tmp->closed ) {
			continue;
		}

		if ( get_mask(mesh_ctx->mask_ctx, tmp->mask) ) {
			tmp->next = ( *linked_node );
			( *linked_node ) = tmp;
			tmp->reserve = border->opposite;
			vector3_copy(&tmp->pos, &border->center);
		}
	}
}

static inline double
G_COST(struct nav_node* from, struct nav_node* to) {
	double dx = from->pos.x - to->pos.x;
	double dy = 0;
	double dz = from->pos.z - to->pos.z;
	return sqrt(dx*dx + dy* dy + dz* dz) * GRATE;
}

static inline double
H_COST(struct nav_node* from, struct vector3* to) {
	double dx = from->center.x - to->x;
	double dy = 0;
	double dz = from->center.z - to->z;
	return sqrt(dx*dx + dy* dy + dz* dz) * HRATE;
}

bool
raycast(struct nav_mesh_context* ctx, struct vector3* pt0, struct vector3* pt1, struct vector3* result, search_dumper dumper, void* userdata) {
	struct nav_node* node = search_node(ctx, pt0->x, pt0->y, pt0->z);

	int index = 0;
	struct vector3 vt10;
	vector3_sub(pt1, pt0, &vt10);

	while ( node ) {
		if ( inside_node(ctx, node->id, pt1->x, pt1->y, pt1->z) ) {
			vector3_copy(result, pt1);
			return true;
		}

		bool crossed = false;
		int i;
		for ( i = 0; i < node->size; i++ ) {
			struct nav_border* border = get_border(ctx, node->border[i]);

			struct vector3* pt3 = &ctx->vertices[border->a];
			struct vector3* pt4 = &ctx->vertices[border->b];

			struct vector3 vt30, vt40;
			vector3_sub(pt3, pt0, &vt30);
			vector3_sub(pt4, pt0, &vt40);

			if ( vt_inside_vt(&vt10, &vt30, &vt40)) {
				int next = -1;
				if ( border->node[0] != -1 ) {
					if ( border->node[0] == node->id )
						next = border->node[1];
					else
						next = border->node[0];
				}
				else
					assert(border->node[1] == node->id);

				if ( next == -1 ) {
					cross_point(pt3, pt4, pt1, pt0, result);
					return true;
				}
				else {
					struct nav_node* next_node = get_node(ctx, next);
					if ( get_mask(ctx->mask_ctx, next_node->mask) == 0 ) {
						cross_point(pt3, pt4, pt1, pt0, result);
						return true;
					}
					if ( dumper )
						dumper(userdata, next);

					crossed = true;
					node = next_node;
					break;
				}
			}
		}

		if ( !crossed ) {
			assert(index == 0);
			pt0->x = node->center.x;
			pt0->z = node->center.z;
			vector3_sub(pt1, pt0, &vt10);
		}

		++index;
	}
	return false;
}


static inline void
clear_node(struct nav_node* n) {
	n->link_parent = NULL;
	n->link_border = -1;
	n->F = n->G = n->H = 0;
#ifdef MINHEAP_USE_LIBEVENT
	n->elt.index = -1;
#else
	n->elt.index = 0;
#endif
	n->next = NULL;
	n->closed = 0;
}

static inline void
heap_clear(struct element* elt) {
	struct nav_node *node = ( struct nav_node *)elt;
	clear_node(node);
}

static inline void
reset(struct nav_mesh_context* ctx) {
	struct nav_node * node = ctx->closelist;
	while ( node ) {
		struct nav_node * tmp = node;
		node = tmp->next;
		clear_node(tmp);
	}
	ctx->closelist = NULL;
#ifdef MINHEAP_USE_LIBEVENT
	min_heap_clear_(&ctx->openlist, heap_clear);
#else
	minheap_clear(&ctx->openlist, heap_clear);
#endif
}

struct nav_node*
next_border(struct nav_mesh_context* ctx, struct nav_node* node, struct vector3* wp, int *link_border) {
	struct vector3 vt0, vt1;
	*link_border = node->link_border;
	while ( *link_border != -1 ) {
		struct nav_border* border = get_border(ctx, *link_border);
		vector3_sub(&ctx->vertices[border->a], wp, &vt0);
		vector3_sub(&ctx->vertices[border->b], wp, &vt1);
		if ( ( vt0.x == 0 && vt0.z == 0 ) || ( vt1.x == 0 && vt1.z == 0 ) ) {
			node = node->link_parent;
			*link_border = node->link_border;
		}
		else
			break;
	}
	if ( *link_border != -1 )
		return node;

	return NULL;
}

static inline void
path_init(struct nav_mesh_context* mesh_ctx) {
	mesh_ctx->result.offset = 0;
}

void
path_add(struct nav_mesh_context* mesh_ctx, struct vector3* wp) {
	if ( mesh_ctx->result.offset >= mesh_ctx->result.size ) {
		mesh_ctx->result.size *= 2;
		mesh_ctx->result.wp = ( struct vector3* )realloc(mesh_ctx->result.wp, sizeof( struct vector3 )*mesh_ctx->result.size);
	}

	mesh_ctx->result.wp[mesh_ctx->result.offset].x = wp->x;
	mesh_ctx->result.wp[mesh_ctx->result.offset].z = wp->z;
	mesh_ctx->result.offset++;
}

void
make_waypoint(struct nav_mesh_context* mesh_ctx, struct vector3* pt0, struct vector3* pt1, struct nav_node * node) {
	path_add(mesh_ctx, pt1);

	struct vector3* pt_wp = pt1;

	int link_border = node->link_border;

	struct nav_border* border = get_border(mesh_ctx, link_border);

	struct vector3 pt_left, pt_right;
	vector3_copy(&pt_left, &mesh_ctx->vertices[border->a]);
	vector3_copy(&pt_right, &mesh_ctx->vertices[border->b]);

	struct vector3 vt_left, vt_right;
	vector3_sub(&pt_left, pt_wp, &vt_left);
	vector3_sub(&pt_right, pt_wp, &vt_right);

	struct nav_node* left_node = node->link_parent;
	struct nav_node* right_node = node->link_parent;

	struct nav_node* tmp = node->link_parent;
	while ( tmp )
	{
		int link_border = tmp->link_border;
		if ( link_border == -1 )
		{
			struct vector3 tmp_target;
			tmp_target.x = pt0->x - pt_wp->x;
			tmp_target.z = pt0->z - pt_wp->z;

			double forward_a = cross_product_direction(&vt_left, &tmp_target);
			double forward_b = cross_product_direction(&vt_right, &tmp_target);

			if ( forward_a < 0 && forward_b > 0 )
			{
				path_add(mesh_ctx, pt0);
				break;
			}
			else
			{
				if ( forward_a > 0 && forward_b > 0 )
				{
					pt_wp->x = pt_left.x;
					pt_wp->z = pt_left.z;

					path_add(mesh_ctx, pt_wp);

					left_node = next_border(mesh_ctx, left_node, pt_wp, &link_border);
					if ( left_node == NULL )
					{
						path_add(mesh_ctx, pt0);
						break;
					}

					border = get_border(mesh_ctx, link_border);
					pt_left.x = mesh_ctx->vertices[border->a].x;
					pt_left.z = mesh_ctx->vertices[border->a].z;

					pt_right.x = mesh_ctx->vertices[border->b].x;
					pt_right.z = mesh_ctx->vertices[border->b].z;

					vt_left.x = pt_left.x - pt_wp->x;
					vt_left.z = pt_left.z - pt_wp->z;

					vt_right.x = pt_right.x - pt_wp->x;
					vt_right.z = pt_right.z - pt_wp->z;

					tmp = left_node->link_parent;
					left_node = tmp;
					right_node = tmp;
					continue;
				}
				else if ( forward_a < 0 && forward_b < 0 )
				{
					pt_wp->x = pt_right.x;
					pt_wp->z = pt_right.z;

					path_add(mesh_ctx, pt_wp);

					right_node = next_border(mesh_ctx, right_node, pt_wp, &link_border);
					if ( right_node == NULL )
					{
						path_add(mesh_ctx, pt0);
						break;
					}

					border = get_border(mesh_ctx, link_border);
					pt_left.x = mesh_ctx->vertices[border->a].x;
					pt_left.z = mesh_ctx->vertices[border->a].z;

					pt_right.x = mesh_ctx->vertices[border->b].x;
					pt_right.z = mesh_ctx->vertices[border->b].z;

					vt_left.x = pt_left.x - pt_wp->x;
					vt_left.z = pt_left.z - pt_wp->z;

					vt_right.x = pt_right.x - pt_wp->x;
					vt_right.z = pt_right.z - pt_wp->z;

					tmp = right_node->link_parent;
					left_node = tmp;
					right_node = tmp;
					continue;
				}
				break;
			}

		}

		border = get_border(mesh_ctx, link_border);

		struct vector3 tmp_pt_left, tmp_pt_right;
		vector3_copy(&tmp_pt_left, &mesh_ctx->vertices[border->a]);
		vector3_copy(&tmp_pt_right, &mesh_ctx->vertices[border->b]);

		struct vector3 tmp_vt_left, tmp_vt_right;
		vector3_sub(&tmp_pt_left, pt_wp, &tmp_vt_left);
		vector3_sub(&tmp_pt_right, pt_wp, &tmp_vt_right);

		double forward_left_a = cross_product_direction(&vt_left, &tmp_vt_left);
		double forward_left_b = cross_product_direction(&vt_right, &tmp_vt_left);
		double forward_right_a = cross_product_direction(&vt_left, &tmp_vt_right);
		double forward_right_b = cross_product_direction(&vt_right, &tmp_vt_right);

		if ( forward_left_a < 0 && forward_left_b > 0 )
		{
			left_node = tmp->link_parent;
			vector3_copy(&pt_left, &tmp_pt_left);
			vector3_sub(&pt_left, pt_wp, &vt_left);
		}

		if ( forward_right_a < 0 && forward_right_b > 0 )
		{
			right_node = tmp->link_parent;
			vector3_copy(&pt_right, &tmp_pt_right);
			vector3_sub(&pt_right, pt_wp, &vt_right);
		}

		if ( forward_left_a > 0 && forward_left_b > 0 && forward_right_a > 0 && forward_right_b > 0 )
		{
			vector3_copy(pt_wp, &pt_left);

			left_node = next_border(mesh_ctx, left_node, pt_wp, &link_border);
			if ( left_node == NULL )
			{
				path_add(mesh_ctx, pt0);
				break;
			}

			border = get_border(mesh_ctx, link_border);
			vector3_copy(&pt_left, &mesh_ctx->vertices[border->a]);
			vector3_copy(&pt_right, &mesh_ctx->vertices[border->b]);

			vector3_sub(&mesh_ctx->vertices[border->a], pt_wp, &vt_left);
			vector3_sub(&mesh_ctx->vertices[border->b], pt_wp, &vt_right);

			path_add(mesh_ctx, pt_wp);

			tmp = left_node->link_parent;
			left_node = tmp;
			right_node = tmp;

			continue;
		}

		if ( forward_left_a < 0 && forward_left_b < 0 && forward_right_a < 0 && forward_right_b < 0 )
		{
			vector3_copy(pt_wp, &pt_right);

			right_node = next_border(mesh_ctx, right_node, pt_wp, &link_border);
			if ( right_node == NULL )
			{
				path_add(mesh_ctx, pt0);
				break;
			}

			border = get_border(mesh_ctx, link_border);
			vector3_copy(&pt_left, &mesh_ctx->vertices[border->a]);
			vector3_copy(&pt_right, &mesh_ctx->vertices[border->b]);

			vector3_sub(&mesh_ctx->vertices[border->a], pt_wp, &vt_left);
			vector3_sub(&mesh_ctx->vertices[border->b], pt_wp, &vt_right);

			path_add(mesh_ctx, pt_wp);

			tmp = right_node->link_parent;
			left_node = tmp;
			right_node = tmp;
			continue;
		}

		tmp = tmp->link_parent;
	}
}

struct nav_path*
astar_find(struct nav_mesh_context* mesh_ctx, struct vector3* pt_start, struct vector3* pt_over, search_dumper dumper, void* userdata) {
	path_init(mesh_ctx);

	struct nav_node* node_start = search_node(mesh_ctx, pt_start->x, pt_start->y, pt_start->z);
	struct nav_node* node_over = search_node(mesh_ctx, pt_over->x, pt_over->y, pt_over->z);

	if ( !node_start || !node_over )
		return NULL;

	if ( node_start == node_over ) {
		path_add(mesh_ctx, pt_over);
		path_add(mesh_ctx, pt_start);
		return &mesh_ctx->result;
	}

	vector3_copy(&node_start->pos, pt_start);

#ifdef MINHEAP_USE_LIBEVENT
	min_heap_push_(&mesh_ctx->openlist, &node_start->elt);
#else
	minheap_push(&mesh_ctx->openlist, &node_start->elt);
#endif
	struct nav_node* node = NULL;

#ifdef MINHEAP_USE_LIBEVENT
	while ( ( node = ( struct nav_node* )min_heap_pop_(&mesh_ctx->openlist) ) != NULL ) {
#else
	while ( ( node = ( struct nav_node* )minheap_pop(&mesh_ctx->openlist) ) != NULL ) {
#endif
		node->closed = 1;
		node->next = mesh_ctx->closelist;
		mesh_ctx->closelist = node;
		
		if ( node == node_over ) {
			make_waypoint(mesh_ctx, pt_start, pt_over, node);
			reset(mesh_ctx);
			return &mesh_ctx->result;
		}

		struct nav_node* linked_node = NULL;
		get_link(mesh_ctx, node, &linked_node);

		while ( linked_node ) {
#ifdef MINHEAP_USE_LIBEVENT
			if ( linked_node->elt.index >= 0 ) {
#else
			if ( linked_node->elt.index ) {
#endif
				double nG = node->G + G_COST(node, linked_node);
				if ( nG < linked_node->G ) {
					linked_node->G = nG;
					linked_node->F = linked_node->G + linked_node->H;
					linked_node->link_parent = node;
					linked_node->link_border = linked_node->reserve;
#ifdef MINHEAP_USE_LIBEVENT
					min_heap_adjust_(&mesh_ctx->openlist, &linked_node->elt);
#else
					minheap_change(&mesh_ctx->openlist, &linked_node->elt);
#endif
				}
			}
			else {
				linked_node->G = node->G + G_COST(node, linked_node);
				linked_node->H = H_COST(linked_node, pt_over);
				linked_node->F = linked_node->G + linked_node->H;
				linked_node->link_parent = node;
				linked_node->link_border = linked_node->reserve;
#ifdef MINHEAP_USE_LIBEVENT
				min_heap_push_(&mesh_ctx->openlist, &linked_node->elt);
#else
				minheap_push(&mesh_ctx->openlist, &linked_node->elt);
#endif
				if ( dumper != NULL )
					dumper(userdata, linked_node->id);
			}
			struct nav_node* tmp = linked_node;
			linked_node = linked_node->next;
			tmp->next = NULL;
		}
	}
	reset(mesh_ctx);
	return NULL;
}

struct vector3*
around_movable(struct nav_mesh_context* ctx, double x, double z, int range, int* center_node, search_dumper dumper, void* userdata) {
	if ( ctx->tile == NULL )
		return NULL;

	struct vector3 pt = { x, 0, z };

		struct vector3* result = NULL;
		int result_node = -1;
		double min_dt = DBL_MAX;

		int x_index = ( x - ctx->lt.x ) / ctx->tile_unit;
		int z_index = ( z - ctx->lt.z ) / ctx->tile_unit;

		int r;
		for ( r = 1; r <= range; ++r ) {
			int x_min = x_index - r;
			int x_max = x_index + r;
			int z_min = z_index - r;
			int z_max = z_index + r;

			int x, z;

			int z_range[2] = { z_min, z_max };

			int j;
			for ( j = 0; j < 2; j++ ) {
				z = z_range[j];

				if ( z < 0 || z >= ctx->tile_heigh )
					continue;

				for ( x = x_min; x <= x_max; x++ ) {

					if ( x < 0 || x >= ctx->tile_width )
						continue;

					int index = x + z * ctx->tile_width;
					struct nav_tile* tile = &ctx->tile[index];
					if ( dumper )
						dumper(userdata, index);

					if ( tile->center_node != -1 ) {
						double dt = dot2dot(&pt, &tile->center);
						if ( dt < min_dt ) {
							result = &tile->center;
							result_node = tile->center_node;
							min_dt = dt;
						}
					}
				}
			}

			int x_range[2] = { x_min, x_max };

			for ( j = 0; j < 2; j++ ) {
				x = x_range[j];
				if ( x < 0 || x >= ctx->tile_width )
					continue;

				for ( z = z_min; z < z_max; z++ ) {
					if ( z < 0 || z >= ctx->tile_heigh )
						continue;

					int index = x + z * ctx->tile_width;
					struct nav_tile* tile = &ctx->tile[index];
					if ( dumper )
						dumper(userdata, index);
					if ( tile->center_node != -1 ) {
						double dt = dot2dot(&pt, &tile->center);
						if ( dt < min_dt ) {
							result = &tile->center;
							result_node = tile->center_node;
							min_dt = dt;
						}
					}

				}
			}
		}

		if ( center_node ) {
			*center_node = result_node;
		}
	return result;
}

#define INIT_RECORD_SIZE 8
struct dt_poly_record {
	int init[INIT_RECORD_SIZE];
	int offset;
	int size;
	int* record;
};

static inline void
dt_record_init(struct dt_poly_record* dt_record) {
	dt_record->record = dt_record->init;
	dt_record->offset = 0;
	dt_record->size = INIT_RECORD_SIZE;
}

static inline void
dt_record_add(struct dt_poly_record* dt_record, int poly) {
	if ( dt_record->offset >= dt_record->size ) {
		int nsize = dt_record->size * 2;
		int* nrecord = (int*)malloc(sizeof(int)* nsize);
		memcpy(nrecord, dt_record->record, sizeof(int)* dt_record->size);
		if (dt_record->record != dt_record->init) {
			free(dt_record->record);
		}
		dt_record->record = nrecord;
		dt_record->size = nsize;
	}

	dt_record->record[dt_record->offset++] = poly;
}

static inline void
dt_record_release(struct dt_poly_record* dt_record) {
	if ( dt_record->record != dt_record->init ) {
		free(dt_record->record);
	}
}

#define MOVABLE_USE_RECORD

bool
point_movable(struct nav_mesh_context* ctx, double x, double z, double fix, double* dt_offset) {
	if ( x < ctx->lt.x || x > ctx->br.x )
		return false;

	if ( z < ctx->lt.z || z > ctx->br.z )
		return false;

	if (ctx->tile == NULL) {
		return false;
	}

	if ( dt_offset ) {
		*dt_offset = 0;
	}

	struct nav_node* node = NULL;

	int x_index = ( x - ctx->lt.x ) / ctx->tile_unit;
	int z_index = ( z - ctx->lt.z ) / ctx->tile_unit;
	int index = x_index + z_index * ctx->tile_width;

	struct nav_tile* tile = &ctx->tile[index];

	int i;
	for ( i = 0; i < tile->offset; i++ ) {
		if ( inside_node(ctx, tile->node[i], x, 0, z) ) {
			node = &ctx->node[tile->node[i]];
			break;
		}
	}
	if ( node ) {
		if ( get_mask(ctx->mask_ctx, node->mask) == 1 ) {
			return true;
		}
		return false;
	}

	if ( fix == 0 ) {
		return false;
	}

#ifdef MOVABLE_USE_RECORD
	struct dt_poly_record dt_record;
	dt_record_init(&dt_record);
#endif

	struct vector3 pt = { x, 0, z };
	double dt_min = -1;

	int x_axis;
	for ( x_axis = x_index - 1; x_axis <= x_index + 1; x_axis++ )
	{
		if ( x_axis < 0 || x_axis >= ctx->tile_width ) {
			continue;
		}
		int z_axis;
		for ( z_axis = z_index - 1; z_axis <= z_index + 1; z_axis++ )
		{
			if ( z_axis < 0 || z_axis >= ctx->tile_heigh ) {
				continue;
			}
			index = x_axis + z_axis * ctx->tile_width;
			tile = &ctx->tile[index];
			int i;
			for ( i = 0; i < tile->offset; i++ ) {
				int poly_id = tile->node[i];
#ifdef MOVABLE_USE_RECORD
				node = get_node(ctx, poly_id);
				if ( node->dt_recorded == 0 ) {
					node->dt_recorded = 1;
					dt_record_add(&dt_record, poly_id);
					double dt = dot2poly(ctx, poly_id, &pt);
					if ( dt_min < 0 || dt_min > dt ) {
						dt_min = dt;
					}
				}
#else
				double dt = dot2poly(ctx, poly_id, &pt);
				if ( dt_min < 0 || dt_min > dt ) {
					dt_min = dt;
				}
#endif

			}
		}
	}

#ifdef MOVABLE_USE_RECORD
	for ( i = 0; i < dt_record.offset; i++ )
	{
		int poly_id = dt_record.record[i];
		node = get_node(ctx, poly_id);
		assert(node->dt_recorded == 1);
		node->dt_recorded = 0;
	}
	dt_record_release(&dt_record);
#endif

	if (dt_offset) {
		*dt_offset = dt_min;
	}
	if ( dt_min > 0 && dt_min <= fix ) {
		return true;
	}
	
	return false;
}


void
point_random(struct nav_mesh_context* ctx, struct vector3* result, int poly) {
	int rand_poly = -1;

	if ( poly >= 0 && poly < ctx->node_size ) {
		rand_poly = poly;
	} else {
		double ratio = ( rand() % 10000 ) / 10000.0f;
		double weight = ctx->area * ratio;
		double tmp = 0.0f;
		int i;
		for ( i = 0; i < ctx->node_size;i++ ){
			tmp += ctx->node[i].area;
			if (weight <= tmp) {
				rand_poly = i;
				break;
			}
		}
	}
	
	struct nav_node* node = &ctx->node[rand_poly];
	int rand_triangle = rand() % ( node->size - 2 ) + 1;

	struct vector3* PA = &ctx->vertices[node->poly[0]];
	struct vector3* PB = &ctx->vertices[node->poly[rand_triangle]];
	struct vector3* PC = &ctx->vertices[node->poly[rand_triangle + 1]];

	struct vector3 AB;
	struct vector3 AC;

	vector3_sub(PB, PA, &AB);
	vector3_sub(PC, PA, &AC);

	float x = (rand() % 10000) / 10000.0f;
	float z = (rand() % 10000) / 10000.0f;

	if (x + z > 1) {
		x = 1 - x;
		z = 1 - z;
	}

	result->x = PA->x + AB.x * x + AC.x * z;
	result->z = PA->z + AB.z * x + AC.z * z;
}

#define HEIGHT_USE_LERP
#ifdef HEIGHT_USE_LERP

bool
point_height(struct nav_mesh_context* ctx, double x, double z, double* height) {
	struct nav_node* node = search_node(ctx, x, 0, z);
	if ( !node ) {
		return false;
	}

	struct vector3 start = { node->center.x, 0, node->center.z };
	struct vector3 over = { x, 0, z };

	struct vector3 vt10;
	vector3_sub(&over, &start, &vt10);

	struct vector3 result;
	struct nav_border* cross_border = NULL;

	int i;
	for ( i = 0; i < node->size; i++ ) {
		struct nav_border* border = get_border(ctx, node->border[i]);

		struct vector3* pt3 = &ctx->vertices[border->a];
		struct vector3* pt4 = &ctx->vertices[border->b];

		struct vector3 vt30, vt40;
		vector3_sub(pt3, &start, &vt30);
		vector3_sub(pt4, &start, &vt40);

		double sign_a = cross_product_direction(&vt30, &vt10);
		double sign_b = cross_product_direction(&vt40, &vt10);

		if ( ( sign_a < 0 && sign_b > 0 ) || ( sign_a == 0 && sign_b > 0 ) || ( sign_a < 0 && sign_b == 0 ) ) {
			cross_point(pt3, pt4, &over, &start, &result);
			cross_border = border;
			break;
		}
	}

	if ( !cross_border ) {
		return false;
	}

	struct vector3* pt_border0 = &ctx->vertices[cross_border->a];
	struct vector3* pt_border1 = &ctx->vertices[cross_border->b];

	double dt_border = dot2dot(pt_border0, pt_border1);
	double dt_cross = dot2dot(pt_border0, &result);

	double border_height;
	if ( dt_border < 0.0001 ) {
		border_height = pt_border1->y;
	}
	else {
		border_height = pt_border0->y + ( pt_border1->y - pt_border0->y ) * ( dt_cross / dt_border );
	}

	double center_height = node->center.y;

	double dt_center = dot2dot(&node->center, &result);
	double dt_point = dot2dot(&over, &result);

	double pt_height;
	if ( dt_center < 0.0001 ) {
		pt_height = node->center.y;
	}
	else {
		pt_height = border_height + ( center_height - border_height ) * ( dt_point / dt_center );
	}

	*height = pt_height;

	return true;
}

#else
/*
射线:p(t) = p0 + tu
u:射线方向向量
平量:n.(p - p0) = 0
n:平面的法向量
t = n.(p1-p0)/n.u
*/
bool
point_height(struct nav_mesh_context* ctx, double x, double z, double* height) {
	struct nav_node* node = search_node(ctx, x, 0, z);
	if ( !node ) {
		return false;
	}

	//射线起点
	struct vector3 from = { x, 0, z };
	//射线方向
	static const struct vector3 direction = { 0, 1, 0 };

	int triangle[3] = { 0, 0, 0 };
	int i;
	for ( i = 1; i < node->size - 1; i++ ) {
		triangle[0] = node->poly[0];
		triangle[1] = node->poly[i];
		triangle[2] = node->poly[i + 1];
		if ( inside_poly(ctx, triangle, 3, &from) ) {
			break;
		}
	}
	if ( i == node->size - 1 ) {
		return false;
	}

	//求平面法向量
	struct vector3* PA = &ctx->vertices[triangle[0]];
	struct vector3* PB = &ctx->vertices[triangle[1]];
	struct vector3* PC = &ctx->vertices[triangle[2]];

	struct vector3 AB;
	vector3_sub(PB, PA, &AB);

	struct vector3 AC;
	vector3_sub(PC, PA, &AC);

	struct vector3 normal;
	cross_product(&AB, &AC, &normal);

	struct vector3 vt;
	vector3_sub(PA, &from, &vt);

	float ratio = ( cross_dot(&normal, &vt) / ( cross_dot(&normal, &direction) ) );
	if ( ratio < 0 ) {
		return false;
	}
	*height = ratio;
	return true;
}

#endif