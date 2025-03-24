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

static int blkdev_clean(unsigned int t_id, unsigned int blkdev_id)
{
        char alive_path[PCACHE_PATH_LEN];
        char adm_path[PCACHE_PATH_LEN];
        char cmd[PCACHE_PATH_LEN * 3] = { 0 };
        struct sysfs_attribute *sysattr;
        int ret;

        /* Construct the path to the 'alive' file for blkdev */
        blkdev_alive_path(t_id, blkdev_id, alive_path, sizeof(alive_path));

        /* Open and read the 'alive' status */
        sysattr = sysfs_open_attribute(alive_path);
        if (!sysattr) {
                printf("Failed to open '%s'\n", alive_path);
                return -1;
        }

        /* Read the attribute value */
        ret = sysfs_read_attribute(sysattr);
        if (ret < 0) {
                printf("Failed to read attribute '%s'\n", alive_path);
                sysfs_close_attribute(sysattr);
                return -1;
        }

        if (strncmp(sysattr->value, "true", 4) == 0) {
                sysfs_close_attribute(sysattr);
                return 0;
        }
        sysfs_close_attribute(sysattr);

        /* Construct the command and admin path */
        snprintf(cmd, sizeof(cmd), "op=dev-clear,dev_id=%u", blkdev_id);
        cache_adm_path(t_id, adm_path, sizeof(adm_path));

        /* Open the admin interface and write the command */
        sysattr = sysfs_open_attribute(adm_path);
        if (!sysattr) {
                printf("Failed to open '%s'\n", adm_path);
                return -1;
        }

        ret = sysfs_write_attribute(sysattr, cmd, strlen(cmd));
        sysfs_close_attribute(sysattr);
        if (ret != 0) {
                printf("Failed to write '%s'. Error: %s\n", cmd, strerror(ret));
        }

        return ret;
}

