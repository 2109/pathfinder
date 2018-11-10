#ifndef _LIST_H
#define _LIST_H
#include <stdint.h>
#include <stdio.h>



struct list_node {
	struct list_node * pre;
	struct list_node * next;
};

struct list {
	struct list_node head;
	struct list_node tail;
};


#define list_head(dl) 			(dl->head.next)
#define list_tail(dl) 			(&dl->tail)
#define list_empty(dl) 			((dl)->head.next == &(dl)->tail ? 1:0)

#define list_init(dl) \
do {\
 	(dl)->head.pre = (dl)->tail.next = NULL; \
 	(dl)->head.next = &(dl)->tail;\
 	(dl)->tail.pre = &(dl)->head;\
 } while (0)


#define list_remove(node) \
do {\
	if ((node)->pre && (node)->next)\
	{\
		(node)->pre->next = (node)->next;\
		(node)->next->pre = (node)->pre;\
		(node)->pre = (node)->next = NULL;\
	}\
}while (0)


#define list_push(dl,node) 		\
do {\
	if ((node)->pre == NULL && (node)->next == NULL)\
	{\
		(dl)->tail.pre->next = node; \
		(node)->pre = (dl)->tail.pre; \
		(dl)->tail.pre = (node); \
		(node)->next = &(dl)->tail;\
	}\
} while (false)

static inline struct list_node *
list_pop(struct list * dl) {
	struct list_node * node = NULL;
	if (!list_empty(dl)) {
		node = dl->head.next;
		list_remove(node);
	}
	return node;
}
#endif