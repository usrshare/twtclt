#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include "twt.h"
#include "hashtable.h"

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

	int r = ht_insert(tweetht,tweet->id,new); if (r != 0) free(new);
	if ( r == 2) {

		if (cbeh == no_replace) return 2;

		if (cbeh == update) {
			struct t_tweet* old = tht_search(tweet->id);
			if ((tweet->perspectival >= old->perspectival) && (tweet->retrieved_on < old->retrieved_on)) return 2; //tweet already in ht is better than one we have here.
			tweetdel(old);

		}

		r = tht_delete(tweet->id); if (r != 0) printf("Error while deleting old tweet to replace with new.\n");
		r = tht_insert(tweet,no_replace); if (r != 0) printf("Error while replacing tweet.\n");



		//TODO replace existing tweet with new

	} else return r;
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
	if ((cbeh == replace) && (r == 2)) {

		//TODO replace existing user with new

	} else {if (r != 0) free(new); return r;}
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

struct t_tweet* tweetdup(struct t_tweet* orig) {
	struct t_tweet* new = malloc(sizeof(struct t_tweet));
	*new = *orig; //copy all values
	if (orig->created_at) new->created_at = strdup(orig->created_at);
	if (orig->text) new->text = strdup(orig->text);
	if (orig->source) new->source = strdup(orig->source);
	if (orig->lang) new->lang = strdup(orig->lang);
	return new;
}

struct t_user* userdup(struct t_user* orig) {
	struct t_user* new = malloc(sizeof(struct t_user));
	*new = *orig; //copy all values
	if (orig->created_at) new->created_at = strdup(orig->created_at);
	if (orig->description) new->description = strdup(orig->description);
	if (orig->location) new->location = strdup(orig->location);
	if (orig->lang) new->lang = strdup(orig->lang);
	if (orig->name) new->name = strdup(orig->name);
	if (orig->time_zone) new->time_zone = strdup(orig->time_zone);
	if (orig->url) new->url = strdup(orig->url);
	if (orig->withheld_in_countries) new->withheld_in_countries = strdup(orig->withheld_in_countries);
	return new;
}

void tweetdel(struct t_tweet* ptr) {
	free(ptr->created_at);
	free(ptr->text);
	free(ptr->source);
	free(ptr->lang);
	free(ptr);
}

void userdel(struct t_user* ptr) {
	free(ptr->created_at);
	free(ptr->description);
	free(ptr->location);
	free(ptr->lang);
	free(ptr->name);
	free(ptr->time_zone);
	free(ptr->url);
	free(ptr->withheld_in_countries);
	free(ptr);
}

char* strrecat(char* orig, const char* append) {
	char* new = realloc(orig,strlen(orig) + strlen(append) + 1);
	new = strcat(new,append);
	return new;
}

char* addparam(char* orig, const char* parname, const char* parvalue, int addq) {
	char* append = malloc(1 + strlen(parname) + 1 + strlen(parvalue) + 1);
	sprintf(append,"%s%s=%s",(strchr(orig,'?') ? (addq ? "?" : "") : "&"),parname,parvalue); 
	return strrecat(orig,append);
}

char* addparam_int(char* orig, const char* parname, int64_t parvalue, int addq) {
	char pvchr[22]; //enough to store any 64bit number.
	sprintf(pvchr,"%lld",parvalue);
	return addparam(orig,parname,pvchr,addq);
}

struct t_account* newAccount() {
	struct t_account* na = malloc(sizeof (struct t_account));
	na->name = NULL;
	na->tkey = NULL;
	na->tsct = NULL;
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

	if (rc != 3) { printf("twitter's request_token is supposed to return 3 values.\n"); }

	char *tkey = NULL, *tsct = NULL;

	for (int i=0; i<rc; i++) {

		char* curpar = rv[i];

		if (strncmp(curpar,"oauth_token=",12) == 0) tkey = strdup(curpar+12); //if key found, get key
		if (strncmp(curpar,"oauth_token_secret=",19) == 0) tsct = strdup(curpar+19); //if secret found, get secret

	}

