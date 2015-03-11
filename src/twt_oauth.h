// vim: cin:sts=4:sw=4 
#ifndef TWT_OAUTH_H
#define TWT_OAUTH_H

#include <stdint.h>
#include <oauth.h>
#include "globals.h"

enum errorcodes {
    OK = 0,

    //Twitter errors.

    EAUTHFL = 32, //Could not authenticate you.
    ENOEXST = 34, //Page does not exist
    ESUSPEN = 64, //Acct suspended
    EOLDAPI = 68, //REST API obsolete
    ERATELM = 88, //Rate limit exceeded
    EINVTKN = 89, //Invalid token
    ESSLREQ = 92, //SSL is required
    EOVERCP = 130, //Over capacity
    EINTERR = 131, //Internal error
    EAUTHTM = 135, //Invalid oauth timestamp
    ENOFLLW = 161, //Unable to follow more
    ESTATPR = 179, //Status protected
    EULIMIT = 185, //Over daily update limit
    EDUPLIC = 187, //Duplicate status
    EBADAUT = 215, //Bad auth data
    ESPAM = 226, //Could be automated
		//also results if you try to post anything with an old official Twitter client keys.

    EVERIFY = 231, //User must verify
    ERETIRE = 251, //Endpoint retired
    ERDONLY = 261, //App can't write
    EMTSELF = 271, //Can't mute self
    ENOMUTE = 272, //Not muting this user.

    //HTTP errors
    
    HTTPOK = 200, //OK

    ENOTMOD = 304, //Not Modified
    EBADREQ = 400, //Bad Request
    EUNAUTH = 401, //Unauthotized
    EFORBID = 403, //Forbidden
    ENOTFND = 404, //Not Found
    ENOTACC = 406, //Not Acceptable.
    EGONE = 410, //Gone.
    ESLOW = 420, //Slow down, please.
    EUNPROC = 422, //Unprocessable Entity
    EOVERLD = 429, //Too Many Requests
    EINTERN = 500, //Internal Server Error
    EBADGW = 502, //Bad Gateway
    EUAVAIL = 503, //Service Unavailable
    EGWTIME = 504, //Gateway Timeout
};

struct t_oauthkey; //oblique oauth data struct

struct t_account {
    //this structure refers to twitter log-in accounts
    char name[16]; //acct screen name
    uint64_t userid; //acct user id
    uint8_t auth; //is authorized(access token) or not(req token)?
    char show_in_timeline; //show this account's tweets in global timelines.
    struct t_oauthkey* key; //account's oauth data
    enum errorcodes t_errno; //HTTP or Twitter error code
};

// add or remove acct in acctlist
int add_acct(struct t_account* acct);
int del_acct(struct t_account* acct);

// return account specified or default account if NULL
struct t_account* def_acct(struct t_account* acct);

int acct_id(struct t_account* acct);

// returns an oauth-signed URL to use. the returned URL will be malloc'd, so don't forget to free() it.
char* acct_sign_url2(const char* url, char** postargs, OAuthMethod method, const char* http_method, struct t_account* acct);

#endif
