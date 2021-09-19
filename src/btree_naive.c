#include "btree.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Definitions */
typedef unsigned char byte;

struct node {
	size_t       n; /* number of items/keys/elements */
	size_t       c; /* number of children */
	byte        *items;
	struct node **children;
};

struct btree {
	/* Memory stuffs */
	void *(*alloc)(size_t);
	void  (*dealloc)(void*);

	/* Size stuffs */
	size_t elem_size;
	size_t degree;

	struct node *root;

	/* comparison */
	int (*cmp)(const void *a, const void *b);
};

/* Node memory */

/* `node_new` allocates a new leaf node, children should be added and allocated
 * when the node is no longer a leaf node */
struct node* node_new(struct btree *tree) {
	const size_t max_items = 2 * tree->degree - 1;
	struct node *retval = malloc(sizeof(struct node));

	retval->n        = 0;
	retval->c        = 0;
	retval->items    = calloc(max_items, sizeof(tree->elem_size));
	retval->children = NULL;

	return retval;
}

/* `node_transition` turns a leaf node into a non-leaf and allocates children
 * for it.
 * returnvalue: `false` if we we're unable to allocate space for the new
 * children. */
bool node_transition(struct btree *tree, struct node *node) {
	const int max_children = 2 * tree->degree;
	int c;
	/* Allocate pointers for children */
	node->children = calloc(max_children, sizeof(struct node*));

	if (node->children == NULL) {
		perror("could not allocate space for children pointers");
		return false;
	}

	/* Allocate children */
	for (c = 0; c < max_children; c++) {
		node->children[c] = node_new(tree);
		if (node->children[c] == NULL) {
			perror("could not allocate space for all children, freeing...");
			for (c = c - 1;c >= 0; c--) {
				free(node->children[c]);
			}
			free(node->children);
			return false;
		}
	}

	node->c = c;

	return true;
}

void node_free(struct node *node, size_t elem_size, void (*dealloc)(void*)) {
	size_t i;
	if (node == NULL) return;
	for (i = 0; i < node->c; i++) {
		node_free((node->children)[i], elem_size, dealloc);
	}

	dealloc(node->items);

	free(node);
}

/* Node functionality */
#define \
node_leaf(node) (node->children)

#define \
node_maxdegree(t) (2 * t - 1)

#define \
node_mindegree(t) (t - 1)

/* Split a child of `nonfull` of index `i` */
node_tree_split_child(struct node *nonfull, size_t i) {}

/* `node_split` splits a _full_ node (c = 2t-1 items) into two nodes with t-1
 * items each.
 * The median key/item/element moves up to the original nodes parent, to signify
 * the split.
 * If the parent is full too, we need to split it before inserting the median
 * key.
 * This can potentially split full nodes all the way up throughout the tree. */
/* Instead of waiting to find out wether we should split the nodes, we split the
 * full nodes we encounter on the way down, including the leafs themselves.
 * By doing this, we are assured that whenever we split a node, its parent has
 * room for the median key. */
struct node *node_split() {
	/* TODO implement */
	return NULL;
}

int node_insert(struct node *node, void *elem, size_t elem_size) {
	/* TODO: test to see if a node already contains elem */
	/* TODO: balance the tree */
	memcpy((node->items)+node->c*elem_size, elem, elem_size);
	(node->n)++;
	return 0;
}

void* node_search(struct node *x,
                  void *key,
                  int(*cmp)(const void *a, const void *b),
                  const size_t elem_size) {
	size_t i            = 0;
	int    last_cmp_res = cmp(key, (const void*)x->items);

	while (i < x->n && last_cmp_res > 0) {
		i++;
		last_cmp_res = cmp(key, (const void*)(x->items + (i * elem_size)));
	}

	if (i < x->n && last_cmp_res == BTREE_CMP_EQ) {
		return (void*)(x->items + (i * elem_size));
	} else if (node_leaf(x)) {
		return NULL;
	}

	/* Assumption: ¬node_leaf(x) → x.children is allocated */
	return node_search(x->children[i], key, cmp, elem_size);
}


/* Btree functionality */
struct btree* btree_new(size_t elem_size,
                        size_t t,
                        int(*cmp)(const void *a, const void *b)) {
	return btree_new_with_allocator(elem_size, t, cmp, malloc, free);
}

struct btree* btree_new_with_allocator(size_t elem_size,
                        size_t t,
                        int(*cmp)(const void *a, const void *b),
                        void *(*alloc)(size_t),
                        void (*dealloc)(void*)) {
	struct btree *new_tree = malloc(sizeof(struct btree));

	new_tree->alloc     = alloc;
	new_tree->dealloc   = dealloc;

	new_tree->elem_size = elem_size;
	new_tree->degree    = t;

	new_tree->cmp       = cmp;
}

void btree_free(struct btree *btree) {
	node_free(btree->root, btree->elem_size, btree->dealloc);
	free(btree);
	btree = NULL;
}

void* btree_insert(struct btree *btree, void *elem) {
	if (btree->root == NULL) {
		btree->root = node_new(btree);
		node_insert(btree->root, elem, btree->elem_size);
	}
}

void* btree_search(struct btree *btree, void *elem) {
	return node_search(btree->root, elem, btree->cmp, btree->elem_size);
}

void* btree_delete(struct btree *btree, void *elem) {}
void* btree_update(struct btree *btree, void *elem_key, void *elem) {}
