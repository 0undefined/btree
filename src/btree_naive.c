#include "btree.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>

/* Definitions */
typedef unsigned char byte;

struct node {
	ssize_t      n; /* number of items/keys/elements */
	ssize_t      c; /* number of children */
	byte        *items;
	struct node **children;
};

struct btree {
	/* Memory stuffs */
	void *(*alloc)(size_t);
	void  (*dealloc)(void*);

	/* Size stuffs */
	size_t elem_size;
	ssize_t degree;

	struct node *root;

	/* comparison */
	int (*cmp)(const void *a, const void *b);
};

/**********************/
/* Node functionality */
/**********************/
#define \
node_leaf(node) (node->children == NULL)

#define \
node_maxdegree(t) (2 * t - 1)

#define \
node_mindegree(t) (t - 1)

#define \
node_full(degree, t) (t->n >= 2 * degree - 1)

/* Node memory */

/* `node_new` allocates a new leaf node, children should be added and allocated
 * when the node is no longer a leaf node */
struct node* node_new(const ssize_t degree, const size_t elem_size) {
	const size_t max_items = 2 * degree;
	struct node *retval = calloc(1, sizeof(struct node));

	retval->n = 0;
	retval->c = 0;
	retval->items    = calloc(max_items, elem_size);
	retval->children = NULL;

	return retval;
}

/* `node_transition` turns a leaf node into a non-leaf and allocates children
 * for it.
 * returnvalue: `false` if we we're unable to allocate space for the new
 * children. */
bool node_transition(struct node *node, const ssize_t degree, const size_t elem_size) {
	const int max_children = 2 * degree + 1;
	int c;
	/* Allocate pointers for children */
	node->children = calloc(max_children, sizeof(struct node*));

	if (node->children == NULL) {
		perror("could not allocate space for children pointers");
		return false;
	}

	/* Allocate children */
	for (c = 0; c < max_children; c++) {
		node->children[c] = node_new(degree, elem_size);
		if (node->children[c] == NULL) {
			perror("could not allocate space for all children, freeing...");
			for (c = c - 1;c >= 0; c--) {
				free(node->children[c]);
			}
			free(node->children);
			return false;
		}
	}

	return true;
}

void node_free(struct node *node, size_t elem_size, void (*dealloc)(void*)) {
	ssize_t i;
	if (node == NULL) return;
	if (!node_leaf(node)) {
		for (i = 0; i < node->c; i++) {
			node_free((node->children)[i], elem_size, dealloc);
		}
	}

	dealloc(node->items);
	node->items = NULL;

	free(node);
	node = NULL;
}


/* `node_tree_split_child` splits a _full_ node (c = 2t-1 items) into two nodes
 * with t-1 items each.
 * The median key/item/element moves up to the original nodes parent, to signify
 * the split.  If the parent is full too, we need to split it before inserting
 * the median key.
 * This can potentially split full nodes all the way up throughout the tree. */
/* Instead of waiting to find out wether we should split the nodes, we split the
 * full nodes we encounter on the way down, including the leafs themselves.
 * By doing this, we are assured that whenever we split a node, its parent has
 * room for the median key. */
void node_tree_split_child(
		const ssize_t t,
		const size_t elem_size,
		struct node *nonfull,
		ssize_t i) {
	struct node *z = node_new(t, elem_size);
	struct node *y = nonfull->children[i];
	ssize_t j;

	/* `z` should be a branching node if `y` is */
	if (!node_leaf(y)) {
		node_transition(z, t, elem_size);
	}

	z->n = t - 1;

	/* Move last `t-1` items to new node `z` */
	/* TODO This can be done with one memcpy */
	for (j = 0; j < t-1; j++) {
		const size_t offset_dst = elem_size * j;
		const size_t offset_src = elem_size * (t+j);
		memcpy((z->items) + offset_dst, (y->items) + offset_src, elem_size);
	}
	/* Set unused item-memory to zero? */

	/* Move children t..2t, if applicable*/
	if (!node_leaf(y)) {
		for (j = 0; j < t+1; j++) {
			z->children[j] = y->children[j + t];
		}
		y->c = t;
		z->c = t;
	}

	y->n = t - 1;

	/* Move children +1 */
	for (j = nonfull->n; j > i; j--) {
		nonfull->children[j+1] = nonfull->children[j];
	}

	/* new child */
	nonfull->children[i+1] = z;
	nonfull->c++;

	/* moving keys i..n + 1*/
	/* TODO This can be done with one memcpy */
	for (j = nonfull->n; j >= i; j--) {
		const size_t offset = j * elem_size;
		memcpy((nonfull->items) + offset + elem_size,
		       (nonfull->items) + offset,
		        elem_size);
	}

	/* Lastly, copy the median element to nonfull-parent*/
	memcpy((nonfull->items) + i * elem_size,
	       (y->items)       + (t-1) * elem_size,
	       elem_size);

	nonfull->n++;
}

