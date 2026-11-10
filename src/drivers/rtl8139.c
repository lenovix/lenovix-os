#include "rtl8139.h"
#include "pci.h"
#include "io_util.h"

#define RTL8139_VENDOR_ID 0x10EC
#define RTL8139_DEVICE_ID 0x8139

static unsigned short io_base = 0;
static unsigned char mac_addr[6] = {0};
static int is_device_present = 0;

static unsigned char rx_buffer[8192 + 2048] __attribute__((aligned(4)));
static unsigned char tx_buffer[2048] __attribute__((aligned(4)));

static unsigned int packets_tx = 0;
static unsigned int packets_rx = 0;
static unsigned char current_tx_desc = 0;
static unsigned int rx_offset = 0;

int rtl8139_init(void)
{
    pci_device_t *dev = pci_find_device(RTL8139_VENDOR_ID, RTL8139_DEVICE_ID);
    if (!dev)
    {
        is_device_present = 0;
        return -1;
    }

    io_base = (unsigned short)(dev->bar0 & ~0x3);
    if (io_base == 0)
    {
        is_device_present = 0;
        return -2;
    }

    unsigned short pci_cmd = pci_read_config_word(dev->bus, dev->slot, dev->func, 0x04);
    pci_cmd |= 0x0005;
    pci_write_config_word(dev->bus, dev->slot, dev->func, 0x04, pci_cmd);

    outb(io_base + 0x52, 0x00);

    outb(io_base + 0x37, 0x10);
    int timeout = 10000;
    while ((inb(io_base + 0x37) & 0x10) != 0 && timeout--)
    {
    }

    for (int i = 0; i < 6; i++)
    {
        mac_addr[i] = inb(io_base + i);
    }

    outl(io_base + 0x30, (unsigned int)rx_buffer);

    outw(io_base + 0x3C, 0x0005);

    outl(io_base + 0x44, 0x0000008F);

    outb(io_base + 0x37, 0x0C);

    is_device_present = 1;
    return 0;
}

int rtl8139_is_present(void)
{
    return is_device_present;
}

void rtl8139_get_mac(unsigned char *mac_out)
{
    if (!mac_out)
        return;
    for (int i = 0; i < 6; i++)
    {
        mac_out[i] = mac_addr[i];
    }
}

int rtl8139_send_packet(const void *data, unsigned int len)
{
    if (!is_device_present || !data || len == 0 || len > 1792)
    {
        return -1;
    }

    const unsigned char *src = (const unsigned char *)data;
    for (unsigned int i = 0; i < len; i++)
    {
        tx_buffer[i] = src[i];
    }

    unsigned short tsd_port = io_base + 0x10 + (current_tx_desc * 4);
    unsigned short tsad_port = io_base + 0x20 + (current_tx_desc * 4);

    outl(tsad_port, (unsigned int)tx_buffer);
    outl(tsd_port, len & 0x1FFF);

    current_tx_desc = (current_tx_desc + 1) % 4;
    packets_tx++;

    return 0;
}

int rtl8139_receive_packet(unsigned char *out_buf, unsigned int max_len)
{
    if (!is_device_present || !out_buf)
        return 0;

    if (inb(io_base + 0x37) & 0x01)
    {
        return 0;
    }

    unsigned short *header = (unsigned short *)(rx_buffer + rx_offset);
    unsigned short status = header[0];
    unsigned short length = header[1];

    if (!(status & 0x01) || length < 4 || length > 1792)
    {
        outb(io_base + 0x37, 0x04);
        outb(io_base + 0x37, 0x0C);
        rx_offset = 0;
        return 0;
    }

    unsigned int packet_len = length - 4;
    if (packet_len > max_len)
        packet_len = max_len;

    const unsigned char *src = rx_buffer + rx_offset + 4;
    for (unsigned int i = 0; i < packet_len; i++)
    {
        out_buf[i] = src[i];
    }

    packets_rx++;
    rx_offset = (rx_offset + length + 4 + 3) & ~3;
    rx_offset %= 8192;
    outw(io_base + 0x38, (unsigned short)(rx_offset - 16));
    outw(io_base + 0x3E, 0x0001);
    return packet_len;
}

void rtl8139_get_stats(unsigned int *tx_count, unsigned int *rx_count)
{
    if (tx_count)
        *tx_count = packets_tx;
    if (rx_count)
        *rx_count = packets_rx;
}
