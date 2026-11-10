#include "vmm.h"

#define IDENTITY_TABLES_COUNT 64
#define FB_TABLES_COUNT 4

static unsigned int page_directory[1024] __attribute__((aligned(4096)));
static unsigned int identity_tables[IDENTITY_TABLES_COUNT][1024] __attribute__((aligned(4096)));
static unsigned int fb_tables[FB_TABLES_COUNT][1024] __attribute__((aligned(4096)));

void vmm_map_page(unsigned int phys_addr, unsigned int virt_addr, unsigned int flags)
{
    unsigned int pde_idx = virt_addr >> 22;
    unsigned int pte_idx = (virt_addr >> 12) & 0x3FF;

    if (!(page_directory[pde_idx] & PAGE_PRESENT))
    {
        return;
    }

    unsigned int *table = (unsigned int *)(page_directory[pde_idx] & ~0xFFF);
    table[pte_idx] = (phys_addr & ~0xFFF) | flags;

    __asm__ __volatile__("invlpg (%0)" : : "r"(virt_addr) : "memory");
}

void init_vmm(multiboot_info_t *mb_info)
{
    for (int i = 0; i < 1024; i++)
    {
        page_directory[i] = 0;
    }

    for (int t = 0; t < IDENTITY_TABLES_COUNT; t++)
    {
        for (int p = 0; p < 1024; p++)
        {
            unsigned int addr = (t * 1024 + p) * PAGE_SIZE;
            identity_tables[t][p] = addr | PAGE_PRESENT | PAGE_WRITE;
        }
        page_directory[t] = ((unsigned int)&identity_tables[t]) | PAGE_PRESENT | PAGE_WRITE;
    }

    if (mb_info && (mb_info->flags & (1 << 12)) && mb_info->framebuffer_addr)
    {
        unsigned int fb_start = (unsigned int)mb_info->framebuffer_addr;
        unsigned int fb_size = mb_info->framebuffer_pitch * mb_info->framebuffer_height;
        if (fb_size == 0)
        {
            fb_size = 1024 * 768 * 4;
        }

        unsigned int fb_end = fb_start + fb_size;
        int fb_table_idx = 0;

        for (unsigned int addr = fb_start; addr < fb_end; addr += PAGE_SIZE)
        {
            unsigned int pde_idx = addr >> 22;
            unsigned int pte_idx = (addr >> 12) & 0x3FF;

            if (!(page_directory[pde_idx] & PAGE_PRESENT))
            {
                if (fb_table_idx < FB_TABLES_COUNT)
                {
                    for (int j = 0; j < 1024; j++)
                    {
                        fb_tables[fb_table_idx][j] = 0;
                    }
                    page_directory[pde_idx] = ((unsigned int)&fb_tables[fb_table_idx]) | PAGE_PRESENT | PAGE_WRITE;
                    fb_table_idx++;
                }
            }

            unsigned int *table = (unsigned int *)(page_directory[pde_idx] & ~0xFFF);
            table[pte_idx] = (addr & ~0xFFF) | PAGE_PRESENT | PAGE_WRITE;
        }
    }

    __asm__ __volatile__(
        "mov %0, %%cr3\n\t"
        "mov %%cr0, %%eax\n\t"
        "or $0x80000000, %%eax\n\t"
        "mov %%eax, %%cr0\n\t"
        :
        : "r"(page_directory)
        : "eax", "memory");
}
