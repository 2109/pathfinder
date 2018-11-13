#ifndef MINHEAP_ADAPTER_H
#define MINHEAP_ADAPTER_H

#ifdef MINHEAP_USE_LIBEVENT
#include "minheap-internal.h"
typedef min_heap_t mh_t;
typedef min_elt_t mh_elt_t;

#define mh_elt_has_init(elt) (((elt)->index) >= 0)
#define mh_init min_heap_elem_init_
#define mh_clear min_heap_clear_
#define mh_ctor min_heap_ctor_
#define mh_dtor min_heap_dtor_
#define mh_push min_heap_push_
#define mh_pop min_heap_pop_
#define mh_adjust min_heap_adjust_
#else
#include "minheap.h"
typedef struct minheap mh_t;
typedef struct element mh_elt_t;
#define mh_elt_has_init(elt) (((elt)->index) > 0)
#define mh_init minheap_elt_init
#define mh_clear minheap_clear
#define mh_ctor minheap_ctor
#define mh_dtor minheap_dtor
#define mh_push minheap_push
#define mh_pop minheap_pop
#define mh_adjust minheap_change
#endif


#endif