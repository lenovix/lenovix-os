#include "player.h"
#include "audio_codec.h"
#include "vesa.h"
#include "vfs.h"
#include "string_util.h"

static int playlist_idx = 0;
static char current_track_name[32] = "sample.mp3";

static void format_seconds(unsigned int sec, char *buf)
{
    unsigned int m = sec / 60;
    unsigned int s = sec % 60;
    buf[0] = (m / 10) + '0';
    buf[1] = (m % 10) + '0';
    buf[2] = ':';
    buf[3] = (s / 10) + '0';
    buf[4] = (s % 10) + '0';
    buf[5] = '\0';
}

void player_init(void)
{
    audio_init();
    audio_load_file(current_track_name);
}

static void play_next_audio_file(void)
{
    int total = vfs_get_count();
    if (total == 0)
        return;

    for (int i = 1; i <= total; i++)
    {
        int idx = (playlist_idx + i) % total;
        vfs_node_t *node = vfs_get_node(idx);
        if (node && node->name[0] != '\0')
        {
            int len = 0;
            while (node->name[len])
                len++;
            if (len >= 4)
            {
                const char *ext = node->name + len - 4;
                if ((ext[0] == '.' && (ext[1] == 'm' || ext[1] == 'M') && (ext[2] == 'p' || ext[2] == 'P') && ext[3] == '3') ||
                    (ext[0] == '.' && (ext[1] == 'w' || ext[1] == 'W') && (ext[2] == 'a' || ext[2] == 'A') && (ext[3] == 'v' || ext[3] == 'V')) ||
                    (ext[0] == '.' && (ext[1] == 'm' || ext[1] == 'M') && (ext[2] == 'o' || ext[2] == 'O') && (ext[3] == 'd' || ext[3] == 'D')))
                {
                    playlist_idx = idx;
                    string_copy(current_track_name, node->name);
                    audio_load_file(current_track_name);
                    return;
                }
            }
        }
    }
}

