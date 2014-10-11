#include <stdlib.h>
#include "hashtable.h"

struct hashtable* ht_create(unsigned int n) {
	if (n == 0) return NULL;
	struct hashtable* new = malloc(sizeof(struct hashtable));

	new->n = n;
	new->items = malloc(sizeof(struct hashtable_items*) * n);
	for (int i=0; i<n; i++)
		new->items[i] = NULL;

	return new;
}

unsigned int hash(unsigned int n, uint64_t key) {
	return (key % n);
}

int ht_insert(struct hashtable* ht, uint64_t key, void* value) {
	//returns 0 if OK, 1 if error and 2 if item with same key already in table.
	unsigned int h = hash(ht->n,key);

	struct ht_item* new = malloc(sizeof(struct ht_item));
	if (new == NULL) return 1;
	new->key = key;
	new->value = value;
	new->next = NULL;

	struct ht_item* hi = ht->items[h];

	if (hi == NULL) {
		ht->items[h] = new; return 0; }

	if (hi->key == key) {free(new); return 2; }

	while (hi->next != NULL) {
		hi = hi->next;
		if (hi->key == key) {free(new); return 2;}
	}

	hi->next = new;
}

void* ht_search(struct hashtable* ht, uint64_t key) {
	unsigned int h = hash(ht->n,key);

	struct ht_item* i = ht->items[h];

	while (i != NULL) {
		if (i->key == key) return (i->value);
		i = i->next;
	}

	return NULL;
}

int ht_delete(struct hashtable* ht, uint64_t key) {
	//returns 0 if OK, 1 if error and 2 if item was not in table.
	unsigned int h = hash(ht->n,key);

	struct ht_item* i = ht->items[h];

	if (i == NULL) return 2;

	if (i->key == key) {free(i); ht->items[h] = NULL; return 0;}

	while (i->next != NULL) {
		if (i->next->key == key) {
			if (i->next->next != NULL) {
				free(i->next);
				i->next = i->next->next;
			} else {free(i->next); i->next = NULL; }
		}
		i = i->next;
	}
	
	return 2;
}
