// vim: cin:sts=4:sw=4 

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <curl/curl.h>
#include <time.h>
#include <mojibake.h>
#include <assert.h>
#include "config.h"
#include "log.h"
#include "twitter.h"
#include "hashtable.h"
#include "stringex.h"
#include "twt_time.h"

#define infree(x) if(x) free(x);

const char twt_reqt[] = "https://api.twitter.com/oauth/request_token?oauth_callback=oob"; //request url
const char twt_apin[] = "https://api.twitter.com/oauth/access_token?oauth_verifier="; //access url, append PIN

const char twt_auth[] = "https://api.twitter.com/oauth/authorize"; //authorize url
const char twt_auth2[] = "?oauth_token="; //authorize url part 2

const char app_ckey[] = "EpnGWtYpzGUo6DflEPcQYy3y8"; //consumer key
const char app_csct[] = "6Vue8PKdb6j879pheDHPGPzOrKsVFZ5Eg6DPJ8WbcOADDq8DdV"; //consumer secret here.

const char twt_rest[] = ""; //REST url
const char twt_strm[] = ""; //streaming url

// tweet and user hashtable functions BEGIN

int inithashtables(){
    tweetht = ht_create(1024);
    userht = ht_create(256);

    return 0;
}

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
		return 0;
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

struct t_account* newAccount() {
    struct t_account* na = malloc(sizeof (struct t_account));
    memset(na->name,'\0',sizeof na->name);
    memset(na->tkey,'\0',sizeof na->tkey);
    memset(na->tsct,'\0',sizeof na->tsct);
    na->auth = 0;
    na->timelinebt = bt_create();
    na->userbt = bt_create();
    na->mentionbt = bt_create();
    return na;
}
void destroyAccount(struct t_account* acct){
    if (acct->name) free(acct->name);
    if (acct->tkey) free(acct->tkey);
    if (acct->tsct) free(acct->tsct);
    free(acct);
}

