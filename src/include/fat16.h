#ifndef FAT16_H
#define FAT16_H

typedef struct
{
    char name[13];
    unsigned int size;
    unsigned short cluster;
    unsigned char is_directory;
} fat16_file_t;

int fat16_init(void);
int fat16_is_available(void);
int fat16_list_root(fat16_file_t *out_files, int max_files);
int fat16_read_file(const char *name, unsigned char *buf, unsigned int max_len);
int fat16_write_file(const char *name, const unsigned char *buf, unsigned int len);

#endif
