#include <stdint.h>

int init_random();
int init_process(struct task_struct *process, uint32_t id);
void on_exec(struct task_struct *process, uint32_t iteration);
