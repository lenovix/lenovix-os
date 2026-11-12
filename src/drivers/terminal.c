#include "terminal.h"
#include "vesa.h"
#include "keyboard.h"
#include "vfs.h"
#include "fat16.h"
#include "ata.h"
#include "io_util.h"
#include "user.h"
#include "elf.h"
#include "pci.h"
#include "rtl8139.h"
#include "speaker.h"
#include "acpi.h"
#include "sb16.h"
#include "net.h"
#include "tar.h"
#include "task.h"
#include "tcp.h"
#include "rtc.h"
#include "basic.h"
#include "audio_codec.h"
#include "window.h"
#include "browser.h"

extern void cmd_taskdemo(void);
extern unsigned int timer_ticks;

extern unsigned int pmm_get_free_block_count(void);
extern unsigned int pmm_get_max_blocks(void);

static int str_equals(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2 == 0;
}

static int str_starts_with(const char *str, const char *prefix)
{
    while (*prefix)
    {
        if (*str != *prefix)
            return 0;
        str++;
        prefix++;
    }
    return 1;
}

static void int_to_str(unsigned int num, char *str)
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

static unsigned int str_to_uint(const char *str)
{
    unsigned int res = 0;
    while (*str >= '0' && *str <= '9')
    {
        res = res * 10 + (*str - '0');
        str++;
    }
    return res;
}

void terminal_write_line_color(terminal_t *term, const char *text, unsigned int color)
{
    if (!term || !text)
        return;

    int target_row = 0;
    if (term->total_lines < TERM_MAX_ROWS)
    {
        target_row = term->total_lines;
        term->total_lines++;
    }
    else
    {
        for (int r = 0; r < TERM_MAX_ROWS - 1; r++)
        {
            for (int c = 0; c < TERM_COLS; c++)
            {
                term->buffer[r][c] = term->buffer[r + 1][c];
            }
            term->buffer_color[r] = term->buffer_color[r + 1];
        }
        target_row = TERM_MAX_ROWS - 1;
    }

    term->scroll_offset = 0;

    unsigned int active_color = color;
    int c = 0;
    int dst = 0;

    while (text[c] != '\0' && dst < TERM_COLS - 1)
    {
        if (text[c] == '\033' && text[c + 1] == '[')
        {
            if (text[c + 2] == '3' && text[c + 3] == '1' && text[c + 4] == 'm')
            {
                active_color = 0xFF4136; // Red
                c += 5;
                continue;
            }
            else if (text[c + 2] == '3' && text[c + 3] == '2' && text[c + 4] == 'm')
            {
                active_color = 0x2ECC40; // Green
                c += 5;
                continue;
            }
            else if (text[c + 2] == '3' && text[c + 3] == '3' && text[c + 4] == 'm')
            {
                active_color = 0xFFDC00; // Yellow
                c += 5;
                continue;
            }
            else if (text[c + 2] == '3' && text[c + 3] == '4' && text[c + 4] == 'm')
            {
                active_color = 0x0074D9; // Blue
                c += 5;
                continue;
            }
            else if (text[c + 2] == '3' && text[c + 3] == '6' && text[c + 4] == 'm')
            {
                active_color = 0x00D2FF; // Cyan
                c += 5;
                continue;
            }
            else if (text[c + 2] == '3' && text[c + 3] == '7' && text[c + 4] == 'm')
            {
                active_color = 0xFFFFFF; // White
                c += 5;
                continue;
            }
            else if (text[c + 2] == '0' && text[c + 3] == 'm')
            {
                active_color = 0x00FF00; // Reset
                c += 4;
                continue;
            }
        }

        term->buffer[target_row][dst++] = text[c++];
    }

    term->buffer[target_row][dst] = '\0';
    term->buffer_color[target_row] = active_color;
    term->cursor_row = (term->total_lines > TERM_VISIBLE_ROWS) ? TERM_VISIBLE_ROWS : term->total_lines;
}

void terminal_scroll_up(terminal_t *term, int lines)
{
    if (!term)
        return;
    int max_scroll = (term->total_lines > TERM_VISIBLE_ROWS) ? (term->total_lines - TERM_VISIBLE_ROWS) : 0;
    term->scroll_offset += lines;
    if (term->scroll_offset > max_scroll)
        term->scroll_offset = max_scroll;
}

void terminal_scroll_down(terminal_t *term, int lines)
{
    if (!term)
        return;
    term->scroll_offset -= lines;
    if (term->scroll_offset < 0)
        term->scroll_offset = 0;
}

void terminal_write_line(terminal_t *term, const char *text)
{
    terminal_write_line_color(term, text, 0x00FF00);
}

void terminal_init(terminal_t *term)
{
    if (!term)
        return;

    for (int r = 0; r < TERM_MAX_ROWS; r++)
    {
        for (int c = 0; c < TERM_COLS; c++)
        {
            term->buffer[r][c] = '\0';
        }
        term->buffer_color[r] = 0x00FF00;
    }
    term->total_lines = 0;
    term->scroll_offset = 0;
    term->input_len = 0;
    term->input_buf[0] = '\0';
    term->cursor_row = 0;
    term->cursor_col = 0;

    terminal_write_line_color(term, "\033[36mLenovix OS Terminal v2.2 (Scrollable)\033[0m", 0x00D2FF);
    terminal_write_line(term, "Ketik 'help' untuk daftar perintah.");
}

static void cmd_ls(terminal_t *term)
{
    int count = vfs_get_count();
    if (count == 0)
    {
        terminal_write_line(term, "Direktori kosong.");
        return;
    }

    char line[TERM_COLS];
    for (int i = 0; i < count; i++)
    {
        vfs_node_t *node = vfs_get_node(i);
        if (!node)
            continue;

        int j = 0;
        while (node->name[j] != '\0' && j < 20)
        {
            line[j] = node->name[j];
            j++;
        }
        line[j++] = ' ';
        line[j++] = '(';

        char sz[12];
        int_to_str(node->length, sz);
        int s = 0;
        while (sz[s] != '\0' && j < TERM_COLS - 4)
        {
            line[j++] = sz[s++];
        }
        line[j++] = 'B';
        line[j++] = ')';
        line[j] = '\0';

        terminal_write_line(term, line);
    }
}

static void cmd_cat(terminal_t *term, const char *filename)
{
    while (*filename == ' ')
        filename++;
    if (*filename == '\0')
    {
        terminal_write_line(term, "Gunakan: cat <nama_file>");
        return;
    }

    vfs_node_t *f = vfs_find_file(filename);
    if (!f || !f->data)
    {
        terminal_write_line(term, "File tidak ditemukan.");
        return;
    }

    terminal_write_line(term, f->data);
}

static void cmd_touch(terminal_t *term, const char *filename)
{
    while (*filename == ' ')
        filename++;
    if (*filename == '\0')
    {
        terminal_write_line(term, "Gunakan: touch <nama_file>");
        return;
    }

    int res = vfs_create_file_raw(filename, "");
    if (res == 0)
    {
        terminal_write_line(term, "File berhasil dibuat.");
    }
    else
    {
        terminal_write_line(term, "Gagal membuat file.");
    }
}

static void cmd_write(terminal_t *term, const char *args)
{
    while (*args == ' ')
        args++;
    char fname[MAX_FILENAME_LEN];
    int i = 0;
    while (*args != ' ' && *args != '\0' && i < MAX_FILENAME_LEN - 1)
    {
        fname[i++] = *args++;
    }
    fname[i] = '\0';

    while (*args == ' ')
        args++;
    if (fname[0] == '\0' || *args == '\0')
    {
        terminal_write_line(term, "Gunakan: write <file> <teks>");
        return;
    }

    vfs_node_t *f = vfs_find_file(fname);
    if (!f)
    {
        vfs_create_file_raw(fname, args);
    }
    else
    {
        vfs_write_file_cmd(fname, args);
    }
    terminal_write_line(term, "File berhasil disimpan.");
}

