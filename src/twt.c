#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <json-c/json.h>
#include "twt.h"

#define infree(x) if(x) free(x);

const char twt_reqt[] = "https://api.twitter.com/oauth/request_token?oauth_callback=oob"; //request url
const char twt_apin[] = "https://api.twitter.com/oauth/access_token?oauth_verifier="; //access url, append PIN

const char twt_auth[] = "https://api.twitter.com/oauth/authorize"; //authorize url
const char twt_auth2[] = "?oauth_token="; //authorize url part 2

const char app_ckey[] = "EpnGWtYpzGUo6DflEPcQYy3y8"; //consumer key
const char app_csct[] = "6Vue8PKdb6j879pheDHPGPzOrKsVFZ5Eg6DPJ8WbcOADDq8DdV"; //consumer secret here.

const char twt_rest[] = ""; //REST url
const char twt_strm[] = ""; //streaming url


struct t_account* newAccount() {
	struct t_account* na = malloc(sizeof (struct t_account));
	na->name = NULL;
	na->tkey = NULL;
	na->tsct = NULL;
	na->auth = 0;
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

	req_url = oauth_sign_url2(twt_reqt, &postarg, OA_HMAC, NULL, app_ckey, app_csct, NULL, NULL);
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

	printf("logged in as %s (uid %d)\n",name,acct->userid);

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

int save_accounts() {
	FILE* db = fopen("accounts.db","w"); if (db == NULL) { perror("fopen"); return 1;}
	for (int i=0; i < acct_n; i++)
		fprintf(db,"%d %s %s %s %d\n",acctlist[i]->userid,acctlist[i]->name,acctlist[i]->tkey,acctlist[i]->tsct,acctlist[i]->auth);

	fflush(db);
	fclose(db);
}
int load_accounts() {
	FILE* db = fopen("accounts.db","r"); if (db == NULL) { perror("fopen"); return 1;}
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
