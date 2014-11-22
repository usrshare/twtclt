#include "twt_json.h"
#include "log.h"
#include "stringex.h"

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
	    if ( (ient->index_e == i) || (len == 0) ) {
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
		    append1 = " error "; append2 = " error "; break;
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
    if (s_eq(fn,"user")) { nt->user_id = parse_json_user(fv,1); return 0;}
    if (s_eq(fn,"truncated")) { nt->truncated = json_object_get_boolean(fv); return 0;}
    if (s_eq(fn,"favorited")) { nt->favorited = json_object_get_boolean(fv); return 0;}
    if (s_eq(fn,"retweeted")) { nt->retweeted = json_object_get_boolean(fv); return 0;}
    if (s_eq(fn,"favorite_count")) { nt->favorite_count = json_object_get_int64(fv); return 0;}
    if (s_eq(fn,"retweet_count")) { nt->retweet_count = json_object_get_int64(fv); return 0;}
    if (s_eq(fn,"retweeted_status")) { nt->retweeted_status_id = parse_json_tweet(fv,1); return 0;}
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

uint64_t parse_json_user(json_object* user, int perspectival) {
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

    int r = uht_insert(&nu,no_replace); if (r != 0) lprintf("uht_insert user returned %d\n",r);

    r = urt_insert(nu.screen_name, nu.id, replace);
    if (r != 0) lprintf("urt_insert screen name returned %d\n",r);

    return nu.id;
}
uint64_t parse_json_tweet(json_object* tweet, int perspectival) {
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

int parse_timeline(struct btree* timeline, enum timelinetype tt, char* timelinereply) {

    struct json_tokener* jt = json_tokener_new();
    enum json_tokener_error jerr;

    struct json_object* timelineobj = json_tokener_parse_ex(jt,timelinereply,strlen(timelinereply));
    jerr = json_tokener_get_error(jt);
    if (jerr != json_tokener_success) {
	lprintf("JSON Tokener Error: %s\n",json_tokener_error_desc(jerr));
    }

    if (json_object_get_type(timelineobj) != json_type_array) {
	lprintf("Something is wrong. Timeline's JSON isn't an array, but a %s\n",json_type_to_name(json_object_get_type(timelineobj)));
	lprintf("full reply:%s\n",timelinereply);
	return 1; }
    int tla_len = json_object_array_length(timelineobj);

    for (int i=0; i<tla_len; i++) {
	json_object* tweet = json_object_array_get_idx(timelineobj,i);
	uint64_t tweet_id = parse_json_tweet(tweet,0);
	//printf("Adding tweet %lld to timeline...\n",tweet_id);
	bt_insert(timeline,tweet_id);
    }

    json_tokener_free(jt);
    return 0;
}
uint64_t parse_single_tweet(char* timelinereply) {

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

    uint64_t tweet_id = parse_json_tweet(tweet,0);
    json_tokener_free(jt);
    return tweet_id;
}
uint64_t parse_single_user(char* timelinereply) {

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

    uint64_t user_id = parse_json_user(user,0);
    json_tokener_free(jt);
    return user_id;
}

uint64_t parsestreamingmsg(char *msg, size_t msgsize) {

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
    
    free((void*)fn);

    if (parse_as_tweet) {
	uint64_t tweet_id = parse_json_tweet(streamobj,0);
	return tweet_id;
    }

    return 0;
}