/* `node_child_merge`: Merges two children around the key at index `i` (k)
 * by appending k to the left child (y) followed by
 * appending the right child (z) to y
 *
 * `x`: The parent node of y and z
 * `i`: Index of the item that acts as the new median of the merged node
 *
 * WARNING: THIS FUNCTION ASSUMES THAT `i` IS A VALID INDEX
 */
void node_child_merge(
		struct node *x,
		ssize_t i,
		const ssize_t t,
		const size_t elem_size) {
	struct node* y = x->children[i  ];
	struct node* z = x->children[i+1];
	int j = 0;

	/* append k to y */
	memcpy(y->items + (elem_size * y->n++),
	       x->items + (elem_size * i),
	       elem_size);

	/* append keys in z to y */
	memcpy(y->items + (elem_size * y->n),
	       z->items,
	       elem_size * z->n);
	y->n += z->n;

	/* Move children from z to y */
	for (j = 0; j < z->c; j++) {
		y->children[j+y->c++] = z->children[j];
	}

	/* Remove z from x */
	for (j = i+1; j < x->c; j++) {
		x->children[j] = x->children[j+1];
	}
	x->c--;

	/* remove k from x */
	/* TODO check if we need to use (x->n - 1 - i) instead */
	memmove(x->items + (elem_size * i),
	        x->items + (elem_size * (i+1)),
	        elem_size * (x->n - i));
	x->n--;

	free(z); /* DO NOT USE THE RECURSIVE ONE AS CHILDREN WILL BE LOST!!! */
}

/* ASSUME i < x->c */
void node_shift_left(
		struct node *x,
		ssize_t i,
		const ssize_t t,
		const size_t elem_size) {
	struct node* y = x->children[i  ];
	struct node* z = x->children[i+1];
	byte *x_k = x->items + (elem_size * i);
	long unsigned j = 0;

	/* Append x.k[i] to y */
	memcpy(y->items + (elem_size * y->n++),
	       x_k,
	       elem_size);

	/* Move first element of z to x.k[i] */
	memcpy(x_k,
	       z->items,
	       elem_size);

	/* Shift z's items left */
	memmove(z->items,
	        z->items + elem_size,
	        elem_size * (z->n - 1));

	if (!node_leaf(z)) {
		/* append first child of z to y */
		y->children[y->c++] = z->children[0];

		/* Shift z's children left */
		for (j = 0; j < z->c; j++) {
			z->children[j] = z->children[j+1];
		}
		z->c--;
	}

	z->n--;
}

void node_shift_right(
		struct node *x,
		ssize_t i,
		const ssize_t t,
		const size_t elem_size) {
	struct node* y = x->children[i  ];
	struct node* z = x->children[i+1];
	byte *x_k = x->items + (elem_size * i);
	long unsigned j = 0;

	/* Shift z's items right */
	memmove(z->items + elem_size,
	        z->items,
	        elem_size * z->n);

	/* Prepend x.k[i] to z */
	memcpy(z->items,
	       x_k,
	       elem_size);

	/* Move last element of y to x.k[i] */
	memcpy(x_k,
	       y->items + (elem_size * --(y->n)),
	       elem_size);

	if (!node_leaf(z)) {
		/* Shift z's children right */
		for (j = z->c; j > 0; j--) {
			z->children[j] = z->children[j-1];
		}
		z->c++;

		/* prepend last child of y to z */
		z->children[0] = y->children[--(y->c)];
	}

	z->n++;
}

