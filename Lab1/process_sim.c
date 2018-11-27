#include "memory.h"

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>

#define MAX_PAGE_NUM 30

int init_random(){
	uint32_t seed;
	FILE *urand = fopen("/dev/urandom", "rb");
	if (NULL == urand)
		return -1;
	fread(&seed, sizeof(seed), 1, urand);
	printf("seed = %d\n", seed);
	if(ferror(urand))
		return -1;
	srand(seed);
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
	double r = (double) rand() / RAND_MAX;
	printf("r=%2f\n", r);
	uint32_t divider = count/5;
	uint32_t e = 0;
	if (r < 0.9){
		 return (rand()*count)%divider;
	}
	else
		return ( (rand()*count)%(4*divider) + divider );
}

void on_exec(struct task_struct *process, uint32_t iteration){
	uint32_t i, page;
	/* Defines randomly, 0 is read, 1 is write */
	for(i = 0; i < iteration; i++){
		page = get_rand_page(process->page_count);
		mem_op(process, page, i);
	}

}


