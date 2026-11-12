#include "basic.h"
#include "terminal.h"
#include "keyboard.h"
#include "mouse.h"
#include "vfs.h"
#include "string_util.h"

extern void render_desktop(mouse_state_t *mouse);

#define BASIC_MAX_LINES 128
#define BASIC_MAX_LINE_LEN 80

#define BASIC_STATUS_OK 0
#define BASIC_STATUS_END 1
#define BASIC_STATUS_ERROR 2

typedef struct
{
    int num;
    char text[BASIC_MAX_LINE_LEN];
} basic_line_t;

static basic_line_t prog_lines[BASIC_MAX_LINES];
static int prog_line_count = 0;
static int vars[26];
static int in_basic_repl = 0;
static int is_running = 0;
static int next_pc = -1;

static void skip_spaces(const char **p)
{
    while (**p == ' ' || **p == '\t')
        (*p)++;
}

static char to_upper(char c)
{
    if (c >= 'a' && c <= 'z')
        return c - ('a' - 'A');
    return c;
}

static int match_keyword(const char **p, const char *kw)
{
    const char *orig = *p;
    while (*kw)
    {
        if (to_upper(**p) != to_upper(*kw))
        {
            *p = orig;
            return 0;
        }
        (*p)++;
        kw++;
    }
    char next = to_upper(**p);
    if ((next >= 'A' && next <= 'Z') || (next >= '0' && next <= '9') || next == '_')
    {
        *p = orig;
        return 0;
    }
    return 1;
}

static int parse_int(const char **p)
{
    skip_spaces(p);
    int sign = 1;
    if (**p == '-')
    {
        sign = -1;
        (*p)++;
    }
    else if (**p == '+')
    {
        (*p)++;
    }
    int val = 0;
    while (**p >= '0' && **p <= '9')
    {
        val = val * 10 + (**p - '0');
        (*p)++;
    }
    return val * sign;
}

static void format_int(int val, char *buf)
{
    if (val == 0)
    {
        buf[0] = '0';
        buf[1] = '\0';
        return;
    }
    int pos = 0;
    unsigned int uval;
    if (val < 0)
    {
        buf[pos++] = '-';
        uval = (unsigned int)(-val);
    }
    else
    {
        uval = (unsigned int)val;
    }
    char tmp[12];
    int tpos = 0;
    while (uval > 0)
    {
        tmp[tpos++] = (uval % 10) + '0';
        uval /= 10;
    }
    while (tpos > 0)
    {
        buf[pos++] = tmp[--tpos];
    }
    buf[pos] = '\0';
}

static int eval_expr(const char **p, int *err);

static int eval_factor(const char **p, int *err)
{
    skip_spaces(p);
    if (**p == '(')
    {
        (*p)++;
        int val = eval_expr(p, err);
        skip_spaces(p);
        if (**p == ')')
        {
            (*p)++;
        }
        else
        {
            *err = 1;
        }
        return val;
    }
    if (**p == '-')
    {
        (*p)++;
        return -eval_factor(p, err);
    }
    if (**p == '+')
    {
        (*p)++;
        return eval_factor(p, err);
    }
    if (**p >= '0' && **p <= '9')
    {
        int val = 0;
        while (**p >= '0' && **p <= '9')
        {
            val = val * 10 + (**p - '0');
            (*p)++;
        }
        return val;
    }
    char c = to_upper(**p);
    if (c >= 'A' && c <= 'Z')
    {
        (*p)++;
        return vars[c - 'A'];
    }
    *err = 1;
    return 0;
}

static int eval_term(const char **p, int *err)
{
    int left = eval_factor(p, err);
    while (1)
    {
        skip_spaces(p);
        if (**p == '*')
        {
            (*p)++;
            int right = eval_factor(p, err);
            left *= right;
        }
        else if (**p == '/')
        {
            (*p)++;
            int right = eval_factor(p, err);
            if (right != 0)
            {
                left /= right;
            }
            else
            {
                *err = 2;
            }
        }
        else if (**p == '%')
        {
            (*p)++;
            int right = eval_factor(p, err);
            if (right != 0)
            {
                left %= right;
            }
            else
            {
                *err = 2;
            }
        }
        else
        {
            break;
        }
    }
    return left;
}

