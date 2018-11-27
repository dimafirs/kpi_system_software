#include "process_sim.h"


#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define PROC_NUM 10


static struct task_struct tasks[PROC_NUM] = {0};
static unsigned long sys_time = 0;

int init(void) {
	uint32_t i;

	if(init_random())
		return -1;
	if(init_memory())
		return -1;

	for(i = 0; i < PROC_NUM; i++){
		init_process(&tasks[i], i);
	}

	return 0;

}

int main(){
	int err;
	printf("Initialize...\n");
	if(err = init()){
		printf("Error occurs during initialization\n");
		exit(err);
	}

	for(uint32_t i = 0; i<1000; i++){
		for(uint32_t j = 0; j<PROC_NUM; j++){
			on_exec(&tasks[j], 10);
		}
		mem_deamon(tasks, PROC_NUM, i);
	}
	exit(0);
}
