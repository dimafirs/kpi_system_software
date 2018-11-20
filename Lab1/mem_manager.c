#include "memory.h"

#include "process_sim.c"

#include <stdio.h>
#include <stdlib.h>

#define POOL_SIZE 200
#define PAGE_SIZE 1024
#define PROC_NUM 10

struct task_struct {
	uint32_t pid;
	virt_mem_page *pages;
	uint32_t page_count;
};

static phys_mem_page mem_pool[POOL_SIZE] = {0};
static struct task_struct tasks[PROC_NUM] = {0};
static unsigned long sys_time = 0;

int main(){
	printf("Initialize memory pool...");
	init_memory();

}

void init_memory() {
	uint32_t i;

	for(i = 0; i < POOL_SIZE; i++){
		init_phys_page(mem_pool[i], i, PAGE_SIZE);
	}

	for(i = 0; i < PROC_NUM; i++){
		init_process(&tasks[i], i);
	}
}
