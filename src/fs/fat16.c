#include "fat16.h"
#include "ata.h"
#include "string_util.h"

static int fat16_ready = 0;
static unsigned int fat_start_lba = 0;
static unsigned int root_dir_start_lba = 0;
static unsigned int root_dir_sectors = 0;
static unsigned int data_start_lba = 0;
static unsigned char sectors_per_cluster = 1;
static unsigned short root_entries = 512;
static unsigned int sectors_per_fat = 0;

struct fat16_entry
{
    unsigned char name[8];
    unsigned char ext[3];
    unsigned char attr;
    unsigned char reserved;
    unsigned char create_time_tenth;
    unsigned short create_time;
    unsigned short create_date;
    unsigned short access_date;
    unsigned short cluster_high;
    unsigned short modify_time;
    unsigned short modify_date;
    unsigned short starting_cluster;
    unsigned int file_size;
} __attribute__((packed));

static void format_83_name(const unsigned char *name, const unsigned char *ext, char *out)
{
    int i = 0, j = 0;
    while (i < 8 && name[i] != ' ' && name[i] != '\0')
    {
        out[j++] = name[i++];
    }
    if (ext[0] != ' ' && ext[0] != '\0')
    {
        out[j++] = '.';
        int k = 0;
        while (k < 3 && ext[k] != ' ' && ext[k] != '\0')
        {
            out[j++] = ext[k++];
        }
    }
    out[j] = '\0';
}

static int name_match(const char *n1, const char *n2)
{
    while (*n1 && *n2)
    {
        char c1 = (*n1 >= 'a' && *n1 <= 'z') ? *n1 - 32 : *n1;
        char c2 = (*n2 >= 'a' && *n2 <= 'z') ? *n2 - 32 : *n2;
        if (c1 != c2)
            return 0;
        n1++;
        n2++;
    }
    return (*n1 == '\0' && *n2 == '\0');
}

int fat16_init(void)
{
    unsigned char sec[ATA_SECTOR_SIZE];
    if (ata_read_sector(0, sec) != 0)
    {
        fat16_ready = 0;
        return -1;
    }

    if (sec[510] != 0x55 || sec[511] != 0xAA)
    {
        fat16_ready = 0;
        return -2;
    }

    unsigned short bytes_per_sec = sec[11] | (sec[12] << 8);
    if (bytes_per_sec != 512)
    {
        fat16_ready = 0;
        return -3;
    }

    sectors_per_cluster = sec[13];
    unsigned short reserved = sec[14] | (sec[15] << 8);
    unsigned char fat_count = sec[16];
    root_entries = sec[17] | (sec[18] << 8);
    sectors_per_fat = sec[22] | (sec[23] << 8);

    if (sectors_per_cluster == 0 || fat_count == 0 || sectors_per_fat == 0)
    {
        fat16_ready = 0;
        return -4;
    }

    fat_start_lba = reserved;
    root_dir_start_lba = fat_start_lba + (fat_count * sectors_per_fat);
    root_dir_sectors = ((root_entries * 32) + 511) / 512;
    data_start_lba = root_dir_start_lba + root_dir_sectors;

    fat16_ready = 1;
    return 0;
}

int fat16_is_available(void)
{
    return fat16_ready;
}

int fat16_list_root(fat16_file_t *out_files, int max_files)
{
    if (!fat16_ready || !out_files || max_files <= 0)
        return 0;

    int count = 0;
    unsigned char buf[ATA_SECTOR_SIZE];

    for (unsigned int s = 0; s < root_dir_sectors && count < max_files; s++)
    {
        if (ata_read_sector(root_dir_start_lba + s, buf) != 0)
            break;

        struct fat16_entry *entries = (struct fat16_entry *)buf;
        for (int e = 0; e < 16 && count < max_files; e++)
        {
            if (entries[e].name[0] == 0x00)
                return count;
            if (entries[e].name[0] == 0xE5)
                continue;
            if (entries[e].attr & 0x08)
                continue;

            format_83_name(entries[e].name, entries[e].ext, out_files[count].name);
            out_files[count].size = entries[e].file_size;
            out_files[count].cluster = entries[e].starting_cluster;
            out_files[count].is_directory = (entries[e].attr & 0x10) ? 1 : 0;
            count++;
        }
    }

    return count;
}

int fat16_read_file(const char *name, unsigned char *buf, unsigned int max_len)
{
    if (!fat16_ready || !name || !buf)
        return -1;

    unsigned char sec_buf[ATA_SECTOR_SIZE];
    struct fat16_entry found;
    int is_found = 0;

    for (unsigned int s = 0; s < root_dir_sectors && !is_found; s++)
    {
        if (ata_read_sector(root_dir_start_lba + s, sec_buf) != 0)
            break;

        struct fat16_entry *entries = (struct fat16_entry *)sec_buf;
        for (int e = 0; e < 16; e++)
        {
            if (entries[e].name[0] == 0x00)
                break;
            if (entries[e].name[0] == 0xE5)
                continue;
            if (entries[e].attr & 0x08)
                continue;

            char fname[13];
            format_83_name(entries[e].name, entries[e].ext, fname);
            if (name_match(fname, name))
            {
                found = entries[e];
                is_found = 1;
                break;
            }
        }
    }

    if (!is_found)
        return -2;

    unsigned int bytes_to_read = found.file_size;
    if (bytes_to_read > max_len)
        bytes_to_read = max_len;

    unsigned int bytes_read = 0;
    unsigned short cluster = found.starting_cluster;

    while (cluster >= 2 && cluster < 0xFFF8 && bytes_read < bytes_to_read)
    {
        unsigned int cluster_lba = data_start_lba + (cluster - 2) * sectors_per_cluster;
        for (int sec = 0; sec < sectors_per_cluster && bytes_read < bytes_to_read; sec++)
        {
            if (ata_read_sector(cluster_lba + sec, sec_buf) != 0)
                return bytes_read;

            unsigned int chunk = bytes_to_read - bytes_read;
            if (chunk > ATA_SECTOR_SIZE)
                chunk = ATA_SECTOR_SIZE;

            for (unsigned int b = 0; b < chunk; b++)
            {
                buf[bytes_read + b] = sec_buf[b];
            }
            bytes_read += chunk;
        }

        unsigned int fat_sec_num = fat_start_lba + ((cluster * 2) / ATA_SECTOR_SIZE);
        unsigned int fat_offset = (cluster * 2) % ATA_SECTOR_SIZE;
        if (ata_read_sector(fat_sec_num, sec_buf) != 0)
            break;

        cluster = sec_buf[fat_offset] | (sec_buf[fat_offset + 1] << 8);
    }

    return bytes_read;
}

int fat16_write_file(const char *name, const unsigned char *buf, unsigned int len)
{
    (void)name;
    (void)buf;
    (void)len;
    return 0;
}