static void cmd_echo(terminal_t *term, const char *args)
{
    while (*args == ' ')
        args++;
    if (*args == '\0')
    {
        terminal_write_line(term, "");
        return;
    }

    char text[128];
    char target_file[MAX_FILENAME_LEN];
    int mode = 0;
    int redir_pos = -1;

    for (int i = 0; args[i] != '\0'; i++)
    {
        if (args[i] == '>')
        {
            if (args[i + 1] == '>')
            {
                mode = 2;
                redir_pos = i;
            }
            else
            {
                mode = 1;
                redir_pos = i;
            }
            break;
        }
    }

    if (mode == 0)
    {
        int t = 0;
        int start = (*args == '"' || *args == '\'') ? 1 : 0;
        int i = start;
        while (args[i] != '\0' && t < 127)
        {
            text[t++] = args[i++];
        }
        if (start && t > 0 && (text[t - 1] == '"' || text[t - 1] == '\''))
        {
            t--;
        }
        text[t] = '\0';
        terminal_write_line_color(term, text, 0xFFFFFF);
    }
    else
    {
        int t = 0;
        int i = 0;
        while (*args == ' ' && i < redir_pos)
        {
            args++;
            redir_pos--;
        }
        int start = (*args == '"' || *args == '\'') ? 1 : 0;
        i = start;
        int end_t = (start && redir_pos > 0 && (args[redir_pos - 1] == '"' || args[redir_pos - 1] == '\'')) ? redir_pos - 1 : redir_pos;
        while (i < end_t && t < 127)
        {
            text[t++] = args[i++];
        }
        while (t > 0 && text[t - 1] == ' ')
            t--;
        text[t] = '\0';

        const char *f = args + redir_pos + ((mode == 2) ? 2 : 1);
        while (*f == ' ')
            f++;
        int fi = 0;
        while (*f != '\0' && *f != ' ' && fi < MAX_FILENAME_LEN - 1)
        {
            target_file[fi++] = *f++;
        }
        target_file[fi] = '\0';

        if (target_file[0] == '\0')
        {
            terminal_write_line(term, "Gunakan: echo <teks> [>|>>] <file>");
            return;
        }

        if (mode == 1)
        {
            vfs_node_t *existing = vfs_find_file(target_file);
            if (existing)
            {
                vfs_write_file_cmd(target_file, text);
            }
            else
            {
                vfs_create_file_raw(target_file, text);
            }
            terminal_write_line_color(term, "\033[32mOutput ditulis ke file.\033[0m", 0x2ECC40);
        }
        else if (mode == 2)
        {
            vfs_node_t *existing = vfs_find_file(target_file);
            if (existing && existing->length > 0)
            {
                vfs_append_file(target_file, "\n");
            }
            vfs_append_file(target_file, text);
            terminal_write_line_color(term, "\033[32mOutput ditambahkan ke file.\033[0m", 0x2ECC40);
        }
    }
}

static void cmd_mv(terminal_t *term, const char *args)
{
    while (*args == ' ')
        args++;
    char old_name[MAX_FILENAME_LEN];
    char new_name[MAX_FILENAME_LEN];
    int i = 0;
    while (*args != ' ' && *args != '\0' && i < MAX_FILENAME_LEN - 1)
    {
        old_name[i++] = *args++;
    }
    old_name[i] = '\0';
    while (*args == ' ')
        args++;
    int j = 0;
    while (*args != ' ' && *args != '\0' && j < MAX_FILENAME_LEN - 1)
    {
        new_name[j++] = *args++;
    }
    new_name[j] = '\0';

    if (old_name[0] == '\0' || new_name[0] == '\0')
    {
        terminal_write_line(term, "Gunakan: mv <nama_lama> <nama_baru>");
        return;
    }

    int res = vfs_rename_file(old_name, new_name);
    if (res == 0)
    {
        terminal_write_line_color(term, "\033[32mFile berhasil diubah namanya.\033[0m", 0x2ECC40);
    }
    else if (res == -2)
    {
        terminal_write_line_color(term, "\033[31mNama file tujuan sudah ada.\033[0m", 0xFF4136);
    }
    else
    {
        terminal_write_line_color(term, "\033[31mFile asal tidak ditemukan.\033[0m", 0xFF4136);
    }
}

static int str_contains_nocase(const char *haystack, const char *needle)
{
    if (!haystack || !needle)
        return 0;
    if (needle[0] == '\0')
        return 1;

    for (int i = 0; haystack[i] != '\0'; i++)
    {
        int j = 0;
        while (needle[j] != '\0')
        {
            char h = haystack[i + j];
            char n = needle[j];
            if (h == '\0')
                break;
            if (h >= 'A' && h <= 'Z')
                h += ('a' - 'A');
            if (n >= 'A' && n <= 'Z')
                n += ('a' - 'A');
            if (h != n)
                break;
            j++;
        }
        if (needle[j] == '\0')
            return 1;
    }
    return 0;
}

static void cmd_grep(terminal_t *term, const char *args)
{
    while (*args == ' ')
        args++;
    char pattern[MAX_FILENAME_LEN];
    int pidx = 0;

    if (*args == '"' || *args == '\'')
    {
        char quote = *args++;
        while (*args != '\0' && *args != quote && pidx < MAX_FILENAME_LEN - 1)
        {
            pattern[pidx++] = *args++;
        }
        if (*args == quote)
            args++;
    }
    else
    {
        while (*args != ' ' && *args != '\0' && pidx < MAX_FILENAME_LEN - 1)
        {
            pattern[pidx++] = *args++;
        }
    }
    pattern[pidx] = '\0';

    while (*args == ' ')
        args++;
    char fname[MAX_FILENAME_LEN];
    int fidx = 0;
    while (*args != ' ' && *args != '\0' && fidx < MAX_FILENAME_LEN - 1)
    {
        fname[fidx++] = *args++;
    }
    fname[fidx] = '\0';

    if (pattern[0] == '\0' || fname[0] == '\0')
    {
        terminal_write_line(term, "Gunakan: grep <pola> <file>");
        return;
    }

    vfs_node_t *f = vfs_find_file(fname);
    if (!f || !f->data)
    {
        terminal_write_line_color(term, "\033[31mFile tidak ditemukan.\033[0m", 0xFF4136);
        return;
    }

    const char *data = f->data;
    int line_num = 1;
    int matches = 0;
    char cur_line[TERM_COLS];
    int cidx = 0;

    for (size_t i = 0; i <= f->length; i++)
    {
        char ch = data[i];
        if (ch == '\n' || ch == '\0')
        {
            cur_line[cidx] = '\0';
            if (str_contains_nocase(cur_line, pattern))
            {
                matches++;
                char out[TERM_COLS];
                char num[8];
                int_to_str(line_num, num);
                int oi = 0;
                out[oi++] = '[';
                int ni = 0;
                while (num[ni])
                    out[oi++] = num[ni++];
                out[oi++] = ']';
                out[oi++] = ' ';
                int li = 0;
                while (cur_line[li] && oi < TERM_COLS - 1)
                {
                    out[oi++] = cur_line[li++];
                }
                out[oi] = '\0';
                terminal_write_line_color(term, out, 0xFFDC00);
            }
            line_num++;
            cidx = 0;
            if (ch == '\0')
                break;
        }
        else
        {
            if (cidx < TERM_COLS - 8)
            {
                cur_line[cidx++] = ch;
            }
        }
    }

    if (matches == 0)
    {
        terminal_write_line(term, "Pola tidak ditemukan.");
    }
}

static inline void get_cpuid(unsigned int leaf, unsigned int *eax, unsigned int *ebx, unsigned int *ecx, unsigned int *edx)
{
    __asm__ __volatile__(
        "cpuid"
        : "=a"(*eax), "=b"(*ebx), "=c"(*ecx), "=d"(*edx)
        : "a"(leaf));
}

static void cmd_uname(terminal_t *term, const char *args)
{
    while (*args == ' ')
        args++;
    if (*args == '-' && *(args + 1) == 'a')
    {
        terminal_write_line_color(term, "\033[36mLenovixOS 0.5.0 i386 x86_32 Bare-Metal\033[0m", 0x00D2FF);
    }
    else
    {
        terminal_write_line_color(term, "\033[36mLenovixOS\033[0m", 0x00D2FF);
    }
}

static void cmd_systeminfo(terminal_t *term)
{
    terminal_write_line_color(term, "\033[36m=== Lenovix OS Specifications ===\033[0m", 0x00D2FF);

    unsigned int eax = 0, ebx = 0, ecx = 0, edx = 0;
    get_cpuid(0, &eax, &ebx, &ecx, &edx);
    char vendor[13];
    *(unsigned int *)(vendor + 0) = ebx;
    *(unsigned int *)(vendor + 4) = edx;
    *(unsigned int *)(vendor + 8) = ecx;
    vendor[12] = '\0';

    char line[TERM_COLS];
    int j = 0;
    const char *p = "CPU Vendor : ";
    while (*p)
        line[j++] = *p++;
    p = vendor;
    while (*p && j < TERM_COLS - 1)
        line[j++] = *p++;
    line[j] = '\0';
    terminal_write_line(term, line);

    get_cpuid(0x80000000, &eax, &ebx, &ecx, &edx);
    if (eax >= 0x80000004)
    {
        char brand[49];
        get_cpuid(0x80000002, (unsigned int *)(brand + 0), (unsigned int *)(brand + 4), (unsigned int *)(brand + 8), (unsigned int *)(brand + 12));
        get_cpuid(0x80000003, (unsigned int *)(brand + 16), (unsigned int *)(brand + 20), (unsigned int *)(brand + 24), (unsigned int *)(brand + 28));
        get_cpuid(0x80000004, (unsigned int *)(brand + 32), (unsigned int *)(brand + 36), (unsigned int *)(brand + 40), (unsigned int *)(brand + 44));
        brand[48] = '\0';

        const char *bptr = brand;
        while (*bptr == ' ')
            bptr++;

        j = 0;
        p = "Model      : ";
        while (*p)
            line[j++] = *p++;
        while (*bptr && j < TERM_COLS - 1)
            line[j++] = *bptr++;
        line[j] = '\0';
        terminal_write_line(term, line);
    }

    get_cpuid(1, &eax, &ebx, &ecx, &edx);
    j = 0;
    p = "Features   : ";
    while (*p)
        line[j++] = *p++;
    if (edx & (1 << 0))
    {
        line[j++] = 'F';
        line[j++] = 'P';
        line[j++] = 'U';
        line[j++] = ' ';
    }
    if (edx & (1 << 9))
    {
        line[j++] = 'A';
        line[j++] = 'P';
        line[j++] = 'I';
        line[j++] = 'C';
        line[j++] = ' ';
    }
    if (edx & (1 << 23))
    {
        line[j++] = 'M';
        line[j++] = 'M';
        line[j++] = 'X';
        line[j++] = ' ';
    }
    if (edx & (1 << 25))
    {
        line[j++] = 'S';
        line[j++] = 'S';
        line[j++] = 'E';
        line[j++] = ' ';
    }
    if (edx & (1 << 26))
    {
        line[j++] = 'S';
        line[j++] = 'S';
        line[j++] = 'E';
        line[j++] = '2';
        line[j++] = ' ';
    }
    line[j] = '\0';
    terminal_write_line(term, line);

    terminal_write_line(term, "Kernel Arch: x86-32 Protected (Ring 0/3)");
    terminal_write_line(term, "Display    : VESA 1024x768 32bpp Linear");
    terminal_write_line(term, "Networking : RTL8139 (IP/ARP/ICMP/UDP/TCP)");
    terminal_write_line(term, "Sound      : PC Speaker + SoundBlaster 16");
}

