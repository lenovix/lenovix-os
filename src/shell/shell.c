#include "shell.h"
#include "tty.h"
#include "vfs.h"
#include "heap.h"
#include "task.h"
#include "user.h"
#include "vesa.h"

#define HISTORY_MAX 5

char command_buffer[256];
int command_index = 0;
static char history[HISTORY_MAX][256];
static int history_count = 0;
static int history_index = -1;
static void *test_ptr = NULL;

extern void shutdown_qemu(void);
extern unsigned int timer_ticks;
extern unsigned int pmm_get_free_block_count(void);
extern unsigned int pmm_get_max_blocks(void);

// Task & User Mode Demos
void task_a_func(void) {
    while (1) {
        // Cetak sesuatu untuk menandakan Task A aktif
        kprint("[A]", 0x0B); 
        
        // Berikan giliran ke task berikutnya
        task_yield();
    }
}

void task_b_func(void) {
    while (1) {
        // Cetak sesuatu untuk menandakan Task B aktif
        kprint("[B]", 0x0E);
        
        // Berikan giliran ke task berikutnya
        task_yield();
    }
}

// Handler perintah taskdemo di Shell
void cmd_taskdemo(void) {
    create_task("task_a", task_a_func);
    create_task("task_b", task_b_func);
    kprint("Task A & Task B berhasil dibuat!\n", 0x0A);
}

static void user_function(void) {
    sys_print("\n=== APLIKASI USER MODE INTERAKTIF ===\n");
    sys_print("Tekan sebarang tombol di keyboard Anda: ");
    char key = sys_getkey();
    sys_print("\n[User Space] Anda menekan tombol: ");
    sys_putchar(key);
    sys_print("\n=====================================\n");
    sys_exit();
    while(1);
}

static void parse_command(const char *input, char *cmd, char *arg) {
    int i = 0, j = 0;
    while (input[i] != ' ' && input[i] != '\0') { cmd[i] = input[i]; i++; }
    cmd[i] = '\0';
    if (input[i] == ' ') {
        i++;
        while (input[i] != '\0') { arg[j++] = input[i++]; }
    }
    arg[j] = '\0';
}

static int string_compare(const char *str1, const char *str2) {
    int i = 0;
    while (str1[i] != '\0' && str2[i] != '\0') {
        if (str1[i] != str2[i]) return 0;
        i++;
    }
    return (str1[i] == '\0' && str2[i] == '\0');
}

void handle_history_up(void) {
    if (history_count == 0) return;
    if (history_index == -1) history_index = history_count - 1;
    else if (history_index > 0) history_index--;

    while (command_index > 0) { kprint_char('\b', 0x0F); command_index--; }
    for (int i = 0; history[history_index][i] != '\0'; i++) {
        command_buffer[i] = history[history_index][i];
        kprint_char(command_buffer[i], 0x0F);
        command_index++;
    }
    command_buffer[command_index] = '\0';
}

void handle_history_down(void) {
    if (history_index == -1) return;
    history_index++;
    while (command_index > 0) { kprint_char('\b', 0x0F); command_index--; }

    if (history_index < history_count) {
        for (int i = 0; history[history_index][i] != '\0'; i++) {
            command_buffer[i] = history[history_index][i];
            kprint_char(command_buffer[i], 0x0F);
            command_index++;
        }
        command_buffer[command_index] = '\0';
    } else {
        history_index = -1;
    }
}

