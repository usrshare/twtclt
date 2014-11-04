#ifndef TWT_OAUTH_H
#define TWT_OAUTH_H

#include <stdint.h>
#include <oauth.h>
#include "globals.h"

struct t_oauthkey; //oblique oauth data struct

struct t_account {
    //this structure refers to twitter log-in accounts
    char name[16]; //acct screen name
    uint64_t userid; //acct user id
    uint8_t auth; //is authorized(access token) or not(req token)?
    char show_in_timeline; //show this account's tweets in global timelines.
    struct t_oauthkey* key; //account's oauth data
};

// add or remove acct in acctlist
int add_acct(struct t_account* acct);
int del_acct(struct t_account* acct);

// returns an oauth-signed URL to use. the returned URL will be malloc'd, so don't forget to free() it.
char* acct_sign_url2(const char* url, char** postargs, OAuthMethod method, const char* http_method, struct t_account* acct);

#endif
