#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <jansson.h>

#include "pcache.h"
#include "libpcachesys.h"

#define PCACHE_PROGRAM_NAME "pcache"

static void usage ()
{
	fprintf(stdout, "usage: %s <command> [<args>]\n\n", PCACHE_PROGRAM_NAME);
	fprintf(stdout, "Description:\n");
	fprintf(stdout, "   pcache-utils: userspace tools to manage PCACHE\n");
	fprintf(stdout, "   See the documentation for details on PCACHE:\n");
	fprintf(stdout, "   https://datatravelguide.github.io/dtg-blog/pcache/pcache.html\n\n");

	fprintf(stdout, "These are common pcache commands used in various situations:\n\n");

	fprintf(stdout, "Managing cache device:\n");
	fprintf(stdout, "   cache-start     Register a new cache device\n");
	fprintf(stdout, "                   -p, --path <path>            Specify cache device path\n");
	fprintf(stdout, "                   -f, --format                 Format path (default: false)\n");
	fprintf(stdout, "                   -F, --force                  Force format path (default: false)\n");
	fprintf(stdout, "                   -h, --help                   Print this help message\n");
	fprintf(stdout, "                   Example: %s cache-start -p /path -F -f\n\n", PCACHE_PROGRAM_NAME);

	fprintf(stdout, "   cache-stop      Unregister a cache\n");
	fprintf(stdout, "                   -c, --cache <cid>            Specify cache ID\n");
	fprintf(stdout, "                   -h, --help                   Print this help message\n");
	fprintf(stdout, "                   Example: %s cache-stop --cache 0\n\n", PCACHE_PROGRAM_NAME);

	fprintf(stdout, "   cache-list      List all cache\n");
	fprintf(stdout, "                   -h, --help                   Print this help message\n");
	fprintf(stdout, "                   Example: %s cache-list\n\n", PCACHE_PROGRAM_NAME);

	fprintf(stdout, "Managing backings:\n");
	fprintf(stdout, "   backing-start   Start a backing\n");
	fprintf(stdout, "                   -c, --cache <cid>        Specify cache ID\n");
	fprintf(stdout, "                   -p, --path <path>            Specify backing path\n");
	fprintf(stdout, "                   -q, --queues <queues>        number of queues\n");
	fprintf(stdout, "                   -s, --cache-size <size>      Set cache size (units: K, M, G)\n");
	fprintf(stdout, "                   -h, --help                   Print this help message\n");
	fprintf(stdout, "                   Example: %s backing-start -p /path -s 512M \n\n", PCACHE_PROGRAM_NAME);

	fprintf(stdout, "   backing-stop    Stop a backing\n");
	fprintf(stdout, "                   -c, --cache <cid>        Specify cache ID\n");
	fprintf(stdout, "                   -b, --backing <bid>          Specify backing ID\n");
	fprintf(stdout, "                   -h, --help                   Print this help message\n");
	fprintf(stdout, "                   Example: %s backing-stop --backing 0\n\n", PCACHE_PROGRAM_NAME);

	fprintf(stdout, "   backing-list    List all backings \n");
	fprintf(stdout, "                   -c, --cache <cid>        Specify cache ID\n");
	fprintf(stdout, "                   -h, --help                   Print this help message\n");
	fprintf(stdout, "                   Example: %s backing-list\n\n", PCACHE_PROGRAM_NAME);
}

static void pcache_options_init(pcache_opt_t* options)
{
	memset(options, 0, sizeof(pcache_opt_t));
}

enum PCACHE_CMD_TYPE pcache_get_cmd_type(char *cmd_str)
{
	int i;

	if (!cmd_str || (strlen(cmd_str) == 0)) {
		return CCT_INVALID;
	}

	for (i = 0; i <= CCT_INVALID; i++) {
		pcache_cmd_t cmd = pcache_cmd_tables[i];
		if (!strncmp(cmd_str, cmd.cmd_name, strlen(cmd.cmd_name))) {
			return cmd.cmd_type;
		}
	}
	return CCT_INVALID;
}

