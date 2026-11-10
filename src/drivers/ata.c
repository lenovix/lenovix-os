#include "ata.h"
#include "io_util.h"

#define ATA_DATA 0x1F0
#define ATA_ERROR 0x1F1
#define ATA_SECT_COUNT 0x1F2
#define ATA_LBA_LO 0x1F3
#define ATA_LBA_MID 0x1F4
#define ATA_LBA_HI 0x1F5
#define ATA_DRIVE_HEAD 0x1F6
#define ATA_STATUS 0x1F7
#define ATA_COMMAND 0x1F7

#define ATA_STATUS_ERR 0x01
#define ATA_STATUS_DRQ 0x08
#define ATA_STATUS_DF 0x20
#define ATA_STATUS_BSY 0x80

#define ATA_CMD_READ_PIO 0x20
#define ATA_CMD_WRITE_PIO 0x30
#define ATA_CMD_CACHE_FLUSH 0xE7

static int ata_present = 0;

static int ata_wait_bsy(void)
{
    int timeout = 100000;
    while ((inb(ATA_STATUS) & ATA_STATUS_BSY) && timeout--)
    {
    }
    return timeout > 0 ? 0 : -1;
}

static int ata_wait_drq(void)
{
    int timeout = 100000;
    while (timeout--)
    {
        unsigned char status = inb(ATA_STATUS);
        if (status & ATA_STATUS_ERR)
            return -1;
        if (status & ATA_STATUS_DF)
            return -1;
        if ((status & ATA_STATUS_DRQ) && !(status & ATA_STATUS_BSY))
            return 0;
    }
    return -1;
}

int ata_init(void)
{
    outb(ATA_DRIVE_HEAD, 0xA0);
    outb(ATA_SECT_COUNT, 0);
    outb(ATA_LBA_LO, 0);
    outb(ATA_LBA_MID, 0);
    outb(ATA_LBA_HI, 0);

    unsigned char status = inb(ATA_STATUS);
    if (status == 0xFF)
    {
        ata_present = 0;
        return -1;
    }

    ata_wait_bsy();
    ata_present = 1;
    return 0;
}

int ata_read_sector(unsigned int lba, unsigned char *buffer)
{
    if (!buffer)
        return -1;
    if (ata_wait_bsy() != 0)
        return -2;

    outb(ATA_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_SECT_COUNT, 1);
    outb(ATA_LBA_LO, (unsigned char)lba);
    outb(ATA_LBA_MID, (unsigned char)(lba >> 8));
    outb(ATA_LBA_HI, (unsigned char)(lba >> 16));
    outb(ATA_COMMAND, ATA_CMD_READ_PIO);

    if (ata_wait_drq() != 0)
        return -3;

    unsigned short *ptr = (unsigned short *)buffer;
    for (int i = 0; i < 256; i++)
    {
        ptr[i] = inw(ATA_DATA);
    }

    return 0;
}

int ata_write_sector(unsigned int lba, const unsigned char *buffer)
{
    if (!buffer)
        return -1;
    if (ata_wait_bsy() != 0)
        return -2;

    outb(ATA_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_SECT_COUNT, 1);
    outb(ATA_LBA_LO, (unsigned char)lba);
    outb(ATA_LBA_MID, (unsigned char)(lba >> 8));
    outb(ATA_LBA_HI, (unsigned char)(lba >> 16));
    outb(ATA_COMMAND, ATA_CMD_WRITE_PIO);

    if (ata_wait_drq() != 0)
        return -3;

    const unsigned short *ptr = (const unsigned short *)buffer;
    for (int i = 0; i < 256; i++)
    {
        outw(ATA_DATA, ptr[i]);
    }

    outb(ATA_COMMAND, ATA_CMD_CACHE_FLUSH);
    ata_wait_bsy();

    return 0;
}