int request_token(struct t_account* acct) {
    // STEP 1 of oauth process. Get a request token for your application.
    char* postarg = NULL;
    char* req_url = NULL;
    char* reply = NULL;
    req_url = oauth_sign_url2(twt_reqt, &postarg, OA_HMAC, NULL, app_ckey, app_csct, NULL, NULL);
    reply = oauth_http_post(req_url,postarg);

    if (req_url) free(req_url);
    if (postarg) free(postarg);

    if (!reply) return 1;

    char** rv = NULL;

    int rc = oauth_split_url_parameters(reply,&rv);

    if (rc != 3) { lprintf("twitter's request_token is supposed to return 3 values.\n"); }

    char *tkey = NULL, *tsct = NULL;

    for (int i=0; i<rc; i++) {

	char* curpar = rv[i];

	if (strncmp(curpar,"oauth_token=",12) == 0) tkey = strdup(curpar+12); //if key found, get key
	if (strncmp(curpar,"oauth_token_secret=",19) == 0) tsct = strdup(curpar+19); //if secret found, get secret

    }

    if ((tkey == NULL) || (tsct == NULL)) {
	lprintf("token key or secret not found in reply\n");
	lprintf("full reply:%s\n",reply);
	free(reply);
	if (tkey) free(tkey);
	if (tsct) free(tsct);

	return 2;
    }

    lprintf("key: %s, secret: %s\n",tkey,tsct);

    strncpy(acct->tkey,tkey,128);
    strncpy(acct->tsct,tsct,128);

    for (int i=0; i<rc; i++) free(rv[i]); //free ALL the strings!

    free (reply);

    return 0;
}
int authorize(struct t_account* acct, char** url) {
    // STEP 2 of oauth process. Return a URL which should be opened in the browser to get the PIN.
    if (acct->auth == 1) {
	lprintf("This account is already authorized.\n");
	return 1;
    }

    char* full_url = malloc(strlen(twt_auth) + strlen(twt_auth2) + strlen(acct->tkey) + 1);

    sprintf(full_url,"%s%s%s",twt_auth,twt_auth2,acct->tkey);

    printf("Please navigate your browser to the following URL: %s\n",full_url);

    if (url != NULL) *url = strdup(full_url);

    free(full_url);

    return 0; //TODO
}
int oauth_verify(struct t_account* acct, int pin) {
    char* verify_url = malloc(strlen(twt_apin) + 7 + 1);
    if ((pin < 0) || (pin > 9999999)) {
	printf("Invalid PIN. PIN is supposed to be a 7-digit number.\n");
	return 1; }
    sprintf(verify_url,"%s%07d",twt_apin,pin);

    char* req_url = NULL;
    char* postarg = NULL;
    char* reply = NULL;

    req_url = oauth_sign_url2(verify_url, &postarg, OA_HMAC, NULL, app_ckey, app_csct, acct->tkey, acct->tsct);
    reply = oauth_http_post(req_url,postarg);

    if (req_url) free(req_url);
    if (postarg) free(postarg);

    if (!reply) return 1;

    char** rv = NULL;

    int rc = oauth_split_url_parameters(reply,&rv);

    if (rc != 3) { lprintf("twitter's request_token is supposed to return 3 values.\n"); }

    char *tkey = NULL, *tsct = NULL, *name = NULL, *uid = NULL;

    for (int i=0; i<rc; i++) {

	char* curpar = rv[i];

	if (strncmp(curpar,"oauth_token=",12) == 0) tkey = strdup(curpar+12); //if key found, get key
	if (strncmp(curpar,"oauth_token_secret=",19) == 0) tsct = strdup(curpar+19); //if secret found, get secret
	if (strncmp(curpar,"user_id=",8) == 0) uid = strdup(curpar+8); //if secret found, get secret
	if (strncmp(curpar,"screen_name=",12) == 0) name = strdup(curpar+12); //if secret found, get secret
    }

    if ((tkey == NULL) || (tsct == NULL)) {
	lprintf("token key or secret not found in reply\n");
	lprintf("full reply:%s\n",reply);
	free(reply);
	if (tkey) free(tkey);
	if (tsct) free(tsct);
	if (uid) free(uid);
	if (name) free(name);
	return 2;
    }

    acct->auth = 1;
    strncpy(acct->tkey,tkey,128);
    strncpy(acct->tsct,tsct,128);
    strncpy(acct->name,name,16);
    if (uid != NULL) acct->userid = strtol(uid,NULL,10);

    free(uid);

    lprintf("logged in as %s (uid %" PRId64 ")\n",name,acct->userid);

    for (int i=0; i<rc; i++) free(rv[i]); //free ALL the strings!

    free (reply);

    return 0;
}

int add_acct(struct t_account* acct) {
    int n = acct_n;
    acctlist = realloc(acctlist,sizeof(struct t_account *) * (n+1));
    acct_n++;
    acctlist[n] = acct;
    return 0;
}
int del_acct(struct t_account* acct) {
    int q = -1;
    for (int i=0; i<acct_n; i++) {
	if (acctlist[i] == acct) q=i;
	if ((q != -1) && (i >= q)) acctlist[i] = ((i+1) < acct_n ? acctlist[i+1] : NULL);
    }
    return 0;
}

int json_object_get_nullbool(json_object* obj) {
    //returns -1 for null, 0 for false and 1 for true.
    if (json_object_get_type(obj) == 0) return -1;
    return json_object_get_boolean(obj);
}

struct t_entity* get_entity(json_object* entity_obj, enum entitytype et) {

    struct json_object_iterator it_c, it_e;
    it_c = json_object_iter_begin(entity_obj);
    it_e = json_object_iter_end(entity_obj);

    struct t_entity entity;

    const char* fn; json_object* fv;// enum json_type ft;

    memset(&entity,'\0',sizeof entity);

    while (!json_object_iter_equal(&it_c,&it_e)) {

	fn = json_object_iter_peek_name(&it_c);
	fv = json_object_iter_peek_value(&it_c);
	//ft = json_object_get_type(fv); //unused

	entity.type = et;

	if ( (s_eq(fn,"text")) || (s_eq(fn,"url")) || (s_eq(fn,"screen_name")) ) {
	    entity.text = strdup(json_object_get_string(fv)); }

	if (s_eq(fn,"indices")) {
	    json_object* indices[2];
	    for (int i=0; i<2; i++) indices[i] = json_object_array_get_idx(fv,i);
	    entity.index_s = json_object_get_int(indices[0]);
	    entity.index_e = json_object_get_int(indices[1]);
	}
	if ( (s_eq(fn,"name")) || (s_eq(fn,"display_url")) ) {
	    entity.name = strdup(json_object_get_string(fv)); }

	if (s_eq(fn,"expanded_url")) {
	    entity.url = strdup(json_object_get_string(fv)); }

	if (s_eq(fn,"id")) entity.id = json_object_get_int64(fv);

	json_object_iter_next(&it_c);

    }

