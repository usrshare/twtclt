// vim: cin:sts=4:sw=4 
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "btree.h"
#include "twt_oauth.h"
#include "config.h"
#include "errno.h"
#include "log.h"

struct t_oauthkey {
    char customurl[128]; //if not empty, allows to use a custom url instead of api.twitter.com
    char tkey[128]; //token key
    char tsct[128]; //token secret
};

const char twt_base[] = "https://api.twitter.com/"; //base url for twitter

const char twt_reqt[] = "https://api.twitter.com/oauth/request_token?oauth_callback=oob"; //request url
const char twt_apin[] = "https://api.twitter.com/oauth/access_token?oauth_verifier="; //access url, append PIN
const char twt_auth[] = "https://api.twitter.com/oauth/authorize"; //authorize url
const char twt_auth2[] = "?oauth_token="; //authorize url part 2

const char app_ckey[] = "EpnGWtYpzGUo6DflEPcQYy3y8"; //consumer key
const char app_csct[] = "6Vue8PKdb6j879pheDHPGPzOrKsVFZ5Eg6DPJ8WbcOADDq8DdV"; //consumer secret here.

struct t_account* newAccount2(char* baseurl) {
    struct t_account* na = malloc(sizeof (struct t_account));
    memset(na->name,'\0',sizeof na->name);
    struct t_oauthkey* ok = malloc(sizeof (struct t_oauthkey));
    memset(ok->customurl,'\0',sizeof ok->customurl);
    if ( (baseurl != NULL) && (strlen(baseurl) != 0) ) strncpy(ok->customurl,baseurl,127);
    memset(ok->tkey,'\0',sizeof ok->tkey);
    memset(ok->tsct,'\0',sizeof ok->tsct);
    na->auth = 0;
    na->show_in_timeline = 1;
    na->key = ok;
    return na;
}

struct t_account* newAccount() {
    return newAccount2(NULL);
}


void destroyAccount(struct t_account* acct){
    if (acct->name) free(acct->name);
    if (acct->key) {
	    struct t_oauthkey* ok = acct->key;
	    free(ok->tkey);
	    free(ok->tsct);
	    free(acct->key);
    }
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

    if (rc != 3) { lprintf("twitter's request_token is supposed to return 3 values.\n"); }

    char *tkey = NULL, *tsct = NULL;

    for (int i=0; i<rc; i++) {

	char* curpar = rv[i];

	if (strncmp(curpar,"oauth_token=",12) == 0) tkey = strdup(curpar+12); //if key found, get key
	if (strncmp(curpar,"oauth_token_secret=",19) == 0) tsct = strdup(curpar+19); //if secret found, get secret

    }

    if ((tkey == NULL) || (tsct == NULL)) {
	lprintf("token key or secret not found in reply\n");
	lprintf("full reply:%s\n",reply);
	free(reply);
	if (tkey) free(tkey);
	if (tsct) free(tsct);

	return 2;
    }

    lprintf("key: %s, secret: %s\n",tkey,tsct);

    strncpy(acct->key->tkey,tkey,128);
    strncpy(acct->key->tsct,tsct,128);

    for (int i=0; i<rc; i++) free(rv[i]); //free ALL the strings!

    free (reply);

    return 0;
}
int authorize(struct t_account* acct, char** url) {
    // STEP 2 of oauth process. Return a URL which should be opened in the browser to get the PIN.
    if (acct->auth == 1) {
	lprintf("This account is already authorized.\n");
	return 1;
    }

    char* full_url = malloc(strlen(twt_auth) + strlen(twt_auth2) + strlen(acct->key->tkey) + 1);

    sprintf(full_url,"%s%s%s",twt_auth,twt_auth2,acct->key->tkey);

    //printf("Please visit the URL shown below in your browser and authenticate your Twitter account: \n\n%s\n\n",full_url);

    if (url != NULL) *url = strdup(full_url);

    free(full_url);

    return 0; //TODO
}
int oauth_verify(struct t_account* acct, int pin) {
    char* verify_url = malloc(strlen(twt_apin) + 7 + 1);
    if ((pin < 0) || (pin > 9999999)) {
	printf("Invalid PIN. PIN is supposed to be a 7-digit number.\n");
	return 1; }
    sprintf(verify_url,"%s%07d",twt_apin,pin);

    char* req_url = NULL;
    char* postarg = NULL;
    char* reply = NULL;

    req_url = oauth_sign_url2(verify_url, &postarg, OA_HMAC, NULL, app_ckey, app_csct, acct->key->tkey, acct->key->tsct);
    reply = oauth_http_post(req_url,postarg);

    if (req_url) free(req_url);
    if (postarg) free(postarg);

    if (!reply) return 1;

    char** rv = NULL;

    int rc = oauth_split_url_parameters(reply,&rv);

    if (rc != 3) { lprintf("twitter's request_token is supposed to return 3 values.\n"); }

    char *tkey = NULL, *tsct = NULL, *name = NULL, *uid = NULL;

    for (int i=0; i<rc; i++) {

	char* curpar = rv[i];

	if (strncmp(curpar,"oauth_token=",12) == 0) tkey = strdup(curpar+12); //if key found, get key
	if (strncmp(curpar,"oauth_token_secret=",19) == 0) tsct = strdup(curpar+19); //if secret found, get secret
	if (strncmp(curpar,"user_id=",8) == 0) uid = strdup(curpar+8); //if secret found, get secret
	if (strncmp(curpar,"screen_name=",12) == 0) name = strdup(curpar+12); //if secret found, get secret
    }

    if ((tkey == NULL) || (tsct == NULL)) {
	lprintf("token key or secret not found in reply\n");
	lprintf("full reply:%s\n",reply);
	free(reply);
	if (tkey) free(tkey);
	if (tsct) free(tsct);
	if (uid) free(uid);
	if (name) free(name);
	return 2;
    }

    acct->auth = 1;
    strncpy(acct->key->tkey,tkey,128);
    strncpy(acct->key->tsct,tsct,128);
    strncpy(acct->name,name,16);
    if (uid != NULL) acct->userid = strtol(uid,NULL,10);

    free(uid);

    lprintf("logged in as %s\n",name);

    for (int i=0; i<rc; i++) free(rv[i]); //free ALL the strings!

    free (reply);

    return 0;
}

