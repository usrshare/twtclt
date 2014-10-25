// vim: cin:sts=4:sw=4 

#include <stdint.h>

#ifndef _HASHTABLE_H_
#define _HASHTABLE_H_

enum ht_returncodes {
    HT_SUCCESS = 0,
    HT_ERROR = 1,
    HT_ALREADYEXISTS = 2,
    HT_NOTFOUND = 3,
};

struct ht_item {
	uint64_t key;
	void* value;
	struct ht_item* next;
};

struct hashtable {
	int n;
	int unique; //all items must have unique keys
	struct ht_item** items;
};

unsigned int hash(unsigned int n,uint64_t key);

struct hashtable* ht_create2(unsigned int n, int unique);
struct hashtable* ht_create(unsigned int n);

int ht_insert(struct hashtable* ht, uint64_t key, void* value);

void* ht_search(struct hashtable* ht, uint64_t key);
void* ht_search_ind(struct hashtable* ht, uint64_t key, int index);

int ht_delete(struct hashtable* ht, uint64_t key);
int ht_delete_ind(struct hashtable* ht, uint64_t key, int index);

#endif
