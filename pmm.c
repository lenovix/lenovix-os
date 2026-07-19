#include "multiboot.h"

#define PMM_BLOCK_SIZE 4096

// Pointer ke area bitmap diletakkan tepat setelah kode kernel selesai di RAM
unsigned int* pmm_bitmap;
unsigned int  pmm_max_blocks = 0;

void pmm_set_bit(unsigned int bit) {
    pmm_bitmap[bit / 32] |= (1 << (bit % 32));
}

void pmm_clear_bit(unsigned int bit) {
    pmm_bitmap[bit / 32] &= ~(1 << (bit % 32));
}

int pmm_test_bit(unsigned int bit) {
    return pmm_bitmap[bit / 32] & (1 << (bit % 32));
}

// Algoritma pencarian frame kosong pertama di RAM
int pmm_find_first_free(void) {
    for (unsigned int i = 0; i < pmm_max_blocks / 32; i++) {
        if (pmm_bitmap[i] != 0xFFFFFFFF) { // Jika tidak penuh semua 32-bit
            for (int j = 0; j < 32; j++) {
                unsigned int bit = 1 << j;
                if (!(pmm_bitmap[i] & bit)) {
                    return (i * 32) + j;
                }
            }
        }
    }
    return -1; // RAM Penuh (Out of Memory)
}

// Fungsi inisialisasi PMM
void init_pmm(struct multiboot_info* mb_info, unsigned int kernel_end_addr) {
    // Alokasikan lokasi bitmap aman setelah address berakhirnya kernel
    pmm_bitmap = (unsigned int*)kernel_end_addr;
    
    // Total memori terdeteksi dari Multiboot dalam Kilobyte (diubah ke byte)
    unsigned int mem_size = (mb_info->mem_lower + mb_info->mem_upper) * 1024;
    pmm_max_blocks = mem_size / PMM_BLOCK_SIZE;

    // Bersihkan bitmap (set semua RAM terpakai dulu untuk pengamanan)
    unsigned int bitmap_bytes = pmm_max_blocks / 8;
    for (unsigned int i = 0; i < bitmap_bytes / 4; i++) {
        pmm_bitmap[i] = 0xFFFFFFFF; 
    }

    // Baca peta memori dari GRUB/QEMU dan buka blok RAM yang berjenis "Available RAM" (Type = 1)
    struct multiboot_mmap_entry* mmap = (struct multiboot_mmap_entry*)mb_info->mmap_addr;
    while ((unsigned int)mmap < mb_info->mmap_addr + mb_info->mmap_length) {
        if (mmap->type == 1) { // 1 = Free RAM
            unsigned int start_block = mmap->base_addr_low / PMM_BLOCK_SIZE;
            unsigned int end_block = (mmap->base_addr_low + mmap->length_low) / PMM_BLOCK_SIZE;
            for (unsigned int b = start_block; b < end_block; b++) {
                pmm_clear_bit(b); // Set bit ke 0 (Tandai kosong)
            }
        }
        mmap = (struct multiboot_mmap_entry*)((unsigned int)mmap + mmap->size + sizeof(mmap->size));
    }

    // Proteksi: Kunci kembali area RAM lokasi kernel & lokasi bitmap agar tidak tertimpa
    unsigned int kernel_blocks = (kernel_end_addr + bitmap_bytes) / PMM_BLOCK_SIZE;
    for (unsigned int b = 0; b <= kernel_blocks; b++) {
        pmm_set_bit(b);
    }
}

// Mengambil 1 block memori kosong (Setara malloc internal 4KB)
void* pmm_alloc_block(void) {
    int free_block = pmm_find_first_free();
    if (free_block == -1) return 0;

    pmm_set_bit(free_block);
    return (void*)(free_block * PMM_BLOCK_SIZE);
}

// Mengembalikan block memori yang telah selesai digunakan (free)
void pmm_free_block(void* ptr) {
    unsigned int addr = (unsigned int)ptr;
    unsigned int block = addr / PMM_BLOCK_SIZE;
    pmm_clear_bit(block);
}