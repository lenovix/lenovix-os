#include "window.h"
#include "vesa.h"
#include "terminal.h"
#include "vfs.h"
#include "bmp.h"
#include "task.h"
#include "heap.h"

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
    vesa_draw_string(app_x + 10, app_y + 8, "Task & System Monitor", 0x0074D9);

    unsigned int free_blks = pmm_get_free_block_count();
    unsigned int max_blks = pmm_get_max_blocks();
    if (max_blks == 0)
        max_blks = 1;

    unsigned int used_blks = (max_blks > free_blks) ? (max_blks - free_blks) : 0;
    int bar_width = app_w - 20;
    int fill_width = (used_blks * bar_width) / max_blks;
    if (fill_width > bar_width)
        fill_width = bar_width;

    vesa_draw_rect(app_x + 10, app_y + 26, bar_width, 10, 0x333333);
    vesa_draw_rect(app_x + 10, app_y + 26, fill_width, 10, 0x2ECC40);

    char mem_info[40];
    char temp[16];
    int mi = 0;
    const char *p = "RAM: ";
    while (*p)
        mem_info[mi++] = *p++;
    int_to_str_local((free_blks * 4) / 1024, temp);
    int ti = 0;
    while (temp[ti])
        mem_info[mi++] = temp[ti++];
    p = "/";
    while (*p)
        mem_info[mi++] = *p++;
    int_to_str_local((max_blks * 4) / 1024, temp);
    ti = 0;
    while (temp[ti])
        mem_info[mi++] = temp[ti++];
    p = "MB Free";
    while (*p)
        mem_info[mi++] = *p++;
    mem_info[mi] = '\0';
    vesa_draw_string(app_x + 10, app_y + 40, mem_info, 0xAAAAAA);
    vesa_draw_rect(app_x + 10, app_y + 58, app_w - 20, 18, 0x222222);
    vesa_draw_string(app_x + 14, app_y + 62, "PID", 0x00D2FF);
    vesa_draw_string(app_x + 50, app_y + 62, "PROCESS NAME", 0x00D2FF);
    vesa_draw_string(app_x + 190, app_y + 62, "STATUS", 0x00D2FF);
    vesa_draw_string(app_x + app_w - 65, app_y + 62, "ACTION", 0x00D2FF);

    task_info_t tasks[5];
    int tcount = task_get_list(tasks, 5);

    for (int i = 0; i < tcount; i++)
    {
        int row_y = app_y + 80 + (i * 22);
        vesa_draw_rect(app_x + 10, row_y, app_w - 20, 20, (i % 2 == 0) ? 0x1A1A1A : 0x262626);

        char pid_str[8];
        int_to_str_local(tasks[i].id, pid_str);
        vesa_draw_string(app_x + 14, row_y + 3, pid_str, 0xFFFFFF);

        vesa_draw_string(app_x + 50, row_y + 3, tasks[i].name, 0xFFFFFF);

        const char *st = (tasks[i].state == TASK_RUNNING) ? "RUN" : (tasks[i].state == TASK_READY) ? "READY"
                                                                                                   : "STOP";
        unsigned int sc = (tasks[i].state == TASK_RUNNING) ? 0x2ECC40 : (tasks[i].state == TASK_READY) ? 0xFFDC00
                                                                                                       : 0x888888;
        vesa_draw_string(app_x + 190, row_y + 3, st, sc);

        if (tasks[i].id != 0 && tasks[i].state != TASK_TERMINATED)
        {
            vesa_draw_rect(app_x + app_w - 68, row_y + 2, 45, 16, 0xFF4136);
            vesa_draw_string(app_x + app_w - 62, row_y + 3, "Kill", 0xFFFFFF);
        }
    }
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

static int calc_stored = 0;
static int calc_current = 0;
static char calc_op = 0;
static int calc_reset_next = 1;
static char calc_disp_str[16] = "0";

void calc_init(void)
{
    calc_stored = 0;
    calc_current = 0;
    calc_op = 0;
    calc_reset_next = 1;
    calc_disp_str[0] = '0';
    calc_disp_str[1] = '\0';
}

