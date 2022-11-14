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

struct btree_iter_t {
	size_t head;
	struct stack {
		int pos;
		struct node* node;
	} stack[512];
	/* This heavily relies on the assumption that a tree never grows deeper than
	 * 512 nodes */
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

/* `node_transition` turns a leaf node into a non-leaf. Children are not
 * allocated.
 * returnvalue: `false` if we we're unable to allocate space for the new
 * children. */
bool node_transition(struct node *node, const ssize_t degree) {
	const int max_children = 2 * degree + 1;
	int c;

	if (!node_leaf(node)) {
		perror("node_transition was called on an already non-leaf node?");
		return false;
	}

	/* Allocate pointers for children */
	node->children = calloc(max_children, sizeof(struct node*));

	if (node->children == NULL) {
		perror("could not allocate space for children pointers");
		return false;
	}

	/* Allocate children */
	for (c = 0; c < max_children; c++) {
		node->children[c] = NULL;
	}

	return true;
}

void node_free(struct node **node, size_t elem_size, void (*dealloc)(void*)) {
	if (*node == NULL) return;

	if (!node_leaf((*node))) {
		ssize_t i;
		for (i = 0; i < (*node)->c; i++) {
			node_free(&((*node)->children[i]), elem_size, dealloc);
		}
		dealloc((*node)->children);
	}

	dealloc((*node)->items);
	(*node)->items = NULL;

	dealloc(*node);
	*node = NULL;
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
		node_transition(z, t);
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
		const size_t elem_size, void (*dealloc)(void*)) {
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
		y->children[y->c + j] = z->children[j];
	}
	y->c += z->c;

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

	dealloc(z); /* DO NOT USE THE RECURSIVE ONE AS CHILDREN WILL BE LOST!!! */
}

/* ASSUME i < x->c */
void node_shift_left(
		struct node *x,
		ssize_t i,
		const size_t elem_size) {
	struct node* y = x->children[i  ];
	struct node* z = x->children[i+1];
	byte *x_k = x->items + (elem_size * i);

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
		ssize_t j;
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
		const size_t elem_size) {
	struct node* y = x->children[i  ];
	struct node* z = x->children[i+1];
	byte *x_k = x->items + (elem_size * i);

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
		size_t j;
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
void node_insert_nonfull(
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
		node_insert_nonfull(nextchild, elem, degree, elem_size, cmp);
	}
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
		if (s == NULL) {
			fputs("BTree error: Failed to allocate new node for insertion!\n", stderr);
			return NULL;
		}
		node_transition(s, degree);
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
                const size_t elem_size, void *(*alloc)(size_t), void (*dealloc)(void*)) {
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
				byte *kk = alloc(elem_size);

				/* Find the predecessor, k' of k in y */
				{
					struct node* tmp = y;
					while (!node_leaf(tmp)) {
						tmp = tmp->children[tmp->n-1];
					}

					/* copy kk */
					memcpy(kk, tmp->items + elem_size * (tmp->n - 1), elem_size);
				}

				/* Recursively delete kk from y */
				return node_delete(y, kk, cmp, degree, elem_size, alloc, dealloc);

				/* replace k with kk */
				memcpy(x->items  + (elem_size * i),
				       kk,
				       elem_size);

				dealloc(kk);

				return 1;

			} else if (x->children[i+1]->n >= degree) {
				struct node* z = x->children[i+1];
				byte *kk = alloc(elem_size);

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
				return node_delete(z, kk, cmp, degree, elem_size, alloc, dealloc);

				/* replace k with kk */
				memcpy(x->items + (elem_size * i),
				       kk,
				       elem_size);

				dealloc(kk);

				return 1;
			} else {
				/* Merge k and z into y */
				node_child_merge(x, i, elem_size, dealloc);

				/* recurse */
				return node_delete(x->children[i], key, cmp, degree, elem_size, alloc, dealloc);
			}
		}
	} else if (node_leaf(x)) {
		return 0;
	} else {
		/* 3. !(k ϵ x) */

		/*  if x is a leaf, then it is not in the tree */

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
				node_shift_right(x, ii-1, elem_size);

			} else if (ii < x->c - 1 && x->children[ii+1]->n >= degree) {
				node_shift_left (x, ii,   elem_size);

			} else {
				/* We need to determine wether we merge left or right, if possible */
				if (ii > 0)             {
					node_child_merge(x, ii - 1, elem_size, dealloc);
					y = x->children[ii - 1];
				}
				else if (ii < x->c - 1) {
					node_child_merge(x, ii,     elem_size, dealloc);
				}
				else {
					perror("Cannot merge!");
				}
			}

		}

		return node_delete(y, key, cmp, degree, elem_size, alloc, dealloc);
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
	struct btree *new_tree = alloc(sizeof(struct btree));

	new_tree->alloc     = alloc;
	new_tree->dealloc   = dealloc;

	new_tree->elem_size = elem_size;
	new_tree->degree    = t;

	new_tree->root      = NULL;

	new_tree->cmp       = cmp;

	return new_tree;
}

void btree_free(struct btree **btree) {
	node_free(&((*btree)->root), (*btree)->elem_size, (*btree)->dealloc);
	(*btree)->dealloc(*btree);
	*btree = NULL;
}