char* acct_sign_url2(const char* url, char** postargs, OAuthMethod method, const char* http_method, struct t_account* acct) {

	char* req_url = NULL;

	req_url = oauth_sign_url2(url,postargs,method,http_method,app_ckey,app_csct,acct->key->tkey,acct->key->tsct);
	return req_url;
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

struct t_account* def_acct(struct t_account* acct) {
    return (acct != NULL ? acct : acctlist[0]);
}

int acct_id(struct t_account* acct) {

    for (int i=0; i<acct_n; i++) {
	if (acct == acctlist[i]) return i;
    }
    return -1;
}

int save_accounts() {
    FILE* db = cfopen("accounts.db","w"); if ((db == NULL) && (errno != ENOENT)) { perror("fopen"); return 1;}
    for (int i=0; i < acct_n; i++)
	fprintf(db,"%" PRId64 " %s %s %s %d %d\n",acctlist[i]->userid,acctlist[i]->name,acctlist[i]->key->tkey,acctlist[i]->key->tsct,acctlist[i]->show_in_timeline,acctlist[i]->auth);

    fflush(db);
    fclose(db);
    return 0;
}
int load_accounts() {
    FILE* db = cfopen("accounts.db","r"); if (db == NULL) { perror("fopen"); if (errno != ENOENT) return 1; else return 0; }
    int userid = 0, auth = 0;
    char name[16], tkey[128], tsct[128]; char show_in_timeline;
    while (!feof(db)) {
	int r = fscanf(db,"%d %16s %128s %128s %hhd %d\n",&userid,name,tkey,tsct,&show_in_timeline,&auth);
	if (r != 6) { lprintf("%d fields returned instead of 6\n",r); return 1;}
	struct t_account* na = newAccount();
	na->userid = userid; na->auth = auth;
	na->show_in_timeline = show_in_timeline;

	strncpy(na->name,name,16);
	strncpy(na->key->tkey,tkey,128);
	strncpy(na->key->tsct,tsct,128);
	add_acct(na);
    }
    fclose(db);
    return 0;
}
