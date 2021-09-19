#ifndef BTREE_H
#define BTREE_H

#include <stddef.h>

#define BTREE_DEGREE_DEFAULT 4

#define BTREE_SIZE_MIN 8
#define BTREE_SIZE_MAX 4096

#define BTREE_CMP_LT        ( -1 )
#define BTREE_CMP_EQ        (  0 )
#define BTREE_CMP_GT        (  1 )

struct btree;

/* elem_size: the size of the elements, typically `sizeof(struct <your struct>)`
 * t: degree of the btree, if you're in doubt, use `BTREE_SIZE_DEFAULT`
 * cmp: comparison function, in order to support any operations on the tree.
 *
 * This function just calls `btree_new_with_allocator` with `free` and `malloc`
 * as initializers.
 */
struct btree* btree_new(size_t elem_size,
                        size_t t,
                        int    (*cmp)(const void *a, const void *b));

/* Same as `btree_new`, except that it actually initializes a btree, but with
 * the given allocators.
 */
struct btree* btree_new_with_allocator(
                        size_t elem_size,
                        size_t t,
                        int    (*cmp)(const void *a, const void *b),
                        void  *(*alloc)(size_t),
                        void   (*dealloc)(void*));

void btree_free(struct btree *btree);

void* btree_search(struct btree *btree, void *elem);
void* btree_insert(struct btree *btree, void *elem);
void* btree_delete(struct btree *btree, void *elem);
void* btree_update(struct btree *btree, void *elem_key, void *elem);

#endif
