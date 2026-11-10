#include "heap.h"
#include "multiboot.h"

#define HEAP_START 0x01000000
#define HEAP_MAX_SIZE (4 * 1024 * 1024)
#define HEAP_END (HEAP_START + HEAP_MAX_SIZE)
#define MIN_BLOCK_SIZE 16

static heap_block_t *heap_head = NULL;

static int is_within_heap_bounds(void *ptr)
{
    unsigned int addr = (unsigned int)ptr;
    return (addr >= HEAP_START && addr < HEAP_END);
}

void init_heap(void)
{
    heap_head = (heap_block_t *)HEAP_START;
    heap_head->size = HEAP_MAX_SIZE - sizeof(heap_block_t);
    heap_head->is_free = 1;
    heap_head->next = NULL;
}

void *kmalloc(size_t size)
{
    if (size == 0 || size > HEAP_MAX_SIZE)
        return NULL;

    heap_block_t *curr = heap_head;

    while (curr != NULL)
    {
        if (!is_within_heap_bounds((void *)curr))
        {
            return NULL;
        }

        if (curr->is_free && curr->size >= size)
        {
            unsigned int block_end = (unsigned int)curr + sizeof(heap_block_t) + curr->size;
            if (block_end > HEAP_END)
            {
                return NULL;
            }

            if (curr->size >= size + sizeof(heap_block_t) + MIN_BLOCK_SIZE)
            {
                heap_block_t *new_block = (heap_block_t *)((char *)curr + sizeof(heap_block_t) + size);

                if (!is_within_heap_bounds((void *)new_block))
                {
                    return NULL;
                }

                new_block->size = curr->size - size - sizeof(heap_block_t);
                new_block->is_free = 1;
                new_block->next = curr->next;

                curr->size = size;
                curr->next = new_block;
            }

            curr->is_free = 0;
            return (void *)((char *)curr + sizeof(heap_block_t));
        }
        curr = curr->next;
    }

    return NULL;
}

void kfree(void *ptr)
{
    if (ptr == NULL)
        return;

    if (!is_within_heap_bounds(ptr))
    {
        return;
    }

    heap_block_t *block = (heap_block_t *)((char *)ptr - sizeof(heap_block_t));

    if (!is_within_heap_bounds((void *)block))
    {
        return;
    }

    block->is_free = 1;

    heap_block_t *curr = heap_head;
    while (curr != NULL && curr->next != NULL)
    {
        if (!is_within_heap_bounds((void *)curr) || !is_within_heap_bounds((void *)curr->next))
        {
            break;
        }

        if (curr->is_free && curr->next->is_free)
        {
            curr->size += sizeof(heap_block_t) + curr->next->size;
            curr->next = curr->next->next;
        }
        else
        {
            curr = curr->next;
        }
    }
}