int pcachesys_backing_blkdevs_clear(struct pcache_cache *pcachet, unsigned int backing_id)
{
	unsigned int i;
	int ret = 0;

	for (i = 0; i < pcachet->blkdev_num; i++) {
		struct pcache_blkdev blkdev;

		// Initialize the blkdev structure
		ret = pcachesys_blkdev_init(pcachet, &blkdev, i);
		if (ret < 0) {
			continue; // Skip if initialization fails
		}

		// Check if blkdev's backing_id matches the given backing_id
		if (blkdev.backing_id == backing_id) {
			ret = blkdev_clean(pcachet->cache_id, i);
			if (ret < 0) {
				printf("Failed to clear blkdev %u\n", i);
				return ret;
			}
		}
	}

	return 0;
}

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
		} else if (strcmp(attribute, "host_area_off") == 0) {
			pcachet->host_area_off = (typeof(pcachet->host_area_off))value;
		} else if (strcmp(attribute, "bytes_per_host_info") == 0) {
			pcachet->bytes_per_host_info = (typeof(pcachet->bytes_per_host_info))value;
		} else if (strcmp(attribute, "host_num") == 0) {
			pcachet->host_num = (typeof(pcachet->host_num))value;
		} else if (strcmp(attribute, "backing_area_off") == 0) {
			pcachet->backing_area_off = (typeof(pcachet->backing_area_off))value;
		} else if (strcmp(attribute, "bytes_per_backing_info") == 0) {
			pcachet->bytes_per_backing_info = (typeof(pcachet->bytes_per_backing_info))value;
		} else if (strcmp(attribute, "backing_num") == 0) {
			pcachet->backing_num = (typeof(pcachet->backing_num))value;
		} else if (strcmp(attribute, "blkdev_area_off") == 0) {
			pcachet->blkdev_area_off = (typeof(pcachet->blkdev_area_off))value;
		} else if (strcmp(attribute, "bytes_per_blkdev_info") == 0) {
			pcachet->bytes_per_blkdev_info = (typeof(pcachet->bytes_per_blkdev_info))value;
		} else if (strcmp(attribute, "blkdev_num") == 0) {
			pcachet->blkdev_num = (typeof(pcachet->blkdev_num))value;
		} else if (strcmp(attribute, "segment_area_off") == 0) {
			pcachet->segment_area_off = (typeof(pcachet->segment_area_off))value;
		} else if (strcmp(attribute, "bytes_per_segment") == 0) {
			pcachet->bytes_per_segment = (typeof(pcachet->bytes_per_segment))value;
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

int pcachesys_host_init(struct pcache_cache *pcachet, struct pcache_host *host, unsigned int host_id)
{
	char path[PCACHE_PATH_LEN];
	FILE *file;

	// Initialize host_id
	host->host_id = host_id;

	// Read hostname
	host_hostname_path(pcachet->cache_id, host_id, path, PCACHE_PATH_LEN);
	file = fopen(path, "r");
	if (file == NULL) {
		perror("Error opening hostname file");
		return -1;
	}
	if (fgets(host->hostname, sizeof(host->hostname), file) == NULL) {
		fclose(file);
		return -1;
	}
	// Remove newline character if present
	host->hostname[strcspn(host->hostname, "\n")] = '\0';
	fclose(file);

	// Read alive status
	host_alive_path(pcachet->cache_id, host_id, path, PCACHE_PATH_LEN);
	file = fopen(path, "r");
	if (file == NULL) {
		perror("Error opening alive file");
		return -1;
	}
	char alive_str[8];
	if (fgets(alive_str, sizeof(alive_str), file) == NULL) {
		perror("Error reading alive status");
		fclose(file);
		return -1;
	}
	fclose(file);

	// Convert alive status string to boolean
	host->alive = (strcmp(alive_str, "true\n") == 0);

	return 0;
}

#define PCACHE_DEV_NAME_FORMAT "/dev/pcache%u"

int pcachesys_blkdev_init(struct pcache_cache *pcachet, struct pcache_blkdev *blkdev, unsigned int blkdev_id) {
	char path[PCACHE_PATH_LEN];
	FILE *file;
	unsigned int mapped_id;
	char buffer[16];

	// Load blkdev_id
	blkdev->blkdev_id = blkdev_id;

	// Load host_id
	blkdev_host_id_path(pcachet->cache_id, blkdev_id, path, PCACHE_PATH_LEN);
	file = fopen(path, "r");
	if (!file) {
		perror("Error opening host_id");
		return -ENOENT;
	}
	if (fscanf(file, "%u", &blkdev->host_id) != 1) {
		fclose(file);
		return -ENOENT;
	}
	fclose(file);

	// Load backing_id
	blkdev_backing_id_path(pcachet->cache_id, blkdev_id, path, PCACHE_PATH_LEN);
	file = fopen(path, "r");
	if (!file) {
		perror("Error opening backing_id");
		return -ENOENT;
	}
	if (fscanf(file, "%u", &blkdev->backing_id) != 1) {
		fclose(file);
		return -ENOENT;
	}
	fclose(file);

	// Load alive status
	blkdev_alive_path(pcachet->cache_id, blkdev_id, path, PCACHE_PATH_LEN);
	file = fopen(path, "r");
	if (!file) {
		perror("Error opening alive");
		return -ENOENT;
	}
	if (fgets(buffer, sizeof(buffer), file)) {
		// Trim newline if present
		buffer[strcspn(buffer, "\n")] = '\0';
		blkdev->alive = (strcmp(buffer, "true") == 0);
	}
	fclose(file);

	// Load mapped_id and set dev_name
	blkdev_mapped_id_path(pcachet->cache_id, blkdev_id, path, PCACHE_PATH_LEN);
	file = fopen(path, "r");
	if (!file) {
		perror("Error opening mapped_id");
		return -ENOENT;
	}
	if (fscanf(file, "%u", &mapped_id) != 1) {
		fclose(file);
		return -ENOENT;
	}
	fclose(file);
	snprintf(blkdev->dev_name, sizeof(blkdev->dev_name), PCACHE_DEV_NAME_FORMAT, mapped_id);

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

	// Read host_id
	backing_host_id_path(pcachet->cache_id, backing_id, path, PCACHE_PATH_LEN);
	ret = read_sysfs_value(path, buf, sizeof(buf));
	if (ret < 0 || buf[0] == '\0') {
		return -ENOENT; // Return if host_id is empty
	}
	backing->host_id = atoi(buf);

	// Read backing_path
	backing_path_path(pcachet->cache_id, backing_id, path, PCACHE_PATH_LEN);
	ret = read_sysfs_value(path, backing->backing_path, sizeof(backing->backing_path));
	if (ret < 0) {
		return ret;
	}

	// Read alive status
	backing_alive_path(pcachet->cache_id, backing_id, path, PCACHE_PATH_LEN);
	ret = read_sysfs_value(path, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}
	backing->alive = (strcmp(buf, "true") == 0);

	// Read cache_segs directly from the new path
	backing_cache_segs_path(pcachet->cache_id, backing_id, path, PCACHE_PATH_LEN);
	ret = read_sysfs_value(path, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}
	backing->cache_segs = (unsigned int)atoi(buf);

	// Read cache_gc_percent directly from the new path
	backing_cache_gc_percent_path(pcachet->cache_id, backing_id, path, PCACHE_PATH_LEN);
	ret = read_sysfs_value(path, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}
	backing->cache_gc_percent = (unsigned int)atoi(buf);

	backing_cache_used_segs_path(pcachet->cache_id, backing_id, path, PCACHE_PATH_LEN);
	ret = read_sysfs_value(path, buf, sizeof(buf));
	if (ret < 0) {
		return ret;
	}
	backing->cache_used_segs = (unsigned int)atoi(buf);

	// Initialize block devices
	backing->dev_num = 0;
	for (unsigned int i = 0; i < pcachet->blkdev_num; i++) {
		struct pcache_blkdev blkdev;
		ret = pcachesys_blkdev_init(pcachet, &blkdev, i);
		if (ret < 0) {
			continue;
		}

		// Check if blkdev's backing_id matches the current backing_id
		if (blkdev.backing_id == backing_id) {
			// Add to backing's blkdevs array if it matches
			if (backing->dev_num < PCACHEB_BLKDEV_COUNT_MAX) {
				memcpy(&backing->blkdevs[backing->dev_num++], &blkdev, sizeof(struct pcache_blkdev));
			} else {
				fprintf(stderr, "Warning: Exceeded max blkdev count for backing %u\n", backing_id);
				break;
			}
		}
	}

	return 0;
}

int pcachesys_find_backing_id_from_path(struct pcache_cache *pcachet, char *path, unsigned int *backing_id)
{
	int ret;

	for (unsigned int i = 0; i < pcachet->backing_num; i++) {
		struct pcache_backing backing = { 0 };

		ret = pcachesys_backing_init(pcachet, &backing, i);
		if (ret)
			continue;

		if (backing.host_id != pcachet->host_id)
			continue;

		if (strcmp(backing.backing_path, path) == 0) {
			*backing_id = backing.backing_id;
			return 0;
		}
	}

	return -ENOENT;
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