/* return: Returns the new root, if a split happens */
struct node* node_insert_nonfull(
		struct node *root,
		void *elem,
		const ssize_t degree,
		const size_t elem_size,
		int (*cmp)(const void *a, const void *b)) {
	/* TODO check correctness */
	ssize_t i = root->n - 1;
	if (node_leaf(root)) {
		size_t offset = elem_size * i;
		while (i >= 0 && cmp(elem, root->items + offset) < 0) {
			/* TODO This can be done with one memcpy */
			memcpy(root->items + offset + elem_size,
			       root->items + offset,
			       elem_size);

			i--;
			offset = elem_size * i;
		}
		offset = elem_size * (++i);
		memcpy(root->items + offset, elem, elem_size);
		root->n++;

	} else {
		size_t offset = elem_size * i;
		struct node *nextchild = NULL;
		while (i >= 0 && cmp(elem, root->items + offset) < 0) {
			i--;
			offset = elem_size * i;
		}
		i++;
		nextchild = root->children[i];
		if (node_full(degree, nextchild)) {
			/* TODO Check if the root has changed */
			node_tree_split_child(degree, elem_size, root, i);
			if (cmp(elem, root->items + elem_size * i) > 0) {
				nextchild = root->children[++i];
			}
		}
		return node_insert_nonfull(nextchild, elem, degree, elem_size, cmp);
	}
	return NULL; /* TODO: Fix return value */
}

/* Returns the new root, if a split occurs */
struct node* node_insert(
		struct node *root,
		void *elem,
		const ssize_t degree,
		const size_t elem_size,
		int (*cmp)(const void *a, const void *b)) {

	struct node *s = root;

	if (node_full(degree, root)) {
		s = node_new(degree, elem_size);
		node_transition(s, degree, elem_size);
		s->children[s->c++] = root;
		/* TODO Check if the root has changed */
		node_tree_split_child(degree, elem_size, s, 0);
		node_insert_nonfull(s, elem, degree, elem_size, cmp);
	}
	else {
		node_insert_nonfull(s, elem, degree, elem_size, cmp);
	}
	return s;
}

void* node_search(struct node *x,
                  void *key,
                  int(*cmp)(const void *a, const void *b),
                  const size_t elem_size) {
	/* We set to one, since we pre-emptively do a comparison with the assumption
	 * that there's already one in the items */
	ssize_t i = 0;
	int    last_cmp_res;

	while (i < x->n
	   &&  (last_cmp_res = cmp(key, (const void*)(x->items + (i * elem_size))))
	      > 0) {
		i++;
	}

	if ((ssize_t)i < x->n && last_cmp_res == 0) {
		return (void*)(x->items + (i * elem_size));
	} else if (node_leaf(x)) {
		return NULL;
	}

	/* Assumption: ¬node_leaf(x) → x.children is allocated */
	return node_search(x->children[i], key, cmp, elem_size);
}

int node_delete(struct node *x,
                void *key,
                int(*cmp)(const void *a, const void *b),
                const ssize_t degree,
                const size_t elem_size) {
	/* Determine wether the key is in the node */
	int i = 0; /* Index of `k`, if found */
	int last_cmp_res;

	while (i < x->n
	   && (last_cmp_res = cmp(key, (const void*)(x->items + (i * elem_size))))
	      > 0) {
		i++;
	}

	if (last_cmp_res == 0) {

		if (node_leaf(x)) {
			/* 1. k ϵ x && node_leaf(x) */
			/* Delete k from x */
			int j = i;
			while (j + 1 < x->n) {
				const size_t offset_dst = elem_size * j;
				const size_t offset_src = elem_size * (j+1);
				memcpy((x->items) + offset_dst,
				       (x->items) + offset_src,
				       elem_size);
				j++;
			}
			x->n--;
			return 1;
		} else {
			/* 2. k ϵ x && !node_leaf(x) */
			/* let i be the index of k in x */
			/* 2a: if size(child[i]) >= t; find the largest k' in child[i] */
			/* replace k with k' */
			if (x->children[i]->n >= degree) {
				struct node* y = x->children[i];
				byte *kk = malloc(elem_size);

				/* Find the predecessor, k' of k in y */
				{
					struct node* tmp = y->children[y->n-1];
					while (!node_leaf(tmp)) {
						tmp = tmp->children[tmp->n-1];
					}

					/* copy kk */
					memcpy(kk, tmp->items + elem_size * (tmp->n - 1), elem_size);
				}

				/* Recursively delete kk from y */
				node_delete(y, kk, cmp, degree, elem_size);

				/* replace k with kk */
				memcpy(x->items  + (elem_size * i),
				       kk,
				       elem_size);

				free(kk);

				return 1;

			} else if (x->children[i+1]->n >= degree) {
				struct node* z = x->children[i+1];
				byte *kk = malloc(elem_size);

				/* Find the successor, k' of k in z */
				{
					struct node* tmp = z->children[0];
					while (!node_leaf(tmp)) {
						tmp = tmp->children[0];
					}

					/* copy kk */
					memcpy(kk, tmp->items + elem_size * (tmp->n - 1), elem_size);
				}

				/* Recursively delete kk from y */
				node_delete(z, kk, cmp, degree, elem_size);

				/* replace k with kk */
				memcpy(x->items + (elem_size * i),
				       kk,
				       elem_size);

				free(kk);

				return 1;
			} else {
				/* Merge k and z into y */
				node_child_merge(x, i, degree, elem_size);

				/* recurse */
				return node_delete(x->children[i], key, cmp, degree, elem_size);
			}
		}
	} else {
		/* 3. !(k ϵ x) */

		/*  if x is a leaf, then it is not in the tree */
		if (node_leaf(x)) return 0;

		struct node* y;
		int ii; /* index of x.c[i] */

		/* Determine x.c[i] that must contain k */
		/* if last cmp < 0, then the child must be in the left child of x.items[i]*/
		if (last_cmp_res < 0) ii = i;
		/* Otherwise, it must be the very last child */
		else if (i < x->n) ii = i+1;
		else ii = i;

		y = x->children[ii];

		if (y->n < degree) {
			/* we are left biased */
			if        (ii > 0        && x->children[ii-1]->n >= degree) {
				node_shift_right(x, ii-1, degree, elem_size);

			} else if (ii < x->c - 1 && x->children[ii+1]->n >= degree) {
				node_shift_left (x, ii,   degree, elem_size);

			} else {
				/* We need to determine wether we merge left or right, if possible */
				if (ii > 0) node_child_merge(x, ii - 1, degree, elem_size);
				else        node_child_merge(x, ii,     degree, elem_size);
			}

		}

		return node_delete(y, key, cmp, degree, elem_size);
	}
	return 0;
}


