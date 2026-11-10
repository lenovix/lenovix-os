#ifndef VMM_H
#define VMM_H

#include "multiboot.h"

#define PAGE_SIZE 4096
#define PAGE_PRESENT 0x1
#define PAGE_WRITE 0x2
#define PAGE_USER 0x4

void init_vmm(multiboot_info_t *mb_info);
void vmm_map_page(unsigned int phys_addr, unsigned int virt_addr, unsigned int flags);

#endif
