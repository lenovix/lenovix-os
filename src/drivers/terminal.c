#include "terminal.h"
#include "vesa.h"

// Helper sederhana untuk membandingkan string
static int str_equals(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(unsigned char *)s1 - *(unsigned char *)s2 == 0;
}

// Helper untuk menyalin string
static void str_copy(char *dest, const char *src) {
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

void terminal_init(terminal_t *term) {
    for (int r = 0; r < TERM_ROWS; r++) {
        for (int c = 0; c < TERM_COLS; c++) {
            term->buffer[r][c] = '\0';
        }
    }
    term->input_len = 0;
    term->input_buf[0] = '\0';
    term->cursor_row = 0;
    term->cursor_col = 0;

    terminal_write_line(term, "Lenovix OS Terminal v1.0");
    terminal_write_line(term, "Ketik 'help' untuk daftar perintah.");
}

void terminal_write_line(terminal_t *term, const char *str) {
    // Jika baris penuh, scroll ke atas 1 baris
    if (term->cursor_row >= TERM_ROWS) {
        for (int r = 0; r < TERM_ROWS - 1; r++) {
            str_copy(term->buffer[r], term->buffer[r + 1]);
        }
        for (int c = 0; c < TERM_COLS; c++) {
            term->buffer[TERM_ROWS - 1][c] = '\0';
        }
        term->cursor_row = TERM_ROWS - 1;
    }

    str_copy(term->buffer[term->cursor_row], str);
    term->cursor_row++;
}

static void terminal_execute_command(terminal_t *term) {
    char line[60];
    line[0] = '>';
    line[1] = ' ';
    line[2] = '\0';
    
    // Tampilkan perintah yang diketik user ke history
    for (int i = 0; i < term->input_len && i < 35; i++) {
        line[i + 2] = term->input_buf[i];
        line[i + 3] = '\0';
    }
    terminal_write_line(term, line);

    // Proses Perintah
    if (str_equals(term->input_buf, "help")) {
        terminal_write_line(term, "Perintah: help, clear, ver, about");
    } else if (str_equals(term->input_buf, "clear")) {
        for (int r = 0; r < TERM_ROWS; r++) {
            for (int c = 0; c < TERM_COLS; c++) {
                term->buffer[r][c] = '\0';
            }
        }
        term->cursor_row = 0;
    } else if (str_equals(term->input_buf, "ver")) {
        terminal_write_line(term, "Lenovix OS Kernel v0.3.0 (VESA)");
    } else if (str_equals(term->input_buf, "about")) {
        terminal_write_line(term, "Dibuat oleh Kamil Sudarmi.");
    } else if (term->input_len > 0) {
        terminal_write_line(term, "Perintah tidak dikenal!");
    }

    // Reset input buffer
    term->input_len = 0;
    term->input_buf[0] = '\0';
}

void terminal_handle_key(terminal_t *term, char c) {
    if (c == '\n') { // Tombol Enter
        terminal_execute_command(term);
    } else if (c == '\b') { // Tombol Backspace
        if (term->input_len > 0) {
            term->input_len--;
            term->input_buf[term->input_len] = '\0';
        }
    } else if (c >= 32 && c <= 126) { // Karakter ASCII yang dapat dicetak
        if (term->input_len < MAX_INPUT_LEN - 1) {
            term->input_buf[term->input_len] = c;
            term->input_len++;
            term->input_buf[term->input_len] = '\0';
        }
    }
}

void terminal_draw(terminal_t *term, int win_x, int win_y) {
    // Area dalam Window (Background Hitam Terminal)
    int term_x = win_x + 10;
    int term_y = win_y + 35;
    int term_w = 380;
    int term_h = 205;

    vesa_draw_rect(term_x, term_y, term_w, term_h, 0x000000);

    // Gambar History Teks Terminal
    for (int r = 0; r < TERM_ROWS; r++) {
        if (term->buffer[r][0] != '\0') {
            vesa_draw_string(term_x + 5, term_y + 5 + (r * 18), term->buffer[r], 0x00FF00); // Teks Hijau Terminal
        }
    }

    // Gambar Prompt Input Saat Ini (> input...)
    int prompt_y = term_y + 5 + (term->cursor_row * 18);
    if (prompt_y < term_y + term_h - 20) {
        vesa_draw_string(term_x + 5, prompt_y, ">", 0x00FF00);
        vesa_draw_string(term_x + 20, prompt_y, term->input_buf, 0xFFFFFF);

        // Kursor Kedip sederhanakan dengan kursor blok (_)
        int cursor_x = term_x + 20 + (term->input_len * 8);
        vesa_draw_string(cursor_x, prompt_y, "_", 0x00FF00);
    }
}