static void calc_update_display(int val)
{
    calc_current = val;
    int is_neg = (val < 0);
    if (is_neg)
        val = -val;

    char tmp[16];
    int idx = 0;
    if (val == 0)
    {
        tmp[idx++] = '0';
    }
    else
    {
        while (val > 0 && idx < 12)
        {
            tmp[idx++] = '0' + (val % 10);
            val /= 10;
        }
    }
    int d = 0;
    if (is_neg)
        calc_disp_str[d++] = '-';
    while (idx > 0)
    {
        calc_disp_str[d++] = tmp[--idx];
    }
    calc_disp_str[d] = '\0';
}

static void calc_eval(void)
{
    if (calc_op == '+')
        calc_stored += calc_current;
    else if (calc_op == '-')
        calc_stored -= calc_current;
    else if (calc_op == '*')
        calc_stored *= calc_current;
    else if (calc_op == '/')
    {
        if (calc_current != 0)
            calc_stored /= calc_current;
        else
            calc_stored = 0;
    }
    else
    {
        calc_stored = calc_current;
    }
    calc_update_display(calc_stored);
    calc_reset_next = 1;
}

void calc_handle_key(char c)
{
    if (c >= '0' && c <= '9')
    {
        int digit = c - '0';
        if (calc_reset_next)
        {
            calc_current = digit;
            calc_reset_next = 0;
        }
        else
        {
            calc_current = calc_current * 10 + digit;
        }
        calc_update_display(calc_current);
    }
    else if (c == '+' || c == '-' || c == '*' || c == '/')
    {
        if (calc_op != 0 && !calc_reset_next)
        {
            calc_eval();
        }
        else
        {
            calc_stored = calc_current;
        }
        calc_op = c;
        calc_reset_next = 1;
    }
    else if (c == '=' || c == '\n')
    {
        calc_eval();
        calc_op = 0;
    }
    else if (c == 'c' || c == 'C')
    {
        calc_init();
    }
}

static const char *calc_btn_labels[4][4] = {
    {"7", "8", "9", "/"},
    {"4", "5", "6", "*"},
    {"1", "2", "3", "-"},
    {"C", "0", "=", "+"}};

static void draw_calc_app(window_t *win)
{
    int app_x = win->x + 10;
    int app_y = win->y + 35;
    int app_w = win->width - 20;
    int app_h = win->height - 45;

    vesa_draw_rect(app_x, app_y, app_w, app_h, 0x1E1E1E);
    vesa_draw_rect(app_x + 10, app_y + 10, app_w - 20, 32, 0x0A0A0A);
    vesa_draw_rect(app_x + 9, app_y + 9, app_w - 18, 1, 0x333333);
    vesa_draw_string(app_x + 20, app_y + 18, calc_disp_str, 0x00FF88);

    int btn_start_y = app_y + 50;
    for (int r = 0; r < 4; r++)
    {
        for (int c = 0; c < 4; c++)
        {
            int bx = app_x + 10 + c * (46 + 4);
            int by = btn_start_y + r * (36 + 4);
            unsigned int bg = (c == 3 || (r == 3 && c == 2)) ? 0x0074D9 : (r == 3 && c == 0) ? 0xFF4136
                                                                                             : 0x2A2A2A;
            vesa_draw_rect(bx, by, 46, 36, bg);
            vesa_draw_string(bx + 18, by + 10, calc_btn_labels[r][c], 0xFFFFFF);
        }
    }
}

int calc_handle_click(window_t *win, int mouse_x, int mouse_y)
{
    if (win->app_type != WIN_APP_CALC)
        return 0;
    int app_x = win->x + 10;
    int app_y = win->y + 35;
    int btn_start_y = app_y + 50;

    for (int r = 0; r < 4; r++)
    {
        for (int c = 0; c < 4; c++)
        {
            int bx = app_x + 10 + c * (46 + 4);
            int by = btn_start_y + r * (36 + 4);
            if (mouse_x >= bx && mouse_x <= bx + 46 && mouse_y >= by && mouse_y <= by + 36)
            {
                calc_handle_key(calc_btn_labels[r][c][0]);
                return 1;
            }
        }
    }
    return 0;
}

#define PAINT_CANVAS_W 340
#define PAINT_CANVAS_H 160

