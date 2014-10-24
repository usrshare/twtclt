// vim: cin:sts=4:sw=4 

#include <stdio.h>
#include <locale.h>
#include <unistd.h>
#include "twitter.h"
#include "twt_time.h"
#include "ui.h"
#include "utf8.h"

int addAccount() {

		struct t_account *myacct = newAccount();
		int r = request_token(myacct);

		if (r != 0) { printf("request_token returned %d\n",r); return 1;}
		char* url;
		r = authorize(myacct,&url);
		if (r != 0) { printf("authorize returned %d\n",r); return 1;}
		int pin = 0;
		printf("Please navigate to the following URL, authorize your account and type in the PIN given:");
		scanf("%d",&pin);
		r = oauth_verify(myacct,pin);
		if (r != 0) { printf("oauth_verify returned %d\n",r); return 1;}

		add_acct(myacct);

		return 0;
}

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

void show_help() {
    printf("todo help\n");
}

void show_version() {
    printf("todo version\n");
}


int main(int argc, char* argv[])
{

	//Used to ensure proper work of wcwidth in utf8.c. Will probably fail on non-UTF8 locales.
        char* locale = setlocale(LC_ALL,"");


	//parse arguments with getopt here.

	int c=0, qparam = 0, use_curses=0;

	while ( (c = getopt(argc,argv,":chvq:")) != -1) {

	    switch (c) {
		case 'c':
		    use_curses = 1;
		    break;
		case 'h':
		    show_help();
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

	//end parse argumets with getopt here.

	inithashtables();

	acct_n = 0;
	int r = load_accounts();
	if (r != 0) { printf("load_accounts returned %d\n",r); return 1;}

	if (acct_n == 0) addAccount();

	printf("%d accounts loaded.\n",acct_n);

	if (use_curses) init_ui();

	load_timeline(acctlist[0]);

	if (use_curses) 
	    bt_read(acctlist[0]->timelinebt,print_nc_tweet,NULL,desc); else 
		bt_read(acctlist[0]->timelinebt,print_tweet,NULL,desc);

	save_accounts();

	if (use_curses) destroy_ui();

	//FIXME: right now, the program tests strings for length

	//utf8_test();

	return 0;
}
