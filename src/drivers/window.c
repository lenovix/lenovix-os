#include "window.h"
#include "vesa.h"
#include "terminal.h"
#include "vfs.h"
#include "bmp.h"

extern terminal_t main_term;
extern unsigned int pmm_get_free_block_count(void);
extern unsigned int pmm_get_max_blocks(void);
extern unsigned int timer_ticks;

static void int_to_str_local(unsigned int num, char *str)
{
    if (num == 0)
    {
        str[0] = '0';
        str[1] = '\0';
        return;
    }
    char temp[16];
    int i = 0;
    while (num > 0)
    {
        temp[i++] = (num % 10) + '0';
        num /= 10;
    }
    int j = 0;
    while (i > 0)
    {
        str[j++] = temp[--i];
    }
    str[j] = '\0';
}

void window_init(window_t *win, int x, int y, int w, int h, const char *title, int app_type)
{
    win->x = x;
    win->y = y;
    win->width = w;
    win->height = h;
    win->title = title;
    win->app_type = app_type;
    win->is_dragging = 0;
    win->is_visible = (app_type == WIN_APP_TERMINAL);
    win->drag_offset_x = 0;
    win->drag_offset_y = 0;
}

static void draw_explorer_app(window_t *win)
{
    int app_x = win->x + 10;
    int app_y = win->y + 35;
    int app_w = win->width - 20;
    int app_h = win->height - 45;

    vesa_draw_rect(app_x, app_y, app_w, app_h, 0x1E1E1E);
    vesa_draw_string(app_x + 10, app_y + 10, "Virtual File System (VFS):", 0x2ECC40);

    int count = vfs_get_count();
    char line[40];

    for (int i = 0; i < count && i < 7; i++)
    {
        vfs_node_t *node = vfs_get_node(i);
        if (!node)
            continue;

        int row_y = app_y + 35 + (i * 24);
        vesa_draw_rect(app_x + 10, row_y, app_w - 20, 20, (i % 2 == 0) ? 0x2A2A2A : 0x333333);

        int j = 0;
        line[j++] = '[';
        line[j++] = 'F';
        line[j++] = ']';
        line[j++] = ' ';

        int k = 0;
        while (node->name[k] && j < 22)
        {
            line[j++] = node->name[k++];
        }
        line[j++] = ' ';
        line[j++] = '(';

        char sz[12];
        int_to_str_local(node->length, sz);
        int s = 0;
        while (sz[s] && j < 35)
        {
            line[j++] = sz[s++];
        }
        line[j++] = 'B';
        line[j++] = ')';
        line[j] = '\0';

        vesa_draw_string(app_x + 15, row_y + 2, line, 0xFFFFFF);
    }
}

