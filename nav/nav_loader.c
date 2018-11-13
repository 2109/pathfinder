#include "nav.h"

struct nav_border_searcher
{
	struct nav_border_searcher* next;
	int index;
	int id;
};

struct vertex_sort_info
{
	struct nav_mesh_context* ctx;
	int index;
	struct vector3 center;
};

static double
poly_area(struct nav_mesh_context* ctx, int size, int* poly) {
	assert(size >= 3);
	double area = 0.0f;
	int i;
	for ( i = 0; i < size;i++ ) {
		int current = i;
		int next = ( i + 1 ) % size;

		struct vector3* pt0 = &ctx->vertices[poly[current]];
		struct vector3* pt1 = &ctx->vertices[poly[next]];
		area += pt0->x * pt1->z - pt0->z * pt1->x;
	}
	return fabs(area / 2.0);
}

static int
node_cmp_less(struct element * left, struct element * right) {
	struct nav_node *l = ( struct nav_node *) left;
	struct nav_node *r = ( struct nav_node *) right;
	return l->F < r->F;
}

static int
node_cmp_great(struct element * left, struct element * right) {
	struct nav_node *l = ( struct nav_node * ) left;
	struct nav_node *r = ( struct nav_node * ) right;
	return l->F > r->F;
}

int
angle_cmp(const void * left, const void * right) {
	struct vertex_sort_info *l = ( struct vertex_sort_info* )left;
	struct vertex_sort_info *r = ( struct vertex_sort_info* )right;

	struct vector3 pt0, pt1;
	vector3_copy(&pt0, &l->ctx->vertices[l->index]);
	vector3_copy(&pt1, &l->ctx->vertices[r->index]);

	struct vector3 vt0, vt1;
	vector3_sub(&l->center, &pt0, &vt0);
	vector3_sub(&r->center, &pt1, &vt1);

	static double pi = 3.14159265358979323846;
	double angle0 = atan2(vt0.z, vt0.x) * 180 / pi;
	if ( angle0 < 0 ) {
		angle0 += 360;
	}
	double angle1 = atan2(vt1.z, vt1.x) * 180 / pi;
	if ( angle1 < 0 ) {
		angle1 += 360;
	}
	return angle0 > angle1;
}

struct nav_border*
add_border(struct nav_mesh_context* mesh_ctx, int a, int b) {
	if ( mesh_ctx->border_offset + 1 >= mesh_ctx->border_size ) {
		mesh_ctx->border_size *= 2;
		mesh_ctx->borders = ( struct nav_border* )realloc(mesh_ctx->borders, sizeof( struct nav_border ) * mesh_ctx->border_size);
	}

	struct nav_border * border = &mesh_ctx->borders[mesh_ctx->border_offset];
	border->id = mesh_ctx->border_offset;
	border->a = a;
	border->b = b;
	border->node[0] = -1;
	border->node[1] = -1;
	border->opposite = -1;
	border->center.x = ( mesh_ctx->vertices[a].x + mesh_ctx->vertices[b].x );
	border->center.y = ( mesh_ctx->vertices[a].y + mesh_ctx->vertices[b].y );
	border->center.z = ( mesh_ctx->vertices[a].z + mesh_ctx->vertices[b].z );

	mesh_ctx->border_offset++;

	return border;
}

struct nav_border*
search_border(struct nav_mesh_context* ctx, struct nav_border_searcher** searcher, int begin, int end) {
	if ( begin >= ctx->vertices_size || end >= ctx->vertices_size )
		assert(0);

	struct nav_border_searcher* node = searcher[begin];
	while ( node != NULL ) {
		if ( node->index == end )
			return get_border(ctx, node->id);
		node = node->next;
	}

	node = ( struct nav_border_searcher* )malloc(sizeof( *node ));
	struct nav_border* border = add_border(ctx, begin, end);
	node->id = border->id;
	node->index = end;
	node->next = searcher[begin];
	searcher[begin] = node;
	return border;
}

void
release_border_searcher(struct nav_mesh_context* ctx, struct nav_border_searcher** searcher) {
	int i;
	for ( i = 0; i < ctx->vertices_size; i++ ) {
		struct nav_border_searcher* node = searcher[i];
		while ( node != NULL ) {
			struct nav_border_searcher* tmp = node;
			node = node->next;
			free(tmp);
		}
	}
	free(searcher);
}

