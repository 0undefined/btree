#include <stdio.h>
#include <stdlib.h>

#include "btree.h"

enum gender {
	gender_male,
	gender_female,
	gender_other
};

struct userdat {
	const char *name;
	unsigned short age;
	enum gender gender;
};

int userdat_cmp(const void *a, const void *b) {
	const struct userdat *ua = a;
	const struct userdat *ub = b;

	if (ua->age == ub->age) {
		     if (ua->gender > ub->gender) { return BTREE_CMP_GT; }
		else if (ua->gender < ub->gender) { return BTREE_CMP_LT; }
		                                  { return BTREE_CMP_EQ; }

	} else if (ua->age > ub->age) {
		return BTREE_CMP_GT;
	} return BTREE_CMP_LT;
}

int main() {
	struct userdat *retval;
	struct btree *tree;
	struct userdat jd = {"John Doe", 42, gender_male};

	tree = btree_new(sizeof(struct userdat),
	                               BTREE_DEGREE_DEFAULT,
	                               &userdat_cmp);


	btree_insert(tree, &jd);

	retval = btree_search(tree, &jd);

	if (retval != NULL) {
		printf("Query: %s, age:%d, %c\n", retval->name, retval->age, (retval->gender == gender_male) ? 'M' : 'F' );
	} else {
		printf("Query: not found\n");
	}

	btree_free(tree);

	return 0;
}
