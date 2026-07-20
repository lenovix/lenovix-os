#ifndef VFS_H
#define VFS_H

#include <stddef.h>

#define FS_FILE 0x01
#define FS_DIRECTORY 0x02

#define MAX_FILENAME_LEN 32
#define MAX_FILES 16

// Header struktur Node File/Direktori
typedef struct vfs_node {
    char name[MAX_FILENAME_LEN];
    unsigned int flags;       // FS_FILE atau FS_DIRECTORY
    unsigned int length;      // Ukuran file dalam byte
    char *data;               // Pointer ke isi data file
} vfs_node_t;

void init_vfs(void);
int vfs_create_file(const char *name, const char *content);
vfs_node_t *vfs_find_file(const char *name);
void vfs_list_files(void);

#endif