    return entitydup(&entity);
}

char* parse_tweet_entities(struct t_tweet* tweet) {
    //TODO this function is supposed to get a tweet and replace its content with whatever is provided by the entities so that it looks like what you'd see on the Twitter website.

    char* text = strdup("");

    struct t_entity* curent = NULL, *addent = NULL;

    char* append1 = NULL, *append2 = NULL;

    int32_t utfchar = 0;

    int i=0, charents = 0; const uint8_t* iter = (const uint8_t*)(tweet->text); int l = strlen((const char*)iter); int len = 1;

    do {
	len = utf8proc_iterate(iter,l,&utfchar);

	for (int e=0; e < tweet->entity_count; e++) {
	    struct t_entity* ient = tweet->entities[e];
	    if (ient->index_s == i) {
		charents++;
		if (charents == 1) curent = addent = ient; else if (charents >= 2) curent = addent = NULL; //no support for multiple entities.
	    }
	    if (ient->index_e == i) {
		charents--;
		if (charents == 0) curent = addent = NULL;
	    }
	}


	if (curent == NULL) {
	    text = strnrecat(text,(const char*)iter,len);
	}

	if (addent != NULL) {

	    switch(addent->type) {
		case hashtag:
		    append1 = "#"; append2 = addent->text; break;
		case symbol:
		    append1 = "$"; append2 = addent->text; break;
		case mention:
		    append1 = "@"; append2 = addent->text; break;
		case url:
		    append1 = ""; append2 = addent->url; break;
		case media:
		    append1 = ""; append2 = addent->text; break;
		default:
		    append1 = " путин "; append2 = " хуйло "; break;
	    }

	    text = strrecat(text,append1); text = strrecat(text, append2);

	    addent = NULL;
	}

	iter += len;
	l -= len;
	i++;

    } while (len != 0);

    return text;
}


struct t_entity** load_tweet_entities(json_object* entities_obj, int* out_entity_count) {

    struct json_object_iterator it_c, it_e;
    it_c = json_object_iter_begin(entities_obj);
    it_e = json_object_iter_end(entities_obj);

    struct t_entity** entarr = NULL;

    const char* fn; json_object* fv; enum json_type ft; enum entitytype et; int i,n = 0;

    //memset(&nu,'\0',sizeof nu);

    while (!json_object_iter_equal(&it_c,&it_e)) {

	fn = json_object_iter_peek_name(&it_c);
	fv = json_object_iter_peek_value(&it_c);
	ft = json_object_get_type(fv);

	if (ft != json_type_array) lprintf("%s's type is %s instead of array\n", fn, json_type_to_name(ft));

	if (s_eq(fn,"hashtags")) et=hashtag;
	if (s_eq(fn,"symbols")) et=symbol;
	if (s_eq(fn,"urls")) et=url;
	if (s_eq(fn,"user_mentions")) et=mention;
	if (s_eq(fn,"media")) et=media;

	int c = json_object_array_length(fv); 

	n += c; if (n != 0) entarr = realloc(entarr,sizeof(struct t_entity *) * n);

	for (int j=0; j<c; j++) {
	    json_object* entity = json_object_array_get_idx(fv,j);
	    entarr[i] = get_entity(entity,et); i++;
	}

	json_object_iter_next(&it_c);

    }

    *out_entity_count = n; return entarr;

}

