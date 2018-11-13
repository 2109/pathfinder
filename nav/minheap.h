#ifndef _MINHEAP_H
#define _MINHEAP_H

#include <math.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


#ifndef _MSC_VER
#include <stdbool.h>
#else
#define inline __inline
#define false 0
#endif

struct element {
	int index;
};

struct minheap {
	int cap;
	int size;
	int(*less)( struct element *l, struct element *r );
	struct element **elts;
};


void minheap_ctor(struct minheap* mh, int(*less)( struct element *l, struct element *r ));
void minheap_dtor(struct minheap *mh);
void minheap_clear(struct minheap * mh, void(*clear)(struct element *elt));
void minheap_push(struct minheap * mh, struct element * elt);
void minheap_change(struct minheap * mh,struct element * elt);
void minheap_delete(struct minheap * mh,struct element * elt);
struct element * minheap_pop(struct minheap * mh);
struct element * minheap_top(struct minheap * mh);

#endif