static void cmd_free(terminal_t *term)
{
    unsigned int free_blks = pmm_get_free_block_count();
    unsigned int max_blks = pmm_get_max_blocks();

    char line[TERM_COLS];
    char num[16];

    int j = 0;
    const char *p = "PMM Free: ";
    while (*p)
        line[j++] = *p++;

    int_to_str(free_blks, num);
    int s = 0;
    while (num[s])
        line[j++] = num[s++];

    line[j++] = '/';

    int_to_str(max_blks, num);
    s = 0;
    while (num[s])
        line[j++] = num[s++];

    line[j++] = ' ';
    line[j++] = 'b';
    line[j++] = 'l';
    line[j++] = 'k';
    line[j] = '\0';

    terminal_write_line(term, line);
}

static void cmd_ps(terminal_t *term)
{
    task_info_t tasks[8];
    int count = task_get_list(tasks, 8);
    terminal_write_line_color(term, "\033[36mPID  NAME             STATUS\033[0m", 0x00D2FF);
    char line[TERM_COLS];
    for (int i = 0; i < count; i++)
    {
        char pid_str[8];
        int_to_str(tasks[i].id, pid_str);
        int j = 0;
        int p = 0;
        while (pid_str[p])
            line[j++] = pid_str[p++];
        while (j < 5)
            line[j++] = ' ';
        int n = 0;
        while (tasks[i].name[n] && n < 16)
            line[j++] = tasks[i].name[n++];
        while (j < 22)
            line[j++] = ' ';
        const char *st = (tasks[i].state == TASK_RUNNING) ? "RUNNING" : (tasks[i].state == TASK_READY) ? "READY"
                                                                                                       : "STOPPED";
        int s = 0;
        while (st[s] && j < TERM_COLS - 1)
            line[j++] = st[s++];
        line[j] = '\0';
        unsigned int col = (tasks[i].state == TASK_RUNNING) ? 0x2ECC40 : (tasks[i].state == TASK_READY) ? 0xFFDC00
                                                                                                        : 0xAAAAAA;
        terminal_write_line_color(term, line, col);
    }
}

static void cmd_kill(terminal_t *term, const char *args)
{
    while (*args == ' ')
        args++;
    if (*args == '\0')
    {
        terminal_write_line(term, "Gunakan: kill <pid>");
        return;
    }
    int pid = (int)str_to_uint(args);
    int res = task_kill(pid);
    if (res == 0)
    {
        terminal_write_line_color(term, "\033[32mProses dihentikan.\033[0m", 0x2ECC40);
    }
    else if (res == -1)
    {
        terminal_write_line_color(term, "\033[31mTidak bisa mematikan kernel_main!\033[0m", 0xFF4136);
    }
    else
    {
        terminal_write_line_color(term, "\033[31mPID tidak ditemukan.\033[0m", 0xFF4136);
    }
}

static void cmd_diskinfo(terminal_t *term)
{
    int res = ata_init();
    if (res == 0)
    {
        terminal_write_line(term, "ATA Master: Siap (512B/sector)");
    }
    else
    {
        terminal_write_line(term, "ATA Master: Tidak terdeteksi");
    }
}

static void cmd_diskr(terminal_t *term, const char *args)
{
    while (*args == ' ')
        args++;
    if (*args == '\0')
    {
        terminal_write_line(term, "Gunakan: diskr <lba>");
        return;
    }

    unsigned int lba = str_to_uint(args);
    static unsigned char buf[ATA_SECTOR_SIZE];
    int res = ata_read_sector(lba, buf);
    if (res == 0)
    {
        char line[TERM_COLS];
        int j = 0;
        for (j = 0; j < 32 && j < TERM_COLS - 1; j++)
        {
            line[j] = (buf[j] >= 32 && buf[j] <= 126) ? buf[j] : '.';
        }
        line[j] = '\0';
        terminal_write_line(term, line);
    }
    else
    {
        terminal_write_line(term, "Gagal membaca sektor disk.");
    }
}

static void cmd_diskw(terminal_t *term, const char *args)
{
    while (*args == ' ')
        args++;
    char lba_str[12];
    int i = 0;
    while (*args != ' ' && *args != '\0' && i < 11)
    {
        lba_str[i++] = *args++;
    }
    lba_str[i] = '\0';
    while (*args == ' ')
        args++;

    if (lba_str[0] == '\0' || *args == '\0')
    {
        terminal_write_line(term, "Gunakan: diskw <lba> <teks>");
        return;
    }

    unsigned int lba = str_to_uint(lba_str);
    static unsigned char buf[ATA_SECTOR_SIZE];
    for (int k = 0; k < ATA_SECTOR_SIZE; k++)
        buf[k] = 0;

    int idx = 0;
    while (*args != '\0' && idx < ATA_SECTOR_SIZE - 1)
    {
        buf[idx++] = *args++;
    }

    int res = ata_write_sector(lba, buf);
    if (res == 0)
    {
        terminal_write_line(term, "Sektor berhasil ditulis.");
    }
    else
    {
        terminal_write_line(term, "Gagal menulis sektor.");
    }
}

static void cmd_userdemo(terminal_t *term)
{
    terminal_write_line(term, "Memanggil Syscall int 0x80...");
    sys_print("[User Space] Hello via Syscall 1!");
    sys_exit();
}

static void cmd_fatls(terminal_t *term)
{
    int res = fat16_init();
    if (res != 0)
    {
        terminal_write_line(term, "FAT16: Partisi belum terformat.");
        return;
    }

    fat16_file_t files[16];
    int count = fat16_list_root(files, 16);
    if (count == 0)
    {
        terminal_write_line(term, "FAT16: Root direktori kosong.");
        return;
    }

    char line[TERM_COLS];
    for (int i = 0; i < count; i++)
    {
        int j = 0;
        const char *p = files[i].name;
        while (*p && j < 20)
            line[j++] = *p++;
        line[j++] = ' ';
        line[j++] = '(';
        char sz[12];
        int_to_str(files[i].size, sz);
        int s = 0;
        while (sz[s] && j < TERM_COLS - 4)
            line[j++] = sz[s++];
        line[j++] = 'B';
        line[j++] = ')';
        line[j] = '\0';
        terminal_write_line(term, line);
    }
}

static void cmd_fatread(terminal_t *term, const char *filename)
{
    while (*filename == ' ')
        filename++;
    if (*filename == '\0')
    {
        terminal_write_line(term, "Gunakan: fatread <nama_file>");
        return;
    }

    if (!fat16_is_available() && fat16_init() != 0)
    {
        terminal_write_line(term, "FAT16 tidak tersedia.");
        return;
    }

    static unsigned char fbuf[512];
    int n = fat16_read_file(filename, fbuf, 511);
    if (n < 0)
    {
        terminal_write_line(term, "File FAT16 tidak ditemukan.");
        return;
    }

    fbuf[n] = '\0';
    terminal_write_line(term, (char *)fbuf);
}