// these are helper functions for parse_json_user and parse_json tweet created so that indentation won't break.
int fill_json_user_fields(struct t_user* nu, enum json_type ft, const char* fn, json_object* fv) {
    if (s_eq(fn,"id")) { nu->id = json_object_get_int64(fv); return 0; }
    if (s_eq(fn,"id_str")) { return 0; }
    if (s_eq(fn,"screen_name") && ft) { strncpy(nu->screen_name,json_object_get_string(fv),16); return 0; }
    if (s_eq(fn,"followers_count")) { nu->followers_count = json_object_get_int64(fv); return 0; }
    if (s_eq(fn,"friends_count")) { nu->friends_count = json_object_get_int64(fv); return 0; }
    if (s_eq(fn,"protected")) { nu->user_protected = json_object_get_nullbool(fv); return 0; }
    if (s_eq(fn,"geo_enabled")) { nu->geo_enabled = json_object_get_nullbool(fv); return 0; }
    if (s_eq(fn,"verified")) { nu->verified = json_object_get_nullbool(fv); return 0; }
    if (s_eq(fn,"contributors_enabled")) { nu->contributors_enabled = json_object_get_nullbool(fv); return 0; }
    if (s_eq(fn,"following")) { nu->following = json_object_get_nullbool(fv); return 0; }
    if (s_eq(fn,"follow_request_sent")) { nu->follow_request_sent = json_object_get_nullbool(fv); return 0; }
    if (s_eq(fn,"is_translator")) { nu->is_translator = json_object_get_nullbool(fv); return 0; }
    if (s_eq(fn,"is_translation_enabled")) { return 0; }
    if (s_eq(fn,"favourites_count")) { nu->favorites_count = json_object_get_int64(fv); return 0; }
    if (s_eq(fn,"listed_count")) { nu->listed_count = json_object_get_int64(fv); return 0; }
    if (s_eq(fn,"followers_count")) { nu->followers_count = json_object_get_int64(fv); return 0; }
    if (s_eq(fn,"friends_count")) { nu->friends_count = json_object_get_int64(fv); return 0; }
    if (s_eq(fn,"statuses_count")) { nu->statuses_count = json_object_get_int64(fv); return 0; }
    if (s_eq(fn,"utc_offset")) { nu->utc_offset = json_object_get_int(fv); return 0; }
    if (s_eq(fn,"name")) { nu->name = strdup(json_object_get_string(fv)); return 0; }
    if (s_eq(fn,"created_at")) { nu->created_at = gettweettime(json_object_get_string(fv)); return 0; }
    if (s_eq(fn,"location")) { nu->location = strdup(json_object_get_string(fv)); return 0; }
    if (s_eq(fn,"description")) { nu->description = strdup(json_object_get_string(fv)); return 0; }
    if (s_eq(fn,"url")) { if (fv) nu->url = strdup(json_object_get_string(fv)); return 0; }

    /* if (s_eq(fn,"entities")) {
       int entity_count = 0;
       nu->entities = load_entities(fv,&entity_count);
       nu->entity_count = entity_count; 
       return 0;
       }*/

    if (s_eq(fn,"default_profile")) return 0;
    if (s_eq(fn,"default_profile_image")) return 0;
    if (s_eq(fn,"profile_background_color")) return 0;
    if (s_eq(fn,"profile_background_color")) return 0;
    if (s_eq(fn,"profile_background_image_url")) return 0;
    if (s_eq(fn,"profile_background_image_url_https")) return 0;
    if (s_eq(fn,"profile_background_tile")) return 0;
    if (s_eq(fn,"profile_banner_url")) return 0;
    if (s_eq(fn,"profile_image_url")) return 0;
    if (s_eq(fn,"profile_image_url_https")) return 0;
    if (s_eq(fn,"profile_link_color")) return 0;
    if (s_eq(fn,"profile_sidebar_border_color")) return 0;
    if (s_eq(fn,"profile_sidebar_fill_color")) return 0;
    if (s_eq(fn,"profile_sidebar_text_color")) return 0;
    if (s_eq(fn,"profile_use_background_image")) return 0;
    if (s_eq(fn,"profile_text_color")) {}
    return 1;

}
int fill_json_tweet_fields(struct t_tweet* nt, enum json_type ft, const char* fn, json_object* fv) {
    if (s_eq(fn,"id")) { nt->id = json_object_get_int64(fv); return 0;}
    if (s_eq(fn,"id_str")) { return 0;}
    if (s_eq(fn,"in_reply_to_status_id")) { nt->in_reply_to_status_id = json_object_get_int64(fv); return 0;}
    if (s_eq(fn,"in_reply_to_status_id_str")) { return 0;}
    if (s_eq(fn,"in_reply_to_user_id")) { nt->in_reply_to_user_id = json_object_get_int64(fv); return 0;}
    if (s_eq(fn,"in_reply_to_user_id_str")) { return 0;}
    if (s_eq(fn,"in_reply_to_screen_name")) { if(ft) strncpy(nt->in_reply_to_screen_name,json_object_get_string(fv),16); return 0;}
    if (s_eq(fn,"user")) { nt->user_id = parse_json_user(NULL,fv,1); return 0;}
    if (s_eq(fn,"truncated")) { nt->truncated = json_object_get_boolean(fv); return 0;}
    if (s_eq(fn,"favorited")) { nt->favorited = json_object_get_boolean(fv); return 0;}
    if (s_eq(fn,"retweeted")) { nt->retweeted = json_object_get_boolean(fv); return 0;}
    if (s_eq(fn,"favorite_count")) { nt->favorite_count = json_object_get_int64(fv); return 0;}
    if (s_eq(fn,"retweet_count")) { nt->retweet_count = json_object_get_int64(fv); return 0;}
    if (s_eq(fn,"retweeted_status")) { nt->retweeted_status_id = parse_json_tweet(NULL,fv,1); return 0;}
    if (s_eq(fn,"possibly_sensitive")) { nt->possibly_sensitive = json_object_get_nullbool(fv); return 0;}

    if (s_eq(fn,"text")) { nt->text = strdup(json_object_get_string(fv)); return 0;}
    //if (s_eq(fn,"lang")) { nt->lang = strdup(json_object_get_string(fv)); return 0;}
    if (s_eq(fn,"source")) { nt->source = strdup(json_object_get_string(fv)); return 0;}
    if (s_eq(fn,"created_at")) { nt->created_at = gettweettime(json_object_get_string(fv)); return 0;}

    if (s_eq(fn,"entities")) {
	int entity_count = 0;
	nt->entities = load_tweet_entities(fv,&entity_count);
	nt->entity_count = entity_count;
	return 0;	
    }

    return 1;
}

