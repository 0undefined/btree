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

#define ADD(v) {int a = v;  btree_insert(tree, &a);}
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

	btree_print(tree, &print_int);
	printf("\niter:\n");

	{
		struct btree_iter_t *it = btree_iter_t_new(tree);
		int *a    = NULL;
		int limit = 0;

		while ((a = btree_iter(tree, it)) != NULL && limit++ < 35) {
			print_int(a);
		}

	}

	return 0;
}
