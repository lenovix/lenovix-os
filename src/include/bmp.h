#ifndef BMP_H
#define BMP_H

typedef struct
{
    unsigned short type;
    unsigned int size;
    unsigned short reserved1;
    unsigned short reserved2;
    unsigned int offset;
} __attribute__((packed)) bmp_file_header_t;

typedef struct
{
    unsigned int size;
    int width;
    int height;
    unsigned short planes;
    unsigned short bpp;
    unsigned int compression;
    unsigned int image_size;
    int x_ppm;
    int y_ppm;
    unsigned int colors_used;
    unsigned int colors_important;
} __attribute__((packed)) bmp_info_header_t;

int bmp_draw(const void *data, unsigned int len, int dest_x, int dest_y, int max_w, int max_h);
void bmp_draw_wallpaper(const void *data, unsigned int len);
void bmp_draw_default_logo(int dest_x, int dest_y);

#endif
