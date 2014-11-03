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

// plugs the branch into a tree.
int bt_plug(struct btree* bt, struct btree* branch);

// removes a node from the tree, plugging remaining nodes with bt_plug.
int bt_remove(struct btree* bt, uint64_t id);

// outputs the structure of the tree into the logfile with lprintf
int bt_log(struct btree* bt, int level);

// reads the binary tree in ascending (dir = asc) or descending (dir = desc) order, calling the callback function specified in _cb_ for every element found.
int bt_read(struct btree* bt, btree_cb cb, void* ctx, enum bt_direction dir);

// reads the first (or last) _maxel_ elements from a binary tree. _curel_ is to point to an int containing 0, which will store the current element number being parsed. if _maxel_ is 0, all elements are read (like bt_read).
int bt_read2(struct btree* bt, btree_cb cb, void* ctx, enum bt_direction dir, const int maxel, int* curel);

int bt_test();

#endif