static void draw_sysinfo_app(window_t *win)
{
    int app_x = win->x + 10;
    int app_y = win->y + 35;
    int app_w = win->width - 20;
    int app_h = win->height - 45;

    vesa_draw_rect(app_x, app_y, app_w, app_h, 0x111111);
    vesa_draw_string(app_x + 10, app_y + 10, "Lenovix OS Task & System Monitor", 0x0074D9);

    unsigned int free_blks = pmm_get_free_block_count();
    unsigned int max_blks = pmm_get_max_blocks();
    if (max_blks == 0)
        max_blks = 1;

    unsigned int used_blks = (max_blks > free_blks) ? (max_blks - free_blks) : 0;
    int bar_width = app_w - 40;
    int fill_width = (used_blks * bar_width) / max_blks;
    if (fill_width > bar_width)
        fill_width = bar_width;

    vesa_draw_string(app_x + 10, app_y + 35, "RAM Usage (PMM 4KB Pages):", 0xAAAAAA);
    vesa_draw_rect(app_x + 10, app_y + 55, bar_width, 16, 0x333333);
    vesa_draw_rect(app_x + 10, app_y + 55, fill_width, 16, 0x2ECC40);

    char mem_info[40];
    char temp[16];
    int mi = 0;
    const char *p = "Free: ";
    while (*p)
        mem_info[mi++] = *p++;
    int_to_str_local((free_blks * 4) / 1024, temp);
    int ti = 0;
    while (temp[ti])
        mem_info[mi++] = temp[ti++];
    p = "MB / ";
    while (*p)
        mem_info[mi++] = *p++;
    int_to_str_local((max_blks * 4) / 1024, temp);
    ti = 0;
    while (temp[ti])
        mem_info[mi++] = temp[ti++];
    p = "MB";
    while (*p)
        mem_info[mi++] = *p++;
    mem_info[mi] = '\0';
    vesa_draw_string(app_x + 10, app_y + 75, mem_info, 0xFFFFFF);

    char tick_str[40];
    int tki = 0;
    p = "Timer Ticks (PIT): ";
    while (*p)
        tick_str[tki++] = *p++;
    int_to_str_local(timer_ticks, temp);
    ti = 0;
    while (temp[ti])
        tick_str[tki++] = temp[ti++];
    tick_str[tki] = '\0';
    vesa_draw_string(app_x + 10, app_y + 105, tick_str, 0xFFDC00);

    vesa_draw_string(app_x + 10, app_y + 130, "Paging: 2-Level 4KB Active", 0x2ECC40);
    vesa_draw_string(app_x + 10, app_y + 150, "Scheduler: Preemptive PIT IRQ0", 0x2ECC40);
    vesa_draw_string(app_x + 10, app_y + 170, "CPU: x86 Ring 0/3 TSS Loaded", 0x2ECC40);
}

static char notepad_buffer[512] = "Tulis catatan Anda di sini...\n";
static int notepad_len = 31;
static char notepad_filename[32] = "notes.txt";

void notepad_handle_key(char c)
{
    if (c == '\b')
    {
        if (notepad_len > 0)
        {
            notepad_buffer[--notepad_len] = '\0';
        }
    }
    else if (c == '\n')
    {
        if (notepad_len < 500)
        {
            notepad_buffer[notepad_len++] = '\n';
            notepad_buffer[notepad_len] = '\0';
        }
    }
    else if (c >= 32 && c <= 126)
    {
        if (notepad_len < 500)
        {
            notepad_buffer[notepad_len++] = c;
            notepad_buffer[notepad_len] = '\0';
        }
    }
}

void notepad_set_text(const char *name, const char *text)
{
    int i = 0;
    while (name[i] && i < 31)
    {
        notepad_filename[i] = name[i];
        i++;
    }
    notepad_filename[i] = '\0';

    notepad_len = 0;
    if (text)
    {
        while (text[notepad_len] && notepad_len < 500)
        {
            notepad_buffer[notepad_len] = text[notepad_len];
            notepad_len++;
        }
    }
    notepad_buffer[notepad_len] = '\0';
}

const char *notepad_get_text(void)
{
    return notepad_buffer;
}

int explorer_handle_click(window_t *win, int mouse_x, int mouse_y, char *out_name, char *out_content)
{
    if (!win->is_visible || win->app_type != WIN_APP_EXPLORER)
        return -1;

    int app_x = win->x + 10;
    int app_y = win->y + 35;
    int app_w = win->width - 20;

    int count = vfs_get_count();
    for (int i = 0; i < count && i < 7; i++)
    {
        int row_y = app_y + 35 + (i * 24);
        if (mouse_x >= app_x + 10 && mouse_x <= app_x + 10 + app_w - 20 &&
            mouse_y >= row_y && mouse_y <= row_y + 20)
        {
            vfs_node_t *node = vfs_get_node(i);
            if (!node)
                return -1;

            int k = 0;
            while (node->name[k] && k < 31)
            {
                out_name[k] = node->name[k];
                k++;
            }
            out_name[k] = '\0';

            int s = 0;
            if (node->data)
            {
                while (node->data[s] && s < 500)
                {
                    out_content[s] = node->data[s];
                    s++;
                }
            }
            out_content[s] = '\0';
            return i;
        }
    }
    return -1;
}

