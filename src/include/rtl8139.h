#ifndef RTL8139_H
#define RTL8139_H

int rtl8139_init(void);
int rtl8139_is_present(void);
void rtl8139_get_mac(unsigned char *mac_out);
int rtl8139_send_packet(const void *data, unsigned int len);
int rtl8139_receive_packet(unsigned char *out_buf, unsigned int max_len);
void rtl8139_get_stats(unsigned int *tx_count, unsigned int *rx_count);

#endif
