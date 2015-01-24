// vim: cin:sts=4:sw=4 

#include <stdlib.h>
#include <inttypes.h>
#include "log.h"

#include "btree.h"

struct btree* bt_fill(uint64_t id) {
    struct btree* new = malloc(sizeof(struct btree));
    new->id = id;
    new->l = NULL;
    new->r = NULL;
    return new;
}

struct btree* bt_create() {
    struct btree* new = malloc(sizeof(struct btree));
    new->id = 0;
    new->l = NULL;
    new->r = NULL;
    return new;
}

int bt_insert(struct btree* bt, uint64_t id) {
    if (bt->id == 0) { bt->id = id; bt->l = NULL; bt->r = NULL; return 0; }

    if (id < bt->id) {
	if (bt->l != NULL) return bt_insert(bt->l, id);

	bt->l = bt_fill(id); return 0; }

    if (id > bt->id) {
	if (bt->r != NULL) return bt_insert(bt->r, id);

	bt->r = bt_fill(id); return 0; }

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

int _bt_remove(struct btree* bt, uint64_t id, struct btree* parent) {

    if (bt == NULL) return 1;

    if ((bt->l != NULL) && (bt->l->id == id)) {
    }

    if (id < bt->id) return _bt_remove(bt->l,id,bt);
    if (id > bt->id) return _bt_remove(bt->r,id,bt);

    int r;

    if ((bt->l) && (bt->r)) r = rand() % 2; else
	if (bt->l) r = 0; else if (bt->r) r=1; else r=-1;

    struct btree* bl = bt->l; //left branch
    struct btree* br = bt->r; //right branch

    switch (r) {
	case 0:
	    // left branch exists.
	    bt->id = bl->id;
	    bt->l  = bl->l;
	    bt->r  = bl->r;
	    free(bl);
	    if (br) { return bt_plug(bt,br); } else return 0;
	    break;
	case 1:
	    // right branch exists.
	    bt->id = br->id;
	    bt->l  = br->l;
	    bt->r  = br->r;
	    free(br);
	    if (bl) { return bt_plug(bt,bl); } else return 0;
	    break;
	case -1:
	    // none of the branches exist.

	    if (parent == NULL) {bt->id = 0; bt->l = NULL; bt->r = NULL; return 0;}

	    if (parent->l == bt) parent->l = NULL; else
		if (parent->r == bt) parent->r = NULL;

	    free(bt);
	    return 0;
	    break;
    }
    return 1;
}

int bt_remove(struct btree* bt, uint64_t id) {
    return _bt_remove(bt,id,NULL);
}


int bt_read2(struct btree* bt, btree_cb cb, void* ctx, enum bt_direction dir, const int maxel, int* curel) {

    //lprintf("maxel = %d, curel = %p, *curel = %d\n",maxel,curel,*curel);

    if (bt == NULL) return 0;


    switch(dir) {
	case asc:
	    bt_read2(bt->l,cb,ctx,dir,maxel,curel);
	    if ( (maxel != 0) && ((*curel) >= maxel) ) return 1;
	    cb(bt->id,ctx);
	    if (maxel != 0) (*curel)++;
	    bt_read2(bt->r,cb,ctx,dir,maxel,curel);
	    break;
	case desc:
	    bt_read2(bt->r,cb,ctx,dir,maxel,curel);
	    if ( (maxel != 0) && ((*curel) >= maxel) ) return 1;
	    cb(bt->id,ctx);
	    if (maxel != 0) (*curel)++;
	    bt_read2(bt->l,cb,ctx,dir,maxel,curel);
	    break;
    }

    return 0;
}

int bt_log(struct btree* bt, int level) {

    if (bt == NULL) return 0;

    for (int i=0; i<level; i++) lprintf(" ");

    lprintf ("[%" PRId64 "]\n", bt->id);

    if (bt->l) bt_log(bt->l,level+1);
    if (bt->r) bt_log(bt->r,level+1);


    return 0;
}


int bt_read(struct btree* bt, btree_cb cb, void* ctx, enum bt_direction dir) {
    return bt_read2(bt,cb,ctx,dir,0,NULL);
}

void bt_test_cb(uint64_t id, void* ctx) {
    lprintf("%3" PRId64 ,id);
}

int bt_test() {

    //int t = time(NULL); srand((int)t);

    struct btree* tbt = bt_create();

    int n = 0;

    while (n < 20) {

	int q = (rand() % 20) + 1;
	if ( bt_insert(tbt,q) == 0) n++;
    }

    bt_log(tbt,0);
    
    int curel = 0;
    bt_read(tbt,bt_test_cb,&curel,asc);
    lprintf("\n");

    bt_remove(tbt,11);

    lprintf("Removed element 11.\n");

    bt_log(tbt,0);
    bt_read(tbt,bt_test_cb,&curel,asc);
    lprintf("\n");
    lprintf("\n");

    return 0;
}
