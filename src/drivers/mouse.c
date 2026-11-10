#include "mouse.h"
#include "vesa.h"
#include "io_util.h"

static mouse_state_t mouse_state = {512, 384, 0, 0, 0};
static unsigned char mouse_cycle = 0;
static char mouse_packet[3];
static unsigned int mouse_timeout = 0;
#define MOUSE_TIMEOUT_MAX 50000

static const unsigned char cursor_sprite[18][12] = {
    {1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0, 0},
    {1, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0, 0},
    {1, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0, 0},
    {1, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0, 0},
    {1, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0, 0},
    {1, 2, 2, 2, 2, 2, 2, 2, 1, 0, 0, 0},
    {1, 2, 2, 2, 2, 2, 2, 2, 2, 1, 0, 0},
    {1, 2, 2, 2, 2, 2, 1, 1, 1, 1, 1, 0},
    {1, 2, 2, 1, 2, 2, 1, 0, 0, 0, 0, 0},
    {1, 2, 1, 0, 1, 2, 2, 1, 0, 0, 0, 0},
    {1, 1, 0, 0, 1, 2, 2, 1, 0, 0, 0, 0},
    {1, 0, 0, 0, 0, 1, 2, 2, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 2, 2, 1, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}};

static void mouse_wait(unsigned char type)
{
    unsigned int timeout = 100000;
    if (type == 0)
    {
        while (timeout--)
        {
            if ((inb(0x64) & 1) == 1)
                return;
        }
    }
    else
    {
        while (timeout--)
        {
            if ((inb(0x64) & 2) == 0)
                return;
        }
    }
}

static void mouse_write(unsigned char write_val)
{
    mouse_wait(1);
    outb(0x64, 0xD4);
    mouse_wait(1);
    outb(0x60, write_val);
}

static unsigned char mouse_read(void)
{
    mouse_wait(0);
    return inb(0x60);
}

void mouse_init(void)
{
    mouse_wait(1);
    outb(0x64, 0xA8);

    mouse_wait(1);
    outb(0x64, 0x20);
    mouse_wait(0);
    unsigned char status = inb(0x60);

    status |= 0x02;
    status &= ~0x20;

    mouse_wait(1);
    outb(0x64, 0x60);
    mouse_wait(1);
    outb(0x60, status);

    mouse_write(0xF6);
    mouse_read();

    mouse_write(0xF4);
    mouse_read();
}

void mouse_handler(void)
{
    unsigned char status = inb(0x64);
    if (!(status & 0x01))
    {
        mouse_timeout++;
        if (mouse_timeout > MOUSE_TIMEOUT_MAX)
        {
            mouse_cycle = 0;
            mouse_timeout = 0;
        }
        return;
    }

    unsigned char data = inb(0x60);
    mouse_timeout = 0;

    if (!(status & 0x20) && mouse_cycle == 0)
        return;

    switch (mouse_cycle)
    {
    case 0:
        if ((data & 0x08) == 0x08)
        {
            mouse_packet[0] = data;
            mouse_cycle++;
        }
        else
        {
            mouse_cycle = 0;
            if ((data & 0x08) == 0x08)
            {
                mouse_packet[0] = data;
                mouse_cycle++;
            }
        }
        break;
    case 1:
        mouse_packet[1] = data;
        mouse_cycle++;
        break;
    case 2:
        mouse_packet[2] = data;
        mouse_cycle = 0;

        mouse_state.left_button = mouse_packet[0] & 0x01;
        mouse_state.right_button = (mouse_packet[0] >> 1) & 0x01;
        mouse_state.middle_button = (mouse_packet[0] >> 2) & 0x01;

        int rel_x = (int)mouse_packet[1];
        int rel_y = (int)mouse_packet[2];

        if (mouse_packet[0] & 0x10)
            rel_x |= 0xFFFFFF00;
        if (mouse_packet[0] & 0x20)
            rel_y |= 0xFFFFFF00;

        mouse_state.x += rel_x;
        mouse_state.y -= rel_y;

        if (mouse_state.x < 0)
            mouse_state.x = 0;
        if (mouse_state.y < 0)
            mouse_state.y = 0;
        if (mouse_state.x >= 1024)
            mouse_state.x = 1023;
        if (mouse_state.y >= 768)
            mouse_state.y = 767;
        break;
    default:
        mouse_cycle = 0;
        break;
    }
}

mouse_state_t *mouse_get_state(void)
{
    return &mouse_state;
}

void vesa_draw_cursor(int x, int y)
{
    for (int row = 0; row < 18; row++)
    {
        for (int col = 0; col < 12; col++)
        {
            unsigned char pixel = cursor_sprite[row][col];
            if (pixel == 1)
            {
                vesa_put_pixel(x + col, y + row, 0x000000);
            }
            else if (pixel == 2)
            {
                vesa_put_pixel(x + col, y + row, 0xFFFFFF);
            }
        }
    }
}