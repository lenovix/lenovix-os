BUG:
- [x] perintah 'diskr': Perintah tidak dikenal! (Selesai: fix parser command terminal untuk diskr dan diskw tanpa argumen)
- [x] perintah 'netinfo': RTL8139: NIC tidak terdeteksi. (Selesai: update Makefile QEMU -nic model=rtl8139 & diagnosa bus)
- [x] perintah 'ping/ping 8.8.8.8/ping 8.8.8.8 3': NIC tidak aktif untuk ping. (Selesai: loopback ping 127.0.0.1 aktif & fallback gateway MAC)
- [x] perintah 'ifconfig': eth0: Device not present. (Selesai: lo0 selalu aktif 127.0.0.1, status eth0 jelas dengan petunjuk QEMU)

IMPROVE:

UTILITY: 
    - Lenovix Lang (Bahasa Pemograman)
    - set / env (Linux: export / env | Windows: set): Mengelola Environment Variables sistem (variabel lingkungan) dan substitusi $VAR.
    - wc (Linux: wc | Windows PowerShell: Measure-Object): Menghitung jumlah baris (lines), kata (words), dan karakter/byte (bytes) pada berkas VFS.
    - head & tail (Linux: head/tail | Windows PowerShell: Get-Content -Head/-Tail): Menampilkan beberapa baris pertama (head) atau beberapa baris terakhir (tail) dari suatu berkas.
    - base64 (Linux: base64 | Windows: certutil -encode/-decode): Melakukan encoding teks/biner ke format Base64 dan decoding kembali ke teks asli.

SOFTWARE:
    - Lenovix IDE (seperti notepad tapi ada terminalnya atau seperti vscode)
    - Lenovix Word (seperti microsoft Word)
    - Lenovix Excel (seperti microsoft Excel)
    - Lenovix Power Point (seperti microsoft Power Point)
    - Retro Game Console (Pong, Tetris, Space Invaders)