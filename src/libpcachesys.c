#include <stdio.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <sysfs/libsysfs.h>

#include "pcache.h"
#include "libpcachesys.h"

int pcachesys_cache_init(struct pcache_cache *pcachet, int cache_id) {
	char path[PCACHE_PATH_LEN];
	FILE *file;
	char attribute[64];
	char value_str[64];;
	uint64_t value;
	int ret;

	pcachet->cache_id = cache_id;
	/* Construct the file path */
	cache_info_path(cache_id, path, PCACHE_PATH_LEN);

	/* Open the file */
	file = fopen(path, "r");
	if (!file) {
		//printf("failed to open %s\n", path);
		return -errno;  // Return error code
	}

	/* Read and parse each line */
	while (fscanf(file, "%63[^:]: %63s\n", attribute, value_str) == 2) {
		/* Check if the value is in hexadecimal by looking for "0x" prefix */
		if (strncmp(value_str, "0x", 2) == 0) {
			sscanf(value_str, "%lx", &value);
		} else {
			sscanf(value_str, "%lu", &value);
		}

		if (strcmp(attribute, "magic") == 0) {
			pcachet->magic = (typeof(pcachet->magic))value;
		} else if (strcmp(attribute, "version") == 0) {
			pcachet->version = (typeof(pcachet->version))value;
		} else if (strcmp(attribute, "flags") == 0) {
			pcachet->flags = (typeof(pcachet->flags))value;
		} else if (strcmp(attribute, "segment_num") == 0) {
			pcachet->segment_num = (typeof(pcachet->segment_num))value;
		} else {
			/* Unrecognized attribute, ignore */
		}
	}
	/* Close the file */
	fclose(file);

	cache_path_path(cache_id, path, PCACHE_PATH_LEN);

	/* Open the file for reading */
	file = fopen(path, "r");
	if (file == NULL) {
		perror("Error opening file");
		return -errno;
	}

	/* Read content from file into pcachet->path */
	if (fgets(pcachet->path, PCACHE_PATH_LEN, file) == NULL) {
		if (ferror(file)) {
			perror("Error reading file");
			fclose(file);
			return -errno;
		}
	}

	/* Close the file */
	fclose(file);

	/* Ensure null-termination of the string in pcachet->path */
	pcachet->path[PCACHE_PATH_LEN - 1] = '\0';

	return 0;
}

int read_sysfs_value(const char *path, char *buf, size_t buf_len)
{
	FILE *f = fopen(path, "r");
	if (!f) return -1;

	if (fgets(buf, buf_len, f) == NULL) {
		fclose(f);
		return -1;
	}

	// Remove newline if present
	buf[strcspn(buf, "\n")] = '\0';
	fclose(f);
	return 0;
}

int pcachesys_backing_init(struct pcache_cache *pcachet, struct pcache_backing *backing, unsigned int backing_id)
{
	char path[PCACHE_PATH_LEN];
	char buf[PCACHE_PATH_LEN];
	struct stat sb;
	int ret;

	// Initialize backing_id
	backing->backing_id = backing_id;

	// Read backing_path
	backing_dev_path_path(pcachet->cache_id, backing_id, path, PCACHE_PATH_LEN);
	ret = read_sysfs_value(path, backing->backing_path, sizeof(backing->backing_path));
	if (ret < 0) {
		return ret;
	}

	// Read cache_segs directly from the new path
	backing_dev_cache_segs_path(pcachet->cache_id, backing_id, path, PCACHE_PATH_LEN);
	ret = read_sysfs_value(path, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}
	backing->cache_segs = (unsigned int)atoi(buf);

	// Read cache_gc_percent directly from the new path
	backing_dev_cache_gc_percent_path(pcachet->cache_id, backing_id, path, PCACHE_PATH_LEN);
	ret = read_sysfs_value(path, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}
	backing->cache_gc_percent = (unsigned int)atoi(buf);

	backing_dev_cache_used_segs_path(pcachet->cache_id, backing_id, path, PCACHE_PATH_LEN);
	ret = read_sysfs_value(path, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}
	backing->cache_used_segs = (unsigned int)atoi(buf);

	backing_dev_mapped_id_path(pcachet->cache_id, backing_id, path, PCACHE_PATH_LEN);
	ret = read_sysfs_value(path, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}
	backing->logic_dev_id = (unsigned int)atoi(buf);
	sprintf(backing->logic_dev_path, "/dev/pcache%u", backing->logic_dev_id);

	return 0;
}

int pcachesys_write_value(const char *path, const char *value)
{
	struct sysfs_attribute *sysattr;
	int ret;

	sysattr = sysfs_open_attribute(path);
	if (sysattr == NULL) {
		printf("failed to open %s, exit!\n", path);
		return -1;
	}

	ret = sysfs_write_attribute(sysattr, value, strlen(value));
	if (ret != 0) {
		printf("failed to write %s to %s, exit!\n", value, path);
		goto out;
	}
out:
	sysfs_close_attribute(sysattr);
	return ret;
}

int walk_cache_devs(struct pcachesys_walk_ctx *walk_ctx)
{
	unsigned int cache_dev_id;
	struct dirent *entry;
	DIR *dir;
	int ret = 0;

	dir = opendir(walk_ctx->path);
	if (!dir) {
		printf("Failed to open dir: %s, %s\n", walk_ctx->path, strerror(errno));
		ret = -1;
		goto err;
	}

        while ((entry = readdir(dir)) != NULL) {
		if (strncmp(entry->d_name, "cache_dev", strlen("cache_dev")) == 0) {
			cache_dev_id = strtoul(entry->d_name + strlen("cache_dev"), NULL, 10);
			ret = walk_ctx->cb(entry, walk_ctx);
			if (ret) {
				printf("callback failed for cache_dev%u\n", cache_dev_id);
				goto close_dir;
			}
		}
	}

close_dir:
	closedir(dir);
err:
	return ret;
}

int walk_backing_devs(struct pcachesys_walk_ctx *walk_ctx)
{
	unsigned int backing_dev_id;
	struct dirent *entry;
	DIR *dir;
	int ret = 0;

	dir = opendir(walk_ctx->path);
	if (!dir) {
		printf("Failed to open dir: %s, %s\n", walk_ctx->path, strerror(errno));
		ret = -1;
		goto err;
	}

        while ((entry = readdir(dir)) != NULL) {
		if (strncmp(entry->d_name, "backing_dev", strlen("backing_dev")) == 0) {
			backing_dev_id = strtoul(entry->d_name + strlen("backing_dev"), NULL, 10);
			ret = walk_ctx->cb(entry, walk_ctx);
			if (ret) {
				printf("callback failed for backing_dev%u\n", backing_dev_id);
				goto close_dir;
			}
		}
	}

close_dir:
	closedir(dir);
err:
	return ret;
}
