#include "task.h"
#include "heap.h"

static task_t *current_task = NULL;
static task_t *task_list_head = NULL;
static int next_pid = 1;

extern void kprint(const char *str, char color);
extern void switch_to_task(unsigned int *old_esp, unsigned int new_esp);

// Helper menyalin nama task
static void string_copy(char *dest, const char *src) {
    int i = 0;
    while (src[i] != '\0' && i < 31) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

void init_multitasking(void) {
    // Buat Task Kernel Utama (PID 0)
    task_t *kernel_task = (task_t *)kmalloc(sizeof(task_t));
    kernel_task->id = 0;
    string_copy(kernel_task->name, "kernel_main");
    kernel_task->state = TASK_RUNNING;
    kernel_task->stack = NULL; // Menggunakan stack kernel yang sudah ada
    kernel_task->next = kernel_task; // Circular linked list

    current_task = kernel_task;
    task_list_head = kernel_task;
}

task_t *create_task(const char *name, void (*entry_point)(void)) {
    task_t *new_task = (task_t *)kmalloc(sizeof(task_t));
    if (!new_task) return NULL;

    new_task->id = next_pid++;
    string_copy(new_task->name, name);
    new_task->state = TASK_READY;

    // Alokasikan Stack sebesar 4KB untuk task baru ini
    unsigned int stack_size = 4096;
    new_task->stack = (unsigned int *)kmalloc(stack_size);
    
    // Setup Top of Stack
    unsigned int *sp = (unsigned int *)((char *)new_task->stack + stack_size);

    // Persiapkan Stack Awal (seolah-olah dipush oleh interupsi)
    *(--sp) = 0x0202;                // EFLAGS (Interrupts enabled)
    *(--sp) = 0x08;                  // CS (Kernel Code Segment)
    *(--sp) = (unsigned int)entry_point; // EIP (Titik awal eksekusi fungsi)

    // Push dummy general registers (EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI)
    for (int i = 0; i < 8; i++) {
        *(--sp) = 0;
    }

    new_task->regs.esp = (unsigned int)sp;
    new_task->regs.eip = (unsigned int)entry_point;
    new_task->regs.eflags = 0x0202;

    // Masukkan ke Circular Linked List
    new_task->next = task_list_head->next;
    task_list_head->next = new_task;

    return new_task;
}

// Penjadwal Round-Robin Sederhana
void schedule(void) {
    if (!current_task) return;

    task_t *prev_task = current_task;
    task_t *next_task = current_task->next;

    while (next_task->state == TASK_TERMINATED) {
        next_task = next_task->next;
        if (next_task == current_task) break;
    }

    if (next_task != current_task && next_task->state != TASK_TERMINATED) {
        current_task = next_task;
        current_task->state = TASK_RUNNING;

        // PERUBAHAN UTAMA: Lakukan Context Switch nyata di level CPU!
        switch_to_task(&(prev_task->regs.esp), next_task->regs.esp);
    }
}

void task_yield(void) {
    schedule();
}