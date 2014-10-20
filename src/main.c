// vim: cin:sts=4:sw=4 

#include <stdio.h>
#include <locale.h>
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

int main(int argc, char* argv[])
{

        char* locale = setlocale(LC_ALL,"");
    
	printf("Loaded locale %s\n",locale);
    
	/* actual twtclt functionality
	
	inithashtables();

	acct_n = 0;
	int r = load_accounts();
	if (r != 0) { printf("load_accounts returned %d\n",r); return 1;}

	if (acct_n == 0) addAccount();

	printf("%d accounts loaded.\n",acct_n);

	//init_ui();

	load_timeline(acctlist[0]);

	bt_read(acctlist[0]->timelinebt,print_tweet,NULL,desc);

	save_accounts();

	//destroy_ui();

	*/


	//FIXME: right now, the program tests strings for length

	char* text;

	if (argc >= 2) text = argv[1]; else text = "Привет!";

	int i=0; char* test;

	utf8_wrap_text(text, NULL, 200, 30);	

	return 0;
}