void
border_link_node(struct nav_border* border, int id) {
	if ( border->node[0] == -1 )
		border->node[0] = id;
	else if ( border->node[1] == -1 )
		border->node[1] = id;
	else
		assert(0);
}

void
vertex_sort(struct nav_mesh_context* ctx, struct nav_node* node) {
	struct vertex_sort_info* vertex = ( struct vertex_sort_info* )malloc(sizeof( *vertex ) * node->size);
	int i;
	for ( i = 0; i < node->size; i++ ) {
		vertex[i].ctx = ctx;
		vertex[i].index = node->poly[i];
		vector3_copy(&vertex[i].center, &node->center);
	}

	qsort(vertex, node->size, sizeof( struct vertex_sort_info ), angle_cmp);

	for ( i = 0; i < node->size; i++ )
		node->poly[i] = vertex[i].index;

	free(vertex);
}

void
init_mesh(struct nav_mesh_context* mesh_ctx) {
	memset(mesh_ctx, 0, sizeof( *mesh_ctx ));

	mesh_ctx->mask_ctx.size = 8;
	mesh_ctx->mask_ctx.mask = (int*)malloc(sizeof(int)* mesh_ctx->mask_ctx.size);
	int i;
	for ( i = 0; i < mesh_ctx->mask_ctx.size; i++ )
		set_mask(&mesh_ctx->mask_ctx, i, 1);
	set_mask(&mesh_ctx->mask_ctx, 0, 1);

	mesh_ctx->result.size = 8;
	mesh_ctx->result.offset = 0;
	mesh_ctx->result.wp = ( struct vector3* )malloc(sizeof( struct vector3 )*mesh_ctx->result.size);

#ifdef MINHEAP_USE_LIBEVENT
	mh_ctor(&mesh_ctx->openlist, node_cmp_great);
#else
	mh_ctor(&mesh_ctx->openlist, node_cmp_less);
#endif

	mesh_ctx->closelist = NULL;
}

