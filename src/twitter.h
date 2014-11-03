// vim: cin:sts=4:sw=4 

#include <stdint.h>
#include <search.h>
#include <complex.h>
#include <time.h>
#include "globals.h"
#include "hashtable.h"
#include "btree.h"
#include <json-c/json.h>
#include "twt_struct.h"

#ifndef _TWT_H_
#define _TWT_H_

#define s_eq(x,y) (strcmp(x,y) == 0)

#define MAXTWEETLEN 140 //this header line © sabajo @ #geekissues @ irc.efnet.org  

int initStructures();

struct t_account* newAccount();

void destroyAccount(struct t_account* acct);

// oAuth authorization process here
int inithashtables();

uint64_t parse_json_user(struct t_account* acct, json_object* user, int perspectival);

uint64_t parse_json_tweet(struct t_account* acct, json_object* tweet, int perspectival);

char* parse_tweet_entities(struct t_tweet* tweet);

int request_token(struct t_account* acct);
int authorize(struct t_account* acct, char** url);
int oauth_verify(struct t_account* acct, int pin);

int add_acct(struct t_account* acct);
int del_acct(struct t_account* acct);

int load_timeline(struct btree* timeline, struct t_account* acct, enum timelinetype tt, uint64_t userid, char* customtype);
int load_timeline_ext(struct btree* timeline, struct t_account* acct, enum timelinetype tt, uint64_t userid, char* customtype, int since_id, int max_id, int count, int trim_user, int exclude_replies, int contributor_details, int include_entities);

int load_config();

int save_accounts();
int load_accounts();

#endif
