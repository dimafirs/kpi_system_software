#include "fs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE *file_sys;
static struct flink *root_link;
static struct flink *cur_dir;

static struct fdesc *read_fd(FILE *fd, uint32_t offset){
	struct fdesc *result = malloc(sizeof(result));
	result->blocks = malloc(sizeof(*(result->blocks)));
	if(sizeof(*(result->blocks)) !=
			fread(result->blocks, sizeof(*(result->blocks)), 1, fd))
		if(feof(fd) | ferror(fd)) {
			printf("Unsuccessful read from the file");
			goto err_handler;
		}
	result->blocks->state = 1;
	result->desc_id = result->blocks[0]->data[0];
	result->type = result->blocks[0]->data[1];
	result->size = result->blocks[0]->data[2];
	if(result->type == DIR){
		result->link_cnt = result->blocks[0]->data[3];
	}
err_handler: ;
	free(result->blocks);
	free(result);
	return NULL;
}
static int change_dir(char *file_path){
	return 0;
}

static int make_dir(char *file_path){
	return 0;
}

static int remove_dir(char *file_path){
	return 0;
}

static int mount(char *filesys_path){
	file_sys = fopen(filesys_path, "rb");
	if(NULL == file_sys){
		printf("Error while opening file-system image\n");
		return -1;
	}
	root_link = malloc(sizeof(*root_link));
	root_link->fd = read_fd(file_sys, 0);
	root_link->name = "/";
	cur_dir = root_link;
	return 0;
}

static int umount(char *filesys_path){
	return 0;
}

static int create(char *file_path){
	return 0;
}


static int list(char *file_path){
	return 0;
}

int exec_cmd(char **container, uint32_t parts, char *wd){
	if(!strcmp(container[0], "mount")){
		if(parts != 2){
			printf("Incorrect command format\n");
			goto err;
		}
		return mount(container[1]);
	}
	if(!strcmp(container[0], "umount")){
		if(parts != 2){
			printf("Incorrect command format\n");
			goto err;
		}
		return umount(container[1]);
	}
	if(!strcmp(container[0], "ls")){
		if(parts > 2){
			printf("Incorrect command format\n");
			goto err;
		}
		if(parts == 1){
			return list(wd);
		}
		if(parts == 2){
			return list(container[1]);
		}
	}
	if(!strcmp(container[0], "create")){
		if(parts != 2){
			printf("Incorrect command format\n");
			goto err;
		}
		return create(container[1]);
	}
	if(!strcmp(container[0], "open")){

	}
	if(!strcmp(container[0], "close")){

	}
	if(!strcmp(container[0], "read")){

	}
	if(!strcmp(container[0], "write")){

	}
	if(!strcmp(container[0], "link")){

	}
	if(!strcmp(container[0], "unlink")){

	}
	if(!strcmp(container[0], "truncate")){

	}
	if(!strcmp(container[0], "filestat")){

	}
	printf("Unknown command %s\n", container[0]);
	return -1;
err:
	return -1;
}

int parse_cmd(char **cmd_container, uint32_t part_cnt,
		char *cmd, uint32_t cmd_len)
{
	uint32_t i, left, right, part;
	/* Check if cmd is empty(only 'enter' symbol 10),
	 * null with with 0 length passed */
	if((cmd[0] == 10) | (!cmd) | (!cmd_len))
		return -1;
	i = 0;
	part = 0;
	char *cmd_part;
	while((cmd[i] != 0) & (i < cmd_len)){
		if(cmd[i] != ' '){
			left = i;
			i++;
			/* Checks space, enter and zero symbol */
			while((cmd[i] != ' ')& (cmd[i] != 10) & (cmd[i] != 0) & (i<cmd_len)){
				i++;
			}
			if((i == cmd_len) | part == part_cnt){
				printf("Command too large\n");
				return -1;
			}
			right = i;
			cmd_part = malloc(right-left);
			memcpy(cmd_part, cmd+left, right-left);
			cmd_container[part] = cmd_part;
			part++;
		}
		i++;
	}

	return part;
}
