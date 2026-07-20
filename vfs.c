#include "vfs.h"
#include "heap.h"

// Array penampung file sederhana (Root Directory)
static vfs_node_t file_system[MAX_FILES];
static int file_count = 0;

// Helper untuk menyalin string
static void string_copy(char *dest, const char *src) {
    int i = 0;
    while (src[i] != '\0' && i < MAX_FILENAME_LEN - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

// Helper untuk menghitung panjang string
static size_t string_length(const char *str) {
    size_t len = 0;
    while (str[len] != '\0') len++;
    return len;
}

// Helper untuk membandingkan string
static int string_equals(const char *str1, const char *str2) {
    int i = 0;
    while (str1[i] != '\0' && str2[i] != '\0') {
        if (str1[i] != str2[i]) return 0;
        i++;
    }
    return str1[i] == str2[i];
}

void init_vfs(void) {
    file_count = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        file_system[i].name[0] = '\0';
        file_system[i].flags = 0;
        file_system[i].length = 0;
        file_system[i].data = NULL;
    }

    // Buat file bawaan sistem (Default Files)
    vfs_create_file("readme.txt", "Selamat Datang di Lenovix OS Filesystem v0.4!\nSistem ini menggunakan VFS sederhana.");
    vfs_create_file("version.info", "Lenovix OS Kernel v0.4 (Storage Update)");
}

int vfs_create_file(const char *name, const char *content) {
    if (file_count >= MAX_FILES) return -1; // FS Penuh

    // Cek apakah file dengan nama yang sama sudah ada
    for (int i = 0; i < file_count; i++) {
        if (string_equals(file_system[i].name, name)) {
            return -2; // File sudah ada
        }
    }

    vfs_node_t *node = &file_system[file_count];
    string_copy(node->name, name);
    node->flags = FS_FILE;
    node->length = string_length(content);

    // Alokasikan memori dinamis dari Heap untuk menyimpan data file
    node->data = (char *)kmalloc(node->length + 1);
    if (node->data == NULL) return -3; // Out of Memory

    // Salin isi konten ke heap
    for (size_t i = 0; i <= node->length; i++) {
        node->data[i] = content[i];
    }

    file_count++;
    return 0; // Sukses
}

vfs_node_t *vfs_find_file(const char *name) {
    for (int i = 0; i < file_count; i++) {
        if (string_equals(file_system[i].name, name)) {
            return &file_system[i];
        }
    }
    return NULL; // File tidak ditemukan
}

// Fungsi extern kprint dari kernel.c
extern void kprint(const char *str, char color);

void vfs_list_files(void) {
    if (file_count == 0) {
        kprint("Direktori kosong.\n", 0x0C);
        return;
    }

    kprint("Nama Berkas\t\tUkuran\n", 0x0E);
    kprint("----------------------------------\n", 0x07);
    for (int i = 0; i < file_count; i++) {
        kprint(file_system[i].name, 0x0F);
        kprint("\t\t", 0x0F);
        
        // Tampilkan ukuran byte sederhananya
        char size_str[16];
        extern void int_to_string(int n, char str[]);
        int_to_string(file_system[i].length, size_str);
        
        kprint(size_str, 0x0A);
        kprint(" B\n", 0x0F);
    }
}