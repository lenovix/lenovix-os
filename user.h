#ifndef USER_H
#define USER_H

// ==========================================
// Mini LibC untuk Lenovix OS (User Space)
// ==========================================

// Syscall ID 1: Print Teks ke Screen
static inline void sys_print(const char *text) {
    __asm__ __volatile__ (
        "int $0x80"
        : 
        : "a"(1), "b"(text)
    );
}

// Syscall ID 2: Terminate / Exit Task
static inline void sys_exit(void) {
    __asm__ __volatile__ (
        "int $0x80"
        : 
        : "a"(2)
    );
}

// Helper untuk cetak 1 karakter
static inline void sys_putchar(char c) {
    char str[2] = {c, '\0'};
    sys_print(str);
}

// Syscall ID 3: Read Key / Karakter dari Keyboard
static inline char sys_getkey(void) {
    char c = 0;
    __asm__ __volatile__ (
        "int $0x80"
        :
        : "a"(3), "b"(&c)
        : "memory"
    );
    return c;
}

#endif