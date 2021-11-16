#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "btree.h"

struct stats {
	int strength;
	int dexterity;
	int constitution;
	int intelligence;
	int wisdom;
	int charisma;
};

struct player {
	char *name;
	struct stats stats;
};

int str_cmp(const void *a, const void *b) {
	const struct player *pa = a;
	const struct player *pb = b;

	int res = strcmp(pa->name, pb->name);
	return res;
}

void userprint_name(const void *a) {
	const struct player *pa = a;
	printf("%s\n", pa->name);
}

void userprint(const void *a) {
	const struct player *pa = a;
	const struct stats   s  = pa->stats;

	printf("%s:\n"
	       "s:%d\n"
	       "d:%d\n"
	       "c:%d\n"
	       "i:%d\n"
	       "w:%d\n"
	       "c:%d\n",
	       pa->name,
	       s.strength,
	       s.dexterity,
	       s.constitution,
	       s.intelligence,
	       s.wisdom,
	       s.charisma);
}

void userfind(struct btree* tree, const char *name) {
	struct player *retval = NULL;
	struct player tmp = (struct player){name, (struct stats){0,0,0,0,0,0}};
	retval = btree_search(tree, &tmp);

	if (retval != NULL) {
		userprint(retval);
	} else {
		printf("Query: not found\n");
	}
}

const char *names[200] = {
	"Tigger", "Shadow", "Oliver", "Jasper", "Oreo", "Smokey", "Gizmo", "Simba",
	"Charlie", "Tiger", "Jack", "Peanut", "Toby", "Loki", "Harley", "Max", "Milo",
	"Oscar", "Rocky", "Bailey", "Buddy", "George", "Sebastian", "Simon",
	"Boots", "Felix", "Tucker", "Bandit", "Dexter", "Jake", "Romeo", "Snickers",
	"Socks", "Marley", "Blackie", "Chester", "Murphy", "Scooter", "Boo", "Izzy",
	"Rusty", "Dusty", "Noodle", "Panda", "Salem", "Lucky", "Garfield", "Ziggy",
	"Axel", "Sylvester", "Rambo", "Hendrix", "Trooper", "Samson", "Jax", "Echo",
	"Bronson", "Humphrey", "Parker", "Tank", "Wrigley", "Maddox", "Hunter",
	"Dagger", "Hawkeye", "Archer", "Bernard", "Hyde", "Jessejames", "Moose",
	"Chevy", "Bugsy", "Thor", "Brick", "Crash", "Dozer", "Lincoln", "Ozzy",
	"Remington", "Hercules", "Gunner", "Spike", "Bane", "Mrjinx", "Chopper",
	"Tyson", "Shepherd", "Bolt", "Diesel", "Neo", "Brutus", "Hamlet", "Porter",
	"Riggs", "T-bone", "Duke", "Bones", "Gonzo", "Tito", "Ryder", "Bella",
	"Chloe", "Lucy", "Molly", "Kitty", "Luna", "Angel", "Lily", "Baby",
	"Princess", "Sophie", "Missy", "Zoe", "Coco", "Nala", "Sasha", "Kiki",
	"Mittens", "Misty", "Patches", "Pumpkin", "Maggie", "Cali", "Sammy", "Sassy",
	"Phoebe", "Precious", "Daisy", "Fiona", "Lola", "Sadie", "Gracie", "Minnie",
	"Sweetie", "Belle", "Fluffy", "Frankie", "Ginger", "Muffin", "Cleo",
	"Jasmine", "Mimi", "Sugar", "Cupcake", "Alice", "Puddin", "Snowflake",
	"Moon", "Pearl", "Miley", "Lena", "Buttercup", "Sprinkles", "Gidget",
	"Annie", "Kona", "Skittles", "Sunshine", "Trixie", "Winnie", "Rosebud",
	"Pookie", "Sparkle", "Roxy", "Frida", "Pepper", "Juno", "Pebbles", "Elsa",
	"Whiskers", "Paris", "Maya", "Applejack", "Dolly", "Meadow", "Natalie",
	"Pixie", "Willa", "Star", "Emma", "Snokie", "Penelope", "Munchkin",
	"Jellybean", "Honeybee", "Dora", "Melody", "Harper", "Cuddles", "Madonna",
	"Buffy", "Audrey", "Waffles", "Tootsie", "Bindi", "Chichi", "Snowball",
	"Isabella", "Penny", "Lulu"
};

#define _rand(min, max) (min + (rand() % (max + 1)))

int main() {
	const int N = 200;
	struct btree *tree;
	int i;
	srand(0);

	struct player pp[200];
	for (i = 0; i < N; i++) {
		pp[i].name = (char*)names[i];
		pp[i].stats.strength     = _rand(1, 20);
		pp[i].stats.dexterity    = _rand(1, 20);
		pp[i].stats.constitution = _rand(1, 20);
		pp[i].stats.intelligence = _rand(1, 20);
		pp[i].stats.wisdom       = _rand(1, 20);
		pp[i].stats.charisma     = _rand(1, 20);
	}

	tree = btree_new(sizeof(struct player),
	                 4, /*BTREE_DEGREE_DEFAULT / 2,*/
	                 &str_cmp);

	for (i = 0; i < N; i++) {
		btree_insert(tree, &(pp[i]));
	}

	userfind(tree, "Chopper");
	userfind(tree, "Coco");
	userfind(tree, "Oscar");

	btree_print(tree, &userprint_name);
	{
		const char *n = "Oscar";
		struct player tmp = (struct player){n, (struct stats){0,0,0,0,0,0}};
		btree_delete(tree, &tmp);
		userfind(tree, n);
	}

	btree_print(tree, &userprint_name);
	btree_free(tree);

	return 0;
}