static void draw_notepad_app(window_t *win)
{
    int app_x = win->x + 10;
    int app_y = win->y + 35;
    int app_w = win->width - 20;
    int app_h = win->height - 45;

    vesa_draw_rect(app_x, app_y, app_w, app_h, 0xFFFFFF);

    vesa_draw_rect(app_x, app_y, app_w, 24, 0xEEEEEE);
    vesa_draw_string(app_x + 8, app_y + 5, notepad_filename, 0x000000);

    vesa_draw_rect(app_x + app_w - 75, app_y + 3, 70, 18, 0x2ECC40);
    vesa_draw_string(app_x + app_w - 70, app_y + 4, "[Save]", 0xFFFFFF);

    int cur_x = app_x + 8;
    int cur_y = app_y + 32;
    int col = 0;

    char line_buf[48];
    int line_idx = 0;

    for (int i = 0; i < notepad_len; i++)
    {
        char ch = notepad_buffer[i];
        if (ch == '\n' || col >= 40)
        {
            line_buf[line_idx] = '\0';
            vesa_draw_string(cur_x, cur_y, line_buf, 0x111111);
            cur_y += 18;
            col = 0;
            line_idx = 0;
            if (cur_y > app_y + app_h - 20)
                break;
            if (ch == '\n')
                continue;
        }
        line_buf[line_idx++] = ch;
        col++;
    }
    if (line_idx > 0 && cur_y <= app_y + app_h - 20)
    {
        line_buf[line_idx] = '\0';
        vesa_draw_string(cur_x, cur_y, line_buf, 0x111111);
    }

    int cursor_draw_x = cur_x + (col * 8);
    if (cur_y <= app_y + app_h - 20)
    {
        vesa_draw_string(cursor_draw_x, cur_y, "_", 0x0074D9);
    }
}

static void draw_viewer_app(window_t *win)
{
    int app_x = win->x + 10;
    int app_y = win->y + 35;
    int app_w = win->width - 20;
    int app_h = win->height - 45;

    vesa_draw_rect(app_x, app_y, app_w, app_h, 0x181818);
    vesa_draw_string(app_x + 10, app_y + 10, "Lenovix Image Viewer", 0x00D2FF);

    int canvas_x = app_x + 20;
    int canvas_y = app_y + 35;
    vesa_draw_rect(canvas_x - 2, canvas_y - 2, 132, 132, 0x333333);
    vesa_draw_rect(canvas_x, canvas_y, 128, 128, 0x000000);

    bmp_draw_default_logo(canvas_x, canvas_y);

    vesa_draw_string(app_x + 165, app_y + 50, "Format: BMP 24/32-bit", 0xAAAAAA);
    vesa_draw_string(app_x + 165, app_y + 75, "Renderer: VESA 32bpp", 0xAAAAAA);
    vesa_draw_string(app_x + 165, app_y + 100, "Status: Ready", 0x2ECC40);
}

#define SNAKE_GRID_W 26
#define SNAKE_GRID_H 16
#define SNAKE_CELL_SIZE 14

static int snake_x[128];
static int snake_y[128];
static int snake_len = 3;
static int snake_dir = 1; // 0=U, 1=R, 2=D, 3=L
static int snake_food_x = 15;
static int snake_food_y = 8;
static int snake_score = 0;
static int snake_game_over = 0;
static unsigned int snake_seed = 12345;

static int snake_rand(int max)
{
    snake_seed = snake_seed * 1103515245 + 12345;
    int val = (snake_seed / 65536) % max;
    return val < 0 ? -val : val;
}

void snake_init(void)
{
    snake_len = 3;
    snake_dir = 1;
    snake_x[0] = 6;
    snake_y[0] = 5;
    snake_x[1] = 5;
    snake_y[1] = 5;
    snake_x[2] = 4;
    snake_y[2] = 5;
    snake_food_x = 14;
    snake_food_y = 8;
    snake_score = 0;
    snake_game_over = 0;
}

