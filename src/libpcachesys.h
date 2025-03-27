#ifndef PCACHESYS_H
#define PCACHESYS_H

#include <stdint.h>

#include "libpcache.h"

#define SYSFS_PCACHE_CACHE_REGISTER "/sys/bus/pcache/cache_dev_register"
#define SYSFS_PCACHE_CACHE_UNREGISTER "/sys/bus/pcache/cache_dev_unregister"
#define SYSFS_PCACHE_DEVICES_PATH "/sys/bus/pcache/devices/"
#define SYSFS_CACHE_BASE_PATH "/sys/bus/pcache/devices/cache_dev"

static inline void cache_info_path(int cache_id, char *buffer, size_t buffer_size)
{
	snprintf(buffer, buffer_size, "%s%u/info", SYSFS_CACHE_BASE_PATH, cache_id);
}

static inline void cache_path_path(int cache_id, char *buffer, size_t buffer_size)
{
	snprintf(buffer, buffer_size, "%s%u/path", SYSFS_CACHE_BASE_PATH, cache_id);
}

static inline void cache_host_id_path(int cache_id, char *buffer, size_t buffer_size)
{
	snprintf(buffer, buffer_size, "%s%u/host_id", SYSFS_CACHE_BASE_PATH, cache_id);
}

static inline void cache_adm_path(int cache_id, char *buffer, size_t buffer_size)
{
	/* Generate the path with cache_id */
	snprintf(buffer, buffer_size, "%s%u/adm", SYSFS_CACHE_BASE_PATH, cache_id);
}

#define PCACHESYS_PATH(OBJ, MEMBER)                                                                            \
static inline void OBJ##_##MEMBER##_path(unsigned int cache_id, unsigned int obj_id, char *buffer, size_t buffer_size) \
{                                                                                                           \
        snprintf(buffer, buffer_size, "%s%u/" #OBJ "%u/" #MEMBER, SYSFS_CACHE_BASE_PATH, cache_id, obj_id); \
}

PCACHESYS_PATH(host, alive)
PCACHESYS_PATH(host, hostname)

PCACHESYS_PATH(blkdev, host_id)
PCACHESYS_PATH(blkdev, backing_id)
PCACHESYS_PATH(blkdev, alive)
PCACHESYS_PATH(blkdev, mapped_id)

PCACHESYS_PATH(backing_dev, path)
PCACHESYS_PATH(backing_dev, mapped_id)
PCACHESYS_PATH(backing_dev, cache_segs)
PCACHESYS_PATH(backing_dev, cache_gc_percent)
PCACHESYS_PATH(backing_dev, cache_used_segs)

int pcachesys_backing_blkdevs_clear(struct pcache_cache *pcachet, unsigned int backing_id);

int pcachesys_cache_init(struct pcache_cache *pcachet, int cache_id);
int pcachesys_host_init(struct pcache_cache *pcachet, struct pcache_host *host, unsigned int host_id);
int pcachesys_blkdev_init(struct pcache_cache *pcachet, struct pcache_blkdev *blkdev, unsigned int blkdev_id);
int pcachesys_backing_init(struct pcache_cache *pcachet, struct pcache_backing *backing, unsigned int backing_id);
int pcachesys_find_backing_id_from_path(struct pcache_cache *pcachet, char *path, unsigned int *backing_id);
int pcachesys_write_value(const char *path, const char *value);

struct pcachesys_walk_ctx;
typedef int (*pcachesys_cb_t)(struct dirent *entry, struct pcachesys_walk_ctx *walk_ctx);
struct pcachesys_walk_ctx {
        char path[PCACHE_PATH_LEN];
	pcachesys_cb_t	cb;
	void		*data;
};

int walk_cache_devs(struct pcachesys_walk_ctx *walk_ctx);
int walk_backing_devs(struct pcachesys_walk_ctx *walk_ctx);

#endif // PCACHESYS_H
