#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "btree.h"

int cmp_int(const void *a, const void *b) {
	return *(const int*)a - *(const int*)b;
}

void print_int(const void *a) {
	printf("%d\n", *(const int*)a);
}

int main() {
	struct btree *tree = btree_new(sizeof(int), 3, &cmp_int);
	int counter = 0;

#define ADD(v) {int _ = v;  btree_insert(tree, &_);}
	ADD(1);
	ADD(2);
	ADD(3);
	ADD(4);
	ADD(5);
	ADD(6);
	ADD(7);
	ADD(8);
	ADD(9);
	ADD(10);
	ADD(11);
	ADD(12);
	ADD(13);
	ADD(14);
	ADD(15);
	ADD(16);
	ADD(17);
	ADD(18);
	ADD(19);
	ADD(20);
	ADD(21);
	ADD(22);
	ADD(23);
#undef ADD

#ifdef DEBUG
	btree_print(tree, &print_int);
	printf("\niter:\n");
#endif

	{
		struct btree_iter_t *it = btree_iter_t_new(tree);
		int *a = NULL;


#ifdef DEBUG
		printf("Size: %lu\n\n", btree_size(tree));
#endif

		while ((a = btree_iter(tree, it)) != NULL && counter++ < 35) {
#ifdef DEBUG
			print_int(a);
#endif
		}

#ifdef DEBUG
		btree_print(tree, &print_int);
		printf("Size: %lu\n\n", btree_size(tree));
#endif

		{
			int _x = 24;
			int _y = 25;
			int _z = 26;
			btree_insert(tree, &_x);
			btree_insert(tree, &_y);
			btree_insert(tree, &_z);
		}

		btree_iter_t_reset(tree, &it);

		while ((a = btree_iter(tree, it)) != NULL && counter++ < 55) {
#ifdef DEBUG
			print_int(a);
#endif
		}
		free(it);
	}

	btree_free(&tree);

	return !(counter == 49);
}
