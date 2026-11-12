#ifndef VFS_H
#define VFS_H

#include <stddef.h>

#define MAX_FILES 32
#define MAX_FILENAME_LEN 32
#define FS_FILE 0x01

typedef struct vfs_node
{
    char name[MAX_FILENAME_LEN];
    unsigned int flags;
    size_t length;
    char *data;
} vfs_node_t;

void init_vfs(void);
int vfs_create_file_raw(const char *name, const char *content);
int vfs_create_file_binary(const char *name, const void *data, size_t length);
vfs_node_t *vfs_find_file(const char *name);
void vfs_list_files(void);
int vfs_get_count(void);
vfs_node_t *vfs_get_node(int idx);
int vfs_delete_file(const char *name);
int vfs_copy_file(const char *src_name, const char *dest_name);
int vfs_rename_file(const char *old_name, const char *new_name);
int vfs_append_file(const char *filename, const char *content);
size_t vfs_get_total_used_bytes(void);

void vfs_read_file_cmd(const char *name);
void vfs_create_file_cmd(const char *name);
void vfs_write_file_cmd(const char *filename, const char *content);

#endif