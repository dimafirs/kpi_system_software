#ifndef _FS.H_

#include <stdint.h>
#include <stddef.h>

#define _FS.H_
#define BLOCK_SIZE 512
#define DESC_CNT 10000
#define NAME_LENGTH 80

enum file_type {
	REGULAR = 0,
	SYMLINK,
	DIR
};
struct block {
	uint8_t *data;
};

struct fdesc {
	uint32_t desc_id;
	enum file_type type;
	uint32_t link_cnt;
	uint32_t size; //in blocks
	struct block *blocks;
};

struct flink {
	char *name;
	struct fdesc *fd;
};

uint32_t open(char *file_path);
int parse_cmd(char *cmd);


#endif //"_FS.H_"