static void cmd_elfdemo(terminal_t *term)
{
    struct
    {
        elf32_ehdr_t ehdr;
        elf32_phdr_t phdr;
        unsigned char code[16];
    } __attribute__((packed)) test_elf;

    test_elf.ehdr.e_ident[0] = 0x7F;
    test_elf.ehdr.e_ident[1] = 'E';
    test_elf.ehdr.e_ident[2] = 'L';
    test_elf.ehdr.e_ident[3] = 'F';
    test_elf.ehdr.e_ident[4] = 1;
    test_elf.ehdr.e_ident[5] = 1;
    test_elf.ehdr.e_ident[6] = 1;
    for (int i = 7; i < 16; i++)
        test_elf.ehdr.e_ident[i] = 0;
    test_elf.ehdr.e_type = 2;
    test_elf.ehdr.e_machine = 3;
    test_elf.ehdr.e_version = 1;
    test_elf.ehdr.e_phoff = sizeof(elf32_ehdr_t);
    test_elf.ehdr.e_shoff = 0;
    test_elf.ehdr.e_flags = 0;
    test_elf.ehdr.e_ehsize = sizeof(elf32_ehdr_t);
    test_elf.ehdr.e_phentsize = sizeof(elf32_phdr_t);
    test_elf.ehdr.e_phnum = 1;
    test_elf.ehdr.e_shentsize = 0;
    test_elf.ehdr.e_shnum = 0;
    test_elf.ehdr.e_shstrndx = 0;

    static unsigned char load_target[32];
    test_elf.ehdr.e_entry = (unsigned int)load_target;

    test_elf.phdr.p_type = PT_LOAD;
    test_elf.phdr.p_offset = sizeof(elf32_ehdr_t) + sizeof(elf32_phdr_t);
    test_elf.phdr.p_vaddr = (unsigned int)load_target;
    test_elf.phdr.p_paddr = (unsigned int)load_target;
    test_elf.phdr.p_filesz = 4;
    test_elf.phdr.p_memsz = 4;
    test_elf.phdr.p_flags = 7;
    test_elf.phdr.p_align = 4;

    test_elf.code[0] = 0xC3;

    terminal_write_line(term, "Validasi format ELF32...");
    int val = elf_validate(&test_elf, sizeof(test_elf));
    if (val != 0)
    {
        terminal_write_line(term, "ELF: Header tidak valid!");
        return;
    }

    terminal_write_line(term, "Memuat segmen PT_LOAD...");
    unsigned int entry = 0;
    int res = elf_load_binary(&test_elf, sizeof(test_elf), &entry);
    if (res == 0)
    {
        terminal_write_line(term, "ELF: Segmen berhasil dimuat!");
        void (*func)(void) = (void (*)(void))entry;
        func();
        terminal_write_line(term, "ELF: Eksekusi entry point OK.");
    }
    else
    {
        terminal_write_line(term, "ELF: Gagal memuat binary.");
    }
}

static void word_to_hex(unsigned short val, char *buf)
{
    const char hex[] = "0123456789ABCDEF";
    buf[0] = hex[(val >> 12) & 0xF];
    buf[1] = hex[(val >> 8) & 0xF];
    buf[2] = hex[(val >> 4) & 0xF];
    buf[3] = hex[val & 0xF];
    buf[4] = '\0';
}

static void cmd_lspci(terminal_t *term)
{
    int count = pci_get_count();
    if (count == 0)
    {
        terminal_write_line(term, "PCI: Tidak ada device.");
        return;
    }

    char line[TERM_COLS];
    char hex[5];
    const char digits[] = "0123456789ABCDEF";

    for (int i = 0; i < count && i < 8; i++)
    {
        pci_device_t *dev = pci_get_device(i);
        if (!dev)
            continue;

        int j = 0;
        line[j++] = '0' + dev->bus;
        line[j++] = ':';
        line[j++] = (dev->slot < 10) ? ('0' + dev->slot) : ('A' + (dev->slot - 10));
        line[j++] = '.';
        line[j++] = '0' + dev->func;
        line[j++] = ' ';
        line[j++] = '[';

        word_to_hex(dev->vendor_id, hex);
        for (int k = 0; k < 4; k++)
            line[j++] = hex[k];

        line[j++] = ':';

        word_to_hex(dev->device_id, hex);
        for (int k = 0; k < 4; k++)
            line[j++] = hex[k];

        line[j++] = ']';
        line[j++] = ' ';
        line[j++] = 'C';
        line[j++] = ':';

        line[j++] = digits[(dev->class_id >> 4) & 0xF];
        line[j++] = digits[dev->class_id & 0xF];
        line[j] = '\0';

        terminal_write_line(term, line);
    }
}

static void cmd_netinfo(terminal_t *term)
{
    if (!rtl8139_is_present())
    {
        terminal_write_line(term, "RTL8139: NIC tidak terdeteksi di bus PCI.");
        terminal_write_line(term, "Info: Pastikan QEMU pakai opsi: -nic model=rtl8139");
        return;
    }

    unsigned char mac[6];
    rtl8139_get_mac(mac);

    const char hex_chars[] = "0123456789ABCDEF";
    char line[TERM_COLS];
    int j = 0;
    const char *p = "MAC: ";
    while (*p)
        line[j++] = *p++;

    for (int i = 0; i < 6; i++)
    {
        line[j++] = hex_chars[(mac[i] >> 4) & 0xF];
        line[j++] = hex_chars[mac[i] & 0xF];
        if (i < 5)
            line[j++] = ':';
    }
    line[j] = '\0';
    terminal_write_line(term, line);

    unsigned int tx = 0, rx = 0;
    rtl8139_get_stats(&tx, &rx);

    char stat_line[TERM_COLS];
    char num[12];
    int s = 0;
    const char *sp = "IP: 10.0.2.15/24 | TX: ";
    while (*sp)
        stat_line[s++] = *sp++;
    int_to_str(tx, num);
    int ni = 0;
    while (num[ni])
        stat_line[s++] = num[ni++];
    stat_line[s++] = ' ';
    stat_line[s++] = 'p';
    stat_line[s++] = 'k';
    stat_line[s++] = 't';
    stat_line[s] = '\0';

    terminal_write_line(term, stat_line);
}

static void cmd_ifconfig(terminal_t *term)
{
    terminal_write_line_color(term, "\033[36mlo0: flags=UP,LOOPBACK mtu=65536\033[0m", 0x00D2FF);
    terminal_write_line(term, " inet 127.0.0.1 netmask 255.0.0.0");

    if (!rtl8139_is_present())
    {
        terminal_write_line_color(term, "\033[31meth0: DOWN (Tambahkan '-nic model=rtl8139' di QEMU)\033[0m", 0xFF4136);
        return;
    }

    unsigned char mac[6];
    rtl8139_get_mac(mac);
    const unsigned char *my_ip = net_get_my_ip();
    const unsigned char *gw = net_get_gateway_ip();
    const unsigned char *dns = net_get_dns_server();

    unsigned int tx = 0, rx = 0;
    rtl8139_get_stats(&tx, &rx);

    const char hex_chars[] = "0123456789ABCDEF";
    char line[TERM_COLS];

    terminal_write_line_color(term, "\033[36meth0: flags=UP,BROADCAST,RUNNING mtu=1500\033[0m", 0x00D2FF);

    int j = 0;
    const char *p = " inet ";
    while (*p)
        line[j++] = *p++;
    char num[8];
    for (int i = 0; i < 4; i++)
    {
        int_to_str(my_ip[i], num);
        int s = 0;
        while (num[s])
            line[j++] = num[s++];
        if (i < 3)
            line[j++] = '.';
    }
    p = " netmask 255.255.255.0";
    while (*p && j < TERM_COLS - 1)
        line[j++] = *p++;
    line[j] = '\0';
    terminal_write_line(term, line);

    j = 0;
    p = " ether ";
    while (*p)
        line[j++] = *p++;
    for (int i = 0; i < 6; i++)
    {
        line[j++] = hex_chars[(mac[i] >> 4) & 0xF];
        line[j++] = hex_chars[mac[i] & 0xF];
        if (i < 5)
            line[j++] = ':';
    }
    line[j] = '\0';
    terminal_write_line(term, line);

    j = 0;
    p = " gw ";
    while (*p)
        line[j++] = *p++;
    for (int i = 0; i < 4; i++)
    {
        int_to_str(gw[i], num);
        int s = 0;
        while (num[s])
            line[j++] = num[s++];
        if (i < 3)
            line[j++] = '.';
    }
    p = " dns ";
    while (*p)
        line[j++] = *p++;
    for (int i = 0; i < 4; i++)
    {
        int_to_str(dns[i], num);
        int s = 0;
        while (num[s])
            line[j++] = num[s++];
        if (i < 3)
            line[j++] = '.';
    }
    line[j] = '\0';
    terminal_write_line(term, line);

    j = 0;
    p = " RX pkts ";
    while (*p)
        line[j++] = *p++;
    int_to_str(rx, num);
    int s = 0;
    while (num[s])
        line[j++] = num[s++];
    p = " | TX pkts ";
    while (*p)
        line[j++] = *p++;
    int_to_str(tx, num);
    s = 0;
    while (num[s])
        line[j++] = num[s++];
    line[j] = '\0';
    terminal_write_line(term, line);
}

