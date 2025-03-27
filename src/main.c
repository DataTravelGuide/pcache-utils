#include <stdio.h>
#include <stdlib.h>

#include "pcache.h"

/* Function to check if a kernel module is loaded */
static int is_module_loaded(const char *module_name)
{
	char command[128];
	snprintf(command, sizeof(command), "lsmod | grep -q '^%s '", module_name);
	return system(command) == 0;
}

/* Function to load a kernel module */
static int load_module(const char *module_name)
{
	char command[128];
	snprintf(command, sizeof(command), "modprobe %s", module_name);
	return system(command);
}

static int pcache_run(pcache_opt_t *options)
{
	int ret = 0;

	/* Check if 'pcache' module is loaded */
	if (!is_module_loaded("pcache")) {
		if (load_module("pcache") != 0) {
			fprintf(stderr, "Failed to load 'pcache' module. Exiting.\n");
			return -1; /* Return an error if module cannot be loaded */
		}
	}

	switch (options->co_cmd) {
		case CCT_CACHE_START:
			ret = pcache_cache_start(options);
			break;
		case CCT_CACHE_STOP:
			ret = pcache_cache_stop(options);
			break;
		case CCT_CACHE_LIST:
			ret = pcache_cache_list(options);
			break;
		case CCT_BACKING_START:
			ret = pcache_backing_start(options);
			break;
		case CCT_BACKING_STOP:
			ret = pcache_backing_stop(options);
			break;
		case CCT_BACKING_LIST:
			ret = pcache_backing_list(options);
			break;
		default:
			printf("Unknown command: %u\n", options->co_cmd);
			ret = -1;
			break;
	}
	return ret;
}

int main (int argc, char* argv[])
{
	int ret;
	pcache_opt_t options;

	pcache_options_parser(argc, argv, &options);

	ret = pcache_run(&options);

	return ret;
}

