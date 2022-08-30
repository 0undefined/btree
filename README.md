BTree
=====

An array based C-implementation of BTrees attempting a reasonable performance.

_a B-tree is a self-balancing tree data structure that maintains sorted data and
allows searches, sequential access, insertions, and deletions in logarithmic
time_
 -- [Wikipedia](https://en.wikipedia.org/wiki/B-tree)

BTrees are -- algorithm wise -- implemented using pointers to subtrees which is
horribly slow on hardware. This project aims to optimize this, by putting the
whole struct into an array which can be iterated through in-order.

The implementation follows the algorithm described in CLRS.


## Usage

```C
// define a comparator function
int cmp_int(const void *a, const void *b) {
  return *(const int*)a - *(const int*)b;
}
...

// Create a new btree
// 16 is the "degree" of the tree, different sizes might pose better performance
// depending on the data you're storing
struct btree *tree = btree_new(sizeof(int), 16, &cmp_int);

// add values
int a = 42;
int b = 3;
int c = 13;

btree_insert(tree, &a);
btree_insert(tree, &b);
btree_insert(tree, &c);

{
  int searchkey = 42;
  // search returns a pointer to the internal elements, NULL if not found
  int *ret = btree_search(tree, &searchkey);
}

// free up the btree when you're done.
btree_free(&tree);
```

See the respective example branches for more examples.


## Installation

You can run `make lib` to create a shared object file to which you can either
link to and adding the directory of the .so file to your `LD_LIBRARY_PATH`
environment variable, or move into `/usr/lib`.
There's currently not a make target to install it system wide yet.
