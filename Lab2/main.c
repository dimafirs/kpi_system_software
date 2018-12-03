#include "fs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CMD_LEN 100
#define CMD_PARTS 3

static char *cmd;
static char **container;

void clear_cmds(){
	uint32_t i;
	memset(cmd, 0, CMD_LEN);
	for(i = 0; i < CMD_PARTS; i++)
		memset(container[i], 0, CMD_LEN);
}

void alloc_cmds(){
	uint32_t i;
	cmd = malloc(CMD_LEN);
	container = malloc(CMD_PARTS);
	for(i = 0; i< CMD_PARTS; i++)
		container[i] = malloc(CMD_LEN);
	clear_cmds();
}

int main(){
	uint32_t i;
	int parts;
	alloc_cmds();
	char should_stop = 0;
	char *work_dir = "/";
	printf("<<<Simple file system>>>\nEnter command below\n");
	while(!should_stop){
		printf("%s:", work_dir);
		if(!fgets(cmd, CMD_LEN, stdin))
			printf("Error while reading input command\n");
		else {
			parts = parse_cmd(container, CMD_PARTS, cmd, CMD_LEN);

			clear_cmds();
		}
	}

	return 0;
}
