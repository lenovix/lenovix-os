#include "browser.h"
#include "html_parser.h"
#include "vesa.h"
#include "vfs.h"
#include "tcp.h"
#include "net.h"
#include "rtl8139.h"
#include "string_util.h"

#define BROWSER_HIST_MAX 8
#define RX_BUF_SIZE 16384

static char current_url[128] = "about:lenovix";
static char url_input[128] = "about:lenovix";
static int url_input_len = 13;
static int url_focused = 0;
static char status_text[64] = "Ready";
static int scroll_y = 0;

static char history[BROWSER_HIST_MAX][128];
static int hist_count = 0;
static int hist_idx = -1;

static html_doc_t doc;
static char http_rx_buffer[RX_BUF_SIZE];

static const char *default_home_html =
    "<html><head><title>Lenovix Portal</title></head><body>"
    "<h1>Lenovix Web Browser</h1>"
    "<p>Peramban web grafis ringan untuk Lenovix OS.</p>"
    "<hr>"
    "<h2>Halaman & Dokumen Tersedia</h2>"
    "<ul>"
    "<li><a href=\"index.html\">Buka Berkas: index.html</a></li>"
    "<li><a href=\"readme.txt\">Buka Berkas: readme.txt</a></li>"
    "<li><a href=\"http://10.0.2.2/\">Akses Gateway QEMU (10.0.2.2)</a></li>"
    "</ul>"
    "<hr>"
    "<h3>Petunjuk Navigasi:</h3>"
    "<p>Ketik URL di bar alamat lalu tekan [GO] atau Enter. Klik link hijau untuk navigasi langsung.</p>"
    "</body></html>";

static void set_status(const char *st)
{
    int i = 0;
    while (st[i] && i < 63)
    {
        status_text[i] = st[i];
        i++;
    }
    status_text[i] = '\0';
}

static unsigned int parse_port(const char *str)
{
    unsigned int p = 0;
    while (*str >= '0' && *str <= '9')
    {
        p = p * 10 + (*str - '0');
        str++;
    }
    return p;
}