void snake_handle_key(char c)
{
    if (c == 'w' || c == 'W')
    {
        if (snake_dir != 2)
            snake_dir = 0;
    }
    else if (c == 'd' || c == 'D')
    {
        if (snake_dir != 3)
            snake_dir = 1;
    }
    else if (c == 's' || c == 'S')
    {
        if (snake_dir != 0)
            snake_dir = 2;
    }
    else if (c == 'a' || c == 'A')
    {
        if (snake_dir != 1)
            snake_dir = 3;
    }
    else if (c == 'r' || c == 'R')
    {
        snake_init();
    }
}

void snake_update(void)
{
    if (snake_game_over)
        return;

    int next_hx = snake_x[0];
    int next_hy = snake_y[0];

    if (snake_dir == 0)
        next_hy--;
    else if (snake_dir == 1)
        next_hx++;
    else if (snake_dir == 2)
        next_hy++;
    else if (snake_dir == 3)
        next_hx--;

    if (next_hx < 0 || next_hx >= SNAKE_GRID_W || next_hy < 0 || next_hy >= SNAKE_GRID_H)
    {
        snake_game_over = 1;
        return;
    }

    for (int i = 0; i < snake_len; i++)
    {
        if (snake_x[i] == next_hx && snake_y[i] == next_hy)
        {
            snake_game_over = 1;
            return;
        }
    }

    int eats_food = (next_hx == snake_food_x && next_hy == snake_food_y);
    if (eats_food)
    {
        if (snake_len < 120)
            snake_len++;
        snake_score += 10;
        snake_food_x = snake_rand(SNAKE_GRID_W);
        snake_food_y = snake_rand(SNAKE_GRID_H);
    }

    for (int i = snake_len - 1; i > 0; i--)
    {
        snake_x[i] = snake_x[i - 1];
        snake_y[i] = snake_y[i - 1];
    }
    snake_x[0] = next_hx;
    snake_y[0] = next_hy;
}

static void draw_snake_app(window_t *win)
{
    int app_x = win->x + 10;
    int app_y = win->y + 35;
    int app_w = win->width - 20;
    int app_h = win->height - 45;

    vesa_draw_rect(app_x, app_y, app_w, app_h, 0x0A180E);

    char score_str[40];
    char temp[12];
    int s = 0;
    const char *p = "Score: ";
    while (*p)
        score_str[s++] = *p++;
    int_to_str_local(snake_score, temp);
    int t = 0;
    while (temp[t])
        score_str[s++] = temp[t++];
    p = " | W A S D: Move | R: Reset";
    while (*p && s < 38)
        score_str[s++] = *p++;
    score_str[s] = '\0';
    vesa_draw_string(app_x + 8, app_y + 6, score_str, 0x2ECC40);

    int arena_x = app_x + 8;
    int arena_y = app_y + 26;
    vesa_draw_rect(arena_x - 2, arena_y - 2, (SNAKE_GRID_W * SNAKE_CELL_SIZE) + 4, (SNAKE_GRID_H * SNAKE_CELL_SIZE) + 4, 0x1E3A24);
    vesa_draw_rect(arena_x, arena_y, SNAKE_GRID_W * SNAKE_CELL_SIZE, SNAKE_GRID_H * SNAKE_CELL_SIZE, 0x050D07);

    vesa_draw_rect(arena_x + (snake_food_x * SNAKE_CELL_SIZE) + 2,
                   arena_y + (snake_food_y * SNAKE_CELL_SIZE) + 2,
                   SNAKE_CELL_SIZE - 4, SNAKE_CELL_SIZE - 4, 0xFF4136);

    for (int i = 0; i < snake_len; i++)
    {
        unsigned int color = (i == 0) ? 0x00FF88 : 0x2ECC40;
        vesa_draw_rect(arena_x + (snake_x[i] * SNAKE_CELL_SIZE) + 1,
                       arena_y + (snake_y[i] * SNAKE_CELL_SIZE) + 1,
                       SNAKE_CELL_SIZE - 2, SNAKE_CELL_SIZE - 2, color);
    }

    if (snake_game_over)
    {
        vesa_draw_rect(arena_x + 60, arena_y + 80, 240, 40, 0x111111);
        vesa_draw_string(arena_x + 75, arena_y + 88, "GAME OVER!", 0xFF4136);
        vesa_draw_string(arena_x + 75, arena_y + 104, "Tekan 'R' untuk main lagi", 0xFFFFFF);
    }
}