struct nav_mesh_context*
load_mesh(double** v, int vertices_size, int** p, int node_size) {
	struct nav_mesh_context* mesh_ctx = ( struct nav_mesh_context* )malloc(sizeof( *mesh_ctx ));
	init_mesh(mesh_ctx);

	int i, j, k;

	mesh_ctx->vertices_size = vertices_size;
	mesh_ctx->vertices = ( struct vector3 * )malloc(sizeof( struct vector3 ) * mesh_ctx->vertices_size);
	memset(mesh_ctx->vertices, 0, sizeof( struct vector3 ) * mesh_ctx->vertices_size);

	memset(&mesh_ctx->lt, 0, sizeof( struct vector3 ));
	memset(&mesh_ctx->br, 0, sizeof( struct vector3 ));

	//初始化顶点信息，并找到左上和右下的坐标
	for ( i = 0; i < vertices_size; i++ ) {
		mesh_ctx->vertices[i].x = v[i][0];
		mesh_ctx->vertices[i].y = v[i][1];
		mesh_ctx->vertices[i].z = v[i][2];

		if ( mesh_ctx->lt.x == 0 )
			mesh_ctx->lt.x = mesh_ctx->vertices[i].x;
		else {
			if ( mesh_ctx->vertices[i].x < mesh_ctx->lt.x )
				mesh_ctx->lt.x = mesh_ctx->vertices[i].x;
		}

		if ( mesh_ctx->lt.z == 0 )
			mesh_ctx->lt.z = mesh_ctx->vertices[i].z;
		else {
			if ( mesh_ctx->vertices[i].z < mesh_ctx->lt.z )
				mesh_ctx->lt.z = mesh_ctx->vertices[i].z;
		}

		if ( mesh_ctx->br.x == 0 )
			mesh_ctx->br.x = mesh_ctx->vertices[i].x;
		else {
			if ( mesh_ctx->vertices[i].x > mesh_ctx->br.x )
				mesh_ctx->br.x = mesh_ctx->vertices[i].x;
		}

		if ( mesh_ctx->br.z == 0 )
			mesh_ctx->br.z = mesh_ctx->vertices[i].z;
		else {
			if ( mesh_ctx->vertices[i].z > mesh_ctx->br.z )
				mesh_ctx->br.z = mesh_ctx->vertices[i].z;
		}
	}

	//计算出地图的宽和高
	mesh_ctx->width = (uint32_t)( mesh_ctx->br.x - mesh_ctx->lt.x );
	mesh_ctx->heigh = (uint32_t)( mesh_ctx->br.z - mesh_ctx->lt.z );

	mesh_ctx->border_size = 64;
	mesh_ctx->border_offset = 0;
	mesh_ctx->borders = ( struct nav_border * )malloc(sizeof( struct nav_border ) * mesh_ctx->border_size);
	memset(mesh_ctx->borders, 0, sizeof( struct nav_border ) * mesh_ctx->border_size);

	struct nav_border_searcher** border_searcher = ( struct nav_border_searcher** )malloc(sizeof( *border_searcher ) * mesh_ctx->vertices_size);
	memset(border_searcher, 0, sizeof( *border_searcher ) * mesh_ctx->vertices_size);

	mesh_ctx->node_size = node_size;
	mesh_ctx->node = ( struct nav_node * )malloc(sizeof( struct nav_node ) * mesh_ctx->node_size);
	memset(mesh_ctx->node, 0, sizeof( struct nav_node ) * mesh_ctx->node_size);

	//初始化多边形信息，并计算出多边形的边
	for ( i = 0; i < node_size; i++ ) {
		struct nav_node* node = &mesh_ctx->node[i];
		memset(node, 0, sizeof( *node ));
		mh_init(&node->elt);
		node->id = i;

		node->size = p[i][0];

		node->border = (int*)malloc(node->size * sizeof( int ));
		node->poly = (int*)malloc(node->size * sizeof( int ));

		struct vector3 center;
		center.x = center.y = center.z = 0;

		node->link_border = -1;
		node->link_parent = NULL;

		for ( j = 1; j <= node->size; j++ ) {
			node->poly[j - 1] = p[i][j];
			center.x += mesh_ctx->vertices[node->poly[j - 1]].x;
			center.y += mesh_ctx->vertices[node->poly[j - 1]].y;
			center.z += mesh_ctx->vertices[node->poly[j - 1]].z;
		}
		node->mask = 0;
		node->center.x = center.x / node->size;
		node->center.y = center.y / node->size;
		node->center.z = center.z / node->size;

		node->area = poly_area(mesh_ctx, node->size, node->poly);
		mesh_ctx->area += node->area;

		//顶点顺时针排序
		vertex_sort(mesh_ctx, node);

		//初始化多边形的边
		for ( k = 0; k < node->size; k++ ) {
			int k0 = k;
			int k1 = k + 1 >= node->size ? 0 : k + 1;

			int a = node->poly[k0];
			int b = node->poly[k1];

			struct nav_border* border = search_border(mesh_ctx, border_searcher, a, b);
			border_link_node(border, node->id);

			int border_id = border->id;
			node->border[k] = border_id;

			struct nav_border* border_opposite = search_border(mesh_ctx, border_searcher, b, a);
			border_link_node(border_opposite, node->id);
			border_opposite->opposite = border_id;

			border = get_border(mesh_ctx, border_id);
			border->opposite = border_opposite->id;
		}
	}

	release_border_searcher(mesh_ctx, border_searcher);

	return mesh_ctx;
}


void
release_mesh(struct nav_mesh_context* ctx) {
	free(ctx->vertices);
	free(ctx->borders);
	int i;
	for ( i = 0; i < ctx->node_size; i++ ) {
		if ( ctx->node[i].border != NULL )
			free(ctx->node[i].border);
		if ( ctx->node[i].poly != NULL )
			free(ctx->node[i].poly);
	}
	free(ctx->node);
	free(ctx->mask_ctx.mask);
	free(ctx->result.wp);
	mh_dtor(&ctx->openlist);
	if ( ctx->tile != NULL ) {
		release_tile(ctx, ctx->tile);
	}
	free(ctx);
}
