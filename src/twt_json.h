#ifndef TWT_JSON_H
#define TWT_JSON_H

#include <string.h>
#include <malloc.h>

#include "btree.h"
#include "globals.h"
#include "twt_struct.h"
#include "utf8.h"
#include <json-c/json.h>

uint64_t parse_json_user(json_object* user, int perspectival);

uint64_t parse_json_tweet(json_object* tweet, int perspectival);

uint64_t parse_single_tweet(char* timelinereply);
uint64_t parse_single_user(char* timelinereply);

typedef void (*stream_cb) (uint64_t id, void* ctx);

uint64_t parsestreamingmsg(char *msg, size_t msgsize);

int parse_timeline(struct btree* timeline, enum timelinetype tt, char* timelinereply);

#endif