static int eval_expr(const char **p, int *err)
{
    int left = eval_term(p, err);
    while (1)
    {
        skip_spaces(p);
        if (**p == '+')
        {
            (*p)++;
            int right = eval_term(p, err);
            left += right;
        }
        else if (**p == '-')
        {
            (*p)++;
            int right = eval_term(p, err);
            left -= right;
        }
        else
        {
            break;
        }
    }
    return left;
}

static int eval_condition(const char **p, int *err)
{
    int left = eval_expr(p, err);
    skip_spaces(p);
    char op[3] = {0, 0, 0};
    if ((**p == '=' && *(*p + 1) == '=') ||
        (**p == '!' && *(*p + 1) == '=') ||
        (**p == '<' && *(*p + 1) == '=') ||
        (**p == '>' && *(*p + 1) == '=') ||
        (**p == '<' && *(*p + 1) == '>'))
    {
        op[0] = **p;
        op[1] = *(*p + 1);
        *p += 2;
    }
    else if (**p == '=' || **p == '<' || **p == '>')
    {
        op[0] = **p;
        *p += 1;
    }
    else
    {
        *err = 3;
        return 0;
    }
    int right = eval_expr(p, err);
    if (op[1] == 0)
    {
        if (op[0] == '=')
            return left == right;
        if (op[0] == '<')
            return left < right;
        if (op[0] == '>')
            return left > right;
    }
    else
    {
        if (op[0] == '=' && op[1] == '=')
            return left == right;
        if (op[0] == '!' && op[1] == '=')
            return left != right;
        if (op[0] == '<' && op[1] == '>')
            return left != right;
        if (op[0] == '<' && op[1] == '=')
            return left <= right;
        if (op[0] == '>' && op[1] == '=')
            return left >= right;
    }
    return 0;
}

static int find_line_index(int line_num)
{
    for (int i = 0; i < prog_line_count; i++)
    {
        if (prog_lines[i].num == line_num)
        {
            return i;
        }
    }
    return -1;
}

static void exec_input(terminal_t *term, const char **p)
{
    skip_spaces(p);
    char prompt[40] = "? ";
    if (**p == '"')
    {
        (*p)++;
        int pi = 0;
        while (**p && **p != '"' && pi < 36)
        {
            prompt[pi++] = **p;
            (*p)++;
        }
        if (**p == '"')
            (*p)++;
        skip_spaces(p);
        if (**p == ';' || **p == ',')
            (*p)++;
        prompt[pi] = '\0';
    }
    skip_spaces(p);
    char var = to_upper(**p);
    if (var < 'A' || var > 'Z')
    {
        terminal_write_line(term, "INPUT: Variabel tidak valid");
        return;
    }
    (*p)++;

    terminal_write_line(term, prompt);

    char in_buf[32];
    int in_pos = 0;
    in_buf[0] = '\0';

    int row = term->total_lines - 1;
    if (row < 0)
        row = 0;

    mouse_state_t *m = mouse_get_state();
    while (1)
    {
        char k = get_last_key();
        if (k == 0)
        {
            continue;
        }
        if (k == '\n' || k == '\r')
        {
            break;
        }
        if (k == '\b')
        {
            if (in_pos > 0)
            {
                in_pos--;
                in_buf[in_pos] = '\0';
            }
        }
        else if (in_pos < 24 && ((k >= '0' && k <= '9') || k == '-' || (k >= 'a' && k <= 'z') || (k >= 'A' && k <= 'Z') || k == ' '))
        {
            in_buf[in_pos++] = k;
            in_buf[in_pos] = '\0';
        }

        int c_idx = 0;
        while (prompt[c_idx] && c_idx < TERM_COLS - 1)
        {
            term->buffer[row][c_idx] = prompt[c_idx];
            c_idx++;
        }
        int b_idx = 0;
        while (in_buf[b_idx] && c_idx < TERM_COLS - 1)
        {
            term->buffer[row][c_idx++] = in_buf[b_idx++];
        }
        term->buffer[row][c_idx] = '\0';

        if (m)
            render_desktop(m);
    }

    const char *inp = in_buf;
    int val = parse_int(&inp);
    vars[var - 'A'] = val;
}

