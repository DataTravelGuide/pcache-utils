#ifndef PCACHECTRL_H
#define PCACHECTRL_H

#include <stdbool.h>
#include <stdint.h>
#include <getopt.h>

/* Max size of a file name */
#define PCACHE_PATH_LEN 256
#define PCACHE_CACHE_MAX       1024                        /* Maximum number of cache instances */

#define PCACHE_CACHE_START "cache-start"
#define PCACHE_CACHE_STOP "cache-stop"
#define PCACHE_CACHE_LIST "cache-list"
#define PCACHE_BACKING_START "backing-start"
#define PCACHE_BACKING_STOP "backing-stop"
#define PCACHE_BACKING_LIST "backing-list"

#define PCACHE_BACKING_HANDLERS_MAX 128

enum PCACHE_CMD_TYPE {
	CCT_CACHE_START	= 0,
	CCT_CACHE_STOP,
	CCT_CACHE_LIST,
	CCT_BACKING_START,
	CCT_BACKING_STOP,
	CCT_BACKING_LIST,
	CCT_INVALID,
};

/* Defines the pcache command line allowed options struct */
struct pcache_options
{
	enum PCACHE_CMD_TYPE	co_cmd;
	char			co_path[PCACHE_PATH_LEN];
	bool			co_force;
	bool			co_format;
	unsigned int		co_cache_size;
	unsigned int		co_cache_id;
	unsigned int		co_backing_id;
	unsigned int		co_dev_id;
	unsigned int		co_queues;
	bool			co_all;
};

/* Exports options as a global type */
typedef struct pcache_options pcache_opt_t;

typedef struct {
	const char           *cmd_name;
	enum PCACHE_CMD_TYPE  cmd_type;
} pcache_cmd_t;

static pcache_cmd_t pcache_cmd_tables[] = {
	{PCACHE_CACHE_START, CCT_CACHE_START},
	{PCACHE_CACHE_STOP, CCT_CACHE_STOP},
	{PCACHE_CACHE_LIST, CCT_CACHE_LIST},
	{PCACHE_BACKING_START, CCT_BACKING_START},
	{PCACHE_BACKING_STOP, CCT_BACKING_STOP},
	{PCACHE_BACKING_LIST, CCT_BACKING_LIST},
	{"", CCT_INVALID},
};


/* Public functions section */

enum PCACHE_CMD_TYPE pcache_get_cmd_type(char *cmd_str);

void pcache_options_parser(int argc, char* argv[], pcache_opt_t* options);

int pcache_cache_start(pcache_opt_t *options);
int pcache_cache_stop(pcache_opt_t *opt);
int pcache_cache_list(pcache_opt_t *opt);
int pcache_backing_start(pcache_opt_t *options);
int pcache_backing_stop(pcache_opt_t *options);
int pcache_backing_list(pcache_opt_t *options);

#define PCACHE_NAME_LEN            32

struct pcache_cache {
	uint64_t magic;
	int version;
	int flags;
	unsigned int segment_num;
	char path[PCACHE_PATH_LEN];
	unsigned int cache_id;
};

struct pcache_backing {
	unsigned int backing_id;
	char backing_path[PCACHE_PATH_LEN];
	char logic_dev_path[PCACHE_PATH_LEN];
	unsigned int cache_segs;
	unsigned int cache_gc_percent;
	unsigned int cache_used_segs;
	unsigned int logic_dev_id;
};

#endif // PCACHECTRL_H
