/*
 * Copyright (c) 2007-2012 Niels Provos and Nick Mathewson
 *
 * Copyright (c) 2006 Maxim Yegorushkin <maxim.yegorushkin@gmail.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef MINHEAP_INTERNAL_H_INCLUDED_
#define MINHEAP_INTERNAL_H_INCLUDED_

#include <stdlib.h>
#include <string.h>

typedef struct min_elt {
	int index;
} min_elt_t;

typedef struct min_heap
{
	min_elt_t** p;
	int(*elem_greater)( min_elt_t *l, min_elt_t *r );
	unsigned n, a;
} min_heap_t;

static inline void	     min_heap_ctor_(min_heap_t* s, int(*elem_greater)( min_elt_t *l, min_elt_t *r ));
static inline void	     min_heap_dtor_(min_heap_t* s);
static inline void	     min_heap_clear_(min_heap_t* s, void(*clear)(min_elt_t *e));
static inline void	     min_heap_elem_init_(min_elt_t* e);
static inline int	     min_heap_elt_is_top_(const min_elt_t *e);
static inline int	     min_heap_empty_(min_heap_t* s);
static inline unsigned	     min_heap_size_(min_heap_t* s);
static inline min_elt_t*  min_heap_top_(min_heap_t* s);
static inline int	     min_heap_reserve_(min_heap_t* s, unsigned n);
static inline int	     min_heap_push_(min_heap_t* s, min_elt_t* e);
static inline min_elt_t*  min_heap_pop_(min_heap_t* s);
static inline int	     min_heap_adjust_(min_heap_t *s, min_elt_t* e);
static inline int	     min_heap_erase_(min_heap_t* s, min_elt_t* e);
static inline void	     min_heap_shift_up_(min_heap_t* s, unsigned hole_index, min_elt_t* e);
static inline void	     min_heap_shift_up_unconditional_(min_heap_t* s, unsigned hole_index, min_elt_t* e);
static inline void	     min_heap_shift_down_(min_heap_t* s, unsigned hole_index, min_elt_t* e);



void min_heap_ctor_(min_heap_t* s, int(*elem_greater)( min_elt_t *l, min_elt_t *r )) {
	s->p = 0; s->n = 0; s->a = 0;
	s->elem_greater = elem_greater;
}

void min_heap_dtor_(min_heap_t* s) { if (s->p) free(s->p); }
void min_heap_clear_(min_heap_t* s, void(*clear)( min_elt_t *e )) {
	unsigned i;
	for ( i = 0; i < s->n; i++ ) {
		if ( clear != NULL )
			clear(s->p[i]);
		else
			s->p[i]->index = -1;
	}
	s->n = 0;
}
void min_heap_elem_init_(min_elt_t* e) {
	e->index = -1;
}
int min_heap_empty_(min_heap_t* s) { return 0u == s->n; }
unsigned min_heap_size_(min_heap_t* s) { return s->n; }
min_elt_t* min_heap_top_(min_heap_t* s) {
	return s->n ? *s->p : 0;
}

int min_heap_push_(min_heap_t* s, min_elt_t* e)
{
	if (min_heap_reserve_(s, s->n + 1))
		return -1;
	min_heap_shift_up_(s, s->n++, e);
	return 0;
}

min_elt_t* min_heap_pop_(min_heap_t* s)
{
	if (s->n)
	{
		min_elt_t* e = *s->p;
		min_heap_shift_down_(s, 0u, s->p[--s->n]);
		e->index = -1;
		return e;
	}
	return 0;
}

int min_heap_elt_is_top_(const min_elt_t *e)
{
	return e->index == 0;
}

int min_heap_erase_(min_heap_t* s, min_elt_t* e)
{
	if (-1 != e->index)
	{
		min_elt_t *last = s->p[--s->n];
		unsigned parent = (e->index - 1) / 2;
		/* we replace e with the last element in the heap.  We might need to
		   shift it upward if it is less than its parent, or downward if it is
		   greater than one or both its children. Since the children are known
		   to be less than the parent, it can't need to shift both up and
		   down. */
		if ( e->index > 0 && s->elem_greater(s->p[parent], last) )
			min_heap_shift_up_unconditional_(s, e->index, last);
		else
			min_heap_shift_down_(s, e->index, last);
		e->index = -1;
		return 0;
	}
	return -1;
}

int min_heap_adjust_(min_heap_t *s, min_elt_t *e)
{
	if (-1 == e->index) {
		return min_heap_push_(s, e);
	} else {
		unsigned parent = (e->index - 1) / 2;
		/* The position of e has changed; we shift it up or down
		 * as needed.  We can't need to do both. */
		if ( e->index > 0 && s->elem_greater(s->p[parent], e) )
			min_heap_shift_up_unconditional_(s, e->index, e);
		else
			min_heap_shift_down_(s, e->index, e);
		return 0;
	}
}

int min_heap_reserve_(min_heap_t* s, unsigned n)
{
	if (s->a < n)
	{
		min_elt_t** p;
		unsigned a = s->a ? s->a * 2 : 8;
		if (a < n)
			a = n;
		if (!(p = (min_elt_t**)realloc(s->p, a * sizeof *p)))
			return -1;
		s->p = p;
		s->a = a;
	}
	return 0;
}

void min_heap_shift_up_unconditional_(min_heap_t* s, unsigned hole_index, min_elt_t* e)
{
    unsigned parent = (hole_index - 1) / 2;
    do
    {
	(s->p[hole_index] = s->p[parent])->index = hole_index;
	hole_index = parent;
	parent = (hole_index - 1) / 2;
	} while ( hole_index && s->elem_greater(s->p[parent], e) );
    (s->p[hole_index] = e)->index = hole_index;
}

void min_heap_shift_up_(min_heap_t* s, unsigned hole_index, min_elt_t* e)
{
    unsigned parent = (hole_index - 1) / 2;
	while ( hole_index && s->elem_greater(s->p[parent], e) )
    {
	(s->p[hole_index] = s->p[parent])->index = hole_index;
	hole_index = parent;
	parent = (hole_index - 1) / 2;
    }
    (s->p[hole_index] = e)->index = hole_index;
}

void min_heap_shift_down_(min_heap_t* s, unsigned hole_index, min_elt_t* e)
{
    unsigned min_child = 2 * (hole_index + 1);
    while (min_child <= s->n)
	{
	min_child -= min_child == s->n || s->elem_greater(s->p[min_child], s->p[min_child - 1]);
	if ( !( s->elem_greater(e, s->p[min_child]) ) )
	    break;
	(s->p[hole_index] = s->p[min_child])->index = hole_index;
	hole_index = min_child;
	min_child = 2 * (hole_index + 1);
	}
    (s->p[hole_index] = e)->index = hole_index;
}

#endif /* MINHEAP_INTERNAL_H_INCLUDED_ */