static void fetch_http(const char *url_path)
{
    if (!rtl8139_is_present())
    {
        set_status("Error: eth0 DOWN (QEMU -nic model=rtl8139)");
        html_parse("<h1>Koneksi Gagal</h1><p>Kartu jaringan RTL8139 tidak aktif.</p><p>Jalankan QEMU dengan: -nic model=rtl8139</p>", &doc, 50);
        return;
    }

    const char *p = url_path;
    if (p[0] == 'h' && p[1] == 't' && p[2] == 't' && p[3] == 'p' && p[4] == ':' && p[5] == '/' && p[6] == '/')
    {
        p += 7;
    }

    char host[64] = {0};
    int hi = 0;
    while (*p && *p != '/' && *p != ':' && hi < 63)
    {
        host[hi++] = *p++;
    }
    host[hi] = '\0';

    unsigned short port = 80;
    if (*p == ':')
    {
        p++;
        port = (unsigned short)parse_port(p);
        while (*p >= '0' && *p <= '9')
            p++;
    }

    const char *path = "/";
    if (*p == '/')
    {
        path = p;
    }

    unsigned char ip[4] = {0};
    int is_ip = 1;
    for (int i = 0; host[i]; i++)
    {
        if (!((host[i] >= '0' && host[i] <= '9') || host[i] == '.'))
        {
            is_ip = 0;
            break;
        }
    }

    if (is_ip)
    {
        const char *hp = host;
        for (int oct = 0; oct < 4; oct++)
        {
            unsigned int v = 0;
            while (*hp >= '0' && *hp <= '9')
            {
                v = v * 10 + (*hp - '0');
                hp++;
            }
            ip[oct] = (unsigned char)v;
            if (*hp == '.')
                hp++;
        }
    }
    else
    {
        set_status("DNS resolving...");
        net_clear_dns_reply();
        net_send_dns_query(host);
        int ok = 0;
        for (int t = 0; t < 25000; t++)
        {
            net_poll();
            if (net_has_dns_reply())
            {
                net_get_dns_result(ip);
                ok = 1;
                break;
            }
        }
        if (!ok)
        {
            set_status("DNS lookup failed");
            html_parse("<h1>DNS Gagal</h1><p>Tidak dapat menemukan alamat host tujuan.</p>", &doc, 50);
            return;
        }
    }

    set_status("Connecting TCP...");
    int c = tcp_connect(ip, port);
    if (c != 0)
    {
        set_status("TCP connection failed");
        html_parse("<h1>Gagal Menghubungkan</h1><p>Koneksi TCP ke host ditolak atau waktu habis.</p>", &doc, 50);
        return;
    }

    set_status("Sending HTTP GET...");
    char req[256];
    int ri = 0;
    const char *r_get = "GET ";
    while (*r_get)
        req[ri++] = *r_get++;
    const char *r_path = path;
    while (*r_path && ri < 120)
        req[ri++] = *r_path++;
    const char *r_hdr = " HTTP/1.0\r\nHost: ";
    while (*r_hdr)
        req[ri++] = *r_hdr++;
    const char *r_host = host;
    while (*r_host && ri < 180)
        req[ri++] = *r_host++;
    const char *r_end = "\r\nUser-Agent: LenovixBrowser/1.0\r\n\r\n";
    while (*r_end)
        req[ri++] = *r_end++;
    req[ri] = '\0';

    tcp_send(req, ri);

    set_status("Receiving HTTP data...");
    int total_rx = 0;
    for (int t = 0; t < 35000 && total_rx < RX_BUF_SIZE - 1; t++)
    {
        net_poll();
        int n = tcp_get_rx_data(http_rx_buffer + total_rx, RX_BUF_SIZE - 1 - total_rx);
        if (n > 0)
        {
            total_rx += n;
        }
    }
    http_rx_buffer[total_rx] = '\0';
    tcp_close();

    if (total_rx == 0)
    {
        set_status("HTTP Error: Empty response");
        html_parse("<h1>Tidak Ada Respons</h1><p>Server web tidak memberikan data respon.</p>", &doc, 50);
        return;
    }

    // Find end of HTTP headers \r\n\r\n
    const char *body = http_rx_buffer;
    for (int i = 0; i < total_rx - 3; i++)
    {
        if (http_rx_buffer[i] == '\r' && http_rx_buffer[i + 1] == '\n' &&
            http_rx_buffer[i + 2] == '\r' && http_rx_buffer[i + 3] == '\n')
        {
            body = http_rx_buffer + i + 4;
            break;
        }
    }

    set_status("200 OK");
    html_parse(body, &doc, 50);
}

