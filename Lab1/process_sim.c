#include "memory.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>

#define MAX_PAGE_NUM 30
static uint32_t global_time = 0;

int init_random(){
	uint32_t seed;
	FILE *urand = fopen("/dev/urandom", "rb");
	if (NULL == urand)
		return -1;
	if(1 != fread(&seed, sizeof(seed), 1, urand)){
		return -1;
	}
	if(ferror(urand))
		return -1;
	srand(seed);
	printf("Random initialized. Random seed = %d\n", seed);
	return 0;
}
int init_process(struct task_struct *process, uint32_t id) {
	uint32_t i, mem_size;

	process->pid = id;
	process->page_count = rand() % MAX_PAGE_NUM + 1;
	printf("Page count for pid=%u is %u\n", process->pid, process->page_count);
	mem_size = process->page_count * sizeof(virt_mem_page);
	process->pages = malloc(mem_size);
	if(NULL == process->pages)
		return -1;
	memset(process->pages, 0, mem_size);
	for(i = 0; i < process->page_count; i++)
		process->pages[i].page = NULL;
	return 0;
}

/* We must implement reference locality, so process reference to first 1/5
 * pages with probability of 90%.
 */
static uint32_t get_rand_page(uint32_t count) {
	double r;
	uint32_t divider, e;
	r = (double) rand() / RAND_MAX;
	divider = (uint32_t) (count-1)/5 + 1;
	uint32_t random = rand();
	if (r < 0.9){
		return random%divider;
	} else {
		return ((random)%(count));
	}
	return -1;
}

void on_exec(struct task_struct *task_pool, uint32_t proc_num,
		uint32_t glob_iteration, uint32_t iters)
{
	uint32_t i, j, page;
	for(i = 0; i < proc_num; i++){
		for(j = 0; j < iters; j++){
			page = get_rand_page(task_pool[i].page_count);
			/* This is global number of iteration */
			global_time++;
			if(mem_op(&task_pool[i], page, global_time)){
				if(page_fault(&task_pool[i], page)){
					mem_swaping(task_pool, proc_num, global_time);
					page_fault(&task_pool[i], page);
				}
				if(mem_op(&task_pool[i], page, global_time)){
					/* Impossible after memory swapping */
					printf("Something totally go wrong\n");
				}
			}
		}
	}
	mem_deamon(task_pool, proc_num);
}


