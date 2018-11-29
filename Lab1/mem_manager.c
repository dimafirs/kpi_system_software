#include "process_sim.h"


#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define PROC_NUM 10
#define CYCLES 500
#define PROC_ITER 5


static struct task_struct tasks[PROC_NUM] = {0};

int init(void) {
	uint32_t i;

	if(0 != init_random())
		return -1;
	init_memory();
	for(i = 0; i < PROC_NUM; i++){
		init_process(&tasks[i], i);
	}
	return 0;
}

int main(){
	int err = 0;
	uint32_t i, j, virt_pages_count, virt_pages_ref, virt_pages_swapped;
	printf("Initialize...\n");
	if(err = init()){
		printf("Error occurs during initialization\n");
		exit(err);
	}

	for(i = 0; i<CYCLES; i++){
		on_exec(tasks, PROC_NUM, i, PROC_ITER);
	}

	virt_pages_count = virt_pages_ref = virt_pages_swapped = 0;
	for(i = 0; i < PROC_NUM; i++){
		virt_pages_count += tasks[i].page_count;
		for(j = 0; j < tasks[i].page_count; j++){
			if(tasks[i].pages[j].state != NOREF){
				virt_pages_ref++;
				if(tasks[i].pages[j].state == INSWAP){
					virt_pages_swapped++;
				}
			}
		}
	}
	dump_stat();
	printf("\nCount of processes: %u\n", PROC_NUM);
	printf("Total number of memory operation: %u\n", PROC_NUM*PROC_ITER*CYCLES);
	printf("Total count of virtual pages: %u\n",virt_pages_count);
	printf("Count of referenced pages: %u\n", virt_pages_ref);
	printf("Pages in swap in the end of simulation: %u\n", virt_pages_swapped);
	printf("\nSuccessfully end simulation\n");
	exit(0);
}