/* pcache options */
static struct option long_options[] =
{
	{"help", no_argument,0, 'h'},
	{"cache", required_argument,0, 'c'},
	{"backing", required_argument,0, 'b'},
	{"path", required_argument,0, 'p'},
	{"queues", required_argument,0, 'q'},
	{"format", no_argument, 0, 'f'},
	{"cache-size", required_argument,0, 's'},
	{"force", no_argument, 0, 'F'},
	{0, 0, 0, 0},
};

unsigned int opt_to_MB(const char *input)
{
	char *endptr;
	unsigned long size = strtoul(input, &endptr, 10);

	/* Convert to MiB based on unit suffix */
	if (*endptr != '\0') {
		if (strcasecmp(endptr, "K") == 0 || strcasecmp(endptr, "KiB") == 0) {
			size = (size + 1023) / 1024;  /* Convert KiB to MiB, rounding up */
		} else if (strcasecmp(endptr, "M") == 0 || strcasecmp(endptr, "MiB") == 0) {
			/* Already in MiB, no conversion needed */
		} else if (strcasecmp(endptr, "G") == 0 || strcasecmp(endptr, "GiB") == 0) {
			size *= 1024;  /* Convert GiB to MiB */
		} else {
			fprintf(stderr, "Invalid unit for cache size: %s\n", endptr);
			exit(EXIT_FAILURE);
		}
	} else {
		/* Assume bytes if no unit; convert to MiB, rounding up */
		size = (size + (1024 * 1024 - 1)) / (1024 * 1024);
	}

	return (unsigned int)size;
}

/*
 * Public function that loops until command line options were parsed
 */
void pcache_options_parser(int argc, char* argv[], pcache_opt_t* options)
{
	int arg; /* Current option */

	if (argc < 2) {
		usage();
		exit(1);
	}
	pcache_options_init(options);
	options->co_cmd = pcache_get_cmd_type(argv[1]);
	options->co_backing_id = UINT_MAX;
	options->co_dev_id = UINT_MAX;
	options->co_cache_id = 0;
	options->co_queues = 1;

	if (options->co_cmd == CCT_INVALID) {
		usage();
		exit(1);
	}

	while (true) {
		int option_index = 0;

		arg = getopt_long(argc, argv, "a:h:c:H:b:d:p:q:f:s:n:D:F", long_options, &option_index);
		/* End of the options? */
		if (arg == -1) {
			break;
		}

		/* Find the matching case of the argument */
		switch (arg) {
		case 'h':
			usage();
			exit(EXIT_SUCCESS);
		case 'c':
			options->co_cache_id = strtoul(optarg, NULL, 10);
			break;
		case 'f':
			options->co_format = true;
			break;
		case 'F':
			options->co_force = true;
			break;
		case 'b':
			options->co_backing_id = strtoul(optarg, NULL, 10);
			break;
		case 'd':
			options->co_dev_id = strtoul(optarg, NULL, 10);
			break;
		case 'p':
			printf("p: %s\n", optarg);
			if (!optarg || (strlen(optarg) == 0)) {
				printf("path is null or empty!!\n");
				usage();
				exit(EXIT_FAILURE);
			}

			strncpy(options->co_path, optarg, sizeof(options->co_path) - 1);
			break;
		case 'q':
			options->co_queues = strtoul(optarg, NULL, 10);
			break;
		case 's':
			options->co_cache_size = opt_to_MB(optarg);
			break;
		case '?':
			usage();
			exit(EXIT_FAILURE);
		default:
			printf("unrecognized optarg: %s", optarg);
			usage();
			exit(1);
		}
	}
}

void trim_newline(char *str) {
	size_t len = strlen(str);
	if (len > 0 && str[len - 1] == '\n') {
		str[len - 1] = '\0';
	}
}

