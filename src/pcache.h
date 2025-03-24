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
#define PCACHE_HOST_LIST "host-list"
#define PCACHE_BACKEND_START "backing-start"
#define PCACHE_BACKEND_STOP "backing-stop"
#define PCACHE_BACKEND_LIST "backing-list"
#define PCACHE_DEV_START "dev-start"
#define PCACHE_DEV_STOP "dev-stop"
#define PCACHE_DEV_LIST "dev-list"

#define PCACHE_BACKEND_HANDLERS_MAX 128

enum PCACHE_CMD_TYPE {
	CCT_CACHE_START	= 0,
	CCT_CACHE_STOP,
	CCT_CACHE_LIST,
	CCT_HOST_LIST,
	CCT_BACKEND_START,
	CCT_BACKEND_STOP,
	CCT_BACKEND_LIST,
	CCT_DEV_START,
	CCT_DEV_STOP,
	CCT_DEV_LIST,
	CCT_INVALID,
};

/* Defines the pcache command line allowed options struct */
struct pcache_options
{
	enum PCACHE_CMD_TYPE	co_cmd;
	char 			co_host[PCACHE_PATH_LEN];
	char			co_path[PCACHE_PATH_LEN];
	bool			co_force;
	bool			co_format;
	unsigned int		co_cache_size;
	unsigned int		co_handlers;
	unsigned int		co_cache_id;
	unsigned int		co_backing_id;
	unsigned int		co_dev_id;
	bool			co_start_dev;
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
	{PCACHE_HOST_LIST, CCT_HOST_LIST},
	{PCACHE_BACKEND_START, CCT_BACKEND_START},
	{PCACHE_BACKEND_STOP, CCT_BACKEND_STOP},
	{PCACHE_BACKEND_LIST, CCT_BACKEND_LIST},
	{PCACHE_DEV_START, CCT_DEV_START},
	{PCACHE_DEV_STOP, CCT_DEV_STOP},
	{PCACHE_DEV_LIST, CCT_DEV_LIST},
	{"", CCT_INVALID},
};


/* Public functions section */

enum PCACHE_CMD_TYPE pcache_get_cmd_type(char *cmd_str);

void pcache_options_parser(int argc, char* argv[], pcache_opt_t* options);

int pcache_cache_start(pcache_opt_t *options);
int pcache_cache_stop(pcache_opt_t *opt);
int pcache_cache_list(pcache_opt_t *opt);
int pcache_host_list(pcache_opt_t *opt);
int pcache_backing_start(pcache_opt_t *options);
int pcache_backing_stop(pcache_opt_t *options);
int pcache_backing_list(pcache_opt_t *options);
int pcache_dev_start(pcache_opt_t *options);
int pcache_dev_stop(pcache_opt_t *options);
int pcache_dev_list(pcache_opt_t *options);

#endif // PCACHECTRL_H