void player_draw(window_t *win)
{
    if (!win || !win->is_visible)
        return;

    audio_player_state_t *st = audio_get_state();
    int ax = win->x + 8;
    int ay = win->y + 32;
    int aw = win->width - 16;
    int ah = win->height - 40;

    vesa_draw_rect(ax, ay, aw, ah, 0x16161D);

    int sx = ax + 10;
    int sy = ay + 8;
    int sw = aw - 20;
    int sh = 80;

    vesa_draw_rect(sx - 1, sy - 1, sw + 2, sh + 2, 0x333344);
    vesa_draw_rect(sx, sy, sw, sh, 0x080810);

    int bar_w = (sw - 20) / SPECTRUM_BANDS;
    if (bar_w < 6)
        bar_w = 6;
    int max_bar_h = sh - 14;

    for (int b = 0; b < SPECTRUM_BANDS; b++)
    {
        int bx = sx + 10 + b * bar_w;
        int val = (int)st->spectrum[b];
        int bh = (val * max_bar_h) / 100;
        if (bh > max_bar_h)
            bh = max_bar_h;

        for (int h = 0; h < bh; h += 3)
        {
            unsigned int col = 0x00FF88; // Green base
            if (h > (max_bar_h * 2) / 3)
                col = 0xFF4136; // Red top
            else if (h > max_bar_h / 3)
                col = 0xFFDC00; // Yellow mid

            vesa_draw_rect(bx, sy + sh - 6 - h - 2, bar_w - 2, 2, col);
        }

        // Peak line
        int peak_h = ((int)st->peaks[b] * max_bar_h) / 100;
        if (peak_h > max_bar_h)
            peak_h = max_bar_h;
        if (peak_h > 2)
        {
            vesa_draw_rect(bx, sy + sh - 6 - peak_h - 2, bar_w - 2, 1, 0xFFFFFF);
        }
    }

    int tx = sx;
    int ty = sy + sh + 10;

    vesa_draw_rect(tx, ty, sw, 34, 0x22222E);

    const char *fmt_str = "[WAV]";
    if (st->format == AUDIO_FORMAT_MP3)
        fmt_str = "[MP3]";
    else if (st->format == AUDIO_FORMAT_CHIP)
        fmt_str = "[CHIP]";

    vesa_draw_string(tx + 8, ty + 6, fmt_str, 0x00D2FF);
    vesa_draw_string(tx + 60, ty + 6, st->title[0] ? st->title : "Tidak ada berkas", 0xFFFFFF);

    unsigned int cur_sec = (st->sample_rate > 0) ? (st->current_sample / st->sample_rate) : 0;
    unsigned int tot_sec = (st->sample_rate > 0) ? (st->total_samples / st->sample_rate) : 0;
    char time_str[16];
    char c_buf[6], t_buf[6];
    format_seconds(cur_sec, c_buf);
    format_seconds(tot_sec, t_buf);

    int pos = 0;
    for (int i = 0; c_buf[i]; i++)
        time_str[pos++] = c_buf[i];
    time_str[pos++] = ' ';
    time_str[pos++] = '/';
    time_str[pos++] = ' ';
    for (int i = 0; t_buf[i]; i++)
        time_str[pos++] = t_buf[i];
    time_str[pos] = '\0';

    vesa_draw_string(tx + sw - 100, ty + 6, time_str, 0x00FF88);

    int pb_x = tx + 8;
    int pb_y = ty + 22;
    int pb_w = sw - 16;
    int pb_h = 6;
    vesa_draw_rect(pb_x, pb_y, pb_w, pb_h, 0x11111A);

    int fill_w = 0;
    if (st->total_samples > 0)
    {
        fill_w = (int)((st->current_sample * (unsigned int)pb_w) / st->total_samples);
        if (fill_w > pb_w)
            fill_w = pb_w;
    }
    if (fill_w > 0)
    {
        vesa_draw_rect(pb_x, pb_y, fill_w, pb_h, 0x0074D9);
    }

    int by = ty + 42;
    int bw = 60;
    int bh = 28;

    int b1_x = sx;
    vesa_draw_rect(b1_x, by, bw, bh, 0x2A2A38);
    vesa_draw_string(b1_x + 12, by + 8, "FILE", 0xFFFFFF);

    int b2_x = b1_x + bw + 10;
    int is_playing = (st->is_playing && !st->is_paused);
    vesa_draw_rect(b2_x, by, bw + 10, bh, is_playing ? 0x0074D9 : 0x2ECC40);
    vesa_draw_string(b2_x + 12, by + 8, is_playing ? "PAUSE" : "PLAY", 0xFFFFFF);

    int b3_x = b2_x + bw + 20;
    vesa_draw_rect(b3_x, by, bw, bh, 0xFF4136);
    vesa_draw_string(b3_x + 14, by + 8, "STOP", 0xFFFFFF);

    int b4_x = b3_x + bw + 10;
    vesa_draw_rect(b4_x, by, bw, bh, 0x2A2A38);
    vesa_draw_string(b4_x + 14, by + 8, "NEXT", 0xFFFFFF);

    const char *status_str = "STOPPED";
    unsigned int status_col = 0x888888;
    if (st->is_playing)
    {
        if (st->is_paused)
        {
            status_str = "PAUSED";
            status_col = 0xFFDC00;
        }
        else
        {
            status_str = "PLAYING";
            status_col = 0x00FF88;
        }
    }
    vesa_draw_string(b4_x + bw + 20, by + 8, status_str, status_col);
}

int player_handle_click(window_t *win, int mouse_x, int mouse_y)
{
    if (!win || !win->is_visible)
        return 0;

    int ax = win->x + 8;
    int ay = win->y + 32;
    int sx = ax + 10;
    int sh = 80;
    int ty = ay + 8 + sh + 10;
    int by = ty + 42;
    int bw = 60;
    int bh = 28;

    // Button 1: FILE
    int b1_x = sx;
    if (mouse_x >= b1_x && mouse_x <= b1_x + bw && mouse_y >= by && mouse_y <= by + bh)
    {
        play_next_audio_file();
        return 1;
    }

    // Button 2: PLAY / PAUSE
    int b2_x = b1_x + bw + 10;
    if (mouse_x >= b2_x && mouse_x <= b2_x + bw + 10 && mouse_y >= by && mouse_y <= by + bh)
    {
        audio_toggle_play_pause();
        return 1;
    }

    // Button 3: STOP
    int b3_x = b2_x + bw + 20;
    if (mouse_x >= b3_x && mouse_x <= b3_x + bw && mouse_y >= by && mouse_y <= by + bh)
    {
        audio_stop();
        return 1;
    }

    // Button 4: NEXT
    int b4_x = b3_x + bw + 10;
    if (mouse_x >= b4_x && mouse_x <= b4_x + bw && mouse_y >= by && mouse_y <= by + bh)
    {
        play_next_audio_file();
        return 1;
    }

    return 0;
}

void player_handle_key(char c)
{
    if (c == ' ')
    {
        audio_toggle_play_pause();
    }
    else if (c == 's' || c == 'S')
    {
        audio_stop();
    }
    else if (c == 'n' || c == 'N' || c == 'f' || c == 'F')
    {
        play_next_audio_file();
    }
}
