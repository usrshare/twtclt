// vim: cin:sts=4:sw=4 

#include <stdlib.h>
#include "btree.h"

struct btree* bt_create() {
    struct btree* new = malloc(sizeof(struct btree));
    new->id = 0;
    new->l = NULL;
    new->r = NULL;
    return new;
}

int bt_insert(struct btree* bt, uint64_t id) {
    if (bt->id == 0) { bt->id = id; return 0; }

    if (id < bt->id) {
	if (bt->l != NULL) return bt_insert(bt->l, id);

	bt->l = malloc(sizeof(struct btree));
	bt->l->id = id; bt->l->l = NULL; bt->l->r = NULL; return 0; } else
	    if (id > bt->id) {
		if (bt->r != NULL) return bt_insert(bt->r, id);

		bt->r = malloc(sizeof(struct btree));
		bt->r->id = id; bt->r->l = NULL; bt->r->r = NULL; return 0; } else
		    if (id == bt->id) return 2;

		abort();
}

int bt_contains(struct btree* bt, uint64_t id) {
    struct btree* i = bt;

    if (i == NULL) return 1;

    if (id == i->id) return 0; else 
	if (id < i->id) return bt_contains(i->l,id); else
	    if (id > i->id) return bt_contains(i->r,id);

    abort();
}

int bt_plug(struct btree* tree, struct btree* branch) {
    if (tree == NULL) return 1;

    if (branch->id < tree->id) {
	if (tree->l != NULL) return bt_plug(tree->l, branch);

	tree->l = branch; return 0;}

    if (branch->id > tree->id) {
	if (tree->r != NULL) return bt_plug(tree->r, branch);

	tree->r = branch; return 0;}

    if (branch->id == tree->id) return 2;
    return 0;
}

int bt_remove(struct btree* bt, uint64_t id) {

    if (bt == NULL) return 1;

    if (id < bt->id) return bt_remove(bt->l,id);
    if (id > bt->id) return bt_remove(bt->r,id);

    int r = rand() % 2;

    if (r) {
	struct btree* branch = bt->l;
	bt->id = bt->r->id;
	bt->l = bt->r->l;
	bt->r = bt->r->r;
	return bt_plug(bt,branch);
    } else {
	struct btree* branch = bt->r;
	bt->id = bt->l->id;
	bt->l = bt->l->l;
	bt->r = bt->l->r;
	return bt_plug(bt,branch);
    }
}

int bt_read(struct btree* bt, btree_cb cb, void* ctx, enum bt_direction dir) {
    if (bt == NULL) return 0;
    switch(dir) {
	case asc:
	    bt_read(bt->l,cb,ctx,dir);
	    cb(bt->id,ctx);
	    bt_read(bt->r,cb,ctx,dir);
	    break;
	case desc:
	    bt_read(bt->r,cb,ctx,dir);
	    cb(bt->id,ctx);
	    bt_read(bt->l,cb,ctx,dir);
	    break;
    }
    return 0;
}
