// vim: cin:sts=4:sw=4 

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <curl/curl.h>
#include <time.h>
#include <mojibake.h>
#include <assert.h>
#include <pthread.h>

#include "config.h"
#include "log.h"
#include "twt_oauth.h"
#include "twitter.h"
#include "stringex.h"
#include "twt_time.h"

#define infree(x) if(x) free(x);

const char twt_rest[] = ""; //REST url

const char twt_hometl_url[] = "https://api.twitter.com/1.1/statuses/home_timeline.json";
const char twt_usertl_url[] = "https://api.twitter.com/1.1/statuses/user_timeline.json";
const char twt_menttl_url[] = "https://api.twitter.com/1.1/statuses/mentions_timeline.json";
const char twt_search_url[] = "https://api.twitter.com/1.1/search/tweets.json";
const char twt_dirmsg_url[] = "https://api.twitter.com/1.1/direct_messages.json";
const char twt_status_url[] = "https://api.twitter.com/1.1/statuses/show.json"; //load single tweet
const char twt_user_url[] =   "https://api.twitter.com/1.1/users/show.json"; //load single user
const char twt_usrstr_url[] = "https://userstream.twitter.com/1.1/user.json"; //user streams

// tweet and user hashtable functions BEGIN
int initcurl() {
    curl_global_init(CURL_GLOBAL_ALL);
    return 0;
}

int inithashtables(){
    tweetht = ht_create(1024);
    userht = ht_create(512);
    urefht = ht_create(512);

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
		    append1 = ""; append2 = addent->name; break;
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
    if (fv == NULL) return 1;
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
    if (fv == NULL) return 1;
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

    nu.acct = acct;

    nu.perspectival = perspectival;
    nu.retrieved_on = time(NULL); 

    //printf("Parsed user %lld.\n",id);

    int r = uht_insert(&nu,no_replace); if (r != 0) lprintf("uht_insert user returned %d\n",r);

    r = urt_insert(nu.screen_name, nu.id, replace);
    if (r != 0) lprintf("urt_insert screen name returned %d\n",r);

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

    nt.acct = acct;

    nt.perspectival = perspectival;
    nt.retrieved_on = time(NULL); 


    //printf("Parsed tweet %lld.\n",id);
    int r = tht_insert(&nt,no_replace); if (r != 0) lprintf("tht_insert tweet returned %d\n",r);
    return nt.id;
}
int parse_timeline(struct btree* timeline, struct t_account* acct, enum timelinetype tt, char* timelinereply) {

    struct json_tokener* jt = json_tokener_new();

    enum json_tokener_error jerr;

    struct json_object* timelineobj = json_tokener_parse_ex(jt,timelinereply,strlen(timelinereply));

    jerr = json_tokener_get_error(jt);
    if (jerr != json_tokener_success) {
	lprintf("JSON Tokener Error: %s\n",json_tokener_error_desc(jerr));
    }

    //WOOHOO! we have json_object. let's parse it. it's an array, by the way.

    if (json_object_get_type(timelineobj) != json_type_array) {
	lprintf("Something is wrong. Timeline's JSON isn't an array, but a %s\n",json_type_to_name(json_object_get_type(timelineobj)));
	lprintf("full reply:%s\n",timelinereply);
	return 1; }

    int tla_len = json_object_array_length(timelineobj);

    for (int i=0; i<tla_len; i++) {
	json_object* tweet = json_object_array_get_idx(timelineobj,i);
	uint64_t tweet_id = parse_json_tweet(acct,tweet,0);
	//printf("Adding tweet %lld to timeline...\n",tweet_id);
	bt_insert(timeline,tweet_id);
    }

    json_tokener_free(jt);
    return 0;
}