void browser_navigate(const char *url)
{
    while (*url == ' ')
        url++;
    if (*url == '\0')
        url = "about:lenovix";

    string_copy(current_url, url);
    string_copy(url_input, url);
    url_input_len = string_length(url_input);
    scroll_y = 0;

    if (hist_count == 0 || !string_equals(history[hist_idx], url))
    {
        if (hist_count < BROWSER_HIST_MAX)
        {
            hist_idx = hist_count++;
        }
        else
        {
            for (int i = 0; i < BROWSER_HIST_MAX - 1; i++)
            {
                string_copy(history[i], history[i + 1]);
            }
            hist_idx = BROWSER_HIST_MAX - 1;
        }
        string_copy(history[hist_idx], url);
    }

    if (string_equals(url, "about:lenovix") || string_equals(url, "about:blank"))
    {
        set_status("Local Portal");
        html_parse(default_home_html, &doc, 50);
        return;
    }

    if (url[0] == 'h' && url[1] == 't' && url[2] == 't' && url[3] == 'p' && url[4] == ':' && url[5] == '/' && url[6] == '/')
    {
        fetch_http(url);
        return;
    }

    const char *vfs_name = url;
    if (vfs_name[0] == 'f' && vfs_name[1] == 'i' && vfs_name[2] == 'l' && vfs_name[3] == 'e' &&
        vfs_name[4] == ':' && vfs_name[5] == '/' && vfs_name[6] == '/' && vfs_name[7] == '/')
    {
        vfs_name += 8;
    }

    vfs_node_t *f = vfs_find_file(vfs_name);
    if (f && f->data)
    {
        set_status("File Loaded");
        html_parse(f->data, &doc, 50);
    }
    else
    {
        set_status("File Not Found");
        char err_msg[128];
        char p1[] = "<h1>Berkas Tidak Ditemukan</h1><p>Tidak dapat membuka: ";
        int ei = 0;
        for (int i = 0; p1[i]; i++)
            err_msg[ei++] = p1[i];
        for (int i = 0; vfs_name[i] && ei < 110; i++)
            err_msg[ei++] = vfs_name[i];
        err_msg[ei++] = '<';
        err_msg[ei++] = '/';
        err_msg[ei++] = 'p';
        err_msg[ei++] = '>';
        err_msg[ei] = '\0';
        html_parse(err_msg, &doc, 50);
    }
}

void browser_init(void)
{
    hist_count = 0;
    hist_idx = -1;
    url_focused = 0;
    browser_navigate("about:lenovix");
}

void browser_draw(window_t *win)
{
    if (!win || !win->is_visible)
        return;

    int ax = win->x + 8;
    int ay = win->y + 32;
    int aw = win->width - 16;
    int ah = win->height - 40;

    vesa_draw_rect(ax, ay, aw, ah, 0x141824);

    // 1. Navigation Toolbar
    int tb_y = ay + 4;
    int btn_h = 24;

    // [< Back]
    int b_back_x = ax + 4;
    int b_back_w = 26;
    vesa_draw_rect(b_back_x, tb_y, b_back_w, btn_h, (hist_idx > 0) ? 0x2A324B : 0x1C2233);
    vesa_draw_string(b_back_x + 8, tb_y + 5, "<", 0xFFFFFF);

    // [> Fwd]
    int b_fwd_x = b_back_x + b_back_w + 4;
    int b_fwd_w = 26;
    vesa_draw_rect(b_fwd_x, tb_y, b_fwd_w, btn_h, (hist_idx < hist_count - 1) ? 0x2A324B : 0x1C2233);
    vesa_draw_string(b_fwd_x + 8, tb_y + 5, ">", 0xFFFFFF);

    // [R Reload]
    int b_rel_x = b_fwd_x + b_fwd_w + 4;
    int b_rel_w = 26;
    vesa_draw_rect(b_rel_x, tb_y, b_rel_w, btn_h, 0x2A324B);
    vesa_draw_string(b_rel_x + 8, tb_y + 5, "R", 0xFFFFFF);

    // URL Bar
    int url_x = b_rel_x + b_rel_w + 6;
    int go_w = 36;
    int url_w = aw - (url_x - ax) - go_w - 10;
    vesa_draw_rect(url_x, tb_y, url_w, btn_h, url_focused ? 0x080C14 : 0x1A2030);
    vesa_draw_rect(url_x, tb_y, url_w, 1, url_focused ? 0x00D2FF : 0x334466);
    vesa_draw_rect(url_x, tb_y + btn_h - 1, url_w, 1, url_focused ? 0x00D2FF : 0x334466);

    char disp_url[64];
    int di = 0;
    int max_url_disp = (url_w / 8) - 2;
    for (int i = 0; url_input[i] && di < max_url_disp; i++)
    {
        disp_url[di++] = url_input[i];
    }
    disp_url[di] = '\0';
    vesa_draw_string(url_x + 8, tb_y + 5, disp_url, 0xFFFFFF);

    if (url_focused && di < max_url_disp)
    {
        vesa_draw_string(url_x + 8 + di * 8, tb_y + 5, "_", 0x00FF88);
    }

    // [GO]
    int go_x = url_x + url_w + 6;
    vesa_draw_rect(go_x, tb_y, go_w, btn_h, 0x0074D9);
    vesa_draw_string(go_x + 10, tb_y + 5, "GO", 0xFFFFFF);

    // 2. Viewport Area
    int vp_x = ax + 4;
    int vp_y = tb_y + btn_h + 6;
    int vp_w = aw - 8;
    int vp_h = ah - (vp_y - ay) - 22;

    vesa_draw_rect(vp_x, vp_y, vp_w, vp_h, 0x0B0E17);
    vesa_draw_rect(vp_x, vp_y, vp_w, 1, 0x22334A);

    int line_h = 16;
    int visible_lines = vp_h / line_h;
    if (visible_lines > doc.line_count)
        visible_lines = doc.line_count;

    for (int i = 0; i < visible_lines; i++)
    {
        int idx = scroll_y + i;
        if (idx >= doc.line_count)
            break;

        html_line_t *ln = &doc.lines[idx];
        int ly = vp_y + 6 + i * line_h;

        if (ln->is_heading)
        {
            vesa_draw_rect(vp_x + 6, ly + 14, vp_w - 24, 1, 0x005588);
        }
        vesa_draw_string(vp_x + 8, ly, ln->text, ln->color);
    }

    // Scrollbar indicator
    int sb_x = vp_x + vp_w - 12;
    vesa_draw_rect(sb_x, vp_y + 2, 8, vp_h - 4, 0x1A2234);
    if (doc.line_count > visible_lines && doc.line_count > 0)
    {
        int thumb_h = (visible_lines * (vp_h - 4)) / doc.line_count;
        if (thumb_h < 12)
            thumb_h = 12;
        int thumb_y = vp_y + 2 + (scroll_y * (vp_h - 4 - thumb_h)) / (doc.line_count - visible_lines);
        vesa_draw_rect(sb_x, thumb_y, 8, thumb_h, 0x0074D9);
    }

    // 3. Status Bar
    int sb_bar_y = vp_y + vp_h + 3;
    vesa_draw_rect(ax, sb_bar_y, aw, 18, 0x181F2D);
    vesa_draw_string(ax + 6, sb_bar_y + 2, status_text, 0x8899AA);
    vesa_draw_string(ax + aw - 140, sb_bar_y + 2, doc.title, 0x00D2FF);
}

