#include "fs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
	FILE *fs = fopen(filesys_path, "rb");
	if(NULL == fs){
		printf("Error while opening file-system image\n");
		return -1;
	}

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