static unsigned short paint_canvas[PAINT_CANVAS_H][PAINT_CANVAS_W];
static unsigned int paint_palette[8] = {
    0x000000, // Black
    0xFFFFFF, // White
    0xFF4136, // Red
    0x2ECC40, // Green
    0x0074D9, // Blue
    0xFFDC00, // Yellow
    0x00D2FF, // Cyan
    0xB10DC9  // Purple
};
static int paint_color_idx = 0;
static int paint_brush_size = 2;
static int paint_is_eraser = 0;
static const char *paint_status_msg = "Brush: 2px";

void paint_init(void)
{
    for (int y = 0; y < PAINT_CANVAS_H; y++)
    {
        for (int x = 0; x < PAINT_CANVAS_W; x++)
        {
            paint_canvas[y][x] = 0xFFFF;
        }
    }
    paint_color_idx = 0;
    paint_brush_size = 2;
    paint_is_eraser = 0;
    paint_status_msg = "Canvas Siap";
}

static void paint_save_to_bmp(void)
{
    unsigned int row_bytes = ((PAINT_CANVAS_W * 3 + 3) / 4) * 4;
    unsigned int image_size = row_bytes * PAINT_CANVAS_H;
    unsigned int file_size = 54 + image_size;

    char *bmp_data = (char *)kmalloc(file_size);
    if (!bmp_data)
    {
        paint_status_msg = "Error: Out of memory";
        return;
    }

    bmp_file_header_t *bf = (bmp_file_header_t *)bmp_data;
    bf->type = 0x4D42;
    bf->size = file_size;
    bf->reserved1 = 0;
    bf->reserved2 = 0;
    bf->offset = 54;

    bmp_info_header_t *bi = (bmp_info_header_t *)(bmp_data + 14);
    bi->size = 40;
    bi->width = PAINT_CANVAS_W;
    bi->height = PAINT_CANVAS_H;
    bi->planes = 1;
    bi->bpp = 24;
    bi->compression = 0;
    bi->image_size = image_size;
    bi->x_ppm = 2835;
    bi->y_ppm = 2835;
    bi->colors_used = 0;
    bi->colors_important = 0;

    unsigned char *pix = (unsigned char *)(bmp_data + 54);
    for (int y = 0; y < PAINT_CANVAS_H; y++)
    {
        int src_y = PAINT_CANVAS_H - 1 - y;
        unsigned char *row = pix + (y * row_bytes);

        for (int x = 0; x < PAINT_CANVAS_W; x++)
        {
            unsigned short c16 = paint_canvas[src_y][x];
            unsigned char r, g, b;
            if (c16 == 0xFFFF)
            {
                r = 255;
                g = 255;
                b = 255;
            }
            else if (c16 < 8)
            {
                unsigned int c32 = paint_palette[c16];
                r = (c32 >> 16) & 0xFF;
                g = (c32 >> 8) & 0xFF;
                b = c32 & 0xFF;
            }
            else
            {
                r = 0;
                g = 0;
                b = 0;
            }
            row[x * 3 + 0] = b;
            row[x * 3 + 1] = g;
            row[x * 3 + 2] = r;
        }
    }

    vfs_node_t *existing = vfs_find_file("drawing.bmp");
    if (existing)
    {
        if (existing->data)
            kfree(existing->data);
        existing->data = bmp_data;
        existing->length = file_size;
    }
    else
    {
        vfs_create_file_raw("drawing.bmp", "");
        vfs_node_t *created = vfs_find_file("drawing.bmp");
        if (created)
        {
            if (created->data)
                kfree(created->data);
            created->data = bmp_data;
            created->length = file_size;
        }
        else
        {
            kfree(bmp_data);
            paint_status_msg = "Error: VFS Save";
            return;
        }
    }
    paint_status_msg = "Saved: drawing.bmp";
}