static void exec_print(terminal_t *term, const char **p)
{
    skip_spaces(p);
    if (**p == '\0')
    {
        terminal_write_line(term, "");
        return;
    }
    char line[TERM_COLS];
    int lpos = 0;
    line[0] = '\0';

    while (**p != '\0')
    {
        skip_spaces(p);
        if (**p == '\0')
            break;

        if (**p == '"')
        {
            (*p)++;
            while (**p != '\0' && **p != '"' && lpos < TERM_COLS - 1)
            {
                line[lpos++] = **p;
                (*p)++;
            }
            if (**p == '"')
                (*p)++;
        }
        else
        {
            int err = 0;
            int val = eval_expr(p, &err);
            if (err)
            {
                terminal_write_line(term, "PRINT: Kesalahan ekspresi");
                return;
            }
            char num_buf[16];
            format_int(val, num_buf);
            int ni = 0;
            while (num_buf[ni] && lpos < TERM_COLS - 1)
            {
                line[lpos++] = num_buf[ni++];
            }
        }

        skip_spaces(p);
        if (**p == ';')
        {
            (*p)++;
        }
        else if (**p == ',')
        {
            (*p)++;
            if (lpos < TERM_COLS - 1)
            {
                line[lpos++] = ' ';
            }
        }
        else
        {
            break;
        }
    }
    line[lpos] = '\0';
    terminal_write_line(term, line);
}

static int exec_let(terminal_t *term, const char **p)
{
    skip_spaces(p);
    char var = to_upper(**p);
    if (var < 'A' || var > 'Z')
    {
        terminal_write_line(term, "LET: Variabel tidak valid");
        return BASIC_STATUS_ERROR;
    }
    (*p)++;
    skip_spaces(p);
    if (**p != '=')
    {
        terminal_write_line(term, "LET: Simbol '=' diharapkan");
        return BASIC_STATUS_ERROR;
    }
    (*p)++;
    int err = 0;
    int val = eval_expr(p, &err);
    if (err)
    {
        terminal_write_line(term, "LET: Kesalahan ekspresi");
        return BASIC_STATUS_ERROR;
    }
    vars[var - 'A'] = val;
    return BASIC_STATUS_OK;
}

static int exec_statement(terminal_t *term, const char **p);

static int exec_if(terminal_t *term, const char **p)
{
    int err = 0;
    int cond = eval_condition(p, &err);
    if (err)
    {
        terminal_write_line(term, "IF: Sintaks kondisi salah");
        return BASIC_STATUS_ERROR;
    }
    skip_spaces(p);
    if (match_keyword(p, "THEN"))
    {
        skip_spaces(p);
    }
    if (cond)
    {
        if (**p >= '0' && **p <= '9')
        {
            int target = parse_int(p);
            int idx = find_line_index(target);
            if (idx == -1)
            {
                terminal_write_line(term, "IF: Baris target tidak ditemukan");
                return BASIC_STATUS_ERROR;
            }
            next_pc = idx;
            return BASIC_STATUS_OK;
        }
        else
        {
            return exec_statement(term, p);
        }
    }
    return BASIC_STATUS_OK;
}

static int exec_statement(terminal_t *term, const char **p)
{
    skip_spaces(p);
    if (**p == '\0')
        return BASIC_STATUS_OK;

    if (match_keyword(p, "REM"))
    {
        return BASIC_STATUS_OK;
    }
    if (match_keyword(p, "END") || match_keyword(p, "STOP"))
    {
        return BASIC_STATUS_END;
    }
    if (match_keyword(p, "PRINT"))
    {
        exec_print(term, p);
        return BASIC_STATUS_OK;
    }
    if (match_keyword(p, "INPUT"))
    {
        exec_input(term, p);
        return BASIC_STATUS_OK;
    }
    if (match_keyword(p, "GOTO"))
    {
        int target = parse_int(p);
        int idx = find_line_index(target);
        if (idx == -1)
        {
            terminal_write_line(term, "GOTO: Baris tidak ditemukan");
            return BASIC_STATUS_ERROR;
        }
        next_pc = idx;
        return BASIC_STATUS_OK;
    }
    if (match_keyword(p, "IF"))
    {
        return exec_if(term, p);
    }
    if (match_keyword(p, "LET"))
    {
        return exec_let(term, p);
    }
    char c = to_upper(**p);
    if (c >= 'A' && c <= 'Z')
    {
        const char *peek = *p + 1;
        while (*peek == ' ' || *peek == '\t')
            peek++;
        if (*peek == '=')
        {
            return exec_let(term, p);
        }
    }

    terminal_write_line(term, "Sintaks tidak dikenal!");
    return BASIC_STATUS_ERROR;
}

