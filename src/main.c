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

struct userdat_map {
	unsigned id;
	struct userdat *user;
};

int userdat_cmp(const void *a, const void *b) {
	const struct userdat_map *ua = a;
	const struct userdat_map *ub = b;

	if (ua->id == ub->id) return BTREE_CMP_EQ;
	else if (ua->id < ub->id) return BTREE_CMP_LT;

	return BTREE_CMP_GT;
}

int main() {
	struct userdat *retuser;
	struct userdat_map *retval;
	struct btree *tree;
	struct userdat a = {"John Doe", 42, gender_male};
	struct userdat b = {"Michael jordan", 80, gender_male};
	struct userdat c = {"Jennifer Lawrence", 25, gender_female};
	struct userdat d = {"Taylor Switfy", 25, gender_female};
	struct userdat e = {"John Johnson", 54, gender_male};
	struct userdat f = {"Eddison", 130, gender_male};
	struct userdat g = {"Nicolas Tesla", 115, gender_male};
	struct userdat h = {"Bjarne Stroustrup", 83, gender_male};

	struct userdat_map ap = {0, &a};
	struct userdat_map bp = {1, &b};
	struct userdat_map cp = {2, &c};
	struct userdat_map dp = {3, &d};
	struct userdat_map ep = {4, &e};
	struct userdat_map fp = {5, &f};
	struct userdat_map gp = {6, &g};
	struct userdat_map hp = {7, &h};

	tree = btree_new(sizeof(struct userdat_map),
	                               BTREE_DEGREE_DEFAULT,
	                               &userdat_cmp);

	btree_insert(tree, &ap);
	btree_insert(tree, &bp);
	btree_insert(tree, &cp);
	btree_insert(tree, &dp);

	btree_insert(tree, &ep);
	btree_insert(tree, &fp);
	btree_insert(tree, &gp);
	btree_insert(tree, &hp);

	retval = btree_search(tree, &fp);

	if (retval != NULL) {
		retuser = retval->user;
		printf("Query: %s, age:%d, %c\n",
			retuser->name,
			retuser->age,
			(retuser->gender == gender_male) ? 'M' : 'F' );
	} else {
		printf("Query: not found\n");
	}

	btree_free(tree);

	return 0;
}