uint64_t parse_single_tweet(struct t_account* acct, char* timelinereply) {

    struct json_tokener* jt = json_tokener_new();
    enum json_tokener_error jerr;

    struct json_object* tweet = json_tokener_parse_ex(jt,timelinereply,strlen(timelinereply));
    jerr = json_tokener_get_error(jt);
    if (jerr != json_tokener_success) {
	lprintf("JSON Tokener Error: %s\n",json_tokener_error_desc(jerr));
    }

    if (json_object_get_type(tweet) != json_type_object) {
	lprintf("Something is wrong. Timeline's JSON isn't an object, but a %s\n",json_type_to_name(json_object_get_type(tweet)));
	lprintf("full reply:%s\n",timelinereply);
	return 1; }

    uint64_t tweet_id = parse_json_tweet(acct,tweet,0);
    json_tokener_free(jt);
    return tweet_id;
}

uint64_t parse_single_user(struct t_account* acct, char* timelinereply) {

    struct json_tokener* jt = json_tokener_new();
    enum json_tokener_error jerr;

    struct json_object* user = json_tokener_parse_ex(jt,timelinereply,strlen(timelinereply));
    jerr = json_tokener_get_error(jt);
    if (jerr != json_tokener_success) {
	lprintf("JSON Tokener Error: %s\n",json_tokener_error_desc(jerr));
    }

    if (json_object_get_type(user) != json_type_object) {
	lprintf("Something is wrong. Timeline's JSON isn't an object, but a %s\n",json_type_to_name(json_object_get_type(user)));
	lprintf("full reply:%s\n",timelinereply);
	return 1; }

    uint64_t user_id = parse_json_user(acct,user,0);
    json_tokener_free(jt);
    return user_id;
}

int load_timeline_ext(struct btree* timeline, struct t_account* acct, enum timelinetype tt, uint64_t userid, char* customtype, int since_id, int max_id, int count, int trim_user, int exclude_replies, int contributor_details, int include_entities) {

    char *baseurl = NULL;

    switch(tt) {
	case home:
	    baseurl = strdup(twt_hometl_url);
	    break;
	case user:
	    baseurl = strdup(twt_usertl_url);
	    if (userid != 0) {
		baseurl = addparam_int(baseurl,"user_id",userid,1);
	    } else {
		if (customtype != NULL) baseurl = addparam(baseurl,"screen_name",customtype,1);
	    }
	    break;
	case mentions:
	    baseurl = strdup(twt_menttl_url);
	    break;
	case direct_messages:
	    baseurl = strdup(twt_dirmsg_url);
	    break;
	case search:
	    baseurl = strdup(twt_dirmsg_url);
	    if (customtype != NULL) baseurl = addparam(baseurl,"q",customtype,1);
	    break;
    }

    if (since_id) baseurl = addparam_int(baseurl,"since_id",since_id,1);
    if (max_id) baseurl = addparam_int(baseurl,"max_id",max_id,1);
    if (count) baseurl = addparam_int(baseurl,"count",max_id,1);

    if (trim_user) baseurl = addparam(baseurl,"trim_user","1",1);
    if (exclude_replies) baseurl = addparam(baseurl,"exclude_replies","1",1);
    if (contributor_details) baseurl = addparam(baseurl,"contributor_details","1",1);
    if (include_entities) baseurl = addparam(baseurl,"include_entities","1",1);

    char* req_url = NULL;
    char* reply = NULL;

    //printf("logging in with app_ckey %s, app_csct %s, tkey %s, tsct %s\n",app_ckey, app_csct, acct->tkey, acct->tsct);

    req_url = acct_sign_url2(baseurl, NULL, OA_HMAC, NULL, acct);

    reply = oauth_http_get(req_url,NULL);

    if (req_url) free(req_url);

    if (!reply) return 1;

    lprintf("Received a reply.\n");

    parse_timeline(timeline, acct, tt, reply);

    free(reply);
    free(baseurl);
    return 0;
}
int load_timeline(struct btree* timeline, struct t_account* acct, enum timelinetype tt, uint64_t userid, char* customtype) {
    return load_timeline_ext(timeline,acct,tt,userid,customtype,0,0,0,0,0,0,0);
}
int load_global_timeline(struct btree* timeline, enum timelinetype tt, uint64_t userid, char* customtype) {
    for (int i=0; i < acct_n; i++) {
	if (acctlist[i]->show_in_timeline)
	    load_timeline(timeline,acctlist[i],tt,userid,customtype);
    }
    return 0;    
}

