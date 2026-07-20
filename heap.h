#ifndef HEAP_H
#define HEAP_H

#include <stddef.h>

// Header untuk setiap blok memori di heap
typedef struct heap_block {
    size_t size;               // Ukuran data dalam byte
    int is_free;               // Status: 1 jika bebas, 0 jika terpakai
    struct heap_block *next;   // Pointer ke blok berikutnya
} heap_block_t;

void init_heap(void);
void *kmalloc(size_t size);
void kfree(void *ptr);

#endif