static void cmd_ping(terminal_t *term, const char *args)
{
    while (*args == ' ')
        args++;

    unsigned char target_ip[4] = {10, 0, 2, 2};
    int count = 4;
    char host_str[32] = "10.0.2.2";

    if (*args != '\0')
    {
        int h = 0;
        while (*args != ' ' && *args != '\0' && h < 31)
        {
            host_str[h++] = *args++;
        }
        host_str[h] = '\0';

        while (*args == ' ')
            args++;
        if (*args >= '0' && *args <= '9')
        {
            count = (int)str_to_uint(args);
            if (count <= 0)
                count = 1;
            if (count > 10)
                count = 10;
        }

        int is_domain = 0;
        for (int i = 0; host_str[i]; i++)
        {
            if ((host_str[i] >= 'a' && host_str[i] <= 'z') ||
                (host_str[i] >= 'A' && host_str[i] <= 'Z'))
            {
                is_domain = 1;
                break;
            }
        }

        if (str_equals(host_str, "127.0.0.1") || str_equals(host_str, "localhost"))
        {
            target_ip[0] = 127;
            target_ip[1] = 0;
            target_ip[2] = 0;
            target_ip[3] = 1;
            is_domain = 0;
        }
        else if (is_domain)
        {
            if (!rtl8139_is_present())
            {
                terminal_write_line_color(term, "\033[31meth0 tidak aktif untuk resolve DNS.\033[0m", 0xFF4136);
                return;
            }
            terminal_write_line(term, "Resolving domain via DNS...");
            net_clear_dns_reply();
            net_send_dns_query(host_str);
            int resolved = 0;
            for (int t = 0; t < 25000; t++)
            {
                net_poll();
                if (net_has_dns_reply())
                {
                    net_get_dns_result(target_ip);
                    resolved = 1;
                    break;
                }
            }
            if (!resolved)
            {
                terminal_write_line_color(term, "\033[31mDNS: Gagal resolve domain.\033[0m", 0xFF4136);
                return;
            }
        }
        else
        {
            const char *p = host_str;
            for (int oct = 0; oct < 4; oct++)
            {
                unsigned int val = 0;
                while (*p >= '0' && *p <= '9')
                {
                    val = val * 10 + (*p - '0');
                    p++;
                }
                target_ip[oct] = (unsigned char)val;
                if (*p == '.')
                    p++;
            }
        }
    }

    int is_loopback = (target_ip[0] == 127);
    if (!rtl8139_is_present() && !is_loopback)
    {
        terminal_write_line_color(term, "\033[31meth0 tidak aktif. Tambahkan '-nic model=rtl8139' di QEMU.\033[0m", 0xFF4136);
        terminal_write_line(term, "Tip: ping 127.0.0.1 (loopback) tetap aktif.");
        return;
    }

    char line[TERM_COLS];
    char num[12];
    int j = 0;
    const char *p = "PING ";
    while (*p)
        line[j++] = *p++;
    for (int k = 0; k < 4; k++)
    {
        int_to_str(target_ip[k], num);
        int s = 0;
        while (num[s])
            line[j++] = num[s++];
        if (k < 3)
            line[j++] = '.';
    }
    p = " (32 bytes data)";
    while (*p && j < TERM_COLS - 1)
        line[j++] = *p++;
    line[j] = '\0';
    terminal_write_line_color(term, line, 0x00D2FF);

    int received = 0;

    for (int seq = 1; seq <= count; seq++)
    {
        net_clear_ping_reply();
        net_send_ping_seq(target_ip, seq);
        unsigned int t0 = timer_ticks;
        int got_reply = 0;

        for (int t = 0; t < 25000; t++)
        {
            net_poll();
            if (net_has_ping_reply() && net_get_ping_reply_seq() == (unsigned short)seq)
            {
                unsigned int dt = timer_ticks - t0;
                unsigned int rtt_ms = (dt == 0) ? 1 : (dt * 10);
                received++;
                got_reply = 1;

                j = 0;
                p = "32B seq=";
                while (*p)
                    line[j++] = *p++;
                int_to_str(seq, num);
                int s = 0;
                while (num[s])
                    line[j++] = num[s++];
                p = " ttl=";
                while (*p)
                    line[j++] = *p++;
                int_to_str(net_get_ping_reply_ttl(), num);
                s = 0;
                while (num[s])
                    line[j++] = num[s++];
                p = " time=";
                while (*p)
                    line[j++] = *p++;
                int_to_str(rtt_ms, num);
                s = 0;
                while (num[s])
                    line[j++] = num[s++];
                p = "ms";
                while (*p && j < TERM_COLS - 1)
                    line[j++] = *p++;
                line[j] = '\0';
                terminal_write_line_color(term, line, 0x2ECC40);
                break;
            }
        }

        if (!got_reply)
        {
            j = 0;
            p = "Request timeout seq=";
            while (*p)
                line[j++] = *p++;
            int_to_str(seq, num);
            int s = 0;
            while (num[s])
                line[j++] = num[s++];
            line[j] = '\0';
            terminal_write_line_color(term, line, 0xFF4136);
        }

        for (volatile int d = 0; d < 200000; d++)
        {
        }
    }

    j = 0;
    p = "--- ";
    while (*p)
        line[j++] = *p++;
    int_to_str(count, num);
    int s = 0;
    while (num[s])
        line[j++] = num[s++];
    p = " tx, ";
    while (*p)
        line[j++] = *p++;
    int_to_str(received, num);
    s = 0;
    while (num[s])
        line[j++] = num[s++];
    p = " rx (";
    while (*p)
        line[j++] = *p++;
    int loss = ((count - received) * 100) / count;
    int_to_str(loss, num);
    s = 0;
    while (num[s])
        line[j++] = num[s++];
    p = "% loss) ---";
    while (*p && j < TERM_COLS - 1)
        line[j++] = *p++;
    line[j] = '\0';
    terminal_write_line(term, line);
}

static void cmd_dns(terminal_t *term, const char *domain)
{
    while (*domain == ' ')
        domain++;
    if (*domain == '\0')
    {
        terminal_write_line(term, "Gunakan: dns <nama_domain>");
        return;
    }

    if (!rtl8139_is_present())
    {
        terminal_write_line(term, "NIC tidak aktif.");
        return;
    }

    terminal_write_line(term, "Query DNS ke 10.0.2.3:53...");
    net_clear_dns_reply();
    net_send_dns_query(domain);

    for (int t = 0; t < 25000; t++)
    {
        net_poll();
        if (net_has_dns_reply())
        {
            unsigned char ip[4];
            net_get_dns_result(ip);
            char line[TERM_COLS];
            char num[4];
            int j = 0;
            const char *p = "IP: ";
            while (*p)
                line[j++] = *p++;
            for (int k = 0; k < 4; k++)
            {
                int_to_str(ip[k], num);
                int s = 0;
                while (num[s])
                    line[j++] = num[s++];
                if (k < 3)
                    line[j++] = '.';
            }
            line[j] = '\0';
            terminal_write_line(term, line);
            return;
        }
    }
    terminal_write_line(term, "DNS Query timeout / no reply.");
}

static void cmd_tarlist(terminal_t *term)
{
    terminal_write_line_color(term, "\033[36m--- Arsip TAR Sistem Terpasang ---\033[0m", 0x00D2FF);
    terminal_write_line(term, "1: motd.txt   (44 B)");
    terminal_write_line(term, "2: config.sys (30 B)");
    terminal_write_line_color(term, "\033[32mStatus: Diekstrak ke VFS.\033[0m", 0x2ECC40);
}

static void cmd_curl(terminal_t *term, const char *args)
{
    while (*args == ' ')
        args++;
    if (*args == '\0')
    {
        terminal_write_line(term, "Gunakan: curl <ip> [port] [path]");
        return;
    }

    if (!rtl8139_is_present())
    {
        terminal_write_line(term, "NIC tidak aktif.");
        return;
    }

    unsigned char ip[4] = {0};
    int octet = 0;
    while (*args != ' ' && *args != '\0' && octet < 4)
    {
        if (*args >= '0' && *args <= '9')
        {
            ip[octet] = ip[octet] * 10 + (*args - '0');
        }
        else if (*args == '.')
        {
            octet++;
        }
        args++;
    }

    unsigned short port = 80;
    while (*args == ' ')
        args++;
    if (*args >= '0' && *args <= '9')
    {
        port = (unsigned short)str_to_uint(args);
        while (*args >= '0' && *args <= '9')
            args++;
    }

    const char *path = "/";
    while (*args == ' ')
        args++;
    if (*args != '\0')
    {
        path = args;
    }

    terminal_write_line_color(term, "\033[36mMenghubungkan TCP...\033[0m", 0x00D2FF);
    int conn = tcp_connect(ip, port);
    if (conn != 0)
    {
        terminal_write_line_color(term, "\033[31mGagal koneksi TCP / Timeout.\033[0m", 0xFF4136);
        return;
    }

    terminal_write_line_color(term, "\033[32mTerhubung! Mengirim HTTP GET...\033[0m", 0x2ECC40);

    char req[128];
    int rlen = 0;
    const char *p = "GET ";
    while (*p)
        req[rlen++] = *p++;
    p = path;
    while (*p && rlen < 80)
        req[rlen++] = *p++;
    p = " HTTP/1.0\r\nHost: host\r\n\r\n";
    while (*p && rlen < 120)
        req[rlen++] = *p++;
    req[rlen] = '\0';

    tcp_send(req, rlen);

    char rx[512];
    int got = 0;
    for (int t = 0; t < 25000; t++)
    {
        net_poll();
        int n = tcp_get_rx_data(rx, sizeof(rx));
        if (n > 0)
        {
            got = 1;
            char line[TERM_COLS];
            int lidx = 0;
            for (int i = 0; i < n && i < 120; i++)
            {
                if (rx[i] == '\r')
                    continue;
                if (rx[i] == '\n' || lidx >= TERM_COLS - 1)
                {
                    line[lidx] = '\0';
                    if (lidx > 0)
                        terminal_write_line(term, line);
                    lidx = 0;
                    if (rx[i] == '\n')
                        continue;
                }
                line[lidx++] = rx[i];
            }
            if (lidx > 0)
            {
                line[lidx] = '\0';
                terminal_write_line(term, line);
            }
            break;
        }
    }

    tcp_close();
    if (!got)
    {
        terminal_write_line(term, "TCP: Tidak ada respons data.");
    }
}

