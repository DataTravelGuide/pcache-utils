#ifndef PCACHE_H
#define PCACHE_H

#define PCACHE_NAME_LEN            32

struct pcache_cache {
	uint64_t magic;
	int version;
	int flags;
	unsigned int host_area_off;
	unsigned int bytes_per_host_info;
	unsigned int host_num;
	unsigned int backing_area_off;
	unsigned int bytes_per_backing_info;
	unsigned int backing_num;
	unsigned int blkdev_area_off;
	unsigned int bytes_per_blkdev_info;
	unsigned int blkdev_num;
	unsigned int segment_area_off;
	unsigned int bytes_per_segment;
	unsigned int segment_num;
	char path[PCACHE_PATH_LEN];
	unsigned int cache_id;
	unsigned int host_id;
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
	unsigned int host_id;
	char backing_path[PCACHE_PATH_LEN];
	bool alive;
	unsigned int cache_segs;
	unsigned int cache_gc_percent;
	unsigned int cache_used_segs;
	unsigned int dev_num;
	struct pcache_blkdev blkdevs[PCACHEB_BLKDEV_COUNT_MAX];
};

#endif // PCACHE_H
