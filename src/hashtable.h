// vim: cin:sts=4:sw=4 

#include <stdint.h>

#ifndef _HASHTABLE_H_
#define _HASHTABLE_H_

struct ht_item {
	uint64_t key;
	void* value;
	struct ht_item* next;
};

struct hashtable {
	int n;
	struct ht_item** items;
};

struct hashtable* ht_create(unsigned int n);

unsigned int hash(unsigned int n,uint64_t key);

int ht_insert(struct hashtable* ht, uint64_t key, void* value);

void* ht_search(struct hashtable* ht, uint64_t key);

int ht_delete(struct hashtable* ht, uint64_t key);

#endif
