#include "memory.h"

#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

#define POOL_SIZE 200
#define PAGE_SIZE 1024
#define WORKING_SET_ITERATIONS 3

phys_mem_page mem_pool[POOL_SIZE] = {0};

static int init_phys_page(phys_mem_page *page, uint32_t index, uint32_t size){
	page->id = index;
	page->state = FREE;
	page->data = malloc(size);
	if(NULL == page->data){
		printf("mem_error\n");
		return -1;
	}
	page->addr = (uintptr_t) size * index;
	return 0;
}

void mem_swaping(struct task_struct *task_pool, uint32_t proc_num, uint32_t iter){
	uint32_t i, j;
	for(i = 0; i < proc_num; i++){
		for(j = 0; j < task_pool[i].page_count; j++){
			if(task_pool[i].pages[j].flags.presence &&
					!(task_pool[i].pages[j].flags.reference))
			{
				if((task_pool[i].pages[j].ref_time - iter) < WORKING_SET_ITERATIONS) {
					task_pool[i].pages[j].page->state = FREE;
					task_pool[i].pages[j].state = INSWAP;
					task_pool[i].pages[j].flags.presence = 0;
				}
			}
		}
	}
}

phys_mem_page *assign_free_page(void){
	uint32_t i;
	for(i = 0; i < POOL_SIZE; i++) {
		if(mem_pool[i].state == FREE){
			mem_pool[i].state = ALLOC;
			return &mem_pool[i];
		}
	}
	return NULL;
}
int page_fault(struct task_struct *process, uint32_t page){
	if(NULL == (process->pages[page].page = assign_free_page())){
		return -1;
	}
	process->pages[page].state = ACTIVE;
	return 0;
}

int mem_op(struct task_struct *process, uint32_t page, uint32_t iter)
{
	if(!(process->pages[page].flags.presence)){
		return -1;
	}

	process->pages[page].flags.reference = 1;
	process->pages[page].ref_time=iter;
	return 0;
}

void mem_deamon(struct task_struct *task_pool, uint32_t proc_num, uint32_t iter){

}

int init_memory(void) {
	uint32_t i;
	for(i = 0; i < POOL_SIZE; i++){
		if(init_phys_page(&mem_pool[i], i, PAGE_SIZE))
			return -1;
	}
	return 0;
}
