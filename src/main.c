// vim: cin:sts=4:sw=4 

#include <stdio.h>
#include <locale.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "config.h"
#include "globals.h"
#include "log.h"
#include "twitter.h"
#include "twt_time.h"
#include "ui.h"
#include "utf8.h"

int addAccount() {

    printf("Account creation guide:\n\n");
    
    char baseurl[128];
    memset(baseurl,'\0',sizeof baseurl);

    /*
    printf("This client can be used to access both Twitter and Twitter API compatible "
	   "alternative resources, such as StatusNet. This is an experimental feature, "
	   "and no guarantees are provided. \n\n"
	   "If you are going to use this client for Twitter, please press ENTER (leave the field "
	   "empty). Otherwise, input your API base URL in the field below.\n\n");

    printf(">");

    fgets(baseurl,127,stdin);

    baseurl[strlen(baseurl)-1]='\0'; //removing the newline

    */
    struct t_account *myacct = newAccount(baseurl);

    int r = request_token(myacct);

    if (r != 0) { printf("request_token returned %d\n",r); return 1;}
    char* url;
    r = authorize(myacct,&url);
    if (r != 0) { printf("authorize returned %d\n",r); return 1;}
    int pin = 0;
    printf("Please navigate to the following URL, authorize your account and type in the PIN given:\n%s\n\nEnter PIN here:",url);
    free(url);
    scanf("%d",&pin);
    r = oauth_verify(myacct,pin);
    if (r != 0) { printf("oauth_verify returned %d\n",r); return 1;}

    add_acct(myacct);

    return 0;
}
/*
   void print_nc_tweet(uint64_t id, void* ctx) {
   struct t_tweet* tt = tht_search(id);
   draw_tweet(tt);
   tweetdel(tt);
   return;
   }

   void print_tweet(uint64_t id, void* ctx) {

   struct t_tweet* tt = tht_search(id);
   struct t_tweet* ot = NULL;

   if (tt == NULL) return;
   if (tt->retweeted_status_id != 0) ot = tht_search(tt->retweeted_status_id);

   struct t_tweet* rt = (ot ? ot : tt);
   struct t_user* rtu = uht_search(rt->user_id);
   struct t_user* otu = uht_search(tt->user_id);

   char reltime[12];
   char rttime[12];

   char* rusn = (rtu ? rtu->screen_name : "(unknown)");
   char* ousn = (otu ? otu->screen_name : "(unknown)");

   if (ot) reltimestr(tt->created_at,rttime);
   reltimestr(rt->created_at,reltime);

   char* text = parse_tweet_entities(rt);

   if (ot) printf("%16s | %s [%s] (RT by %s %s)\n",rusn,text,reltime,ousn,rttime); else printf("%16s | %s [%s]\n",rusn,text,reltime);

   tweetdel(tt);
   userdel(otu);
   if (ot) tweetdel(ot);
   if (ot) userdel(rtu);
   free(text);

   return;
   }
 */
void show_help() {
    printf("      -d | wait for a keypress before starting. useful for debugging\n");
    printf("      -h | show this help screen\n");
    printf("      -t | run tests and quit\n");
    printf("      -v | show the version screen\n");
    printf(" -q12345 | useless param for getopt testing\n");
}

void show_version() {
    printf("twtclt doesn't have a version number at this point.\n");
}

int run_tests() {
    bt_test();
    return 0;
}

int main(int argc, char* argv[]){

    //Used to ensure proper work of wcwidth in utf8.c. Will probably fail on non-UTF8 locales.
    /* char* locale = */ setlocale(LC_ALL,"");

    int c=0, qparam = 0,waitkey=0; log_enabled = 0;

    while ( (c = getopt(argc,argv,":dhltvq:")) != -1) {

	switch (c) {
	    case 'd':
		waitkey = 1;
		break;
	    case 'l':
		log_enabled = 1;
		exit(0);
		break;
	    case 'h':
		show_help();
		exit(0);
		break;
	    case 't':
		run_tests();
		exit(0);
		break;
	    case 'v':
		show_version();
		exit(0);
		break;
	    case 'q':
		qparam = atoi(optarg);
		break;
	    case ':':
		printf("No parameter for key -%c\n",optopt);
		exit(EXIT_FAILURE);
		break;
	    case '?':
	    default:
		printf("Unknown key -%c\n",optopt);
		exit(EXIT_FAILURE);
		break;
	}
    }

    if (qparam) printf("qparam is %d\n",qparam);

    //end parse argumets with getopt here.
    
    load_config();

    if (waitkey) getchar();

    inithashtables();

    acct_n = 0;
    int r = load_accounts();
    if (r != 0) { lprintf("load_accounts returned %d\n",r); return 1;}

    if (acct_n == 0) addAccount();

    lprintf("%d accounts loaded.\n",acct_n);

    pthread_t* uithread = init_ui();

    r = pthread_join(*uithread,NULL);

    save_accounts();

    //FIXME: right now, the program tests strings for length

    //utf8_test();

    return 0;
}
