// vim: cin:sts=4:sw=4 

#include <oauth.h>
#include <stdint.h>
#include <search.h>
#include <complex.h>
#include <time.h>
#include "hashtable.h"
#include "btree.h"
#include <json-c/json.h>

#ifndef _TWT_H_
#define _TWT_H_

#define s_eq(x,y) (strcmp(x,y) == 0)

#define MAXTWEETLEN 140 //this header line © sabajo @ #geekissues @ irc.efnet.org  

uint16_t acct_n; //known accounts
struct t_account** acctlist; //acct list.

struct hashtable* tweetht; //tweet cache hash table.
struct hashtable* userht; //user cache hash table.

//struct btree* tweetbt; //tweet ID binary tree.
//struct btree* userbt; //tweet ID binary tree.

enum entitytype {
    hashtag, //#hashtag
    symbol, //$TWTR
    url, //URL
    mention, //@mention
    media, //media (img/video)
};
enum timelinetype {
    home,
    user,
    mentions,
};

struct t_entity {
    enum entitytype type; //entity type
    uint8_t index_s,index_e; //indices.
    char* text; //text for #$, screen_name for @, url for urls/media
    char* name; //name for @s, display_url for urls/media
    char* url; //expanded_url for urls/media.
    uint64_t id; //ID for mentions and media
};

struct t_account {
    //this structure refers to twitter log-in accounts
    char name[16]; //acct screen name
    uint64_t userid; //acct user id
    char tkey[128]; //token key
    char tsct[128]; //token secret
    uint8_t auth; //is authorized(access token) or not(req token)?
    struct btree* timelinebt; //timeline btree.
    struct btree* userbt; //user btree.
    struct btree* mentionbt; //mention btree.
};
struct t_timeline {
    struct t_timeline* next;
};
struct t_contributor {
    uint64_t id;
    char screen_name[16];
};

struct t_tweet {
    struct t_account* acct;
    time_t created_at; //should be time
    uint64_t id;
    char* text;
    char* source;
    int truncated; //do we need that?
    uint64_t in_reply_to_status_id;
    uint64_t in_reply_to_user_id;
    char in_reply_to_screen_name[16];
    //twitter screen names can't have unicode chars and are 15ch max.
    //struct t_user* user;
    uint64_t user_id; // not an actual field.
    //geo;
    double complex coordinates; // real = long, imag = lat.
    struct t_place* place;
    struct t_contributor** contributors;
    uint64_t retweeted_status_id;
    uint64_t retweet_count;
    uint64_t favorite_count;
    struct t_entity** entities;
    uint8_t entity_count;
    int favorited;
    int retweeted;
    //char* lang;
    int possibly_sensitive;
    int perspectival;
    time_t retrieved_on;
};
struct t_user {
    int contributors_enabled;
    time_t created_at;
    int default_profile;
    char* description;
    struct t_entity** desc_entities;
    uint8_t desc_entity_count;
    struct t_entity** url_entities;
    uint8_t url_entity_count;
    uint64_t favorites_count;
    int follow_request_sent;
    int following;
    uint32_t followers_count;
    uint32_t friends_count;
    int geo_enabled;
    uint64_t id;
    int is_translator;
    //char* lang;
    uint64_t listed_count;
    char* location;
    char* name;
    int protected_; //not the C++ keyword
    char screen_name[16];
    int show_all_inline_media;
    int status_tweet_id;
    uint64_t statuses_count;
    char* time_zone;
    char* url;
    int32_t utc_offset;
    int verified;
    char* withheld_in_countries;
    char withheld_scope[7];
    int perspectival;
    time_t retrieved_on;
};


enum collision_behavior {
    no_replace,
    replace,
    update, //replaces non-perspectival data with perspectival, and newer data with older.
};

int initStructures();

struct t_account* newAccount();

void destroyAccount(struct t_account* acct);

// oAuth authorization process here
int inithashtables();

int tht_insert(struct t_tweet* tweet, enum collision_behavior cbeh);
int tht_delete(uint64_t id);
struct t_tweet* tht_search(uint64_t id);

int uht_insert(struct t_user* user, enum collision_behavior cbeh);
int uht_delete(uint64_t id);
struct t_user* uht_search(uint64_t id);

struct t_entity* entitydup(struct t_entity* orig);
struct t_tweet* tweetdup(struct t_tweet* orig);
struct t_user* userdup(struct t_user* orig);
void entitydel(struct t_entity* ptr);
void tweetdel(struct t_tweet* ptr);
void userdel(struct t_user* ptr);

uint64_t parse_json_user(struct t_account* acct, json_object* user, int perspectival);

uint64_t parse_json_tweet(struct t_account* acct, json_object* tweet, int perspectival);

char* parse_tweet_entities(struct t_tweet* tweet);

int request_token(struct t_account* acct);
int authorize(struct t_account* acct, char** url);
int oauth_verify(struct t_account* acct, int pin);

int add_acct(struct t_account* acct);
int del_acct(struct t_account* acct);

int load_timeline(struct t_account* acct);
int load_timeline_ext(struct t_account* acct, enum timelinetype tt, int since_id, int max_id, int trim_user, int exclude_replies, int contributor_details, int include_entities);

int save_accounts();
int load_accounts();

#endif
