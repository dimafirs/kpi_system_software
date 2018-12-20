#include "fs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CMD_LEN 100
#define CMD_PARTS 4

static char *cmd;
static char **container;

void clear_cmds(){
	uint32_t i;
	memset(cmd, 0, CMD_LEN);
	/*for(i = 0; i < CMD_PARTS; i++)
		if(container[i] != NULL)
			free(container[i]);*/
}

void alloc_cmds(){
	uint32_t i;
	cmd = malloc(CMD_LEN);
	container = malloc(sizeof(container));
	memset(cmd, 0, CMD_LEN);
	memset(container, 0, sizeof(container));
}

int main(){
	uint32_t i;
	int parts;
	alloc_cmds();
	char should_stop = 0;
	char *work_dir = "";
	printf("<<<Simple file system>>>\nEnter command below\n");
	while(!should_stop){
		printf("%s:", work_dir);
		if(!fgets(cmd, CMD_LEN, stdin))
			printf("Error while reading input command\n");
		else {
			parts = parse_cmd(container, CMD_PARTS, cmd, CMD_LEN);
			if(parts>0)
				exec_cmd(container, parts, work_dir);
			clear_cmds();
		}
	}

	return 0;
}
