#include "rtc.h"
#include "io_util.h"

#define CMOS_ADDRESS 0x70
#define CMOS_DATA 0x71

static int rtc_is_updating(void)
{
    outb(CMOS_ADDRESS, 0x0A);
    return (inb(CMOS_DATA) & 0x80);
}

static unsigned char rtc_read_reg(unsigned char reg)
{
    outb(CMOS_ADDRESS, reg);
    return inb(CMOS_DATA);
}

void rtc_init(void)
{
    outb(CMOS_ADDRESS, 0x0B);
    unsigned char prev = inb(CMOS_DATA);
    outb(CMOS_ADDRESS, 0x0B);
    outb(CMOS_DATA, prev | 0x02);
}

rtc_time_t rtc_get_time(void)
{
    rtc_time_t t;

    while (rtc_is_updating())
    {
    }

    t.second = rtc_read_reg(0x00);
    t.minute = rtc_read_reg(0x02);
    t.hour = rtc_read_reg(0x04);
    t.day = rtc_read_reg(0x07);
    t.month = rtc_read_reg(0x08);
    t.year = rtc_read_reg(0x09);

    unsigned char register_b = rtc_read_reg(0x0B);

    if (!(register_b & 0x04))
    {
        t.second = ((t.second & 0xF0) >> 4) * 10 + (t.second & 0x0F);
        t.minute = ((t.minute & 0xF0) >> 4) * 10 + (t.minute & 0x0F);
        t.hour = ((t.hour & 0xF0) >> 4) * 10 + (t.hour & 0x0F);
        t.day = ((t.day & 0xF0) >> 4) * 10 + (t.day & 0x0F);
        t.month = ((t.month & 0xF0) >> 4) * 10 + (t.month & 0x0F);
        t.year = ((t.year & 0xF0) >> 4) * 10 + (t.year & 0x0F);
    }

    t.year += 2000;
    return t;
}

void rtc_get_time_str(char *buf)
{
    rtc_time_t t = rtc_get_time();

    buf[0] = '0' + (t.hour / 10);
    buf[1] = '0' + (t.hour % 10);
    buf[2] = ':';
    buf[3] = '0' + (t.minute / 10);
    buf[4] = '0' + (t.minute % 10);
    buf[5] = ':';
    buf[6] = '0' + (t.second / 10);
    buf[7] = '0' + (t.second % 10);
    buf[8] = '\0';
}
