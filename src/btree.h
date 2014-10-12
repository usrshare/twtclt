#include <stdint.h>

#ifndef _BTREE_H_
#define _BTREE_H_

//this btree specifically contains tweet and user IDs. That's why it holds uint64_t.

struct btree{
	uint64_t id;
	struct btree* l;
	struct btree* r;
};

struct btree* bt_create();

int bt_insert(struct btree* bt, uint64_t id);

int bt_contains(struct btree* bt, uint64_t id);

int bt_plug(struct btree* bt, struct btree* branch);

int bt_remove(struct btree* bt, uint64_t id);

#endif
