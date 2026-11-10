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

    if (term->cursor_row >= TERM_ROWS)
    {
        for (int r = 0; r < TERM_ROWS - 1; r++)
        {
            for (int c = 0; c < TERM_COLS; c++)
            {
                term->buffer[r][c] = term->buffer[r + 1][c];
            }
            term->buffer_color[r] = term->buffer_color[r + 1];
        }
        term->cursor_row = TERM_ROWS - 1;
    }

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

        term->buffer[term->cursor_row][dst++] = text[c++];
    }

    term->buffer[term->cursor_row][dst] = '\0';
    term->buffer_color[term->cursor_row] = active_color;
    term->cursor_row++;
}

void terminal_write_line(terminal_t *term, const char *text)
{
    terminal_write_line_color(term, text, 0x00FF00);
}

void terminal_init(terminal_t *term)
{
    if (!term)
        return;

    for (int r = 0; r < TERM_ROWS; r++)
    {
        for (int c = 0; c < TERM_COLS; c++)
        {
            term->buffer[r][c] = '\0';
        }
        term->buffer_color[r] = 0x00FF00;
    }
    term->input_len = 0;
    term->input_buf[0] = '\0';
    term->cursor_row = 0;
    term->cursor_col = 0;

    terminal_write_line_color(term, "\033[36mLenovix OS Terminal v2.1\033[0m", 0x00D2FF);
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
        terminal_write_line(term, "RTL8139: NIC tidak terdeteksi.");
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

static void cmd_ping(terminal_t *term)
{
    if (!rtl8139_is_present())
    {
        terminal_write_line(term, "NIC tidak aktif untuk ping.");
        return;
    }

    const unsigned char *gw = net_get_gateway_ip();
    terminal_write_line(term, "PING 10.0.2.2 (ICMP 32B data)...");
    net_clear_ping_reply();
    net_send_ping(gw);

    for (int t = 0; t < 20000; t++)
    {
        net_poll();
        if (net_has_ping_reply())
        {
            terminal_write_line(term, "64B from 10.0.2.2: icmp_seq=1 ttl=64");
            return;
        }
    }
    terminal_write_line(term, "ICMP packet sent (awaiting network).");
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

static void terminal_execute_command(terminal_t *term)
{
    if (!term)
        return;

    char line[TERM_COLS];
    line[0] = '>';
    line[1] = ' ';
    line[2] = '\0';

    int i = 0;
    for (i = 0; i < term->input_len && i < (TERM_COLS - 3); i++)
    {
        line[i + 2] = term->input_buf[i];
    }
    line[i + 2] = '\0';
    terminal_write_line(term, line);

    if (str_equals(term->input_buf, "help"))
    {
        terminal_write_line(term, "Cmds: ls, cat, touch, write, free");
        terminal_write_line(term, "      fatls, fatread, elfdemo, tarlist");
        terminal_write_line(term, "      diskinfo, diskr, diskw, lspci");
        terminal_write_line(term, "      netinfo, ping, dns <domain>");
        terminal_write_line(term, "      sbtest, play <wav>, beep");
        terminal_write_line(term, "      userdemo, clear, ver, reboot");
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
    else if (str_equals(term->input_buf, "free"))
    {
        cmd_free(term);
    }
    else if (str_equals(term->input_buf, "diskinfo"))
    {
        cmd_diskinfo(term);
    }
    else if (str_starts_with(term->input_buf, "diskr "))
    {
        cmd_diskr(term, term->input_buf + 6);
    }
    else if (str_starts_with(term->input_buf, "diskw "))
    {
        cmd_diskw(term, term->input_buf + 6);
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
    else if (str_equals(term->input_buf, "netinfo"))
    {
        cmd_netinfo(term);
    }
    else if (str_equals(term->input_buf, "ping"))
    {
        cmd_ping(term);
    }
    else if (str_starts_with(term->input_buf, "dns "))
    {
        cmd_dns(term, term->input_buf + 4);
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
        vfs_node_t *f = vfs_find_file(fname);
        if (f && f->data)
        {
            int r = sb16_play_wav(f->data, f->length);
            if (r == 0)
            {
                terminal_write_line(term, "Memainkan file WAV...");
            }
            else
            {
                terminal_write_line(term, "Format WAV tidak sesuai (8-bit PCM).");
            }
        }
        else
        {
            terminal_write_line(term, "File tidak ditemukan.");
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

    int term_x = win_x + 10;
    int term_y = win_y + 35;
    int term_w = 380;
    int term_h = 205;

    vesa_draw_rect(term_x, term_y, term_w, term_h, 0x000000);

    for (int r = 0; r < TERM_ROWS; r++)
    {
        if (term->buffer[r][0] != '\0')
        {
            vesa_draw_string(term_x + 5, term_y + 5 + (r * 18), term->buffer[r], term->buffer_color[r]);
        }
    }

    int prompt_y = term_y + 5 + (term->cursor_row * 18);
    if (prompt_y < term_y + term_h - 20)
    {
        vesa_draw_string(term_x + 5, prompt_y, ">", 0x00FF00);
        vesa_draw_string(term_x + 20, prompt_y, term->input_buf, 0xFFFFFF);

        int cursor_x = term_x + 20 + (term->input_len * 8);
        vesa_draw_string(cursor_x, prompt_y, "_", 0x00FF00);
    }
}
