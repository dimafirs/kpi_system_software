#include <stdint.h>
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
	uintptr_t *addr;
	uint8_t *data;
} phys_mem_page;


typedef struct {
	unsigned long ref_time;
	struct virt_page_flags flags;
	enum virt_page_state state;
	phys_mem_page *page;
} virt_mem_page;

#define init_phys_page(page, index, size) ({ \
	page.id = index; 		\
	page.state = FREE;		\
	page.data = malloc(size);	\
	page.addr = size * index;	})

#define init_virt_page(page)	({\
	page.state = NOREF;	\
	page.page = NULL;	})
