#ifndef TAR_H
#define TAR_H

typedef struct
{
    char name[100];
    char mode[8];
    char uid[8];
    char gid[8];
    char size[12];
    char mtime[12];
    char chksum[8];
    char typeflag;
    char linkname[100];
    char magic[6];
    char version[2];
    char uname[32];
    char gname[32];
    char devmajor[8];
    char devminor[8];
    char prefix[155];
    char pad[12];
} __attribute__((packed)) tar_header_t;

int tar_extract(const void *archive, unsigned int len);
int tar_list(const void *archive, unsigned int len, char out_names[][32], int max_files);
void tar_init_embedded(void);

#endif
