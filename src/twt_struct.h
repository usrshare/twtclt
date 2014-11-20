// vim: cin:sts=4:sw=4 

#include <stdint.h>
#include <time.h>

#ifndef TWT_STRUCT_H
#define TWT_STRUCT_H

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
    direct_messages,
    search,
};

struct t_entity {
    enum entitytype type; //entity type
    uint8_t index_s,index_e; //indices.
    char* text; //text for #$, screen_name for @, url for urls/media
    char* name; //name for @s, display_url for urls/media
    char* url; //expanded_url for urls/media.
    uint64_t id; //ID for mentions and media
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
    //double complex coordinates; // real = long, imag = lat.
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
struct t_user_ref {
    uint64_t user_id;
    char screen_name[16];
};
struct t_user {
    struct t_account* acct;
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
    int user_protected; //field actually named "protected", renamed to avoid collisions with the C++ keyword
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
enum eventtype {
    access_revoked,
    block,
    unblock,
    favorite,
    unfavorite,
    follow,
    unfollow,
    list_created,
    list_destroyed,
    list_updated,
    list_member_added,
    list_member_removed,
    list_user_subscribed,
    list_user_unsubscribed,
    user_update,
};
struct t_event {
    uint64_t target_user_id;
    uint64_t source_user_id;
    enum eventtype type;
    uint64_t target_object_id;
    time_t created_at;
};
enum collision_behavior {
    no_replace,
    replace,
    update, //replaces non-perspectival data with perspectival, and newer data with older.
};

struct hashtable* tweetht; //tweet cache hash table.
struct hashtable* userht; //user cache hash table.
struct hashtable* urefht; //user reference cache hash table.

int tht_insert(struct t_tweet* tweet, enum collision_behavior cbeh);
int tht_delete(uint64_t id);
struct t_tweet* tht_search(uint64_t id);

int uht_insert(struct t_user* user, enum collision_behavior cbeh);
int uht_delete(uint64_t id);
struct t_user* uht_search(uint64_t id);

int urt_insert(char* screen_name, uint64_t id, enum collision_behavior cbeh);
int urt_delete(char* screen_name);
uint64_t urt_search(char* screen_name);
uint64_t* urt_search_ptr(char* screen_name);


struct t_user* uht_search_n(char* screen_name);

struct t_entity* entitydup(struct t_entity* orig);
struct t_tweet* tweetdup(struct t_tweet* orig);
struct t_user* userdup(struct t_user* orig);
void entitydel(struct t_entity* ptr);
void tweetdel(struct t_tweet* ptr);
void userdel(struct t_user* ptr);

#endif