uint64_t load_tweet(struct t_account* acct, uint64_t tweetid) {

    acct = def_acct(acct);

    char *baseurl = strdup(twt_status_url);
    baseurl = addparam_int(baseurl,"id",tweetid,1);

    char* req_url = acct_sign_url2(baseurl, NULL, OA_HMAC, NULL, acct);
    char* reply = oauth_http_get(req_url,NULL);
    if (req_url) free(req_url);

    if (!reply) return 1;

    lprintf("Received a reply.\n");

    uint64_t resid = parse_single_tweet(acct,reply);

    free(reply);
    free(baseurl);
    return resid;
}
uint64_t load_user(struct t_account* acct, uint64_t userid, char* username) {
    
    acct = def_acct(acct);

    char *baseurl = strdup(twt_status_url);

    if (username != NULL)
	baseurl = addparam(baseurl,"screen_name",username,1); else
	    baseurl = addparam_int(baseurl,"id",userid,1);

    char* req_url = acct_sign_url2(baseurl, NULL, OA_HMAC, NULL, acct);
    char* reply = oauth_http_get(req_url,NULL);
    if (req_url) free(req_url);

    if (!reply) return 1;

    lprintf("Received a reply.\n");

    uint64_t resid = parse_single_user(acct,reply);

    free(reply);
    free(baseurl);
    return resid;

}

struct t_tweet* get_tweet(struct t_account* acct, uint64_t tweetid) {

    struct t_tweet* t = tht_search(tweetid);
    if (t != NULL) return t;

    uint64_t id = load_tweet(acct,tweetid);
    if (id != tweetid) return NULL;

    t = tht_search(tweetid);
    if (t != NULL) return t;

    return NULL;

}
struct t_user* get_user(struct t_account* acct, uint64_t userid, char* username) {

    uint64_t uid = ( (username == NULL) ? userid : urt_search(username) );

    struct t_user* u = uht_search(uid);
    if (u != NULL) return u;

    uint64_t rid = load_user(acct,userid,username);
    if (rid != uid) return NULL;

    u = uht_search(rid);
    if (u != NULL) return u;

    return NULL;
}

