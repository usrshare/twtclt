// vim: cin:sts=4:sw=4 

#include <stdbool.h>
#include <stdint.h>
#include <search.h>
#include <complex.h>
#include <time.h>
#include "globals.h"
#include "hashtable.h"
#include "btree.h"
#include "twt_struct.h"
#include "twt_json.h"

#ifndef _TWT_H_
#define _TWT_H_

#define MAXTWEETLEN 140 //this header line © sabajo @ #geekissues @ irc.efnet.org  

typedef void (*tl_loaded_cb) (int op_id, int op_type, void* ctx);

struct timeline_params {

    enum timelinetype tt;

    union {
    uint64_t user_id; //users
    uint64_t list_id; //lists
    };

    union {
    char* screen_name; //users
    char* query;       //searches
    char* list_slug;   //lists
    };

    uint64_t owner_id; //lists
    char* owner_screen_name; //lists

    uint64_t since_id;
    uint64_t max_id;
    
    uint16_t count;
    
    bool trim_user;
    bool exclude_replies;
    bool contributor_details;
    bool include_entities;
};

int initStructures();

struct t_account* newAccount();
void destroyAccount(struct t_account* acct);

// oAuth authorization process here
int inithashtables();

char* parse_tweet_entities(struct t_tweet* tweet);

int request_token(struct t_account* acct);
int authorize(struct t_account* acct, char** url);
int oauth_verify(struct t_account* acct, int pin);

int add_acct(struct t_account* acct);
int del_acct(struct t_account* acct);

int load_timeline2(struct btree* timeline, struct t_account* acct, struct timeline_params param, tl_loaded_cb cb, void* cbctx);

//int load_timeline(struct btree* timeline, struct t_account* acct, enum timelinetype tt, uint64_t userid, char* customtype, tl_loaded_cb cb, void* cbctx);
//int load_timeline_ext(struct btree* timeline, struct t_account* acct, enum timelinetype tt, uint64_t userid, char* customtype, int since_id, int max_id, int count, int trim_user, int exclude_replies, int contributor_details, int include_entities, tl_loaded_cb cb, void* cbctx);
//int load_global_timeline(struct btree* timeline, enum timelinetype tt, uint64_t userid, char* customtype, tl_loaded_cb cb, void* cbctx);

uint64_t load_tweet(struct t_account* acct, uint64_t tweetid);
uint64_t load_user(struct t_account* acct, uint64_t userid, char* username);

uint64_t update_status(struct t_account* acct, char* status, uint64_t reply_id);

struct t_tweet* get_tweet(struct t_account* acct, uint64_t tweetid);
struct t_user* get_user(struct t_account* acct, uint64_t userid, char* username);

struct _stream_handle;

typedef struct _stream_handle* streamhnd;

streamhnd startstreaming(struct btree* timeline, struct t_account* acct, enum timelinetype tt, stream_cb cb, void* cbctx);
int stopstreaming(streamhnd str);

int load_config();

int save_accounts();
int load_accounts();

#endif
