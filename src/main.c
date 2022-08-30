#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "btree.h"

int cmp(const void *a, const void *b) {
	const char* ap = a;
	const char* bp = b;
	return (char)*ap - (char)*bp;
}

void print(const void *a) {
	const char* ap = a;
	printf("'%c'\n", (char)*ap);
}

void userfind(struct btree* tree, char a) {
	char *retval;
	retval = btree_search(tree, &a);

	if (retval != NULL) {
		printf("Query: %c found\n", a);
	} else {
		printf("Query: %c not found\n", a);
	}
}

void test_case(struct btree *tree, char num, char *case_num, char elem) {
	printf("\x1b[1;33m(%c) case %s: \x1b[3;33m%c\x1b[0;1;33m deleted\x1b[0m\n",
			num,
			case_num,
			elem);
	btree_delete(tree, &elem);
	btree_print(tree, &print);
}

int main() {
	struct btree *tree = btree_new(sizeof(char), 3, &cmp);


	btree_insert(tree, &"A");
	btree_insert(tree, &"B");
	btree_insert(tree, &"D");
	btree_insert(tree, &"E");
	btree_insert(tree, &"C");

	btree_insert(tree, &"J");
	btree_insert(tree, &"K");
	btree_insert(tree, &"G");
	btree_insert(tree, &"F");

	btree_insert(tree, &"P");

	btree_insert(tree, &"T");
	btree_insert(tree, &"X");
	btree_insert(tree, &"Q");
	btree_insert(tree, &"R");

	btree_insert(tree, &"U");
	btree_insert(tree, &"S");

	btree_insert(tree, &"V");

	btree_insert(tree, &"Y");
	btree_insert(tree, &"Z");
	/* We need to force it a little in order to make it 1:1 to CLRS deletion
	 * example. We need to delete 'a' and 'b' before proceeding. */
	btree_insert(tree, &"a");
	btree_insert(tree, &"b");

	btree_delete(tree, &"a");
	btree_delete(tree, &"b");

	/* Lastly, */
	btree_insert(tree, &"N");
	btree_insert(tree, &"O");
	btree_insert(tree, &"M");
	btree_insert(tree, &"L");


	printf("\x1b[1;33m(a) Initial tree\x1b[0m\n");
	btree_print(tree, &print);

	test_case(tree, 'b', "1", 'F');
	test_case(tree, 'c', "2a", 'M');
	test_case(tree, 'd', "2c", 'G');
	test_case(tree, 'e', "3b", 'D');
	test_case(tree, 'f', "3a", 'B');


	btree_free(&tree);

	return 0;
}