static void draw_paint_app(window_t *win)
{
    int app_x = win->x + 8;
    int app_y = win->y + 34;
    int app_w = win->width - 16;
    int app_h = win->height - 42;

    vesa_draw_rect(app_x, app_y, app_w, app_h, 0x222222);

    int tb_y = app_y + 4;
    vesa_draw_rect(app_x + 4, tb_y, app_w - 8, 30, 0x1A1A1A);

    for (int i = 0; i < 8; i++)
    {
        int px = app_x + 8 + (i * 22);
        int py = tb_y + 6;
        vesa_draw_rect(px, py, 18, 18, paint_palette[i]);
        if (!paint_is_eraser && paint_color_idx == i)
        {
            vesa_draw_rect(px - 1, py - 1, 20, 1, 0xFFFFFF);
            vesa_draw_rect(px - 1, py + 18, 20, 1, 0xFFFFFF);
            vesa_draw_rect(px - 1, py, 1, 18, 0xFFFFFF);
            vesa_draw_rect(px + 18, py, 1, 18, 0xFFFFFF);
        }
    }

    int bx1 = app_x + 190;
    vesa_draw_rect(bx1, tb_y + 5, 26, 20, (paint_brush_size == 1 && !paint_is_eraser) ? 0x0074D9 : 0x333333);
    vesa_draw_string(bx1 + 7, tb_y + 7, "1p", 0xFFFFFF);

    int bx2 = app_x + 220;
    vesa_draw_rect(bx2, tb_y + 5, 26, 20, (paint_brush_size == 2 && !paint_is_eraser) ? 0x0074D9 : 0x333333);
    vesa_draw_string(bx2 + 7, tb_y + 7, "2p", 0xFFFFFF);

    int bx4 = app_x + 250;
    vesa_draw_rect(bx4, tb_y + 5, 26, 20, (paint_brush_size == 4 && !paint_is_eraser) ? 0x0074D9 : 0x333333);
    vesa_draw_string(bx4 + 7, tb_y + 7, "4p", 0xFFFFFF);

    int er_x = app_x + 282;
    vesa_draw_rect(er_x, tb_y + 5, 30, 20, paint_is_eraser ? 0xFFDC00 : 0x333333);
    vesa_draw_string(er_x + 5, tb_y + 7, "Era", paint_is_eraser ? 0x000000 : 0xFFFFFF);

    int clr_x = app_x + 316;
    vesa_draw_rect(clr_x, tb_y + 5, 38, 20, 0xFF4136);
    vesa_draw_string(clr_x + 5, tb_y + 7, "Clr", 0xFFFFFF);

    int save_x = app_x + 358;
    vesa_draw_rect(save_x, tb_y + 5, 42, 20, 0x2ECC40);
    vesa_draw_string(save_x + 5, tb_y + 7, "Save", 0xFFFFFF);

    int cv_x = app_x + 32;
    int cv_y = app_y + 38;
    vesa_draw_rect(cv_x - 1, cv_y - 1, PAINT_CANVAS_W + 2, PAINT_CANVAS_H + 2, 0x000000);

    for (int y = 0; y < PAINT_CANVAS_H; y++)
    {
        for (int x = 0; x < PAINT_CANVAS_W; x++)
        {
            unsigned short c16 = paint_canvas[y][x];
            unsigned int col = 0xFFFFFF;
            if (c16 < 8)
            {
                col = paint_palette[c16];
            }
            vesa_put_pixel(cv_x + x, cv_y + y, col);
        }
    }

    int st_y = app_y + app_h - 20;
    vesa_draw_rect(app_x + 4, st_y, app_w - 8, 18, 0x111111);
    vesa_draw_string(app_x + 10, st_y + 2, paint_status_msg, 0x00D2FF);
}

