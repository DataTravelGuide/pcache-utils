#ifndef PCACHESYS_H
#define PCACHESYS_H

#include <stdint.h>

#include "libpcache.h"

#define SYSFS_PCACHE_CACHE_REGISTER "/sys/bus/pcache/cache_dev_register"
#define SYSFS_PCACHE_CACHE_UNREGISTER "/sys/bus/pcache/cache_dev_unregister"
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
static inline void OBJ##_##MEMBER##_path(unsigned int t_id, unsigned int obj_id, char *buffer, size_t buffer_size) \
{                                                                                                           \
        snprintf(buffer, buffer_size, "%s%u/pcache_" #OBJ "s/" #OBJ "%u/" #MEMBER, SYSFS_CACHE_BASE_PATH, t_id, obj_id); \
}

PCACHESYS_PATH(host, alive)
PCACHESYS_PATH(host, hostname)

PCACHESYS_PATH(blkdev, host_id)
PCACHESYS_PATH(blkdev, backing_id)
PCACHESYS_PATH(blkdev, alive)
PCACHESYS_PATH(blkdev, mapped_id)

PCACHESYS_PATH(backing, host_id)
PCACHESYS_PATH(backing, path)
PCACHESYS_PATH(backing, alive)
PCACHESYS_PATH(backing, cache_segs)
PCACHESYS_PATH(backing, cache_gc_percent)
PCACHESYS_PATH(backing, cache_used_segs)

int pcachesys_backing_blkdevs_clear(struct pcache_cache *pcachet, unsigned int backing_id);

int pcachesys_cache_init(struct pcache_cache *pcachet, int cache_id);
int pcachesys_host_init(struct pcache_cache *pcachet, struct pcache_host *host, unsigned int host_id);
int pcachesys_blkdev_init(struct pcache_cache *pcachet, struct pcache_blkdev *blkdev, unsigned int blkdev_id);
int pcachesys_backing_init(struct pcache_cache *pcachet, struct pcache_backing *backing, unsigned int backing_id);
int pcachesys_find_backing_id_from_path(struct pcache_cache *pcachet, char *path, unsigned int *backing_id);
int pcachesys_write_value(const char *path, const char *value);

#endif // PCACHESYS_H