	if ((tkey == NULL) || (tsct == NULL)) {
		printf("token key or secret not found in reply\n");
		printf("full reply:%s\n",reply);
		free(reply);
		if (tkey) free(tkey);
		if (tsct) free(tsct);

		return 2;
	}

	printf("key: %s, secret: %s\n",tkey,tsct);

	acct->tkey = tkey;
	acct->tsct = tsct;

	for (int i=0; i<rc; i++) free(rv[i]); //free ALL the strings!

	free (reply);

	return 0;
}

int authorize(struct t_account* acct, char** url) {
	// STEP 2 of oauth process. Return a URL which should be opened in the browser to get the PIN.
	if (acct->auth == 1) {
		printf("This account is already authorized.\n");
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
	int r = sprintf(verify_url,"%s%07d",twt_apin,pin);

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

	if (rc != 3) { printf("twitter's request_token is supposed to return 3 values.\n"); }

	char *tkey = NULL, *tsct = NULL, *name = NULL, *uid = NULL;

	for (int i=0; i<rc; i++) {

		char* curpar = rv[i];

		if (strncmp(curpar,"oauth_token=",12) == 0) tkey = strdup(curpar+12); //if key found, get key
		if (strncmp(curpar,"oauth_token_secret=",19) == 0) tsct = strdup(curpar+19); //if secret found, get secret
		if (strncmp(curpar,"user_id=",8) == 0) uid = strdup(curpar+8); //if secret found, get secret
		if (strncmp(curpar,"screen_name=",12) == 0) name = strdup(curpar+12); //if secret found, get secret
	}

	if ((tkey == NULL) || (tsct == NULL)) {
		printf("token key or secret not found in reply\n");
		printf("full reply:%s\n",reply);
		free(reply);
		if (tkey) free(tkey);
		if (tsct) free(tsct);
		if (uid) free(uid);
		if (name) free(name);
		return 2;
	}

	acct->auth = 1;
	acct->tkey = tkey;
	acct->tsct = tsct;
	acct->name = name;
	if (uid != NULL) acct->userid = strtol(uid,NULL,10);

	free(uid);

	printf("logged in as %s (uid %u)\n",name,acct->userid);

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

uint64_t parse_json_user(struct t_account* acct, json_object* user, int perspectival) {
	//returns the user ID of the twitter user. also probably adds the user struct to the hash table or something.
	struct json_object_iterator it_c, it_e;
	it_c = json_object_iter_begin(user);
	it_e = json_object_iter_end(user);

	uint64_t id = 0;

	const char* fn; json_object* fv; enum json_type ft; struct t_user nu;

	nu.created_at = nu.description = nu.lang = nu.location = nu.name = nu.time_zone = nu.url = nu.withheld_in_countries = NULL;

	while (!json_object_iter_equal(&it_c,&it_e)) {

		fn = json_object_iter_peek_name(&it_c);
		fv = json_object_iter_peek_value(&it_c);
		ft = json_object_get_type(fv);

		if (s_eq(fn,"id")) id = nu.id = json_object_get_int64(fv); else
		if (s_eq(fn,"id_str")) {} else
		if (s_eq(fn,"screen_name") && ft) strncpy(nu.screen_name,json_object_get_string(fv),16); else
		if (s_eq(fn,"followers_count")) nu.followers_count = json_object_get_int64(fv); else
		if (s_eq(fn,"friends_count")) nu.friends_count = json_object_get_int64(fv); else
		if (s_eq(fn,"protected")) nu.protected_ = json_object_get_nullbool(fv); else
		if (s_eq(fn,"geo_enabled")) nu.geo_enabled = json_object_get_nullbool(fv); else
		if (s_eq(fn,"verified")) nu.verified = json_object_get_nullbool(fv); else
		if (s_eq(fn,"contributors_enabled")) nu.contributors_enabled = json_object_get_nullbool(fv); else
		if (s_eq(fn,"following")) nu.following = json_object_get_nullbool(fv); else
		if (s_eq(fn,"follow_request_sent")) nu.follow_request_sent = json_object_get_nullbool(fv); else
		if (s_eq(fn,"is_translator")) nu.is_translator = json_object_get_nullbool(fv); else
		if (s_eq(fn,"is_translation_enabled")) {} else
		if (s_eq(fn,"favourites_count")) nu.favorites_count = json_object_get_int64(fv); else
		if (s_eq(fn,"listed_count")) nu.listed_count = json_object_get_int64(fv); else
		if (s_eq(fn,"followers_count")) nu.followers_count = json_object_get_int64(fv); else
		if (s_eq(fn,"friends_count")) nu.friends_count = json_object_get_int64(fv); else
		if (s_eq(fn,"statuses_count")) nu.statuses_count = json_object_get_int64(fv); else
		if (s_eq(fn,"utc_offset")) nu.utc_offset = json_object_get_int(fv); else
		if (s_eq(fn,"name")) nu.name = strdup(json_object_get_string(fv)); else
		if (s_eq(fn,"created_at")) nu.created_at = strdup(json_object_get_string(fv)); else
		if (s_eq(fn,"location")) nu.location = strdup(json_object_get_string(fv)); else
		if (s_eq(fn,"description")) nu.description = strdup(json_object_get_string(fv)); else
		if (s_eq(fn,"url")) { if (fv) nu.url = strdup(json_object_get_string(fv)); } else
		if (s_eq(fn,"default_profile")) {} else
		if (s_eq(fn,"default_profile_image")) {} else
		if (s_eq(fn,"profile_background_color")) {} else
		if (s_eq(fn,"profile_background_color")) {} else
		if (s_eq(fn,"profile_background_image_url")) {} else
		if (s_eq(fn,"profile_background_image_url_https")) {} else
		if (s_eq(fn,"profile_background_tile")) {} else
		if (s_eq(fn,"profile_banner_url")) {} else
		if (s_eq(fn,"profile_image_url")) {} else
		if (s_eq(fn,"profile_image_url_https")) {} else
		if (s_eq(fn,"profile_link_color")) {} else
		if (s_eq(fn,"profile_sidebar_border_color")) {} else
		if (s_eq(fn,"profile_sidebar_fill_color")) {} else
		if (s_eq(fn,"profile_sidebar_text_color")) {} else
		if (s_eq(fn,"profile_use_background_image")) {} else
		if (s_eq(fn,"profile_text_color")) {}
		// else	printf("unparsed user field %s\n", fn);

		nu.perspectival = perspectival;
		nu.retrieved_on = time(NULL); 

		json_object_iter_next(&it_c);

	}

	//printf("Parsed user %lld.\n",id);

	uht_insert(&nu,no_replace);

	return id;
}

uint64_t parse_json_tweet(struct t_account* acct, json_object* tweet, int perspectival) {
	struct json_object_iterator it_c, it_e;
	it_c = json_object_iter_begin(tweet);
	it_e = json_object_iter_end(tweet);

	uint64_t id = 0;

	const char* fn; json_object* fv; enum json_type ft; struct t_tweet nt;

	nt.created_at = nt.text = nt.source = nt.lang = NULL;

	while (!json_object_iter_equal(&it_c,&it_e)) {

		fn = json_object_iter_peek_name(&it_c);
		fv = json_object_iter_peek_value(&it_c);
		ft = json_object_get_type(fv);

		if (s_eq(fn,"id")) id = nt.id = json_object_get_int64(fv); else
		if (s_eq(fn,"id_str")) {} else
		if (s_eq(fn,"in_reply_to_status_id")) nt.in_reply_to_status_id = json_object_get_int64(fv); else
		if (s_eq(fn,"in_reply_to_status_id_str")) {} else
		if (s_eq(fn,"in_reply_to_user_id")) nt.in_reply_to_user_id = json_object_get_int64(fv); else
		if (s_eq(fn,"in_reply_to_user_id_str")) {} else
		if (s_eq(fn,"in_reply_to_screen_name")) { if(ft) strncpy(nt.in_reply_to_screen_name,json_object_get_string(fv),16); } else
		if (s_eq(fn,"user")) nt.user_id = parse_json_user(acct,fv,1); else
		if (s_eq(fn,"truncated")) nt.truncated = json_object_get_boolean(fv); else
		if (s_eq(fn,"favorited")) nt.favorited = json_object_get_boolean(fv); else
		if (s_eq(fn,"retweeted")) nt.retweeted = json_object_get_boolean(fv); else
		if (s_eq(fn,"favorite_count")) nt.favorite_count = json_object_get_int64(fv); else
		if (s_eq(fn,"retweet_count")) nt.retweet_count = json_object_get_int64(fv); else
		if (s_eq(fn,"retweeted_status")) nt.retweeted_status_id = parse_json_tweet(acct,fv,1); else
		if (s_eq(fn,"possibly_sensitive")) nt.possibly_sensitive = json_object_get_nullbool(fv); else

		if (s_eq(fn,"text")) nt.text = strdup(json_object_get_string(fv)); else
		if (s_eq(fn,"lang")) nt.lang = strdup(json_object_get_string(fv)); else
		if (s_eq(fn,"source")) nt.source = strdup(json_object_get_string(fv)); else
		if (s_eq(fn,"created_at")) nt.created_at = strdup(json_object_get_string(fv));
		
		//else printf("unparsed tweet field %s\n", fn);

		nt.perspectival = perspectival;
		nt.retrieved_on = time(NULL); 

		json_object_iter_next(&it_c);

	}

	//printf("Parsed tweet %lld.\n",id);
	tht_insert(&nt,no_replace);
	return id;
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
		printf("Something is wrong. Timeline's JSON isn't an array, but a %s\n",json_type_to_name(json_object_get_type(timeline))); return 1; }

	int tla_len = json_object_array_length(timeline);

	for (int i=0; i<tla_len; i++) {
		json_object* tweet = json_object_array_get_idx(timeline,i);
		uint64_t tweet_id = parse_json_tweet(acct,tweet,0);
		printf("Adding tweet %lld to timeline...\n",tweet_id);
		bt_insert(acct->timelinebt,tweet_id);
	}

	json_tokener_free(jt);
}

int load_timeline_ext(struct t_account* acct, enum timelinetype tt, int since_id, int max_id, int trim_user, int exclude_replies, int contributor_details, int include_entities) {
	const char twt_home[] = "https://api.twitter.com/1.1/statuses/home_timeline.json";
	char temp[100];
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

	printf("Received a reply.\n");

	parse_timeline(acct, tt, reply);


	free(twtcur);
	return 0;
}

int load_timeline(struct t_account* acct) {
	return load_timeline_ext(acct,0,0,0,0,0,0,0);
}

int save_accounts() {
	FILE* db = fopen("accounts.db","w"); if (db == NULL) { perror("fopen"); return 1;}
	for (int i=0; i < acct_n; i++)
		fprintf(db,"%d %s %s %s %d\n",acctlist[i]->userid,acctlist[i]->name,acctlist[i]->tkey,acctlist[i]->tsct,acctlist[i]->auth);

	fflush(db);
	fclose(db);
}
int load_accounts() {
	FILE* db = fopen("accounts.db","r"); if (db == NULL) { perror("fopen"); if (errno != ENOENT) return 1; else return 0; }
	int userid = 0, auth = 0;
	char name[100], tkey[100], tsct[100];
	while (!feof(db)) {
		int r = fscanf(db,"%d %100s %100s %100s %d\n",&userid,name,tkey,tsct,&auth);
		if (r != 5) { printf("%d fields returned instead of 5\n"); return 1;}
		struct t_account* na = newAccount();
		na->userid = userid; na->auth = auth;
		if (name) na->name = strdup(name); if (tkey) na->tkey = strdup(tkey); if (tsct) na->tsct = strdup(tsct);
		add_acct(na);
	}
	fclose(db);

}