void btree_insert(struct btree *btree, void *elem) {
	if (btree == NULL) {
		fputs("BTree error: Inserting into a NULL ptr!\n", stderr);
		return;
	}
	if (elem == NULL) {
		fputs("BTree error: Inserting NULL into a tree!\n", stderr);
		return;
	}
	if (btree->root == NULL) {
		btree->root = node_new(btree->degree, btree->elem_size);
		if (btree->root == NULL) {
			fputs("BTree error: Failed to create new root node!\n", stderr);
			return;
		}
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
	int res = node_delete(btree->root, elem, btree->cmp, btree->degree, btree->elem_size, btree->alloc, btree->dealloc);
	if (newroot->n == 0) {
		/* shrink the tree */
		struct node *newroot_p = newroot->children[0];
		btree->dealloc(newroot);
		btree->root = newroot_p;
	}
	return res;
}

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

void* btree_first(struct btree *btree) {
	struct node *root;
	if (btree == NULL) return NULL;
	root = btree->root;

	if (root == NULL) return NULL;

	while (!node_leaf(root)) root = root->children[0];

	if (root->n == 0) return NULL;
	return root->items; /* Return first element */
}

void* btree_last(struct btree *btree) {
	struct node *root;

	if (btree == NULL) return NULL;
	root = btree->root;

	if (root == NULL) return NULL;

	while (!node_leaf(root)) root = root->children[root->c];

	if (root->n == 0) return NULL;
	return root->items + btree->elem_size * (root->n - 1); /* Return first element */
}

size_t btree_height(struct btree *btree) {
	struct node *root;
	size_t height = 0;

	if (btree == NULL) return 0;
	root = btree->root;

	if (root == NULL) return 0;

	while (!node_leaf(root)) {
		root = root->children[0];
		height++;
	}

	return height;
}

size_t u32_pow(size_t base, size_t exponent) {
	size_t res = 1;
	while (exponent > 0) {
		res *= base;
		exponent--;
	}
	return res;
}


size_t btree_size(struct btree *btree) {
	return u32_pow(2 * btree->degree, btree_height(btree)) - 1;
}


struct btree_iter_t* btree_iter_t_new(struct btree *tree) {
	struct btree_iter_t *iter = NULL;

	if (tree == NULL) return NULL;

	iter = (struct btree_iter_t*)tree->alloc(sizeof(struct btree_iter_t));

	if (tree != NULL) {
		iter->head = 0;
		memset(iter->stack, 0, 512 * sizeof(struct node*));

		iter->stack[iter->head].pos  = 0;
		iter->stack[iter->head].node = tree->root;
	} else {
		perror("Cannot instantiate iterator from null-pointer tree");
	}
	return iter;
}


void btree_iter_t_reset(struct btree *tree, struct btree_iter_t** it) {
	(*it)->head = 0;

	(*it)->stack[0].pos  = 0;
	(*it)->stack[0].node = tree->root;
}


void* btree_iter(struct btree *tree, struct btree_iter_t *iter) {
	register int     pos  = 0;
	register ssize_t head = 0;
	register ssize_t n    = 0;

	if (iter->stack[head].node == NULL) return NULL;

	head = iter->head;
	pos  = iter->stack[head].pos;
	n    = iter->stack[head].node->n;

#define BTREE_ITER_POP(it) {         \
    iter->stack[head].pos  = 0;      \
    iter->stack[head].node = NULL;   \
    iter->head--; head--;            \
    iter->stack[head].pos++;         \
                                     \
    pos = iter->stack[head].pos;     \
    n   = iter->stack[head].node->n; \
}

	/* Check if we have reached the end of a node.
	 * Take note of the difference of inequality in the if statement and
	 * following while */

	/* Leafs are a special case, as, if the only node is the root node, we might
	 * want to exit */
	if (node_leaf(iter->stack[iter->head].node) && pos >= 2 * n) {
		if (head == 0) return NULL;

		/* Pop, if so */
		else BTREE_ITER_POP(iter);
	}

	/* Otherwise, pop while we have reached the end of a node */
	while (pos > 2 * n) {
		if (head == 0) return NULL;

		/* Pop, if so */
		else BTREE_ITER_POP(iter);
	}

#undef BTREE_ITER_POP

	/* On evens, we decent into children */
	if (!node_leaf(iter->stack[head].node)) {
		if (pos % 2 == 0) {
			/* push child node onto iter->stack */
			iter->stack[head + 1].pos  = 0;
			iter->stack[head + 1].node = iter->stack[head].node->children[pos / 2];
			iter->head++; head++;

			/* Decent all the way to the left, if pos == 0 */
			while (!node_leaf(iter->stack[iter->head].node)) {
				iter->stack[head + 1].pos  = 0;
				iter->stack[head + 1].node = iter->stack[head].node->children[0];
				iter->head++; head++;
			}
		}
	}

	/* Finally, update index and return a value */
	if (node_leaf(iter->stack[head].node)) {
		iter->stack[head].pos += 2;
		pos = iter->stack[head].pos;
	}
	else {
		iter->stack[head].pos++;
		pos = iter->stack[head].pos;
	}

	return iter->stack[head].node->items + tree->elem_size * ( (pos - 1) / 2 );
}
