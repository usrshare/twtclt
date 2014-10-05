#include <oauth.h>
#include <stdint.h>

#ifndef _TWT_H_
#define _TWT_H_

uint16_t acct_n; //known accounts
struct t_account** acctlist; //acct list.

struct t_account {
	//this structure refers to twitter log-in accounts
	char* name; //acct screen name
	int userid; //acct user id
	char* tkey; //token key
	char* tsct; //token secret
	int auth; //is authorized(access token) or not(req token)?
};

struct t_timeline {
	struct t_timeline* next;
};

struct t_tweet {
	char* author;
	char* contents;
};

struct t_account* newAccount();

void destroyAccount(struct t_account* acct);

// oAuth authorization process here

int request_token(struct t_account* acct);
int authorize(struct t_account* acct, char** url);
int oauth_verify(struct t_account* acct, int pin);

int add_acct(struct t_account* acct);
int del_acct(struct t_account* acct);

int save_accounts();

int load_accounts();

#endif