uint64_t parse_json_user(struct t_account* acct, json_object* user, int perspectival) {
    //returns the user ID of the twitter user. also probably adds the user struct to the hash table or something.
    struct json_object_iterator it_c, it_e;
    it_c = json_object_iter_begin(user);
    it_e = json_object_iter_end(user);

    const char* fn; json_object* fv; enum json_type ft; struct t_user nu;

    memset(&nu,'\0',sizeof nu);

    while (!json_object_iter_equal(&it_c,&it_e)) {

	fn = json_object_iter_peek_name(&it_c);
	fv = json_object_iter_peek_value(&it_c);
	ft = json_object_get_type(fv);

	fill_json_user_fields(&nu,ft,fn,fv);	

	json_object_iter_next(&it_c);

    }

    nu.perspectival = perspectival;
    nu.retrieved_on = time(NULL); 

    //printf("Parsed user %lld.\n",id);

    int r = uht_insert(&nu,no_replace); if (r != 0) lprintf("uht_insert tweet returned %d\n",r);

    return nu.id;
}
uint64_t parse_json_tweet(struct t_account* acct, json_object* tweet, int perspectival) {
    struct json_object_iterator it_c, it_e;
    it_c = json_object_iter_begin(tweet);
    it_e = json_object_iter_end(tweet);

    const char* fn; json_object* fv; enum json_type ft; struct t_tweet nt;

    memset(&nt,'\0',sizeof nt);

    while (!json_object_iter_equal(&it_c,&it_e)) {

	fn = json_object_iter_peek_name(&it_c);
	fv = json_object_iter_peek_value(&it_c);
	ft = json_object_get_type(fv);

	fill_json_tweet_fields(&nt,ft,fn,fv);

	//else printf("unparsed tweet field %s\n", fn);

	json_object_iter_next(&it_c);

    }

    nt.perspectival = perspectival;
    nt.retrieved_on = time(NULL); 


    //printf("Parsed tweet %lld.\n",id);
    int r = tht_insert(&nt,no_replace); if (r != 0) lprintf("tht_insert tweet returned %d\n",r);
    return nt.id;
}

