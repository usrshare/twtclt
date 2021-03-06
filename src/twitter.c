// vim: cin:sts=4:sw=4 

#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <curl/curl.h>
#include <time.h>
#include <utf8proc.h>
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
const char twt_listtl_url[] = "https://api.twitter.com/1.1/lists/statuses.json"; //list timeline
const char twt_status_url[] = "https://api.twitter.com/1.1/statuses/show.json"; //load single tweet
const char twt_user_url[] =   "https://api.twitter.com/1.1/users/show.json"; //load single user

const char twt_stream_usr_url[] = "https://userstream.twitter.com/1.1/user.json"; //user streams

const char twt_update_url[] = "https://api.twitter.com/1.1/statuses/update.json"; //write tweets

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

int load_timeline_base(struct btree* timeline, enum timelinetype tt, struct t_account* acct, const char* baseurl, tl_loaded_cb cb, void* cbctx) {

    char* req_url = NULL;
    char* reply = NULL;

    //printf("logging in with app_ckey %s, app_csct %s, tkey %s, tsct %s\n",app_ckey, app_csct, acct->tkey, acct->tsct);

    req_url = acct_sign_url2(baseurl, NULL, OA_HMAC, NULL, acct);

    reply = oauth_http_get(req_url,NULL);

    if (req_url) free(req_url);

    if (!reply) return 1;

    lprintf("Received a reply.\n");

    parse_timeline(timeline, reply, acct);

    if (cb) cb(0,0,cbctx);

    free(reply);
    return 0;

}

int load_timeline2(struct btree* timeline, struct t_account* acct, struct timeline_params param, tl_loaded_cb cb, void* cbctx) {

    char *baseurl = NULL;

    switch(param.tt) {
	case home:
	    baseurl = strdup(twt_hometl_url);
	    break;
	case user:
	    baseurl = strdup(twt_usertl_url);
	    if (param.user_id != 0) {
		baseurl = addparam_int(baseurl,"user_id",param.user_id,1);
	    } else {
		if (param.screen_name) baseurl = addparam(baseurl,"screen_name",param.screen_name,1);
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
	    if (param.query != NULL) baseurl = addparam(baseurl,"q",param.query,1);
	    break;
	case list:
	    baseurl = strdup(twt_listtl_url);
	    
    }

    if (param.since_id) baseurl = addparam_int(baseurl,"since_id",param.since_id,1);
    if (param.max_id) baseurl = addparam_int(baseurl,"max_id",param.max_id,1);
    if (param.count) baseurl = addparam_int(baseurl,"count",param.max_id,1);

    if (param.trim_user) baseurl = addparam(baseurl,"trim_user","1",1);
    if (param.exclude_replies) baseurl = addparam(baseurl,"exclude_replies","1",1);
    if (param.contributor_details) baseurl = addparam(baseurl,"contributor_details","1",1);
    if (param.include_entities) baseurl = addparam(baseurl,"include_entities","1",1);

    int r = load_timeline_base(timeline, param.tt, acct, baseurl, cb, cbctx);

    free(baseurl);

    return r;
}

uint64_t update_status(struct t_account* acct, char* status, uint64_t reply_id) {

    acct = def_acct(acct);

    char *baseurl = strdup(twt_update_url);
    
    char *postargs = strdup("\0");
   
    char *esc_status = oauth_url_escape(status);

    baseurl = addparam(baseurl,"status",esc_status,1);
    if (reply_id) baseurl = addparam_int(baseurl,"in_reply_to_status_id",reply_id,1);

    char* req_url = acct_sign_url2(baseurl, &postargs, OA_HMAC, "POST", acct);
    char* reply = oauth_http_post(req_url,postargs);

    if (!reply) return 1;

    lprintf("Received a reply.\n");
    
    uint64_t resid = parse_single_tweet(reply, acct);
    
    if (resid == 0) lprintf("Response is not a tweet. Full response: \n%s\n", reply);
    
    if (req_url) free(req_url);
    if (postargs) free(postargs);

    return resid;
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

    uint64_t resid = parse_single_tweet(reply, acct);

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

    uint64_t resid = parse_single_user(reply, acct);

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
    int* stop;
};
struct _stream_handle {
    pthread_t streamthread;
    int stop;
    struct streamcb_ctx* ctx;
};

size_t streamcb(char *ptr, size_t size, size_t nmemb, void *userdata) {

    struct streamcb_ctx* ctx = (struct streamcb_ctx *)userdata;
    
    if ( *(ctx->stop) ) return 0;

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
	    uint64_t tweet_id = parsestreamingmsg(ctx->acct,msg,appendto+strsize);
	    if (tweet_id != 0) {
		bt_insert(ctx->timeline,tweet_id,NULL);
		if (ctx->cb != NULL) ctx->cb(tweet_id,ctx->cbctx);
	    }
	}
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

    char *signedurl = acct_sign_url2(twt_stream_usr_url, NULL, OA_HMAC, NULL, ctx->acct);

    CURL* streamcurl = curl_easy_init();

    curl_easy_setopt(streamcurl,CURLOPT_URL,signedurl);
    curl_easy_setopt(streamcurl, CURLOPT_FAILONERROR, 1);
    curl_easy_setopt(streamcurl, CURLOPT_WRITEFUNCTION, streamcb); //TODO 

    curl_easy_setopt(streamcurl, CURLOPT_WRITEDATA, ctx); //TODO 

    int curlstatus = curl_easy_perform(streamcurl);
    if (curlstatus != 0) { /* TODO error */ }
    return NULL;
}
streamhnd startstreaming(struct btree* timeline, struct t_account* acct, enum timelinetype tt, stream_cb cb, void* cbctx) {

    streamhnd hnd = malloc(sizeof(struct _stream_handle));
    hnd->stop = 0;

    struct streamcb_ctx* ctx = malloc(sizeof(struct streamcb_ctx));

    hnd->ctx = ctx;

    ctx->timeline = timeline; ctx->acct=acct; ctx->tt = tt;
    ctx->buffer = NULL; ctx->buffersz = 0; ctx->cb = cb; ctx->cbctx = cbctx; ctx->stop = &(hnd->stop);

    int r = pthread_create(&(hnd->streamthread),NULL,startstreaming_tfunc,ctx);

    if (r != 0) printf("pthread_create returned %d\n",r);
    return hnd;
}
int stopstreaming(streamhnd str) {
    pthread_cancel(str->streamthread);
    pthread_join(str->streamthread,NULL);
    
    free(str->ctx);
    free(str);
    return 0;
}
