#include "multiboot.h"

#define PMM_BLOCK_SIZE 4096

unsigned int *pmm_bitmap;
unsigned int pmm_max_blocks = 0;
unsigned int pmm_bitmap_size = 0;

static int pmm_is_valid_bit(unsigned int bit)
{
    return (bit < pmm_max_blocks);
}

void pmm_set_bit(unsigned int bit)
{
    if (!pmm_is_valid_bit(bit))
    {
        return;
    }
    pmm_bitmap[bit / 32] |= (1 << (bit % 32));
}

void pmm_clear_bit(unsigned int bit)
{
    if (!pmm_is_valid_bit(bit))
    {
        return;
    }
    pmm_bitmap[bit / 32] &= ~(1 << (bit % 32));
}

int pmm_test_bit(unsigned int bit)
{
    if (!pmm_is_valid_bit(bit))
    {
        return -1;
    }
    return pmm_bitmap[bit / 32] & (1 << (bit % 32));
}

int pmm_find_first_free(void)
{
    if (pmm_max_blocks == 0 || pmm_bitmap == NULL)
    {
        return -1;
    }

    unsigned int max_dwords = (pmm_max_blocks + 31) / 32;
    for (unsigned int i = 0; i < max_dwords; i++)
    {
        if (pmm_bitmap[i] != 0xFFFFFFFF)
        {
            for (int j = 0; j < 32; j++)
            {
                unsigned int bit_index = (i * 32) + j;
                if (bit_index >= pmm_max_blocks)
                {
                    return -1;
                }
                unsigned int bit = 1 << j;
                if (!(pmm_bitmap[i] & bit))
                {
                    return bit_index;
                }
            }
        }
    }
    return -1;
}

void init_pmm(struct multiboot_info *mb_info, unsigned int kernel_end_addr)
{
    pmm_bitmap = (unsigned int *)kernel_end_addr;

    unsigned int mem_size = (mb_info->mem_lower + mb_info->mem_upper) * 1024;
    pmm_max_blocks = mem_size / PMM_BLOCK_SIZE;

    unsigned int bitmap_bytes = pmm_max_blocks / 8;
    pmm_bitmap_size = bitmap_bytes;

    unsigned int max_dwords = (pmm_max_blocks + 31) / 32;

    for (unsigned int i = 0; i < max_dwords; i++)
    {
        pmm_bitmap[i] = 0xFFFFFFFF;
    }

    struct multiboot_mmap_entry *mmap = (struct multiboot_mmap_entry *)mb_info->mmap_addr;
    while ((unsigned int)mmap < mb_info->mmap_addr + mb_info->mmap_length)
    {
        if (mmap->type == 1)
        {
            unsigned int start_block = mmap->base_addr_low / PMM_BLOCK_SIZE;
            unsigned int end_block = (mmap->base_addr_low + mmap->length_low) / PMM_BLOCK_SIZE;

            if (start_block < pmm_max_blocks && end_block <= pmm_max_blocks)
            {
                for (unsigned int b = start_block; b < end_block; b++)
                {
                    pmm_clear_bit(b);
                }
            }
        }
        mmap = (struct multiboot_mmap_entry *)((unsigned int)mmap + mmap->size + sizeof(mmap->size));
    }

    unsigned int kernel_blocks = (kernel_end_addr + bitmap_bytes) / PMM_BLOCK_SIZE;
    if (kernel_blocks >= pmm_max_blocks)
    {
        kernel_blocks = pmm_max_blocks - 1;
    }
    for (unsigned int b = 0; b <= kernel_blocks; b++)
    {
        pmm_set_bit(b);
    }
}

void *pmm_alloc_block(void)
{
    if (pmm_bitmap == NULL || pmm_max_blocks == 0)
    {
        return NULL;
    }

    int free_block = pmm_find_first_free();
    if (free_block == -1 || free_block < 0)
        return NULL;

    pmm_set_bit(free_block);
    void *result = (void *)(((unsigned int)free_block) * PMM_BLOCK_SIZE);
    return result;
}

void pmm_free_block(void *ptr)
{
    if (ptr == NULL || pmm_bitmap == NULL || pmm_max_blocks == 0)
    {
        return;
    }

    unsigned int addr = (unsigned int)ptr;
    unsigned int block = addr / PMM_BLOCK_SIZE;

    if (!pmm_is_valid_bit(block))
    {
        return;
    }

    pmm_clear_bit(block);
}

unsigned int pmm_get_free_block_count(void)
{
    if (pmm_bitmap == NULL || pmm_max_blocks == 0)
    {
        return 0;
    }

    unsigned int free_blocks = 0;
    for (unsigned int i = 0; i < pmm_max_blocks; i++)
    {
        if (!pmm_test_bit(i))
        {
            free_blocks++;
        }
    }
    return free_blocks;
}

unsigned int pmm_get_max_blocks(void)
{
    return pmm_max_blocks;
}