int browser_handle_click(window_t *win, int mouse_x, int mouse_y)
{
    if (!win || !win->is_visible)
        return 0;

    int ax = win->x + 8;
    int ay = win->y + 32;
    int aw = win->width - 16;
    int tb_y = ay + 4;
    int btn_h = 24;

    int b_back_x = ax + 4;
    int b_back_w = 26;
    int b_fwd_x = b_back_x + b_back_w + 4;
    int b_fwd_w = 26;
    int b_rel_x = b_fwd_x + b_fwd_w + 4;
    int b_rel_w = 26;

    int url_x = b_rel_x + b_rel_w + 6;
    int go_w = 36;
    int url_w = aw - (url_x - ax) - go_w - 10;
    int go_x = url_x + url_w + 6;

    // Back click
    if (mouse_x >= b_back_x && mouse_x <= b_back_x + b_back_w && mouse_y >= tb_y && mouse_y <= tb_y + btn_h)
    {
        if (hist_idx > 0)
        {
            hist_idx--;
            browser_navigate(history[hist_idx]);
        }
        return 1;
    }

    // Fwd click
    if (mouse_x >= b_fwd_x && mouse_x <= b_fwd_x + b_fwd_w && mouse_y >= tb_y && mouse_y <= tb_y + btn_h)
    {
        if (hist_idx < hist_count - 1)
        {
            hist_idx++;
            browser_navigate(history[hist_idx]);
        }
        return 1;
    }

    // Reload click
    if (mouse_x >= b_rel_x && mouse_x <= b_rel_x + b_rel_w && mouse_y >= tb_y && mouse_y <= tb_y + btn_h)
    {
        browser_navigate(current_url);
        return 1;
    }

    // GO click
    if (mouse_x >= go_x && mouse_x <= go_x + go_w && mouse_y >= tb_y && mouse_y <= tb_y + btn_h)
    {
        url_focused = 0;
        browser_navigate(url_input);
        return 1;
    }

    // URL Bar focus click
    if (mouse_x >= url_x && mouse_x <= url_x + url_w && mouse_y >= tb_y && mouse_y <= tb_y + btn_h)
    {
        url_focused = 1;
        return 1;
    }

    // Viewport clicks (Links or Scroll)
    int vp_x = ax + 4;
    int vp_y = tb_y + btn_h + 6;
    int vp_w = aw - 8;
    int ah = win->height - 40;
    int vp_h = ah - (vp_y - ay) - 22;

    if (mouse_x >= vp_x && mouse_x <= vp_x + vp_w && mouse_y >= vp_y && mouse_y <= vp_y + vp_h)
    {
        url_focused = 0;

        // Check scrollbar click
        int sb_x = vp_x + vp_w - 14;
        if (mouse_x >= sb_x)
        {
            if (mouse_y < vp_y + vp_h / 2)
            {
                if (scroll_y > 3)
                    scroll_y -= 3;
                else
                    scroll_y = 0;
            }
            else
            {
                if (scroll_y + 3 < doc.line_count)
                    scroll_y += 3;
            }
            return 1;
        }

        // Check Link click
        int line_h = 16;
        int clicked_row = (mouse_y - (vp_y + 6)) / line_h;
        int target_line = scroll_y + clicked_row;
        if (target_line >= 0 && target_line < doc.line_count)
        {
            if (doc.lines[target_line].is_link && doc.lines[target_line].href[0])
            {
                browser_navigate(doc.lines[target_line].href);
                return 1;
            }
        }
    }

    url_focused = 0;
    return 0;
}

