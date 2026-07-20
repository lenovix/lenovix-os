#include "vfs.h"
#include "heap.h"
#include "tty.h" // Untuk kprint & int_to_string

static vfs_node_t file_system[MAX_FILES];
static int file_count = 0;

static void string_copy(char *dest, const char *src) {
    int i = 0;
    while (src[i] != '\0' && i < MAX_FILENAME_LEN - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

static size_t string_length(const char *str) {
    size_t len = 0;
    while (str[len] != '\0') len++;
    return len;
}

static int string_equals(const char *str1, const char *str2) {
    int i = 0;
    while (str1[i] != '\0' && str2[i] != '\0') {
        if (str1[i] != str2[i]) return 0;
        i++;
    }
    return str1[i] == str2[i];
}

// Core VFS: Pembuatan File Sederhana
int vfs_create_file_raw(const char *name, const char *content) {
    if (file_count >= MAX_FILES) return -1;

    for (int i = 0; i < file_count; i++) {
        if (string_equals(file_system[i].name, name)) {
            return -2;
        }
    }

    vfs_node_t *node = &file_system[file_count];
    string_copy(node->name, name);
    node->flags = FS_FILE;
    node->length = string_length(content);

    node->data = (char *)kmalloc(node->length + 1);
    if (node->data == NULL) return -3;

    for (size_t i = 0; i <= node->length; i++) {
        node->data[i] = content[i];
    }

    file_count++;
    return 0;
}

void init_vfs(void) {
    file_count = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        file_system[i].name[0] = '\0';
        file_system[i].flags = 0;
        file_system[i].length = 0;
        file_system[i].data = NULL;
    }

    // Default Files
    vfs_create_file_raw("readme.txt", "Selamat Datang di Lenovix OS Filesystem v0.4!\nSistem ini menggunakan VFS sederhana.");
    vfs_create_file_raw("version.info", "Lenovix OS Kernel v0.4 (Storage Update)");
}

vfs_node_t *vfs_find_file(const char *name) {
    for (int i = 0; i < file_count; i++) {
        if (string_equals(file_system[i].name, name)) {
            return &file_system[i];
        }
    }
    return NULL;
}

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
        
        char size_str[16];
        int_to_string(file_system[i].length, size_str);
        
        kprint(size_str, 0x0A);
        kprint(" B\n", 0x0F);
    }
}

// Wrapper untuk Perintah 'cat'
void vfs_read_file_cmd(const char *name) {
    vfs_node_t *file = vfs_find_file(name);
    if (file == NULL) {
        kprint("File tidak ditemukan: ", 0x0C);
        kprint(name, 0x0C);
        kprint("\n", 0x0C);
        return;
    }

    kprint("Isi file '", 0x0A);
    kprint(file->name, 0x0B);
    kprint("':\n", 0x0A);
    kprint(file->data, 0x0F);
    kprint("\n", 0x0F);
}

// Wrapper untuk Perintah 'touch'
void vfs_create_file_cmd(const char *name) {
    int result = vfs_create_file_raw(name, "");
    if (result == -1) {
        kprint("Gagal membuat file: Sistem file penuh.\n", 0x0C);
    } else if (result == -2) {
        kprint("Gagal membuat file: File dengan nama yang sama sudah ada.\n", 0x0C);
    } else if (result == -3) {
        kprint("Gagal membuat file: Memori heap tidak cukup.\n", 0x0C);
    } else {
        kprint("File berhasil dibuat: ", 0x0A);
        kprint(name, 0x0B);
        kprint("\n", 0x0F);
    }
}

// Wrapper untuk Perintah 'write'
void vfs_write_file_cmd(const char *filename, const char *content) {
    vfs_node_t *file = vfs_find_file(filename);
    if (file == NULL) {
        kprint("File tidak ditemukan: ", 0x0C);
        kprint(filename, 0x0C);
        kprint("\n", 0x0C);
        return;
    }

    size_t new_length = string_length(content);

    char *new_data = (char *)kmalloc(new_length + 1);
    if (new_data == NULL) {
        kprint("Gagal menulis ke file: Memori heap tidak cukup.\n", 0x0C);
        return;
    }

    for (size_t i = 0; i <= new_length; i++) {
        new_data[i] = content[i];
    }

    if (file->data != NULL) {
        kfree(file->data);
    }
    file->data = new_data;
    file->length = new_length;

    kprint("File berhasil diperbarui: ", 0x0A);
    kprint(filename, 0x0B);
    kprint("\n", 0x0F);
}