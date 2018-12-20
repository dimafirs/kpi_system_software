#ifndef _FS_HEADER_
#define _FS_HEADER_

#include <stdint.h>
#include <stddef.h>

#define BLOCK_SIZE 512
#define DESC_CNT 10000
#define NAME_LENGTH 80

enum file_type {
	DIR = 0,
	REGULAR,
	SYMLINK,
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
int parse_cmd(char **cmd_container, uint32_t part_cnt, char *cmd, uint32_t cmd_len);
int exec_cmd(char **container, uint32_t parts, char *wd);


#endif //"_FS.H_"