void browser_handle_key(char c)
{
    if (url_focused)
    {
        if (c == '\n' || c == '\r')
        {
            url_focused = 0;
            browser_navigate(url_input);
        }
        else if (c == '\b')
        {
            if (url_input_len > 0)
            {
                url_input_len--;
                url_input[url_input_len] = '\0';
            }
        }
        else if (c == 27)
        { // ESC
            url_focused = 0;
            string_copy(url_input, current_url);
            url_input_len = string_length(url_input);
        }
        else if (url_input_len < 120 && (unsigned char)c >= 32 && (unsigned char)c <= 126)
        {
            url_input[url_input_len++] = c;
            url_input[url_input_len] = '\0';
        }
        return;
    }

    if (c == 0x11 || c == 'k' || c == 'K')
    { // Up arrow
        if (scroll_y > 0)
            scroll_y--;
    }
    else if (c == 0x12 || c == 'j' || c == 'J')
    { // Down arrow
        if (scroll_y + 5 < doc.line_count)
            scroll_y++;
    }
    else if (c == ' ' || c == 0x14)
    { // Page down
        if (scroll_y + 10 < doc.line_count)
            scroll_y += 10;
        else
            scroll_y = (doc.line_count > 10) ? doc.line_count - 10 : 0;
    }
    else if (c == 0x13)
    { // Page up
        if (scroll_y > 10)
            scroll_y -= 10;
        else
            scroll_y = 0;
    }
    else if (c == 'r' || c == 'R')
    {
        browser_navigate(current_url);
    }
    else if (c == 'b' || c == 'B')
    {
        if (hist_idx > 0)
        {
            hist_idx--;
            browser_navigate(history[hist_idx]);
        }
    }
    else if (c == '/')
    {
        url_focused = 1;
    }
}