json_t *pcache_cache_to_json(struct pcache_cache *pcache_cache) {
	/* Create a new JSON object */
	json_t *json_obj = json_object();

	/* Remove trailing newline from path */
	trim_newline(pcache_cache->path);

	/* Format magic as a hexadecimal string */
	char magic_str[19]; // 16 digits + "0x" prefix + null terminator
	snprintf(magic_str, sizeof(magic_str), "0x%016lx", pcache_cache->magic);

	char flags_str[11]; // 8 digits + "0x" prefix + null terminator
	snprintf(flags_str, sizeof(flags_str), "0x%08x", pcache_cache->flags);

	/* Add each field to the JSON object */
	json_object_set_new(json_obj, "magic", json_string(magic_str));

	json_object_set_new(json_obj, "version", json_integer(pcache_cache->version));
	json_object_set_new(json_obj, "flags", json_string(flags_str));
	json_object_set_new(json_obj, "segment_num", json_integer(pcache_cache->segment_num));
	json_object_set_new(json_obj, "cache_id", json_integer(pcache_cache->cache_id));

	/* Add path as a JSON string */
	json_object_set_new(json_obj, "path", json_string(pcache_cache->path));

	return json_obj;
}

int pcache_cache_start(pcache_opt_t *opt)
{
	int ret = 0;
	char tr_buff[PCACHE_PATH_LEN*3] = {0};
	struct sysfs_attribute *sysattr = NULL;

	if (strlen(opt->co_path) == 0) {
		printf("path is null!\n");
		return -EINVAL;
	}

	printf("path=%s,force=%d,format=%d",
		opt->co_path, opt->co_force, opt->co_format);

	sprintf(tr_buff, "path=%s,force=%d,format=%d",
		opt->co_path, opt->co_force, opt->co_format);

	return pcachesys_write_value(SYSFS_PCACHE_CACHE_REGISTER, tr_buff);
}

int pcache_cache_stop(pcache_opt_t *opt)
{
	int ret = 0;
	char tr_buff[PCACHE_PATH_LEN*3] = {0};

	sprintf(tr_buff, "cache_dev_id=%u", opt->co_cache_id);

	return pcachesys_write_value(SYSFS_PCACHE_CACHE_UNREGISTER, tr_buff);
}

static int cache_dev_list_cb(struct dirent *entry, struct pcachesys_walk_ctx *walk_ctx)
{
	unsigned int cache_dev_id;
	struct pcache_cache pcache_cache;
	json_t *array = walk_ctx->data;
	json_t *json_obj;
	int ret;

	cache_dev_id = strtoul(entry->d_name + strlen("cache_dev"), NULL, 10);
	ret = pcachesys_cache_init(&pcache_cache, cache_dev_id);
	if (ret)
		return ret;

	json_obj = pcache_cache_to_json(&pcache_cache);
	json_array_append_new(array, json_obj);

	return 0;
}

int pcache_cache_list(pcache_opt_t *opt)
{
	char tr_buff[PCACHE_PATH_LEN * 3] = {0};
	struct pcache_cache pcache_cache;
	json_t *array = json_array();
	int ret = 0;
	struct pcachesys_walk_ctx walk_ctx = { 0 };

	walk_ctx.cb = cache_dev_list_cb;
	walk_ctx.data = array;
	strcpy(walk_ctx.path, SYSFS_PCACHE_DEVICES_PATH);
	walk_cache_devs(&walk_ctx);

	// Print the JSON array
	char *json_str = json_dumps(array, JSON_INDENT(4));
	printf("%s\n", json_str);

	// Clean up
	json_decref(array);
	free(json_str);

	return ret;
}

int pcache_backing_start(pcache_opt_t *options) {
	char adm_path[PCACHE_PATH_LEN];
	char cmd[PCACHE_PATH_LEN * 3] = { 0 };
	struct pcache_cache pcache_cache;
	unsigned int backing_id;
	int ret;

	pcachesys_cache_init(&pcache_cache, options->co_cache_id);

	snprintf(cmd, sizeof(cmd), "op=backing-start,path=%s,queues=%u", options->co_path, options->co_queues);

	if (options->co_cache_size != 0)
	    snprintf(cmd + strlen(cmd), sizeof(cmd) - strlen(cmd), ",cache_size=%u", options->co_cache_size);

	cache_adm_path(options->co_cache_id, adm_path, sizeof(adm_path));
	ret = pcachesys_write_value(adm_path, cmd);
	if (ret)
		return ret;

	return 0;
}

