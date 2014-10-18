// vim: cin:sts=4:sw=4 

#include <stdint.h>

#ifndef _BTREE_H_
#define _BTREE_H_

//this btree specifically contains tweet and user IDs. That's why it holds uint64_t.

struct btree{
	uint64_t id;
	struct btree* l;
	struct btree* r;
};

enum bt_direction {
	asc,
	desc,
};

typedef void (*btree_cb) (uint64_t id, void* ctx);

struct btree* bt_create();

int bt_insert(struct btree* bt, uint64_t id);

int bt_contains(struct btree* bt, uint64_t id);

int bt_plug(struct btree* bt, struct btree* branch);

int bt_remove(struct btree* bt, uint64_t id);

int bt_read(struct btree* bt, btree_cb cb, void* ctx, enum bt_direction dir);

#endif
