#include "heap.h"
#include "multiboot.h"

// Memulai Heap di alamat 16MB (0x01000000) agar jauh di atas kernel
#define HEAP_START 0x01000000
#define HEAP_MAX_SIZE (4 * 1024 * 1024) // Batas awal Heap: 4MB

static heap_block_t *heap_head = NULL;

void init_heap(void) {
    heap_head = (heap_block_t *) HEAP_START;
    heap_head->size = HEAP_MAX_SIZE - sizeof(heap_block_t);
    heap_head->is_free = 1;
    heap_head->next = NULL;
}

void *kmalloc(size_t size) {
    if (size == 0) return NULL;

    heap_block_t *curr = heap_head;

    // Cari blok memori yang bebas dan ukurannya cukup (First-Fit Algorithm)
    while (curr != NULL) {
        if (curr->is_free && curr->size >= size) {
            // Jika sisa blok cukup besar, pecah blok memori (Split Block)
            if (curr->size >= size + sizeof(heap_block_t) + 16) {
                heap_block_t *new_block = (heap_block_t *)((char *)curr + sizeof(heap_block_t) + size);
                new_block->size = curr->size - size - sizeof(heap_block_t);
                new_block->is_free = 1;
                new_block->next = curr->next;

                curr->size = size;
                curr->next = new_block;
            }
            
            curr->is_free = 0;
            // Kembalikan pointer yang menunjuk ke area DATA (setelah header blok)
            return (void *)((char *)curr + sizeof(heap_block_t));
        }
        curr = curr->next;
    }

    return NULL; // Out of memory
}

void kfree(void *ptr) {
    if (ptr == NULL) return;

    // Ambil header dari pointer data
    heap_block_t *block = (heap_block_t *)((char *)ptr - sizeof(heap_block_t));
    block->is_free = 1;

    // Merge (Gabungkan) blok-blok kosong yang berdampingan
    heap_block_t *curr = heap_head;
    while (curr != NULL && curr->next != NULL) {
        if (curr->is_free && curr->next->is_free) {
            curr->size += sizeof(heap_block_t) + curr->next->size;
            curr->next = curr->next->next;
        } else {
            curr = curr->next;
        }
    }
}