static void basic_insert_line(int num, const char *text)
{
    skip_spaces(&text);
    if (*text == '\0')
    {
        int idx = find_line_index(num);
        if (idx != -1)
        {
            for (int i = idx; i < prog_line_count - 1; i++)
            {
                prog_lines[i] = prog_lines[i + 1];
            }
            prog_line_count--;
        }
        return;
    }

    int idx = find_line_index(num);
    if (idx != -1)
    {
        int j = 0;
        while (text[j] && j < BASIC_MAX_LINE_LEN - 1)
        {
            prog_lines[idx].text[j] = text[j];
            j++;
        }
        prog_lines[idx].text[j] = '\0';
        return;
    }

    if (prog_line_count >= BASIC_MAX_LINES)
        return;

    int pos = 0;
    while (pos < prog_line_count && prog_lines[pos].num < num)
    {
        pos++;
    }

    for (int i = prog_line_count; i > pos; i--)
    {
        prog_lines[i] = prog_lines[i - 1];
    }

    prog_lines[pos].num = num;
    int j = 0;
    while (text[j] && j < BASIC_MAX_LINE_LEN - 1)
    {
        prog_lines[pos].text[j] = text[j];
        j++;
    }
    prog_lines[pos].text[j] = '\0';
    prog_line_count++;
}

static void basic_list_lines(terminal_t *term)
{
    if (prog_line_count == 0)
    {
        terminal_write_line(term, "Program kosong.");
        return;
    }
    for (int i = 0; i < prog_line_count; i++)
    {
        char buf[TERM_COLS];
        char num_buf[16];
        format_int(prog_lines[i].num, num_buf);
        int bi = 0;
        int ni = 0;
        while (num_buf[ni] && bi < TERM_COLS - 2)
        {
            buf[bi++] = num_buf[ni++];
        }
        buf[bi++] = ' ';
        int ti = 0;
        while (prog_lines[i].text[ti] && bi < TERM_COLS - 1)
        {
            buf[bi++] = prog_lines[i].text[ti++];
        }
        buf[bi] = '\0';
        terminal_write_line(term, buf);
    }
}

static void basic_clear_prog(void)
{
    prog_line_count = 0;
    for (int i = 0; i < 26; i++)
        vars[i] = 0;
}

static void basic_run_prog(terminal_t *term)
{
    if (prog_line_count == 0)
    {
        terminal_write_line(term, "Program kosong.");
        return;
    }

    for (int i = 0; i < 26; i++)
        vars[i] = 0;

    int pc = 0;
    int steps = 0;
    is_running = 1;

    while (pc >= 0 && pc < prog_line_count && is_running && steps < 50000)
    {
        steps++;
        next_pc = -1;

        char k = get_last_key();
        if (k == 27)
        {
            terminal_write_line(term, "Program dihentikan (ESC).");
            break;
        }

        const char *p = prog_lines[pc].text;
        int status = exec_statement(term, &p);
        if (status == BASIC_STATUS_END || status == BASIC_STATUS_ERROR)
        {
            break;
        }

        if (next_pc != -1)
        {
            pc = next_pc;
        }
        else
        {
            pc++;
        }
    }

    if (steps >= 50000)
    {
        terminal_write_line(term, "Batas langkah terlampaui (50000).");
    }

    is_running = 0;
}

