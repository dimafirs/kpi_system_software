#include "fs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int chdir(char *file_path){
	return 0;
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
	while((cmd[i] != 0) & (i < cmd_len)){
		if(cmd[i] != ' '){
			left = i;
			i++;
			while((cmd[i] != ' ') & (cmd[i] != 0) & (i<cmd_len)){
				i++;
			}
			if((i == cmd_len) | part == part_cnt){
				printf("Command too large\n");
				return -1;
			}
			right = i;
			memcpy(cmd_container[part], cmd+left, right - left);
			printf("string is: %s\n", cmd_container[part]);
			part++;
		}
		i++;
	}

	return part;
}
