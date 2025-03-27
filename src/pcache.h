#ifndef PCACHECTRL_H
#define PCACHECTRL_H

#include <stdbool.h>
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
#define PCACHE_DEV_START "dev-start"
#define PCACHE_DEV_STOP "dev-stop"
#define PCACHE_DEV_LIST "dev-list"

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

#endif // PCACHECTRL_H