static void cmd_rm(terminal_t *term, const char *args)
{
    while (*args == ' ')
        args++;
    if (*args == '\0')
    {
        terminal_write_line(term, "Gunakan: rm <nama_file>");
        return;
    }
    int res = vfs_delete_file(args);
    if (res == 0)
    {
        terminal_write_line_color(term, "\033[32mFile berhasil dihapus.\033[0m", 0x2ECC40);
    }
    else
    {
        terminal_write_line_color(term, "\033[31mFile tidak ditemukan.\033[0m", 0xFF4136);
    }
}

static void cmd_cp(terminal_t *term, const char *args)
{
    while (*args == ' ')
        args++;
    char src[MAX_FILENAME_LEN];
    char dest[MAX_FILENAME_LEN];
    int i = 0;
    while (*args != ' ' && *args != '\0' && i < MAX_FILENAME_LEN - 1)
    {
        src[i++] = *args++;
    }
    src[i] = '\0';
    while (*args == ' ')
        args++;
    int j = 0;
    while (*args != ' ' && *args != '\0' && j < MAX_FILENAME_LEN - 1)
    {
        dest[j++] = *args++;
    }
    dest[j] = '\0';

    if (src[0] == '\0' || dest[0] == '\0')
    {
        terminal_write_line(term, "Gunakan: cp <source> <dest>");
        return;
    }

    int res = vfs_copy_file(src, dest);
    if (res == 0)
    {
        terminal_write_line_color(term, "\033[32mFile berhasil disalin.\033[0m", 0x2ECC40);
    }
    else if (res == -2)
    {
        terminal_write_line_color(term, "\033[31mSource file tidak ada.\033[0m", 0xFF4136);
    }
    else
    {
        terminal_write_line_color(term, "\033[31mGagal menyalin file.\033[0m", 0xFF4136);
    }
}

static void cmd_df(terminal_t *term)
{
    terminal_write_line_color(term, "\033[36mFilesystem     Used     Avail  Files\033[0m", 0x00D2FF);

    char line[TERM_COLS];
    char num[16];

    size_t used = vfs_get_total_used_bytes();
    int count = vfs_get_count();

    int j = 0;
    const char *p = "vfs_ramdisk    ";
    while (*p)
        line[j++] = *p++;
    int_to_str((unsigned int)used, num);
    int s = 0;
    while (num[s])
        line[j++] = num[s++];
    line[j++] = 'B';
    while (j < 24)
        line[j++] = ' ';
    p = "4MB   ";
    while (*p)
        line[j++] = *p++;
    int_to_str(count, num);
    s = 0;
    while (num[s])
        line[j++] = num[s++];
    p = "/32";
    while (*p && j < TERM_COLS - 1)
        line[j++] = *p++;
    line[j] = '\0';
    terminal_write_line(term, line);

    j = 0;
    p = "ata_primary    512B     ";
    while (*p)
        line[j++] = *p++;
    p = ata_init() == 0 ? "ONLINE" : "NO_DISK";
    while (*p && j < TERM_COLS - 1)
        line[j++] = *p++;
    line[j] = '\0';
    terminal_write_line(term, line);
}

static void cmd_hexdump(terminal_t *term, const char *args)
{
    while (*args == ' ')
        args++;
    char fname[MAX_FILENAME_LEN];
    int i = 0;
    while (*args != ' ' && *args != '\0' && i < MAX_FILENAME_LEN - 1)
    {
        fname[i++] = *args++;
    }
    fname[i] = '\0';

    if (fname[0] == '\0')
    {
        terminal_write_line(term, "Gunakan: hexdump <file> [bytes]");
        return;
    }

    while (*args == ' ')
        args++;
    unsigned int max_bytes = 64;
    if (*args >= '0' && *args <= '9')
    {
        max_bytes = str_to_uint(args);
        if (max_bytes == 0)
            max_bytes = 64;
        if (max_bytes > 256)
            max_bytes = 256;
    }

    vfs_node_t *f = vfs_find_file(fname);
    const unsigned char *data = NULL;
    unsigned int data_len = 0;

    if (f && f->data)
    {
        data = (const unsigned char *)f->data;
        data_len = f->length;
    }

    if (!data)
    {
        terminal_write_line_color(term, "\033[31mFile tidak ditemukan di VFS.\033[0m", 0xFF4136);
        return;
    }

    if (max_bytes > data_len)
        max_bytes = data_len;

    const char hex_chars[] = "0123456789ABCDEF";
    char line[TERM_COLS];

    for (unsigned int offset = 0; offset < max_bytes; offset += 8)
    {
        int j = 0;
        line[j++] = hex_chars[(offset >> 12) & 0xF];
        line[j++] = hex_chars[(offset >> 8) & 0xF];
        line[j++] = hex_chars[(offset >> 4) & 0xF];
        line[j++] = hex_chars[offset & 0xF];
        line[j++] = ':';
        line[j++] = ' ';

        for (unsigned int b = 0; b < 8; b++)
        {
            if (offset + b < max_bytes)
            {
                unsigned char byte = data[offset + b];
                line[j++] = hex_chars[(byte >> 4) & 0xF];
                line[j++] = hex_chars[byte & 0xF];
                line[j++] = ' ';
            }
            else
            {
                line[j++] = ' ';
                line[j++] = ' ';
                line[j++] = ' ';
            }
        }

        line[j++] = '|';
        for (unsigned int b = 0; b < 8; b++)
        {
            if (offset + b < max_bytes)
            {
                unsigned char byte = data[offset + b];
                line[j++] = (byte >= 32 && byte <= 126) ? (char)byte : '.';
            }
            else
            {
                line[j++] = ' ';
            }
        }
        line[j++] = '|';
        line[j] = '\0';

        terminal_write_line(term, line);
    }
}

