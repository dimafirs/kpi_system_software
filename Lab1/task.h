#include "memory.h"
#include <stdint.h>

struct task_struct {
	uint32_t pid;
	virt_mem_page *pages;
	uint32_t page_count;
};
