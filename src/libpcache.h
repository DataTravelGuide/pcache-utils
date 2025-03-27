#ifndef PCACHE_H
#define PCACHE_H

#define PCACHE_NAME_LEN            32

struct pcache_cache {
	uint64_t magic;
	int version;
	int flags;
	unsigned int segment_num;
	char path[PCACHE_PATH_LEN];
	unsigned int cache_id;
};

struct pcache_host {
	int host_id;
	char hostname[PCACHE_NAME_LEN];
	bool alive;
};

struct pcache_blkdev {
	unsigned int blkdev_id;
	unsigned int host_id;
	unsigned int backing_id;
	char dev_name[PCACHE_NAME_LEN];
	bool alive;
};

#define PCACHEB_BLKDEV_COUNT_MAX   1

struct pcache_backing {
	unsigned int backing_id;
	char backing_path[PCACHE_PATH_LEN];
	char logic_dev_path[PCACHE_PATH_LEN];
	unsigned int cache_segs;
	unsigned int cache_gc_percent;
	unsigned int cache_used_segs;
	unsigned int logic_dev_id;
};

#endif // PCACHE_H
