#ifndef TASK_H
#define TASK_H

#include <stddef.h>

typedef enum
{
    TASK_READY,
    TASK_RUNNING,
    TASK_TERMINATED
} task_state_t;

typedef struct registers
{
    unsigned int eax, ebx, ecx, edx;
    unsigned int esi, edi, esp, ebp;
    unsigned int eip, eflags;
} registers_t;

typedef struct task
{
    int id;
    char name[32];
    registers_t regs;
    unsigned int *stack;
    task_state_t state;
    struct task *next;
} task_t;

typedef struct
{
    int id;
    char name[32];
    task_state_t state;
} task_info_t;

void init_multitasking(void);
task_t *create_task(const char *name, void (*entry_point)(void));
void task_yield(void);
void schedule(void);
int task_kill(int pid);
int task_get_list(task_info_t *out, int max);

#endif