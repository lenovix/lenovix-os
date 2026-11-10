#include "tar.h"
#include "vfs.h"
#include "heap.h"
#include "string_util.h"

static unsigned int octal_to_uint(const char *str, int max_len)
{
    unsigned int val = 0;
    for (int i = 0; i < max_len && str[i] >= '0' && str[i] <= '7'; i++)
    {
        val = (val << 3) | (str[i] - '0');
    }
    return val;
}

int tar_extract(const void *archive, unsigned int len)
{
    if (!archive || len < 512)
        return -1;

    unsigned int offset = 0;
    int extracted = 0;

    while (offset + 512 <= len)
    {
        const tar_header_t *hdr = (const tar_header_t *)((const char *)archive + offset);
        if (hdr->name[0] == '\0')
        {
            break;
        }

        unsigned int file_size = octal_to_uint(hdr->size, 11);
        offset += 512;

        if (offset + file_size > len)
            break;

        const char *fname = hdr->name;
        if (fname[0] == '.' && fname[1] == '/')
            fname += 2;

        if (hdr->typeflag == '0' || hdr->typeflag == '\0')
        {
            char *content = (char *)kmalloc(file_size + 1);
            if (content)
            {
                const char *src = (const char *)archive + offset;
                for (unsigned int b = 0; b < file_size; b++)
                {
                    content[b] = src[b];
                }
                content[file_size] = '\0';

                vfs_create_file_raw(fname, content);
                kfree(content);
                extracted++;
            }
        }

        offset += ((file_size + 511) / 512) * 512;
    }

    return extracted;
}

int tar_list(const void *archive, unsigned int len, char out_names[][32], int max_files)
{
    if (!archive || len < 512 || !out_names || max_files <= 0)
        return 0;

    unsigned int offset = 0;
    int count = 0;

    while (offset + 512 <= len && count < max_files)
    {
        const tar_header_t *hdr = (const tar_header_t *)((const char *)archive + offset);
        if (hdr->name[0] == '\0')
            break;

        unsigned int file_size = octal_to_uint(hdr->size, 11);
        offset += 512;

        const char *fname = hdr->name;
        if (fname[0] == '.' && fname[1] == '/')
            fname += 2;

        int k = 0;
        while (fname[k] && k < 31)
        {
            out_names[count][k] = fname[k];
            k++;
        }
        out_names[count][k] = '\0';
        count++;

        offset += ((file_size + 511) / 512) * 512;
    }

    return count;
}

void tar_init_embedded(void)
{
    static char dummy_tar[2048];
    for (int i = 0; i < 2048; i++)
        dummy_tar[i] = 0;

    tar_header_t *h1 = (tar_header_t *)dummy_tar;
    string_copy(h1->name, "motd.txt");
    string_copy(h1->mode, "0000644");
    string_copy(h1->size, "00000000054");
    h1->typeflag = '0';
    string_copy(h1->magic, "ustar");

    const char *text1 = "Selamat datang di Lenovix OS!\nSistem Operasi x86 Mandiri.\n";
    char *c1 = dummy_tar + 512;
    int idx = 0;
    while (text1[idx])
    {
        c1[idx] = text1[idx];
        idx++;
    }

    tar_header_t *h2 = (tar_header_t *)(dummy_tar + 1024);
    string_copy(h2->name, "config.sys");
    string_copy(h2->mode, "0000644");
    string_copy(h2->size, "00000000036");
    h2->typeflag = '0';
    string_copy(h2->magic, "ustar");

    const char *text2 = "VESA=1024x768x32\nAUDIO=SB16\nNET=RTL8139\n";
    char *c2 = dummy_tar + 1536;
    idx = 0;
    while (text2[idx])
    {
        c2[idx] = text2[idx];
        idx++;
    }

    tar_extract(dummy_tar, 2048);
}
