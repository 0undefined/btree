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
	int id;
	struct userdat *user;
};

int userdat_cmp(const void *a, const void *b) {
	const struct userdat_map *ua = a;
	const struct userdat_map *ub = b;

	if (ua->id == ub->id) return BTREE_CMP_EQ;
	else if (ua->id < ub->id) return BTREE_CMP_LT;

	return BTREE_CMP_GT;
}

void userprint(const void *a) {
	const struct userdat_map *ua = a;
	const struct userdat *u = ua->user;

	printf("[%d] %s, age:%d, %c\n",
			ua->id,
			u->name,
			u->age,
			(u->gender == gender_male) ? 'M' : 'F' );
}

void userfind(struct btree* tree, struct userdat_map *usr) {
	struct userdat_map *retval;
	retval = btree_search(tree, usr);

	if (retval != NULL) {
		struct userdat *retuser = retval->user;
		printf("Query: %s, age:%d, %c\n",
			retuser->name,
			retuser->age,
			(retuser->gender == gender_male) ? 'M' : 'F' );
	} else {
		printf("Query: not found\n");
	}
}

#define USER(_id, varp, var, age, g, name) \
	struct userdat var = {name, age, g};\
	struct userdat_map *varp = malloc(sizeof(struct userdat_map));\
	varp->id=_id;varp->user=&var;


int main() {
	struct btree *tree;
	USER(  1,  ap,  a,  23, gender_male,   "John Doe");
	USER(  2,  bp,  b,  69, gender_male,   "Kim Hot");
	USER(  3,  cp,  c,  88, gender_male,   "Ron Swanson");
	USER(  4,  dp,  d,  12, gender_male,   "Nick");
	USER(  5,  ep,  e,  77, gender_male,   "Jay");
	USER(  6,  fp,  f,  54, gender_male,   "Boris");
	USER(  7,  gp,  g,  42, gender_male,   "Vladimir");
	USER(  8,  hp,  h,  41, gender_male,   "C-not-so-sharp");
	USER(  9,  ip,  i,  38, gender_male,   "Jane Doe");
	USER( 10,  jp,  j,  32, gender_male,   "Thomas pilgaard");
	USER( 11,  kp,  k,   9, gender_male,   "Barrack Obama");
	USER( 12,  lp,  l,  99, gender_male,   "Bin File");
	USER( 13,  mp,  m,  78, gender_male,   "Fiske filletter");
	USER( 14,  np,  n,  71, gender_male,   "Luke skywalker");

	tree = btree_new(sizeof(struct userdat_map),
	                               2, /*BTREE_DEGREE_DEFAULT / 2,*/
	                               &userdat_cmp);

	btree_insert((struct btree*)tree, ap);
	btree_insert((struct btree*)tree, bp);
	btree_insert((struct btree*)tree, cp);
	btree_insert((struct btree*)tree, dp);
	btree_insert((struct btree*)tree, ep);
	btree_insert((struct btree*)tree, fp);
	btree_insert((struct btree*)tree, gp);
	btree_insert((struct btree*)tree, hp);
	btree_insert((struct btree*)tree, ip);
	btree_insert((struct btree*)tree, jp);
	btree_insert((struct btree*)tree, kp);
	btree_insert((struct btree*)tree, lp);
	btree_insert((struct btree*)tree, mp);
	btree_insert((struct btree*)tree, np);

	userfind(tree, ap);
	userfind(tree, ep);
	userfind(tree, np);

	printf("Tree:\n");
	btree_print(tree, &userprint);

	btree_free(tree);

	return 0;
}
