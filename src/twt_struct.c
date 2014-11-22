// vim: cin:sts=4:sw=4 
#include <malloc.h>
#include <string.h>

#include "twt_struct.h"
#include "hashtable.h"

int tht_insert(struct t_tweet* tweet, enum collision_behavior cbeh){
    struct t_tweet* new = tweetdup(tweet);

    int r = ht_insert(tweetht,tweet->id,new);

    if (r == 2) {

	struct t_tweet* old = tht_search(tweet->id);

	switch(cbeh) {
	    case no_replace:
		free(old);
		return 2;
		break;
	    case update:
		if ((tweet->perspectival >= old->perspectival) && (tweet->retrieved_on < old->retrieved_on)) {free(old); return 2;}
	    case replace:
		r = tht_delete(tweet->id);
		if (r != 0) {free(old); return r;}
		free(old);
		r = ht_insert(tweetht,tweet->id,new);
		if (r != 0) {free(old); return r;}
		return 0;
		break;
	}
    }

    if (r != 0) {free(new); return r;}
    return 0;
}
int tht_delete(uint64_t id){
    struct t_tweet* old = (struct t_tweet*)ht_search(tweetht,id);
    if (old != NULL) tweetdel(old);
    return ht_delete(tweetht,id);

}
struct t_tweet* tht_search(uint64_t id){
    return tweetdup((struct t_tweet*)ht_search(tweetht,id));
}

int urt_insert(char* screen_name, uint64_t id, enum collision_behavior cbeh){

    char* nsn = strdup(screen_name);

    uint64_t* uid = malloc(sizeof(uint64_t));
    *uid = id;

    int r = ht_insert_a(urefht,nsn,uid);

    if (r == 2) {

	uint64_t* old = urt_search_ptr(nsn);

	switch(cbeh) {
	    case no_replace:
		free(nsn);
		return 2;
		break;
	    case update:
		if (*old == id) { free(nsn); break; }
	    case replace:
		r = urt_delete(nsn);
		if (r != 0) {free(nsn); return r;}
		r = ht_insert_a(urefht,nsn,uid);
		if (r != 0) {free(nsn); return r;}
		break;
	}
    }

    if (r != 0) {free(uid); free(nsn); return r;}
    return 0;

}
int urt_delete(char* screen_name){
    uint64_t* old = (uint64_t*)ht_search_a(urefht,screen_name);
    if (old != NULL) free(old);
    return ht_delete_a(urefht,screen_name);

}
uint64_t* urt_search_ptr(char* screen_name){
    void* r = ht_search_a(urefht,screen_name);
    return (uint64_t*)r;
}

uint64_t urt_search(char* screen_name){
    uint64_t r = *(urt_search_ptr(screen_name));
    return r;
}


int uht_insert(struct t_user* user, enum collision_behavior cbeh){
    struct t_user* new = userdup(user);

    int r = ht_insert(userht,user->id,new);

    if (r == 2) {

	struct t_user* old = uht_search(user->id);

	switch(cbeh) {
	    case no_replace:
		free(old);
		return 2;
		break;
	    case update:
		if ((user->perspectival >= old->perspectival) && (user->retrieved_on < old->retrieved_on)) {free(old); return 2;}
	    case replace:
		r = uht_delete(user->id);
		if (r != 0) {free(old); return r;}
		free(old);
		r = ht_insert(userht,user->id,new);
		if (r != 0) {free(old); return r;}
		break;
	}
    }

    if (r != 0) {free(new); return r;}
    return 0;

}
int uht_delete(uint64_t id){
    struct t_user* old = (struct t_user*)ht_search(userht,id);
    if (old != NULL) userdel(old);
    return ht_delete(userht,id);

}
struct t_user* uht_search(uint64_t id){
    return userdup((struct t_user*)ht_search(userht,id));
}

// tweet and user hashtable functions END
struct t_entity* entitydup(struct t_entity* orig) {
    if (orig == NULL) return NULL;
    struct t_entity* new = malloc(sizeof(struct t_entity));
    *new = *orig; //copy all values
    if (orig->text) new->text = strdup(orig->text);
    if (orig->name) new->name = strdup(orig->name);
    if (orig->url) new->url = strdup(orig->url);
    return new;
}
struct t_tweet* tweetdup(struct t_tweet* orig) {
    if (orig == NULL) return NULL;
    struct t_tweet* new = malloc(sizeof(struct t_tweet));
    *new = *orig; //copy all values
    if (orig->text) new->text = strdup(orig->text);
    if (orig->source) new->source = strdup(orig->source);
    //if (orig->lang) new->lang = strdup(orig->lang);
    if (orig->entities != NULL) {
	new->entities = malloc(sizeof(struct t_entity*) * orig->entity_count);
	for (int i=0; i<orig->entity_count; i++)
	    new->entities[i] = entitydup(orig->entities[i]); }
    return new;
}
struct t_user* userdup(struct t_user* orig) {
    if (orig == NULL) return NULL;
    struct t_user* new = malloc(sizeof(struct t_user));
    *new = *orig; //copy all values
    if (orig->description) new->description = strdup(orig->description);
    if (orig->location) new->location = strdup(orig->location);
    //if (orig->lang) new->lang = strdup(orig->lang);
    if (orig->name) new->name = strdup(orig->name);
    if (orig->time_zone) new->time_zone = strdup(orig->time_zone);
    if (orig->url) new->url = strdup(orig->url);
    if (orig->withheld_in_countries) new->withheld_in_countries = strdup(orig->withheld_in_countries);
    return new;
}

void entitydel(struct t_entity* ptr) {
    if (ptr->text != NULL) free(ptr->text);
    if (ptr->name != NULL) free(ptr->name);
    if (ptr->url != NULL) free(ptr->url);
    free(ptr);
}

void tweetdel(struct t_tweet* ptr) {
    free(ptr->text);
    free(ptr->source);
    //free(ptr->lang);
    if (ptr->entities != NULL) {
	for (int i=0; i<ptr->entity_count; i++)
	    entitydel(ptr->entities[i]);
	free(ptr->entities); }
    free(ptr);
}
void userdel(struct t_user* ptr) {
    free(ptr->description);
    free(ptr->location);
    //free(ptr->lang);
    free(ptr->name);
    free(ptr->time_zone);
    free(ptr->url);
    free(ptr->withheld_in_countries);
    if (ptr->desc_entities != NULL) {
	for (int i=0; i<ptr->desc_entity_count; i++)
	    entitydel(ptr->desc_entities[i]);
	free(ptr->desc_entities); }
    if (ptr->url_entities != NULL) {
	for (int i=0; i<ptr->url_entity_count; i++)
	    entitydel(ptr->url_entities[i]);
	free(ptr->url_entities); }
    free(ptr);
}


