#include "btree.h"

#include <stdlib.h>
#include <stdbool.h>

typedef unsigned char byte;

struct node {
	size_t n; /* number of items */
	bool leaf;
	byte *items;
	struct node *children;
};

struct btree {
	/* Memory stuffs */
	void *(*alloc)(size_t);
	void (*dealloc)(void*);

	/* Size stuffs */
	size_t elem_size;
	size_t degree;

	/* comparison */
	int (*cmp)(const void *a, const void *b);
};

struct btree *btree_new(size_t elem_size,
                        size_t t,
                        int(*cmp)(const void *a, const void *b)) {
	return btree_new_with_allocator(elem_size, t, cmp, malloc, free);
}

struct btree *btree_new_with_allocator(size_t elem_size,
                        size_t t,
                        int(*cmp)(const void *a, const void *b),
                        void *(*alloc)(size_t),
                        void (*dealloc)(void*)) {
	struct btree *new_tree = malloc(sizeof(struct btree));

	new_tree->alloc        = alloc;
	new_tree->dealloc      = dealloc;

	new_tree->elem_size    = elem_size;
	new_tree->degree       = t;

	new_tree->cmp          = cmp;
}

void btree_free(struct btree *btree) {
	/*btree->dealloc(btree->root);*/
	free(btree);
	btree = NULL;
}

void *btree_search(struct btree *btree, void *elem) {}
void *btree_insert(struct btree *btree, void *elem) {}
void *btree_delete(struct btree *btree, void *elem) {}