void window_draw(window_t *win)
{
    if (!win->is_visible)
        return;

    vesa_draw_rect(win->x, win->y, win->width, win->height, 0xC6C6C6);
    vesa_draw_rect(win->x, win->y, win->width, 30, 0x0074D9);

    vesa_draw_rect(win->x + win->width - 35, win->y + 5, 30, 20, 0xFF4136);
    vesa_draw_string(win->x + win->width - 24, win->y + 7, "X", 0xFFFFFF);

    vesa_draw_string(win->x + 10, win->y + 7, win->title, 0xFFFFFF);

    if (win->app_type == WIN_APP_TERMINAL)
    {
        terminal_draw(&main_term, win->x, win->y);
    }
    else if (win->app_type == WIN_APP_EXPLORER)
    {
        draw_explorer_app(win);
    }
    else if (win->app_type == WIN_APP_SYSINFO)
    {
        draw_sysinfo_app(win);
    }
    else if (win->app_type == WIN_APP_NOTEPAD)
    {
        draw_notepad_app(win);
    }
    else if (win->app_type == WIN_APP_VIEWER)
    {
        draw_viewer_app(win);
    }
    else if (win->app_type == WIN_APP_GAME)
    {
        draw_snake_app(win);
    }
}

void window_handle_mouse(window_t *win, int mouse_x, int mouse_y, unsigned char left_btn)
{
    if (!win->is_visible)
        return;

    int titlebar_h = 30;

    if (left_btn)
    {
        if (!win->is_dragging)
        {
            int close_btn_x = win->x + win->width - 35;
            int close_btn_y = win->y + 5;
            int in_close_btn = (mouse_x >= close_btn_x && mouse_x <= close_btn_x + 30 &&
                                mouse_y >= close_btn_y && mouse_y <= close_btn_y + 20);

            if (in_close_btn)
            {
                win->is_visible = 0;
                win->is_dragging = 0;
                return;
            }

            if (win->app_type == WIN_APP_NOTEPAD)
            {
                int app_x = win->x + 10;
                int app_y = win->y + 35;
                int app_w = win->width - 20;
                int save_btn_x = app_x + app_w - 75;
                int save_btn_y = app_y + 3;
                if (mouse_x >= save_btn_x && mouse_x <= save_btn_x + 70 &&
                    mouse_y >= save_btn_y && mouse_y <= save_btn_y + 18)
                {
                    vfs_node_t *f = vfs_find_file(notepad_filename);
                    if (f)
                    {
                        vfs_write_file_cmd(notepad_filename, notepad_buffer);
                    }
                    else
                    {
                        vfs_create_file_raw(notepad_filename, notepad_buffer);
                    }
                    return;
                }
            }

            int in_titlebar = (mouse_x >= win->x && mouse_x <= (win->x + win->width - 40) &&
                               mouse_y >= win->y && mouse_y <= (win->y + titlebar_h));

            if (in_titlebar)
            {
                win->is_dragging = 1;
                win->drag_offset_x = mouse_x - win->x;
                win->drag_offset_y = mouse_y - win->y;
            }
        }
        else
        {
            win->x = mouse_x - win->drag_offset_x;
            win->y = mouse_y - win->drag_offset_y;

            if (win->x < 0)
                win->x = 0;
            if (win->y < 0)
                win->y = 0;
            if (win->x + win->width > 1024)
                win->x = 1024 - win->width;
            if (win->y + win->height > 728)
                win->y = 728 - win->height;
        }
    }
    else
    {
        win->is_dragging = 0;
    }
}
