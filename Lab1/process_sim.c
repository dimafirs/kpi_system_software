#include "task.h"

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <stdint.h>

#define MAX_PAGE_NUM 30

static uint32_t seed = 0;

int init_random(){
	FILE *urand = fopen("/dev/urandom", "rb");
	if (NULL == urand)
		return -1;
	seed = fread(&seed, sizeof(seed), 1, urand);
	if(ferror(urand))
		return -1;
	srand(seed);
}
int init_process(struct task_struct *process, uint32_t id) {
	uint32_t i, mem_size;

	process->pid = id;
	process->page_count = rand() % MAX_PAGE_NUM;
	mem_size = process->page_count * sizeof(virt_mem_page);
	process->pages = malloc(mem_size);
	if(NULL == process->pages)
		return -1;
	memset(process->pages, 0, mem_size);
	for(i = 0; i < process->page_count; i++)
		process->pages[i].page = NULL;
	return 0;
}