struct streamcb_ctx {
    struct btree* timeline;
    struct t_account* acct;
    enum timelinetype tt;
    char* buffer;
    size_t buffersz;
    stream_cb cb;
    void* cbctx;
};
int parsestreamingmsg(char *msg, size_t msgsize, struct streamcb_ctx* ctx) {

    struct json_tokener* jt = json_tokener_new();

    enum json_tokener_error jerr;

    struct json_object* streamobj = json_tokener_parse_ex(jt,msg,msgsize);

    jerr = json_tokener_get_error(jt);

    if (jerr != json_tokener_success) {
	lprintf("JSON Tokener Error: %s\n",json_tokener_error_desc(jerr));
    }

    lprintf("full reply:%s, size: %zd\n",msg,msgsize);

    if (json_object_get_type(streamobj) != json_type_object) {
	lprintf("Something is wrong. Stream's JSON isn't an object, but a %s\n",json_type_to_name(json_object_get_type(streamobj)));
	return 1; }

    struct json_object_iterator it_c;

    it_c = json_object_iter_begin(streamobj);

    struct t_entity entity;

    const char* fn;// enum json_type ft;

    memset(&entity,'\0',sizeof entity);

    fn = json_object_iter_peek_name(&it_c);
    //ft = json_object_get_type(fv); //unused

    int parse_as_tweet = 1;

    if (s_eq(fn,"friends")) { lprintf("received a follower list\n"); parse_as_tweet = 0; }
    if (s_eq(fn,"event")) { lprintf("received an event\n"); parse_as_tweet = 0; }
    if (s_eq(fn,"warning")) { lprintf("received a warning\n"); parse_as_tweet = 0; }
    if (s_eq(fn,"delete")) { lprintf("received a tweet delete message\n"); parse_as_tweet = 0; }
    if (s_eq(fn,"scrub_geo")) { lprintf("received a location delete message\n"); parse_as_tweet = 0; }
    if (s_eq(fn,"limit")) { lprintf("received a limit message\n"); parse_as_tweet = 0; }
    if (s_eq(fn,"status_withheld")) { lprintf("received a withheld status\n"); parse_as_tweet = 0; }
    if (s_eq(fn,"user_withheld")) { lprintf("received a withheld user\n"); parse_as_tweet = 0; }
    if (s_eq(fn,"disconnect")) { lprintf("received a disconnect notice\n"); parse_as_tweet = 0; }

    if (parse_as_tweet) {
	uint64_t tweet_id = parse_json_tweet(ctx->acct,streamobj,0);
	bt_insert(ctx->timeline,tweet_id);
	if (ctx->cb != NULL) ctx->cb(tweet_id,ctx->cbctx);
    }

    free((void*)fn);
    return 0;
}
size_t streamcb(char *ptr, size_t size, size_t nmemb, void *userdata) {

    struct streamcb_ctx* ctx = (struct streamcb_ctx *)userdata;

    char* msgstart = ptr;
    char* delim;
    char* endstr = ptr + (size * nmemb);

    while ( (msgstart < endstr) && ((delim = strstr(msgstart,"\r\n")) != NULL) ) {

	size_t strsize = (delim - msgstart);
	if (strsize != 0) {
	    char* msg = malloc(strsize+ ( (ctx->buffer != NULL) ? ctx->buffersz : 0 ) + 1);
	    size_t appendto = 0;
	    if (ctx->buffer) { memcpy(msg,ctx->buffer,ctx->buffersz); appendto = ctx->buffersz; }
	    free(ctx->buffer); ctx->buffer = NULL ; ctx->buffersz = 0;
	    memcpy(msg,msgstart + appendto,strsize); msg[appendto+strsize] = '\0';
	    parsestreamingmsg(msg,appendto+strsize,ctx); }
	msgstart = delim+2;
    }

    size_t remsize = (size * nmemb) - (msgstart - ptr);

    if (remsize != 0) {

	if (ctx->buffer != NULL) ctx->buffer = realloc(ctx->buffer, ctx->buffersz + remsize + 1); else
	    ctx->buffer = malloc(ctx->buffersz + remsize + 1);
	memcpy(ctx->buffer + ctx->buffersz,msgstart,remsize);
	ctx->buffer[ctx->buffersz + remsize] = '\0';
	ctx->buffersz = ctx->buffersz + remsize;

    }

    return (size * nmemb);
}
void* startstreaming_tfunc(void* param) {

    struct streamcb_ctx* ctx = (struct streamcb_ctx *)param;

    char *signedurl = acct_sign_url2(twt_usrstr_url, NULL, OA_HMAC, NULL, ctx->acct);

    CURL* streamcurl = curl_easy_init();

    curl_easy_setopt(streamcurl,CURLOPT_URL,signedurl);
    curl_easy_setopt(streamcurl, CURLOPT_FAILONERROR, 1);
    curl_easy_setopt(streamcurl, CURLOPT_WRITEFUNCTION, streamcb); //TODO 

    curl_easy_setopt(streamcurl, CURLOPT_WRITEDATA, ctx); //TODO 

    int curlstatus = curl_easy_perform(streamcurl);
    return NULL;
}
int startstreaming(struct btree* timeline, struct t_account* acct, enum timelinetype tt, stream_cb cb, void* cbctx) {

    pthread_t streamthread;

    struct streamcb_ctx* ctx = malloc(sizeof(struct streamcb_ctx));

    ctx->timeline = timeline; ctx->acct=acct; ctx->tt = tt;
    ctx->buffer = NULL; ctx->buffersz = 0; ctx->cb = cb; ctx->cbctx = cbctx;

    int r = pthread_create(&streamthread,NULL,startstreaming_tfunc,ctx);

    if (r != 0) printf("pthread_create returned %d\n",r);
    return 0;
}
int stopstreaming() {
    return 0;
}
