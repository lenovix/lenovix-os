#include "bmp.h"
#include "vesa.h"

int bmp_draw(const void *data, unsigned int len, int dest_x, int dest_y, int max_w, int max_h)
{
    if (!data || len < sizeof(bmp_file_header_t) + sizeof(bmp_info_header_t))
    {
        return -1;
    }

    const bmp_file_header_t *file_hdr = (const bmp_file_header_t *)data;
    if (file_hdr->type != 0x4D42)
    {
        return -2;
    }

    const bmp_info_header_t *info_hdr = (const bmp_info_header_t *)((const char *)data + sizeof(bmp_file_header_t));
    if (info_hdr->compression != 0)
    {
        return -3;
    }

    int width = info_hdr->width;
    int height = info_hdr->height;
    if (width <= 0)
        return -4;

    int is_bottom_up = 1;
    if (height < 0)
    {
        is_bottom_up = 0;
        height = -height;
    }

    unsigned short bpp = info_hdr->bpp;
    if (bpp != 24 && bpp != 32)
    {
        return -5;
    }

    const unsigned char *pixels = (const unsigned char *)data + file_hdr->offset;
    unsigned int row_bytes = (bpp == 24) ? (((width * 3 + 3) / 4) * 4) : (width * 4);

    for (int y = 0; y < height && y < max_h; y++)
    {
        int src_y = is_bottom_up ? (height - 1 - y) : y;
        const unsigned char *row = pixels + (src_y * row_bytes);

        for (int x = 0; x < width && x < max_w; x++)
        {
            unsigned char b, g, r;
            if (bpp == 24)
            {
                b = row[x * 3 + 0];
                g = row[x * 3 + 1];
                r = row[x * 3 + 2];
            }
            else
            {
                b = row[x * 4 + 0];
                g = row[x * 4 + 1];
                r = row[x * 4 + 2];
            }

            unsigned int color = ((unsigned int)r << 16) | ((unsigned int)g << 8) | b;
            vesa_put_pixel(dest_x + x, dest_y + y, color);
        }
    }

    return 0;
}

void bmp_draw_wallpaper(const void *data, unsigned int len)
{
    if (bmp_draw(data, len, 0, 0, 1024, 728) != 0)
    {
        bmp_draw_default_logo(512 - 64, 364 - 64);
    }
}

void bmp_draw_default_logo(int dest_x, int dest_y)
{
    for (int y = 0; y < 128; y++)
    {
        for (int x = 0; x < 128; x++)
        {
            int dx = x - 64;
            int dy = y - 64;
            int dist_sq = dx * dx + dy * dy;

            if (dist_sq < 60 * 60 && dist_sq > 54 * 54)
            {
                vesa_put_pixel(dest_x + x, dest_y + y, 0x00D2FF);
            }
            else if (dist_sq <= 54 * 54 && dist_sq > 48 * 48)
            {
                vesa_put_pixel(dest_x + x, dest_y + y, 0x0074D9);
            }
            // Huruf L di tengah
            else if ((x >= 50 && x <= 62 && y >= 32 && y <= 92) ||
                     (x >= 50 && x <= 88 && y >= 80 && y <= 92))
            {
                vesa_put_pixel(dest_x + x, dest_y + y, 0xFFFFFF);
            }
        }
    }
}
