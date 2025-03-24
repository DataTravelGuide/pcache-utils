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
	fprintf(stdout, "   pcache-utils: userspace tools to manage PCACHE (CXL Block Device)\n");
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
	fprintf(stdout, "                   -c, --cache-size <size>      Set cache size (units: K, M, G)\n");
	fprintf(stdout, "                   -h, --help                   Print this help message\n");
	fprintf(stdout, "                   Example: %s backing-start -p /path -c 512M -n 1\n\n", PCACHE_PROGRAM_NAME);

	fprintf(stdout, "   backing-stop    Stop a backing\n");
	fprintf(stdout, "                   -c, --cache <cid>        Specify cache ID\n");
	fprintf(stdout, "                   -b, --backing <bid>          Specify backing ID\n");
	fprintf(stdout, "                   -F, --force                  Force stop backing\n");
	fprintf(stdout, "                   -h, --help                   Print this help message\n");
	fprintf(stdout, "                   Example: %s backing-stop --backing 0\n\n", PCACHE_PROGRAM_NAME);

	fprintf(stdout, "   backing-list    List all backings \n");
	fprintf(stdout, "                   -c, --cache <cid>        Specify cache ID\n");
	fprintf(stdout, "                   -a, --all                    List backings on all hosts\n");
	fprintf(stdout, "                   -h, --help                   Print this help message\n");
	fprintf(stdout, "                   Example: %s backing-list\n\n", PCACHE_PROGRAM_NAME);

	fprintf(stdout, "Managing block devices:\n");
	fprintf(stdout, "   dev-start       Start a block device\n");
	fprintf(stdout, "                   -c, --cache <cid>        Specify cache ID\n");
	fprintf(stdout, "                   -b, --backing <bid>          Specify backing ID\n");
	fprintf(stdout, "                   -h, --help                   Print this help message\n");
	fprintf(stdout, "                   Example: %s dev-start --backing 0\n\n", PCACHE_PROGRAM_NAME);

	fprintf(stdout, "   dev-stop        Stop a block device\n");
	fprintf(stdout, "                   -c, --cache <cid>        Specify cache ID\n");
	fprintf(stdout, "                   -d, --dev <dev_id>           Specify device ID\n");
	fprintf(stdout, "                   -h, --help                   Print this help message\n");
	fprintf(stdout, "                   Example: %s dev-stop --dev 0\n\n", PCACHE_PROGRAM_NAME);

	fprintf(stdout, "   dev-list        List all blkdevs on this host\n");
	fprintf(stdout, "                   -c, --cache <cid>        Specify cache ID\n");
	fprintf(stdout, "                   -a, --all                    List blkdevs on all hosts\n");
	fprintf(stdout, "                   -h, --help                   Print this help message\n");
	fprintf(stdout, "                   Example: %s blkdev-list\n\n", PCACHE_PROGRAM_NAME);
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

	printf("cmd : %s\n", cmd_str);
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
	{"start-dev", no_argument, 0, 'D'},
	{"dev", required_argument,0, 'd'},
	{"path", required_argument,0, 'p'},
	{"format", no_argument, 0, 'f'},
	{"cache-size", required_argument,0, 'c'},
	{"handlers", required_argument,0, 'n'},
	{"force", no_argument, 0, 'F'},
	{"all", no_argument, 0, 'a'},
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
	options->co_handlers = UINT_MAX;
	options->co_cache_id = 0;

	if (options->co_cmd == CCT_INVALID) {
		usage();
		exit(1);
	}

	while (true) {
		int option_index = 0;

		arg = getopt_long(argc, argv, "a:h:c:H:b:d:p:f:c:n:D:F", long_options, &option_index);
		/* End of the options? */
		if (arg == -1) {
			break;
		}

		/* Find the matching case of the argument */
		switch (arg) {
		case 'h':
			usage();
			exit(EXIT_SUCCESS);
		case 't':
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
		case 'c':
			options->co_cache_size = opt_to_MB(optarg);
			break;

		case 'n':
			options->co_handlers = strtoul(optarg, NULL, 10);
			if (options->co_handlers > PCACHE_BACKEND_HANDLERS_MAX) {
				printf("Handlers exceed maximum of %d!\n", PCACHE_BACKEND_HANDLERS_MAX);
				usage();
				exit(EXIT_FAILURE);
			}
			break;
		case 'a':
			options->co_all = true;
			break;
		case 'D':
			options->co_start_dev = true;
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
	json_object_set_new(json_obj, "backing_area_off", json_integer(pcache_cache->backing_area_off));
	json_object_set_new(json_obj, "bytes_per_backing_info", json_integer(pcache_cache->bytes_per_backing_info));
	json_object_set_new(json_obj, "backing_num", json_integer(pcache_cache->backing_num));
	json_object_set_new(json_obj, "blkdev_area_off", json_integer(pcache_cache->blkdev_area_off));
	json_object_set_new(json_obj, "bytes_per_blkdev_info", json_integer(pcache_cache->bytes_per_blkdev_info));
	json_object_set_new(json_obj, "blkdev_num", json_integer(pcache_cache->blkdev_num));
	json_object_set_new(json_obj, "segment_area_off", json_integer(pcache_cache->segment_area_off));
	json_object_set_new(json_obj, "bytes_per_segment", json_integer(pcache_cache->bytes_per_segment));
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

int pcache_cache_list(pcache_opt_t *opt)
{
	char tr_buff[PCACHE_PATH_LEN * 3] = {0};
	struct pcache_cache pcache_cache;
	json_t *array = json_array();
	int ret = 0;
	printf("into cache-list\n");

	for (int i = 0; i < PCACHE_CACHE_MAX; i++) {
		ret = pcachesys_cache_init(&pcache_cache, i);
		if (ret == -ENOENT) {
			ret = 0;
			break;
		}

		if (ret < 0) {
			json_decref(array);
			return ret;
		}

		json_t *json_obj = pcache_cache_to_json(&pcache_cache);
		json_array_append_new(array, json_obj);
	}
	// Print the JSON array
	char *json_str = json_dumps(array, JSON_INDENT(4));
	printf("%s\n", json_str);

	// Clean up
	json_decref(array);
	free(json_str);

	return ret;
}

static int dev_start(unsigned int cache_id, unsigned int backing_id)
{
	char adm_path[PCACHE_PATH_LEN];
	char cmd[PCACHE_PATH_LEN * 3] = { 0 };
	struct pcache_cache pcache_cache;
	struct pcache_backing old_backing, new_backing;
	bool found = false;
	struct pcache_blkdev *found_dev = NULL;
	int ret;

	ret = pcachesys_cache_init(&pcache_cache, cache_id);
	if (ret)
		return ret;

	/* Clear block devices associated with the backing */
	ret = pcachesys_backing_blkdevs_clear(&pcache_cache, backing_id);
	if (ret)
		return ret;

	/* Get information about the current backing */
	ret = pcachesys_backing_init(&pcache_cache, &old_backing, backing_id);
	if (ret) {
		printf("Failed to get current backing information. Error: %d\n", ret);
		return ret;
	}

	/* Prepare the dev-start command */
	snprintf(cmd, sizeof(cmd), "op=dev-start,backing_id=%u", backing_id);

	/* Get the sysfs attribute path */
	cache_adm_path(cache_id, adm_path, sizeof(adm_path));

	ret = pcachesys_write_value(adm_path, cmd);
	if (ret)
		return ret;

	/* Get information about the backing after dev-start */
	ret = pcachesys_backing_init(&pcache_cache, &new_backing, backing_id);
	if (ret) {
		printf("Failed to get new backing information. Error: %d\n", ret);
		return ret;
	}

	/* Compare old_backing and new_backing to identify new block devices */
	if (new_backing.dev_num > old_backing.dev_num) {
		for (unsigned int i = 0; i < new_backing.dev_num; i++) {
			for (unsigned int j = 0; j < old_backing.dev_num; j++) {
				if (new_backing.blkdevs[i].blkdev_id == old_backing.blkdevs[j].blkdev_id)
					goto next;
				break;
			}

			found_dev = &new_backing.blkdevs[i];
			break;
next:
			continue;
		}
	}

	if (!found_dev) {
		printf("No new block devices were added.\n");
		return 1;
	}

	printf("%s\n", found_dev->dev_name);
	return 0;
}

int pcache_backing_start(pcache_opt_t *options) {
	char adm_path[PCACHE_PATH_LEN];
	char cmd[PCACHE_PATH_LEN * 3] = { 0 };
	struct pcache_cache pcache_cache;
	unsigned int backing_id;
	int ret;

	pcachesys_cache_init(&pcache_cache, options->co_cache_id);

	snprintf(cmd, sizeof(cmd), "op=backing-start,path=%s", options->co_path);

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

int pcache_backing_list(pcache_opt_t *options)
{
	struct pcache_cache pcache_cache;
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

	// Iterate through all backings and generate JSON object for each
	for (unsigned int i = 0; i < pcache_cache.backing_num; i++) {
		struct pcache_backing backing;
		ret = pcachesys_backing_init(&pcache_cache, &backing, i); // Initialize current backing
		if (ret < 0) {
			continue;
		}

		if (!options->co_all && backing.host_id != pcache_cache.host_id)
			continue;

		// Create JSON object and add fields for the backing
		json_t *json_backing = json_object();
		json_object_set_new(json_backing, "backing_id", json_integer(backing.backing_id));
		json_object_set_new(json_backing, "host_id", json_integer(backing.host_id));
		json_object_set_new(json_backing, "backing_path", json_string(backing.backing_path));
		json_object_set_new(json_backing, "alive", json_boolean(backing.alive));
		json_object_set_new(json_backing, "cache_segs", json_integer(backing.cache_segs));
		json_object_set_new(json_backing, "cache_gc_percent", json_integer(backing.cache_gc_percent));
		json_object_set_new(json_backing, "cache_used_segs", json_integer(backing.cache_used_segs));

		// Create JSON array for blkdevs within the backing
		json_t *json_blkdevs = json_array();
		for (unsigned int j = 0; j < backing.dev_num; j++) {
			struct pcache_blkdev *blkdev = &backing.blkdevs[j];

			// Create JSON object for each blkdev and add fields
			json_t *json_blkdev = json_object();
			json_object_set_new(json_blkdev, "blkdev_id", json_integer(blkdev->blkdev_id));
			json_object_set_new(json_blkdev, "host_id", json_integer(blkdev->host_id));
			json_object_set_new(json_blkdev, "backing_id", json_integer(blkdev->backing_id));
			json_object_set_new(json_blkdev, "dev_name", json_string(blkdev->dev_name));
			json_object_set_new(json_blkdev, "alive", json_boolean(blkdev->alive));

			// Append blkdev JSON object to the blkdevs JSON array
			json_array_append_new(json_blkdevs, json_blkdev);
		}

		// Add blkdevs array to the backing JSON object
		json_object_set_new(json_backing, "blkdevs", json_blkdevs);

		// Append backing JSON object to the main JSON array
		json_array_append_new(array, json_backing);
	}

	// Convert JSON array to a formatted string and print to stdout
	char *json_str = json_dumps(array, JSON_INDENT(4));
	if (json_str != NULL) {
		printf("%s\n", json_str);
		free(json_str);
	}

	json_decref(array); // Free JSON array memory
	return 0;
}

int pcache_dev_start(pcache_opt_t *options) {
	if (options->co_backing_id == UINT_MAX) {
		printf("--backing required for dev-start command\n");
		return -EINVAL;
	}

	return dev_start(options->co_cache_id, options->co_backing_id);
}

#define MAX_RETRIES 3
#define RETRY_INTERVAL 500 // in milliseconds

int pcache_dev_stop(pcache_opt_t *options) {
	char adm_path[PCACHE_PATH_LEN];
	char cmd[PCACHE_PATH_LEN * 3] = { 0 };
	int ret;
	int attempt;

	if (options->co_dev_id == UINT_MAX) {
		printf("--dev required for dev-stop command\n");
		return -EINVAL;
	}

	snprintf(cmd, sizeof(cmd), "op=dev-stop,dev_id=%u", options->co_dev_id);

	cache_adm_path(options->co_cache_id, adm_path, sizeof(adm_path));

	// Retry mechanism for sysfs_write_attribute
	for (attempt = 0; attempt < MAX_RETRIES; ++attempt) {
		ret = pcachesys_write_value(adm_path, cmd);
		if (ret == 0)
			break; // Success, exit the loop

		printf("Attempt %d/%d failed to write command '%s'. Error: %s\n",
			attempt + 1, MAX_RETRIES, cmd, strerror(ret));

		// Wait before retrying
		usleep(RETRY_INTERVAL * 1000); // Convert milliseconds to microseconds
	}

	if (ret != 0) {
		printf("Failed to write command '%s' after %d attempts. Final Error: %s\n",
			cmd, MAX_RETRIES, strerror(ret));
	}

	return ret;
}

int pcache_dev_list(pcache_opt_t *options)
{
	struct pcache_cache pcache_cache;
	json_t *array = json_array(); // Create JSON array
	if (array == NULL) {
		fprintf(stderr, "Error creating JSON array\n");
		return -1;
	}

	int ret = pcachesys_cache_init(&pcache_cache, options->co_cache_id);
	if (ret < 0) {
		json_decref(array);
		return ret;
	}

	// Iterate through all blkdevs and generate JSON object for each
	for (unsigned int i = 0; i < pcache_cache.blkdev_num; i++) {
		struct pcache_blkdev blkdev;
		ret = pcachesys_blkdev_init(&pcache_cache, &blkdev, i); // Initialize current blkdev
		if (ret < 0) {
			continue;
		}

		if (!options->co_all && blkdev.host_id != pcache_cache.host_id)
			continue;

		// Create JSON object and add fields
		json_t *json_blkdev = json_object();
		json_object_set_new(json_blkdev, "blkdev_id", json_integer(blkdev.blkdev_id));
		json_object_set_new(json_blkdev, "host_id", json_integer(blkdev.host_id));
		json_object_set_new(json_blkdev, "backing_id", json_integer(blkdev.backing_id));
		json_object_set_new(json_blkdev, "dev_name", json_string(blkdev.dev_name));
		json_object_set_new(json_blkdev, "alive", json_boolean(blkdev.alive));

		// Append JSON object to JSON array
		json_array_append_new(array, json_blkdev);
	}

	// Convert JSON array to a formatted string and print to stdout
	char *json_str = json_dumps(array, JSON_INDENT(4));
	if (json_str != NULL) {
		printf("%s\n", json_str);
		free(json_str);
	}

	json_decref(array); // Free JSON array memory
	return 0;
}
