#include "task.h"
#include "heap.h"
#include "string_util.h"

static task_t *current_task = NULL;
static task_t *task_list_head = NULL;
static int next_pid = 1;

extern void kprint(const char *str, char color);
extern void switch_to_task(unsigned int *old_esp, unsigned int new_esp);
extern void tss_set_stack(unsigned int stack);

void init_multitasking(void)
{
    task_t *kernel_task = (task_t *)kmalloc(sizeof(task_t));
    kernel_task->id = 0;
    string_copy(kernel_task->name, "kernel_main");
    kernel_task->state = TASK_RUNNING;
    kernel_task->stack = NULL;
    kernel_task->next = kernel_task;

    current_task = kernel_task;
    task_list_head = kernel_task;
}

task_t *create_task(const char *name, void (*entry_point)(void))
{
    task_t *new_task = (task_t *)kmalloc(sizeof(task_t));
    if (!new_task)
        return NULL;

    new_task->id = next_pid++;
    string_copy(new_task->name, name);
    new_task->state = TASK_READY;

    unsigned int stack_size = 4096;
    new_task->stack = (unsigned int *)kmalloc(stack_size);
    if (!new_task->stack)
    {
        kfree(new_task);
        return NULL;
    }

    unsigned int *sp = (unsigned int *)((char *)new_task->stack + stack_size);

    *(--sp) = 0x0202;                    // EFLAGS (Interrupts enabled)
    *(--sp) = 0x08;                      // CS (Kernel Code)
    *(--sp) = (unsigned int)entry_point; // EIP

    *(--sp) = 0; // EAX
    *(--sp) = 0; // ECX
    *(--sp) = 0; // EDX
    *(--sp) = 0; // EBX
    *(--sp) = 0; // ESP
    *(--sp) = 0; // EBP
    *(--sp) = 0; // ESI
    *(--sp) = 0; // EDI

    new_task->regs.esp = (unsigned int)sp;
    new_task->regs.eip = (unsigned int)entry_point;
    new_task->regs.eflags = 0x0202;

    new_task->next = task_list_head->next;
    task_list_head->next = new_task;

    return new_task;
}

void schedule(void)
{
    if (!current_task)
        return;

    task_t *prev_task = current_task;
    task_t *next_task = current_task->next;
    task_t *start_task = next_task;
    int iterations = 0;
    int max_iterations = 1000;

    while (next_task->state == TASK_TERMINATED && iterations < max_iterations)
    {
        next_task = next_task->next;
        iterations++;

        if (next_task == start_task)
        {
            break;
        }
    }

    if (iterations >= max_iterations || next_task->state == TASK_TERMINATED)
    {
        if (current_task->state == TASK_TERMINATED)
        {
            task_t *search = task_list_head;
            for (int i = 0; i < max_iterations; i++)
            {
                if (search->id == 0 && search->state != TASK_TERMINATED)
                {
                    current_task = search;
                    current_task->state = TASK_RUNNING;
                    return;
                }
                search = search->next;
                if (search == task_list_head)
                    break;
            }
        }
        return;
    }

    if (next_task != current_task && next_task->state != TASK_TERMINATED)
    {
        current_task = next_task;
        current_task->state = TASK_RUNNING;

        if (next_task->stack)
        {
            tss_set_stack((unsigned int)next_task->stack + 4096);
        }

        switch_to_task(&(prev_task->regs.esp), next_task->regs.esp);
    }
}

void task_yield(void)
{
    schedule();
}