int paint_handle_mouse(window_t *win, int mouse_x, int mouse_y, unsigned char left_btn)
{
    if (win->app_type != WIN_APP_PAINT || !win->is_visible)
        return 0;

    int app_x = win->x + 8;
    int app_y = win->y + 34;
    int tb_y = app_y + 4;

    if (left_btn && mouse_y >= tb_y + 4 && mouse_y <= tb_y + 26)
    {
        for (int i = 0; i < 8; i++)
        {
            int px = app_x + 8 + (i * 22);
            if (mouse_x >= px && mouse_x <= px + 18)
            {
                paint_color_idx = i;
                paint_is_eraser = 0;
                paint_status_msg = "Warna dipilih";
                return 1;
            }
        }

        int bx1 = app_x + 190;
        if (mouse_x >= bx1 && mouse_x <= bx1 + 26)
        {
            paint_brush_size = 1;
            paint_is_eraser = 0;
            paint_status_msg = "Kuas: 1px";
            return 1;
        }

        int bx2 = app_x + 220;
        if (mouse_x >= bx2 && mouse_x <= bx2 + 26)
        {
            paint_brush_size = 2;
            paint_is_eraser = 0;
            paint_status_msg = "Kuas: 2px";
            return 1;
        }

        int bx4 = app_x + 250;
        if (mouse_x >= bx4 && mouse_x <= bx4 + 26)
        {
            paint_brush_size = 4;
            paint_is_eraser = 0;
            paint_status_msg = "Kuas: 4px";
            return 1;
        }

        int er_x = app_x + 282;
        if (mouse_x >= er_x && mouse_x <= er_x + 30)
        {
            paint_is_eraser = 1;
            paint_status_msg = "Mode: Penghapus";
            return 1;
        }

        int clr_x = app_x + 316;
        if (mouse_x >= clr_x && mouse_x <= clr_x + 38)
        {
            for (int y = 0; y < PAINT_CANVAS_H; y++)
                for (int x = 0; x < PAINT_CANVAS_W; x++)
                    paint_canvas[y][x] = 0xFFFF;
            paint_status_msg = "Kanvas dibersihkan";
            return 1;
        }

        int save_x = app_x + 358;
        if (mouse_x >= save_x && mouse_x <= save_x + 42)
        {
            paint_save_to_bmp();
            return 1;
        }
    }

    int cv_x = app_x + 32;
    int cv_y = app_y + 38;

    if (left_btn &&
        mouse_x >= cv_x && mouse_x < cv_x + PAINT_CANVAS_W &&
        mouse_y >= cv_y && mouse_y < cv_y + PAINT_CANVAS_H)
    {
        int cx = mouse_x - cv_x;
        int cy = mouse_y - cv_y;
        unsigned short fill_val = paint_is_eraser ? 0xFFFF : (unsigned short)paint_color_idx;

        int r = (paint_brush_size == 1) ? 0 : (paint_brush_size == 2) ? 1
                                                                      : 2;
        for (int dy = -r; dy <= r; dy++)
        {
            for (int dx = -r; dx <= r; dx++)
            {
                int px = cx + dx;
                int py = cy + dy;
                if (px >= 0 && px < PAINT_CANVAS_W && py >= 0 && py < PAINT_CANVAS_H)
                {
                    paint_canvas[py][px] = fill_val;
                }
            }
        }
        return 1;
    }

    return 0;
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
    else if (win->app_type == WIN_APP_CALC)
    {
        draw_calc_app(win);
    }
    else if (win->app_type == WIN_APP_PAINT)
    {
        draw_paint_app(win);
    }
    else if (win->app_type == WIN_APP_PLAYER)
    {
        player_draw(win);
    }
    else if (win->app_type == WIN_APP_BROWSER)
    {
        browser_draw(win);
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

            if (win->app_type == WIN_APP_TERMINAL)
            {
                if (terminal_handle_click(&main_term, win->x, win->y, mouse_x, mouse_y))
                {
                    return;
                }
            }

            if (win->app_type == WIN_APP_PAINT)
            {
                if (paint_handle_mouse(win, mouse_x, mouse_y, left_btn))
                {
                    return;
                }
            }

            if (win->app_type == WIN_APP_CALC)
            {
                if (calc_handle_click(win, mouse_x, mouse_y))
                {
                    return;
                }
            }

            if (win->app_type == WIN_APP_SYSINFO)
            {
                int app_x = win->x + 10;
                int app_y = win->y + 35;
                int app_w = win->width - 20;
                task_info_t tasks[5];
                int tcount = task_get_list(tasks, 5);
                for (int i = 0; i < tcount; i++)
                {
                    int row_y = app_y + 80 + (i * 22);
                    int kbtn_x = app_x + app_w - 68;
                    if (mouse_x >= kbtn_x && mouse_x <= kbtn_x + 45 &&
                        mouse_y >= row_y + 2 && mouse_y <= row_y + 18)
                    {
                        if (tasks[i].id != 0)
                        {
                            task_kill(tasks[i].id);
                        }
                        return;
                    }
                }
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