/***********************/
/* Btree functionality */
/***********************/
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

	return new_tree;
}

void btree_free(struct btree *btree) {
	node_free(btree->root, btree->elem_size, btree->dealloc);
	free(btree);
	btree = NULL;
}

void btree_insert(struct btree *btree, void *elem) {
	if (btree->root == NULL) {
		btree->root = node_new(btree->degree, btree->elem_size);
		node_insert(btree->root,
		            elem,
		            btree->degree,
		            btree->elem_size,
		            btree->cmp);
	}
	else {
		btree->root = node_insert(btree->root,
		                          elem,
		                          btree->degree,
		                          btree->elem_size,
		                          btree->cmp);
	}
}

void* btree_search(struct btree *btree, void *elem) {
	return node_search(btree->root, elem, btree->cmp, btree->elem_size);
}

int btree_delete(struct btree *btree, void *elem) {
	struct node *newroot = btree->root;
	int res = node_delete(btree->root, elem, btree->cmp, btree->degree, btree->elem_size);
	if (newroot->n == 0) {
		/* shrink the tree */
		struct node *newroot_p = newroot->children[0];
		free(newroot);
		btree->root = newroot_p;
	}
	return res;
}

void* btree_update(struct btree *btree, void *elem_key, void *elem) {}


void node_print(struct node *root, const size_t elem_size, const int indent, void (*print_elem)(const void*)) {
	ssize_t i;
	int t;

	for (t = 0; t < indent - 1; t++) { fputs(" ┃ ", stdout); }
	if (indent > 0) { fputs(" ┣┯", stdout); }
	printf("printing node \x1b[33m%0lx\x1b[0m,"
	       " c:%ld n:%ld\t\t"
	       "\x1b[30m%p\x1b[0m\n",
	       (size_t)root % 0x10000,
	       root->c,
	       root->n,
	       (void*)root);

	if (node_leaf(root)) {
		for (i = 0; i < root->n - 1; i++) {
			const size_t ofst = i * elem_size;
			for (t = 0; t < indent; t++) { fputs(" ┃├", stdout); }
			print_elem(root->items + ofst);
		}
		for (t = 0; t < indent; t++) { fputs(" ┃└", stdout); }
		print_elem(root->items + i * elem_size);
	} else {
		size_t ofst = 0;
		for (i = 0; i < root->c - 1; i++) {
			node_print(root->children[i], elem_size, indent + 1, print_elem);
			for (t = 0; t < indent; t++) { fputs(" ┃ ", stdout); }
			print_elem(root->items + ofst);
			ofst += elem_size;
		}
		node_print(root->children[i], elem_size, indent + 1, print_elem);
	}

}

void btree_print(struct btree *btree, void (*print_elem)(const void*)) {
	printf("BTRee: degree:%ld\n", btree->degree);
	node_print(btree->root, btree->elem_size, 0, print_elem);
}
