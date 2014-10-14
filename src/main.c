#include <stdio.h>
#include "twitter.h"
#include "ui.h"

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

	struct t_tweet* t = tht_search(id);

	if (t != NULL) {

		char* usn;
		struct t_user* u = uht_search(t->user_id);
		if (u != NULL) usn=u->screen_name; else usn = "(unknown)";

		

		printf("%16s | %s\n",usn,t->text);

		userdel(u);

	} else printf("(null for some reason)\n");

	tweetdel(t);

}

int main(int argc, char* argv[])
{
	inithashtables();

	acct_n = 0;
	int r = load_accounts();
	if (r != 0) { printf("load_accounts returned %d\n",r); return 1;}

	if (acct_n == 0) addAccount();

	printf("%d accounts loaded.\n",acct_n);

	init_ui();

	load_timeline(acctlist[0]);

	//bt_read(acctlist[0]->timelinebt,print_tweet,NULL,desc);

	save_accounts();

	destroy_ui();

	return 0;
}
