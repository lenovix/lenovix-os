#ifndef VFS_H
#define VFS_H

#include <stddef.h>

#define MAX_FILES 32
#define MAX_FILENAME_LEN 32
#define FS_FILE 0x01

typedef struct vfs_node {
    char name[MAX_FILENAME_LEN];
    unsigned int flags;
    size_t length;
    char *data;
} vfs_node_t;

void init_vfs(void);
int vfs_create_file_raw(const char *name, const char *content); // Core VFS
vfs_node_t *vfs_find_file(const char *name);
void vfs_list_files(void);

// Wrapper Functions untuk UI Shell
void vfs_read_file_cmd(const char *name);
void vfs_create_file_cmd(const char *name);
void vfs_write_file_cmd(const char *filename, const char *content);

#endif