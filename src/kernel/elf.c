#include "elf.h"
#include "heap.h"
#include "tty.h"

int elf_validate(const void *data, unsigned int len)
{
    if (!data || len < sizeof(elf32_ehdr_t))
    {
        return -1;
    }

    const elf32_ehdr_t *hdr = (const elf32_ehdr_t *)data;

    if (hdr->e_ident[0] != 0x7F ||
        hdr->e_ident[1] != 'E' ||
        hdr->e_ident[2] != 'L' ||
        hdr->e_ident[3] != 'F')
    {
        return -2;
    }

    if (hdr->e_ident[4] != 1 || hdr->e_ident[5] != 1)
    {
        return -3;
    }

    if (hdr->e_machine != 3)
    {
        return -4;
    }

    return 0;
}

int elf_load_binary(const void *data, unsigned int len, unsigned int *out_entry)
{
    int val = elf_validate(data, len);
    if (val != 0)
    {
        return val;
    }

    const elf32_ehdr_t *hdr = (const elf32_ehdr_t *)data;

    if (hdr->e_phoff + (hdr->e_phnum * sizeof(elf32_phdr_t)) > len)
    {
        return -5;
    }

    const elf32_phdr_t *phdrs = (const elf32_phdr_t *)((const char *)data + hdr->e_phoff);

    for (int i = 0; i < hdr->e_phnum; i++)
    {
        if (phdrs[i].p_type == PT_LOAD)
        {
            if (phdrs[i].p_offset + phdrs[i].p_filesz > len)
            {
                return -6;
            }

            unsigned char *dest = (unsigned char *)phdrs[i].p_vaddr;
            const unsigned char *src = (const unsigned char *)data + phdrs[i].p_offset;

            for (unsigned int b = 0; b < phdrs[i].p_filesz; b++)
            {
                dest[b] = src[b];
            }

            for (unsigned int b = phdrs[i].p_filesz; b < phdrs[i].p_memsz; b++)
            {
                dest[b] = 0;
            }
        }
    }

    if (out_entry)
    {
        *out_entry = hdr->e_entry;
    }

    return 0;
}
