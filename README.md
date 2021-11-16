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


## Installation

todo


## Usage

todo


# Progress

## Naive

* [x] Searching
* [x] Insertion
* [x] Deletion


## Flat-sequential

* [ ] Searching
* [ ] Insertion
* [ ] Deletion


## Parallel

* [ ] Searching
* [ ] Insertion
* [ ] Deletion