int parse_timeline(struct t_account* acct, enum timelinetype tt, char* timelinereply) {

    struct json_tokener* jt = json_tokener_new();

    enum json_tokener_error jerr;

    struct json_object* timeline = json_tokener_parse_ex(jt,timelinereply,strlen(timelinereply));

    jerr = json_tokener_get_error(jt);
    if (jerr != json_tokener_success) {
	fprintf(stderr,"Error: %s\n",json_tokener_error_desc(jerr));
    }

    //WOOHOO! we have json_object. let's parse it. it's an array, by the way.

    if (json_object_get_type(timeline) != json_type_array) {
	lprintf("Something is wrong. Timeline's JSON isn't an array, but a %s\n",json_type_to_name(json_object_get_type(timeline))); return 1; }

    int tla_len = json_object_array_length(timeline);

    for (int i=0; i<tla_len; i++) {
	json_object* tweet = json_object_array_get_idx(timeline,i);
	uint64_t tweet_id = parse_json_tweet(acct,tweet,0);
	//printf("Adding tweet %lld to timeline...\n",tweet_id);
	bt_insert(acct->timelinebt,tweet_id);
    }

    json_tokener_free(jt);
    return 0;
}

int load_timeline_ext(struct t_account* acct, enum timelinetype tt, int since_id, int max_id, int trim_user, int exclude_replies, int contributor_details, int include_entities) {
    const char twt_home[] = "https://api.twitter.com/1.1/statuses/home_timeline.json";
    char* twtcur = strdup(twt_home);

    if (since_id) addparam_int(twtcur,"since_id",since_id,1);
    if (max_id) addparam_int(twtcur,"max_id",max_id,1);

    if (trim_user) addparam(twtcur,"trim_user","1",1);
    if (exclude_replies) addparam(twtcur,"exclude_replies","1",1);
    if (contributor_details) addparam(twtcur,"contributor_details","1",1);
    if (include_entities) addparam(twtcur,"include_entities","1",1);

    char* req_url = NULL;
    char* reply = NULL;

    //printf("logging in with app_ckey %s, app_csct %s, tkey %s, tsct %s\n",app_ckey, app_csct, acct->tkey, acct->tsct);

    req_url = oauth_sign_url2(twt_home, NULL, OA_HMAC, NULL, app_ckey, app_csct, acct->tkey, acct->tsct);

    reply = oauth_http_get(req_url,NULL);

    if (req_url) free(req_url);

    if (!reply) return 1;

    lprintf("Received a reply.\n");

    parse_timeline(acct, tt, reply);


    free(twtcur);
    return 0;
}

int load_timeline(struct t_account* acct) {
    return load_timeline_ext(acct,0,0,0,0,0,0,0);
}

int save_accounts() {
    FILE* db = cfopen("accounts.db","w"); if ((db == NULL) && (errno != ENOENT)) { perror("fopen"); return 1;}
    for (int i=0; i < acct_n; i++)
	fprintf(db,"%" PRId64 " %s %s %s %d\n",acctlist[i]->userid,acctlist[i]->name,acctlist[i]->tkey,acctlist[i]->tsct,acctlist[i]->auth);

    fflush(db);
    fclose(db);
    return 0;
}
int load_accounts() {
    FILE* db = cfopen("accounts.db","r"); if (db == NULL) { perror("fopen"); if (errno != ENOENT) return 1; else return 0; }
    int userid = 0, auth = 0;
    char name[16], tkey[128], tsct[128];
    while (!feof(db)) {
	int r = fscanf(db,"%d %16s %128s %128s %d\n",&userid,name,tkey,tsct,&auth);
	if (r != 5) { lprintf("%d fields returned instead of 5\n",r); return 1;}
	struct t_account* na = newAccount();
	na->userid = userid; na->auth = auth;

	strncpy(na->name,name,16);
	strncpy(na->tkey,tkey,128);
	strncpy(na->tsct,tsct,128);
	add_acct(na);
    }
    fclose(db);
    return 0;
}