void process_command(void) {
    kprint("\n", 0x07);
    command_buffer[command_index] = '\0';

    if (command_index == 0) { kprint("> ", 0x0F); return; }

    // Save to History
    if (history_count < HISTORY_MAX) {
        for (int i = 0; i <= command_index; i++) history[history_count][i] = command_buffer[i];
        history_count++;
    } else {
        for (int i = 0; i < HISTORY_MAX - 1; i++) {
            for (int j = 0; j < 256; j++) history[i][j] = history[i+1][j];
        }
        for (int i = 0; i <= command_index; i++) history[HISTORY_MAX - 1][i] = command_buffer[i];
    }
    history_index = -1;

    char cmd[32], arg[224];
    parse_command(command_buffer, cmd, arg);

    if (string_compare(command_buffer, "help")) {
        kprint("Perintah Lenovix OS yang tersedia:\n", 0x0E);
        kprint("  ls       - Menampilkan daftar berkas/file\n", 0x0F);
        kprint("  cat      - Membaca isi berkas\n", 0x0F);
        kprint("  touch    - Membuat berkas baru\n", 0x0F);
        kprint("  write    - Menulis ke berkas\n", 0x0F);
        kprint("  free     - Menampilkan informasi memori\n", 0x0F);
        kprint("  uptime   - Menampilkan durasi aktif OS\n", 0x0F);
        kprint("  clear    - Membersihkan layar shell\n", 0x0F);
        kprint("  alloc    - Memori dialokasikan\n", 0x0F);
        kprint("  dealloc  - Memori didealokasikan\n", 0x0F);
        kprint("  taskdemo - Menjalankan demo multitasking\n", 0x0F);
        kprint("  vesademo - Menjalankan demo VESA\n", 0x0F);
        kprint("  usermode - Menjalankan aplikasi Ring 3\n", 0x0F);
        kprint("  about    - Informasi OS\n", 0x0F);
        kprint("  shutdown - Mematikan sistem\n", 0x0F);
    }
    else if (string_compare(cmd, "vesademo")) {
        // Bersihkan layar dengan warna biru tua (0x000033)
        vesa_clear_screen(0x001F3F);

        // Gambar Jendela GUI Sederhana
        vesa_draw_rect(200, 150, 400, 300, 0xFFFFFF); // Background Putih
        vesa_draw_rect(200, 150, 400, 30, 0x0074D9);  // Header Biru
        vesa_draw_rect(560, 155, 30, 20, 0xFF4136);   // Tombol Close Merah
    }
    else if (string_compare(cmd, "usermode")) {
        kprint("Menjalankan simulasi User Mode (Ring 3)...\n", 0x0E);
        create_task("User_App", user_function);
        task_yield();
    }
    else if (string_compare(cmd, "taskdemo")) {
        create_task("Task_A_Worker", task_a_func);
        create_task("Task_B_Worker", task_b_func);
        kprint("Task A & Task B berhasil dibuat!\n", 0x0A);
    }
    else if (string_compare(cmd, "ls")) {
        vfs_list_files();
    }
    else if (string_compare(cmd, "cat")) {
        if (arg[0] == '\0') {
            kprint("Gunakan: cat <nama_file>\n", 0x0C);
        } else {
            vfs_read_file_cmd(arg);
        }
    }
    else if (string_compare(cmd, "touch")) {
        if (arg[0] == '\0') {
            kprint("Gunakan: touch <nama_file>\n", 0x0C);
        } else {
            vfs_create_file_cmd(arg);
        }
    }
    else if (string_compare(cmd, "write")) {
        if (arg[0] == '\0') {
            kprint("Gunakan: write <nama_file> <teks_baru>\n", 0x0C);
        } else {
            char filename[32];
            char content[192];
            parse_command(arg, filename, content); // Memisah nama file dan isi teksnya

            if (content[0] == '\0') {
                kprint("Gunakan: write <nama_file> <teks_baru>\n", 0x0C);
            } else {
                vfs_write_file_cmd(filename, content);
            }
        }
    }
    else if (string_compare(cmd, "clear")) { clear_screen(); }
    else if (string_compare(cmd, "about")) {
        kprint("Lenovix OS v0.1\n", 0x0A);
        kprint("Sebuah proyek sistem operasi buatan anak bangsa.\n", 0x0B);
    }
    else if (string_compare(cmd, "shutdown")) {
        kprint("Mematikan Lenovix OS...", 0x0C);
        for(volatile int i = 0; i < 50000000; i++);
        shutdown_qemu();
    }
    else if (string_compare(cmd, "uptime")) {
        unsigned int total_seconds = timer_ticks / 100;
        char sec_str[16];
        int_to_string(total_seconds, sec_str);
        kprint("Lenovix OS telah aktif selama: ", 0x0F);
        kprint(sec_str, 0x0A);
        kprint(" detik.\n", 0x0F);
    }
    else if (string_compare(cmd, "free")) {
        unsigned int max_blocks = pmm_get_max_blocks();
        unsigned int free_blocks = pmm_get_free_block_count();
        unsigned int total_mb = (max_blocks * 4) / 1024;
        unsigned int free_mb = (free_blocks * 4) / 1024;
        unsigned int used_mb = ((max_blocks - free_blocks) * 4) / 1024;

        char str_t[16], str_f[16], str_u[16];
        int_to_string(total_mb, str_t); int_to_string(free_mb, str_f); int_to_string(used_mb, str_u);

        kprint("Informasi Memori Fisik RAM:\n", 0x0E);
        kprint("  Total RAM: ", 0x0F); kprint(str_t, 0x0A); kprint(" MB\n", 0x0F);
        kprint("  Terpakai : ", 0x0F); kprint(str_u, 0x0C); kprint(" MB\n", 0x0F);
        kprint("  Bebas    : ", 0x0F); kprint(str_f, 0x0A); kprint(" MB\n", 0x0F);
    }
    else if (string_compare(cmd, "alloc")) {
        if (test_ptr != NULL) {
            kprint("Memori sudah dialokasikan! Ketik 'dealloc' dulu.\n", 0x0C);
        } else {
            test_ptr = kmalloc(64);
            if (test_ptr != NULL) {
                char *str = (char *)test_ptr;
                char sample[] = "Lenovix OS Dynamic Heap Works!";
                for(int i = 0; sample[i] != '\0'; i++) str[i] = sample[i];
                str[30] = '\0';
                kprint("Berhasil alokasi 64 byte! Isi: \"", 0x0A);
                kprint((char*)test_ptr, 0x0B);
                kprint("\"\n", 0x0F);
            }
        }
    }
    else if (string_compare(cmd, "dealloc")) {
        if (test_ptr != NULL) { kfree(test_ptr); test_ptr = NULL; kprint("Memori dibebaskan!\n", 0x0A); }
        else kprint("Belum ada memori dialokasikan!\n", 0x0C);
    }
    else {
        kprint("Perintah '", 0x0C); kprint(command_buffer, 0x0C);
        kprint("' tidak ditemukan. Ketik 'help'.\n", 0x0C);
    }

    kprint("> ", 0x0F);
    command_index = 0;
}