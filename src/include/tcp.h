#ifndef TCP_H
#define TCP_H

#define TCP_FLAG_FIN 0x01
#define TCP_FLAG_SYN 0x02
#define TCP_FLAG_RST 0x04
#define TCP_FLAG_PSH 0x08
#define TCP_FLAG_ACK 0x10

typedef struct
{
    unsigned short src_port;
    unsigned short dest_port;
    unsigned int seq_num;
    unsigned int ack_num;
    unsigned char data_offset;
    unsigned char flags;
    unsigned short window_size;
    unsigned short checksum;
    unsigned short urgent_ptr;
} __attribute__((packed)) tcp_header_t;

typedef enum
{
    TCP_STATE_CLOSED,
    TCP_STATE_SYN_SENT,
    TCP_STATE_ESTABLISHED,
    TCP_STATE_FIN_WAIT
} tcp_state_t;

void tcp_init(void);
int tcp_connect(const unsigned char target_ip[4], unsigned short target_port);
int tcp_send(const void *data, unsigned int len);
int tcp_close(void);
int tcp_is_connected(void);
int tcp_get_rx_data(char *out_buf, unsigned int max_len);
void tcp_handle_packet(const unsigned char *ip_data, unsigned int ip_len);

#endif