int pcache_backing_stop(pcache_opt_t *options) {
	struct pcache_cache pcache_cache;
	char adm_path[PCACHE_PATH_LEN];
	char cmd[PCACHE_PATH_LEN * 3] = { 0 };
	int ret;

	if (options->co_backing_id == UINT_MAX) {
		printf("--backing required for backing-stop command\n");
		return -EINVAL;
	}

	ret = pcachesys_cache_init(&pcache_cache, options->co_cache_id);
	if (ret) {
		printf("tranposrt for id %u not found.", options->co_cache_id);
		return ret;
	}

	snprintf(cmd, sizeof(cmd), "op=backing-stop,backing_id=%u", options->co_backing_id);

	cache_adm_path(options->co_cache_id, adm_path, sizeof(adm_path));
	return pcachesys_write_value(adm_path, cmd);
}

static struct backing_list_ctx_data {
	json_t *array;
	struct pcache_cache *pcache_cache;
};

static int backing_dev_list_cb(struct dirent *entry, struct pcachesys_walk_ctx *walk_ctx)
{
	unsigned int backing_dev_id;
	struct backing_list_ctx_data *data = walk_ctx->data;
	json_t *array = data->array;
	struct pcache_cache *pcache_cache = data->pcache_cache;
	struct pcache_backing backing;
	json_t *json_obj;
	int ret;

	backing_dev_id = strtoul(entry->d_name + strlen("backing_dev"), NULL, 10);
	ret = pcachesys_backing_init(pcache_cache, &backing, backing_dev_id); // Initialize current backing
	if (ret < 0) {
		printf("failed to init backing: %s\n", strerror(ret));
		return ret;
	}

	json_t *json_backing = json_object();
	json_object_set_new(json_backing, "backing_id", json_integer(backing.backing_id));
	json_object_set_new(json_backing, "backing_path", json_string(backing.backing_path));
	json_object_set_new(json_backing, "cache_segs", json_integer(backing.cache_segs));
	json_object_set_new(json_backing, "cache_gc_percent", json_integer(backing.cache_gc_percent));
	json_object_set_new(json_backing, "cache_used_segs", json_integer(backing.cache_used_segs));
	json_object_set_new(json_backing, "logic_dev", json_string(backing.logic_dev_path));

	json_array_append_new(array, json_backing);

	return 0;
}

int pcache_backing_list(pcache_opt_t *options)
{
	struct pcache_cache pcache_cache;
	struct pcachesys_walk_ctx walk_ctx = { 0 };
	struct backing_list_ctx_data ctx_data = { 0 };

	json_t *array = json_array(); // Create JSON array for backings
	if (array == NULL) {
		fprintf(stderr, "Error creating JSON array\n");
		return -1;
	}

	int ret = pcachesys_cache_init(&pcache_cache, options->co_cache_id);
	if (ret < 0) {
		json_decref(array);
		return ret;
	}

	ctx_data.array = array;
	ctx_data.pcache_cache = &pcache_cache;

	walk_ctx.data = &ctx_data;
	walk_ctx.cb = backing_dev_list_cb;
	sprintf(walk_ctx.path, SYSFS_CACHE_BASE_PATH"%u", options->co_cache_id);


	ret = walk_backing_devs(&walk_ctx);
	if (ret)
		return ret;

	// Convert JSON array to a formatted string and print to stdout
	char *json_str = json_dumps(array, JSON_INDENT(4));
	if (json_str != NULL) {
		printf("%s\n", json_str);
		free(json_str);
	}

	json_decref(array); // Free JSON array memory
	return 0;
}
