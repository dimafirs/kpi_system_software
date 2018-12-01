#include "fs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CMD_LEN 100
int main(){
	char should_stop = 0;
	char *work_dir = "/";
	char *cmd = malloc(CMD_LEN);
	printf("<<<Simple file system>>>\nEnter command below\n");
	while(!should_stop){
		printf("%s:", work_dir);
		if(!fgets(cmd, CMD_LEN, stdin))
			printf("Error while reading input command\n");
		else {
			printf("your command is: %s", cmd);
			memset(cmd, 0, CMD_LEN);
		}
	}
}
