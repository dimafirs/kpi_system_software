#include <stdint.h>
#include <stdlib.h>
#include <stddef.h>


struct virt_page_flags {
	unsigned reference:	1;
	unsigned modified:	1;
	unsigned presence:	1;
};

enum phys_page_state {
	FREE = 0,
	ALLOC
};
enum virt_page_state {
	NOREF = 0,
	ACTIVE,
	INSWAP
};

typedef struct {
	uint32_t id;
	enum phys_page_state state;
	uintptr_t addr;
	uint8_t *data;
} phys_mem_page;


typedef struct {
	unsigned long ref_time; //in this version use just number of iteration
	struct virt_page_flags flags;
	enum virt_page_state state;
	phys_mem_page *page;
} virt_mem_page;

struct task_struct {
	uint32_t pid;
	virt_mem_page *pages;
	uint32_t page_count;
};

#define init_virt_page(page)	({\
	page.state = NOREF;	\
	page.page = NULL;	})

int mem_op(struct task_struct *process, uint32_t page, uint32_t iter);
int page_fault(struct task_struct *process, uint32_t page);
void mem_swaping(struct task_struct *task_pool, uint32_t proc_num, uint32_t iter);
void mem_deamon(struct task_struct *task_pool, uint32_t proc_num);
void dump_stat();
int init_memory();
