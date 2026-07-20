#ifndef TASK_H
#define TASK_H

#include <stddef.h>

// Status proses
typedef enum {
    TASK_READY,
    TASK_RUNNING,
    TASK_TERMINATED
} task_state_t;

// Context / Register CPU yang disimpan saat switch
typedef struct registers {
    unsigned int eax, ebx, ecx, edx;
    unsigned int esi, edi, esp, ebp;
    unsigned int eip, eflags;
} registers_t;

// Process Control Block (PCB)
typedef struct task {
    int id;                     // Process ID (PID)
    char name[32];              // Nama proses
    registers_t regs;           // Keadaan register CPU
    unsigned int *stack;        // Pointer ke stack milik task
    task_state_t state;         // Status proses
    struct task *next;          // Pointer ke task berikutnya (Linked List)
} task_t;

void init_multitasking(void);
task_t *create_task(const char *name, void (*entry_point)(void));
void task_yield(void);
void schedule(void);

#endif