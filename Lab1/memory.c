#include <stddef.h>

#define POOL_SIZE 200
#define PAGE_SIZE 1024

phys_mem_page mem_pool[POOL_SIZE] = {0};

void mem_swapping(void){

}
phys_mem_page * assign_free_page(void){
	uint32_t i;
	for(i = 0; i < POOL_SIZE; i++) {
		if(mem_pool[i].state == FREE){
			mem_pool[i].state = ALLOC;
			return &mem_pool[i];
		}
	}
	mem_swapping();
	for(i = 0; i < POOL_SIZE; i++) {
		if(mem_pool[i].state == FREE){
			mem_pool[i].state = ALLOC;
			return &mem_pool[i];
		}
	}
	/* Impossible after mem_swapping()*/
	return NULL;
}
static void page_fault_call(struct task_struct *process, uint32_t page){
	process->pages[page]->page = assign_free_page();
}

void mem_op(struct task_struct *process, uint32_t page, uint32_t iter) {
	if(!(process->pages[page]->flags.presence)){
		page_fault_call(process, page);
	}

	process->pages[page]->flags.reference = 1;
	process->pages[page]->ref_time=iter;

}

void mem_deamon(struct task_struct *task_pool, uint32_t proc_num, uint32_t iter){

}

void init_memory(void) {
	uint32_t i;
	for(i = 0; i < POOL_SIZE; i++){
		init_phys_page(mem_pool[i], i, PAGE_SIZE);
	}
}