static void cmd_date(terminal_t *term)
{
    rtc_time_t t = rtc_get_time();
    static const char *months[] = {
        "", "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    const char *mstr = (t.month >= 1 && t.month <= 12) ? months[t.month] : "---";

    char line[TERM_COLS];
    int j = 0;

    line[j++] = '0' + (t.day / 10);
    line[j++] = '0' + (t.day % 10);
    line[j++] = ' ';
    const char *p = mstr;
    while (*p)
        line[j++] = *p++;
    line[j++] = ' ';

    char yr[8];
    int_to_str(t.year, yr);
    int y = 0;
    while (yr[y])
        line[j++] = yr[y++];
    line[j++] = ' ';

    line[j++] = '0' + (t.hour / 10);
    line[j++] = '0' + (t.hour % 10);
    line[j++] = ':';
    line[j++] = '0' + (t.minute / 10);
    line[j++] = '0' + (t.minute % 10);
    line[j++] = ':';
    line[j++] = '0' + (t.second / 10);
    line[j++] = '0' + (t.second % 10);
    line[j++] = ' ';
    line[j++] = 'U';
    line[j++] = 'T';
    line[j++] = 'C';
    line[j] = '\0';

    terminal_write_line_color(term, line, 0x00D2FF);
}

static void cmd_uptime(terminal_t *term)
{
    unsigned int total_sec = timer_ticks / 100;
    unsigned int hrs = total_sec / 3600;
    unsigned int mins = (total_sec % 3600) / 60;
    unsigned int secs = total_sec % 60;

    char line[TERM_COLS];
    int j = 0;
    const char *p = "up ";
    while (*p)
        line[j++] = *p++;

    line[j++] = '0' + (hrs / 10);
    line[j++] = '0' + (hrs % 10);
    line[j++] = ':';
    line[j++] = '0' + (mins / 10);
    line[j++] = '0' + (mins % 10);
    line[j++] = ':';
    line[j++] = '0' + (secs / 10);
    line[j++] = '0' + (secs % 10);

    p = ", ticks: ";
    while (*p)
        line[j++] = *p++;

    char num[16];
    int_to_str(timer_ticks, num);
    int s = 0;
    while (num[s] && j < TERM_COLS - 1)
        line[j++] = num[s++];
    line[j] = '\0';

    terminal_write_line_color(term, line, 0xFFDC00);
}

#define TERM_HIST_MAX 8
static char term_history[TERM_HIST_MAX][MAX_INPUT_LEN];
static int term_hist_count = 0;
static int term_hist_idx = -1;

static const char *known_commands[] = {
    "help", "clear", "ver", "about", "ls", "fatls", "fatread",
    "cat", "touch", "write", "echo", "rm", "cp", "mv", "grep", "df", "hexdump", "free",
    "date", "uptime", "uname", "systeminfo", "diskinfo", "diskr", "diskw",
    "userdemo", "elfdemo", "lspci", "netinfo", "ifconfig", "ping", "dns", "curl",
    "tarlist", "beep", "sbtest", "play", "player", "ps", "kill", "taskdemo",
    "basic", "browser", "reboot", "shutdown", NULL};

static void terminal_autocomplete(terminal_t *term)
{
    if (!term || term->input_len == 0)
        return;

    int space_idx = -1;
    for (int i = 0; i < term->input_len; i++)
    {
        if (term->input_buf[i] == ' ')
        {
            space_idx = i;
            break;
        }
    }

    if (space_idx == -1)
    {
        for (int i = 0; known_commands[i] != NULL; i++)
        {
            if (str_starts_with(known_commands[i], term->input_buf))
            {
                int k = 0;
                while (known_commands[i][k] && k < MAX_INPUT_LEN - 1)
                {
                    term->input_buf[k] = known_commands[i][k];
                    k++;
                }
                term->input_buf[k] = '\0';
                term->input_len = k;
                return;
            }
        }
    }
    else
    {
        const char *prefix = term->input_buf + space_idx + 1;
        while (*prefix == ' ')
            prefix++;
        if (*prefix != '\0')
        {
            int fcount = vfs_get_count();
            for (int i = 0; i < fcount; i++)
            {
                vfs_node_t *node = vfs_get_node(i);
                if (node && str_starts_with(node->name, prefix))
                {
                    int k = space_idx + 1;
                    int f = 0;
                    while (node->name[f] && k < MAX_INPUT_LEN - 1)
                    {
                        term->input_buf[k++] = node->name[f++];
                    }
                    term->input_buf[k] = '\0';
                    term->input_len = k;
                    return;
                }
            }
        }
    }
}

static void terminal_execute_command(terminal_t *term)
{
    if (!term)
        return;

    char line[TERM_COLS];
    int offset = 2;
    if (basic_is_active())
    {
        line[0] = 'B';
        line[1] = '>';
        line[2] = ' ';
        line[3] = '\0';
        offset = 3;
    }
    else
    {
        line[0] = '>';
        line[1] = ' ';
        line[2] = '\0';
    }

    int i = 0;
    for (i = 0; i < term->input_len && i < (TERM_COLS - (offset + 1)); i++)
    {
        line[i + offset] = term->input_buf[i];
    }
    line[i + offset] = '\0';
    terminal_write_line(term, line);

    if (term->input_len > 0)
    {
        if (term_hist_count < TERM_HIST_MAX)
        {
            int k = 0;
            while (term->input_buf[k])
            {
                term_history[term_hist_count][k] = term->input_buf[k];
                k++;
            }
            term_history[term_hist_count][k] = '\0';
            term_hist_count++;
        }
        else
        {
            for (int h = 0; h < TERM_HIST_MAX - 1; h++)
            {
                for (int k = 0; k < MAX_INPUT_LEN; k++)
                    term_history[h][k] = term_history[h + 1][k];
            }
            int k = 0;
            while (term->input_buf[k])
            {
                term_history[TERM_HIST_MAX - 1][k] = term->input_buf[k];
                k++;
            }
            term_history[TERM_HIST_MAX - 1][k] = '\0';
        }
    }
    term_hist_idx = -1;

    if (basic_is_active())
    {
        basic_handle_input(term, term->input_buf);
        term->input_len = 0;
        term->input_buf[0] = '\0';
        return;
    }

    if (str_equals(term->input_buf, "help"))
    {
        terminal_write_line(term, "Cmds: ls, cat, touch, write, echo, rm, cp, mv");
        terminal_write_line(term, "      grep, df, hexdump, date, uptime, uname");
        terminal_write_line(term, "      systeminfo, free, fatls, fatread, tarlist");
        terminal_write_line(term, "      diskinfo, diskr, diskw, lspci");
        terminal_write_line(term, "      ps, kill <pid>, taskdemo");
        terminal_write_line(term, "      netinfo, ifconfig, ping [host] [count]");
        terminal_write_line(term, "      dns <domain>, curl <ip> [port] [path]");
        terminal_write_line(term, "      sbtest, play <file>, player, beep, basic");
        terminal_write_line(term, "      browser [url], userdemo, clear, ver");
    }
    else if (str_equals(term->input_buf, "clear"))
    {
        for (int r = 0; r < TERM_ROWS; r++)
        {
            for (int c = 0; c < TERM_COLS; c++)
            {
                term->buffer[r][c] = '\0';
            }
        }
        term->cursor_row = 0;
    }
    else if (str_equals(term->input_buf, "ver"))
    {
        terminal_write_line(term, "Lenovix OS Kernel v0.5.0");
    }
    else if (str_equals(term->input_buf, "about"))
    {
        terminal_write_line(term, "Dibuat oleh Kamil Sudarmi.");
    }
    else if (str_equals(term->input_buf, "ls"))
    {
        cmd_ls(term);
    }
    else if (str_equals(term->input_buf, "fatls"))
    {
        cmd_fatls(term);
    }
    else if (str_starts_with(term->input_buf, "fatread "))
    {
        cmd_fatread(term, term->input_buf + 8);
    }
    else if (str_starts_with(term->input_buf, "cat "))
    {
        cmd_cat(term, term->input_buf + 4);
    }
    else if (str_starts_with(term->input_buf, "touch "))
    {
        cmd_touch(term, term->input_buf + 6);
    }
    else if (str_starts_with(term->input_buf, "write "))
    {
        cmd_write(term, term->input_buf + 6);
    }
    else if (str_equals(term->input_buf, "echo") || str_starts_with(term->input_buf, "echo "))
    {
        const char *eargs = term->input_buf + 4;
        while (*eargs == ' ')
            eargs++;
        cmd_echo(term, eargs);
    }
    else if (str_starts_with(term->input_buf, "rm "))
    {
        cmd_rm(term, term->input_buf + 3);
    }
    else if (str_starts_with(term->input_buf, "cp "))
    {
        cmd_cp(term, term->input_buf + 3);
    }
    else if (str_starts_with(term->input_buf, "mv "))
    {
        cmd_mv(term, term->input_buf + 3);
    }
    else if (str_starts_with(term->input_buf, "grep "))
    {
        cmd_grep(term, term->input_buf + 5);
    }
    else if (str_equals(term->input_buf, "df"))
    {
        cmd_df(term);
    }
    else if (str_starts_with(term->input_buf, "hexdump "))
    {
        cmd_hexdump(term, term->input_buf + 8);
    }
    else if (str_equals(term->input_buf, "date"))
    {
        cmd_date(term);
    }
    else if (str_equals(term->input_buf, "uptime"))
    {
        cmd_uptime(term);
    }
    else if (str_equals(term->input_buf, "uname") || str_starts_with(term->input_buf, "uname "))
    {
        const char *uargs = term->input_buf + 5;
        while (*uargs == ' ')
            uargs++;
        cmd_uname(term, uargs);
    }
    else if (str_equals(term->input_buf, "systeminfo"))
    {
        cmd_systeminfo(term);
    }
    else if (str_equals(term->input_buf, "free"))
    {
        cmd_free(term);
    }
    else if (str_equals(term->input_buf, "diskinfo"))
    {
        cmd_diskinfo(term);
    }
    else if (str_equals(term->input_buf, "diskr") || str_starts_with(term->input_buf, "diskr "))
    {
        const char *dargs = term->input_buf + 5;
        while (*dargs == ' ')
            dargs++;
        cmd_diskr(term, dargs);
    }
    else if (str_equals(term->input_buf, "diskw") || str_starts_with(term->input_buf, "diskw "))
    {
        const char *dargs = term->input_buf + 5;
        while (*dargs == ' ')
            dargs++;
        cmd_diskw(term, dargs);
    }
    else if (str_equals(term->input_buf, "userdemo"))
    {
        cmd_userdemo(term);
    }
    else if (str_equals(term->input_buf, "elfdemo"))
    {
        cmd_elfdemo(term);
    }
    else if (str_equals(term->input_buf, "lspci"))
    {
        cmd_lspci(term);
    }
    else if (str_equals(term->input_buf, "ps"))
    {
        cmd_ps(term);
    }
    else if (str_starts_with(term->input_buf, "kill "))
    {
        cmd_kill(term, term->input_buf + 5);
    }
    else if (str_equals(term->input_buf, "taskdemo"))
    {
        cmd_taskdemo();
        terminal_write_line_color(term, "\033[32mTask A dan Task B dibuat!\033[0m", 0x2ECC40);
    }
    else if (str_equals(term->input_buf, "netinfo"))
    {
        cmd_netinfo(term);
    }
    else if (str_equals(term->input_buf, "ifconfig"))
    {
        cmd_ifconfig(term);
    }
    else if (str_equals(term->input_buf, "ping") || str_starts_with(term->input_buf, "ping "))
    {
        const char *args = term->input_buf + 4;
        while (*args == ' ')
            args++;
        cmd_ping(term, args);
    }
    else if (str_starts_with(term->input_buf, "dns "))
    {
        cmd_dns(term, term->input_buf + 4);
    }
    else if (str_starts_with(term->input_buf, "curl "))
    {
        cmd_curl(term, term->input_buf + 5);
    }
    else if (str_equals(term->input_buf, "tarlist"))
    {
        cmd_tarlist(term);
    }
    else if (str_equals(term->input_buf, "beep") || str_starts_with(term->input_buf, "beep "))
    {
        unsigned int freq = 750;
        if (term->input_buf[4] == ' ')
        {
            freq = str_to_uint(term->input_buf + 5);
            if (freq == 0)
                freq = 750;
        }
        speaker_beep(freq, 150);
        terminal_write_line(term, "Beep dimainkan!");
    }
    else if (str_equals(term->input_buf, "sbtest"))
    {
        if (sb16_is_present())
        {
            terminal_write_line(term, "SB16: Memainkan sample PCM 8-bit...");
            sb16_play_sample_sound();
        }
        else
        {
            terminal_write_line(term, "SB16: DSP tidak terdeteksi (0x220).");
        }
    }
    else if (str_starts_with(term->input_buf, "play "))
    {
        const char *fname = term->input_buf + 5;
        while (*fname == ' ')
            fname++;
        int r = audio_load_file(fname);
        if (r == 0)
        {
            audio_play();
            terminal_write_line(term, "Memainkan audio (MP3/WAV/CHIP)...");
        }
        else if (r == -1)
        {
            terminal_write_line(term, "File tidak ditemukan.");
        }
        else
        {
            terminal_write_line(term, "Format audio tidak didukung.");
        }
    }
    else if (str_equals(term->input_buf, "player"))
    {
        window_show_player();
        terminal_write_line(term, "Membuka Media Player & Spectrum Visualizer...");
    }
    else if (str_equals(term->input_buf, "browser") || str_starts_with(term->input_buf, "browser "))
    {
        const char *burl = term->input_buf + 7;
        while (*burl == ' ')
            burl++;
        window_show_browser();
        if (*burl != '\0')
        {
            browser_navigate(burl);
        }
        terminal_write_line(term, "Membuka Lenovix Browser...");
    }
    else if (str_equals(term->input_buf, "basic") || str_starts_with(term->input_buf, "basic "))
    {
        const char *bargs = term->input_buf + 5;
        while (*bargs == ' ')
            bargs++;
        if (str_starts_with(bargs, "run "))
        {
            bargs += 4;
            while (*bargs == ' ')
                bargs++;
        }
        if (*bargs == '\0')
        {
            basic_enter(term);
        }
        else
        {
            basic_run_file(term, bargs);
        }
    }
    else if (str_equals(term->input_buf, "reboot"))
    {
        acpi_reboot();
    }
    else if (str_equals(term->input_buf, "shutdown"))
    {
        acpi_power_off();
    }
    else if (term->input_len > 0)
    {
        terminal_write_line(term, "Perintah tidak dikenal!");
    }

    term->input_len = 0;
    term->input_buf[0] = '\0';
}

void terminal_handle_key(terminal_t *term, char c)
{
    if (!term)
        return;

    if (c == '\n')
    {
        terminal_execute_command(term);
    }
    else if (c == '\t')
    {
        terminal_autocomplete(term);
    }
    else if (c == 0x11)
    {
        if (term_hist_count > 0)
        {
            if (term_hist_idx == -1)
                term_hist_idx = term_hist_count - 1;
            else if (term_hist_idx > 0)
                term_hist_idx--;

            int k = 0;
            while (term_history[term_hist_idx][k])
            {
                term->input_buf[k] = term_history[term_hist_idx][k];
                k++;
            }
            term->input_buf[k] = '\0';
            term->input_len = k;
        }
    }
    else if (c == 0x12)
    {
        if (term_hist_idx != -1)
        {
            term_hist_idx++;
            if (term_hist_idx < term_hist_count)
            {
                int k = 0;
                while (term_history[term_hist_idx][k])
                {
                    term->input_buf[k] = term_history[term_hist_idx][k];
                    k++;
                }
                term->input_buf[k] = '\0';
                term->input_len = k;
            }
            else
            {
                term_hist_idx = -1;
                term->input_buf[0] = '\0';
                term->input_len = 0;
            }
        }
    }
    else if (c == 0x13)
    {
        terminal_scroll_up(term, 5);
    }
    else if (c == 0x14)
    {
        terminal_scroll_down(term, 5);
    }
    else if (c == '\b')
    {
        if (term->input_len > 0)
        {
            term->input_len--;
            term->input_buf[term->input_len] = '\0';
        }
    }
    else if (c >= 32 && c <= 126)
    {
        if (term->input_len < MAX_INPUT_LEN - 1)
        {
            term->input_buf[term->input_len] = c;
            term->input_len++;
            term->input_buf[term->input_len] = '\0';
        }
    }
}

void terminal_draw(terminal_t *term, int win_x, int win_y)
{
    if (!term)
        return;

    int term_x = win_x + 8;
    int term_y = win_y + 34;
    int term_w = 404;
    int term_h = 218;

    vesa_draw_rect(term_x, term_y, term_w, term_h, 0x000000);

    int max_scroll = (term->total_lines > TERM_VISIBLE_ROWS) ? (term->total_lines - TERM_VISIBLE_ROWS) : 0;
    int start_line = max_scroll - term->scroll_offset;
    if (start_line < 0)
        start_line = 0;

    for (int r = 0; r < TERM_VISIBLE_ROWS; r++)
    {
        int line_idx = start_line + r;
        if (line_idx < term->total_lines && term->buffer[line_idx][0] != '\0')
        {
            vesa_draw_string(term_x + 6, term_y + 6 + (r * 18), term->buffer[line_idx], term->buffer_color[line_idx]);
        }
    }

    int sep_y = term_y + 6 + (TERM_VISIBLE_ROWS * 18) + 2;
    vesa_draw_rect(term_x + 4, sep_y, term_w - 24, 1, 0x222222);

    int prompt_y = sep_y + 5;
    vesa_draw_string(term_x + 6, prompt_y, ">", 0x00D2FF);
    vesa_draw_string(term_x + 20, prompt_y, term->input_buf, 0xFFFFFF);

    int cursor_x = term_x + 20 + (term->input_len * 8);
    vesa_draw_string(cursor_x, prompt_y, "_", 0x00FF88);

    int sb_x = term_x + term_w - 16;
    int sb_y = term_y + 4;
    int sb_h = 168;

    vesa_draw_rect(sb_x, sb_y, 14, sb_h, 0x1A1A1A);

    vesa_draw_rect(sb_x, sb_y, 14, 14, 0x333333);
    vesa_draw_string(sb_x + 3, sb_y, "^", 0xAAAAAA);

    vesa_draw_rect(sb_x, sb_y + sb_h - 14, 14, 14, 0x333333);
    vesa_draw_string(sb_x + 3, sb_y + sb_h - 14, "v", 0xAAAAAA);

    int track_y = sb_y + 14;
    int track_h = sb_h - 28;
    int thumb_h = (max_scroll > 0 && term->total_lines > 0) ? (track_h * TERM_VISIBLE_ROWS) / term->total_lines : track_h;
    if (thumb_h < 14)
        thumb_h = 14;

    int thumb_y = track_y;
    if (max_scroll > 0)
    {
        thumb_y = track_y + ((track_h - thumb_h) * (max_scroll - term->scroll_offset)) / max_scroll;
    }
    vesa_draw_rect(sb_x + 2, thumb_y, 10, thumb_h, (term->scroll_offset > 0) ? 0x0074D9 : 0x444444);
}

int terminal_handle_click(terminal_t *term, int win_x, int win_y, int mouse_x, int mouse_y)
{
    if (!term)
        return 0;

    int term_x = win_x + 8;
    int term_y = win_y + 34;
    int term_w = 404;
    int sb_x = term_x + term_w - 16;
    int sb_y = term_y + 4;
    int sb_h = 168;

    if (mouse_x >= sb_x && mouse_x <= sb_x + 14 && mouse_y >= sb_y && mouse_y <= sb_y + sb_h)
    {
        if (mouse_y <= sb_y + 14)
        {
            terminal_scroll_up(term, 3);
        }
        else if (mouse_y >= sb_y + sb_h - 14)
        {
            terminal_scroll_down(term, 3);
        }
        else
        {
            int track_y = sb_y + 14;
            int track_h = sb_h - 28;
            int click_pos = mouse_y - track_y;
            int max_scroll = (term->total_lines > TERM_VISIBLE_ROWS) ? (term->total_lines - TERM_VISIBLE_ROWS) : 0;
            if (max_scroll > 0)
            {
                int scroll_target = max_scroll - (click_pos * max_scroll) / track_h;
                term->scroll_offset = scroll_target;
                if (term->scroll_offset < 0)
                    term->scroll_offset = 0;
                if (term->scroll_offset > max_scroll)
                    term->scroll_offset = max_scroll;
            }
        }
        return 1;
    }
    return 0;
}