static void basic_save(terminal_t *term, const char *fname)
{
    while (*fname == ' ')
        fname++;
    if (*fname == '\0')
    {
        terminal_write_line(term, "Gunakan: SAVE <nama_file>");
        return;
    }
    static char save_buf[4096];
    int sb_pos = 0;
    for (int i = 0; i < prog_line_count && sb_pos < 4000; i++)
    {
        char num_buf[16];
        format_int(prog_lines[i].num, num_buf);
        int ni = 0;
        while (num_buf[ni] && sb_pos < 4000)
        {
            save_buf[sb_pos++] = num_buf[ni++];
        }
        save_buf[sb_pos++] = ' ';
        int ti = 0;
        while (prog_lines[i].text[ti] && sb_pos < 4000)
        {
            save_buf[sb_pos++] = prog_lines[i].text[ti++];
        }
        save_buf[sb_pos++] = '\n';
    }
    save_buf[sb_pos] = '\0';
    int res = vfs_create_file_raw(fname, save_buf);
    if (res == 0)
    {
        terminal_write_line(term, "Program berhasil disimpan.");
    }
    else
    {
        terminal_write_line(term, "Gagal menyimpan file.");
    }
}

static void basic_load(terminal_t *term, const char *fname)
{
    while (*fname == ' ')
        fname++;
    if (*fname == '\0')
    {
        terminal_write_line(term, "Gunakan: LOAD <nama_file>");
        return;
    }
    vfs_node_t *f = vfs_find_file(fname);
    if (!f || !f->data)
    {
        terminal_write_line(term, "File tidak ditemukan.");
        return;
    }
    basic_clear_prog();
    const char *data = f->data;
    while (*data)
    {
        while (*data == '\r' || *data == '\n')
            data++;
        if (*data == '\0')
            break;

        char line_buf[BASIC_MAX_LINE_LEN];
        int li = 0;
        while (*data && *data != '\r' && *data != '\n' && li < BASIC_MAX_LINE_LEN - 1)
        {
            line_buf[li++] = *data++;
        }
        line_buf[li] = '\0';

        const char *lp = line_buf;
        skip_spaces(&lp);
        if (*lp >= '0' && *lp <= '9')
        {
            int num = parse_int(&lp);
            skip_spaces(&lp);
            basic_insert_line(num, lp);
        }
    }
    terminal_write_line(term, "Program berhasil dimuat.");
}

void basic_init(void)
{
    basic_clear_prog();
    in_basic_repl = 0;
    is_running = 0;
}

int basic_is_active(void)
{
    return in_basic_repl;
}

void basic_enter(terminal_t *term)
{
    in_basic_repl = 1;
    terminal_write_line_color(term, "\033[36mLenovix BASIC v1.0\033[0m", 0x00D2FF);
    terminal_write_line(term, "Ketik LIST, RUN, SAVE, LOAD, EXIT");
    terminal_write_line(term, "READY");
}

void basic_exit(terminal_t *term)
{
    in_basic_repl = 0;
    terminal_write_line(term, "Keluar dari BASIC.");
}

void basic_handle_input(terminal_t *term, const char *line)
{
    skip_spaces(&line);
    if (*line == '\0')
        return;

    if (match_keyword(&line, "EXIT") || match_keyword(&line, "QUIT"))
    {
        basic_exit(term);
        return;
    }
    if (match_keyword(&line, "RUN"))
    {
        basic_run_prog(term);
        return;
    }
    if (match_keyword(&line, "LIST"))
    {
        basic_list_lines(term);
        return;
    }
    if (match_keyword(&line, "NEW"))
    {
        basic_clear_prog();
        terminal_write_line(term, "Program dikosongkan.");
        return;
    }
    if (match_keyword(&line, "SAVE"))
    {
        basic_save(term, line);
        return;
    }
    if (match_keyword(&line, "LOAD"))
    {
        basic_load(term, line);
        return;
    }

    if (*line >= '0' && *line <= '9')
    {
        int num = parse_int(&line);
        skip_spaces(&line);
        basic_insert_line(num, line);
        return;
    }

    exec_statement(term, &line);
}

void basic_run_file(terminal_t *term, const char *filename)
{
    while (*filename == ' ')
        filename++;
    basic_load(term, filename);
    if (prog_line_count > 0)
    {
        basic_run_prog(term);
    }
}
