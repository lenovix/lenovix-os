#include "tcp.h"
#include "net.h"

static unsigned char conn_target_ip[4];
static unsigned short conn_target_port = 0;
static unsigned short conn_local_port = 49152;
static unsigned int local_seq = 0x10000;
static unsigned int remote_seq = 0;
static tcp_state_t conn_state = TCP_STATE_CLOSED;

static char rx_buf[2048];
static unsigned int rx_len = 0;

static unsigned short swap16(unsigned short val)
{
    return (val << 8) | (val >> 8);
}

static unsigned int swap32(unsigned int val)
{
    return ((val >> 24) & 0xFF) |
           ((val >> 8) & 0xFF00) |
           ((val << 8) & 0xFF0000) |
           ((val << 24) & 0xFF000000);
}

static unsigned short tcp_checksum(const unsigned char src_ip[4], const unsigned char dest_ip[4], const void *tcp_seg, unsigned short seg_len)
{
    unsigned int sum = 0;

    sum += (src_ip[0] << 8) | src_ip[1];
    sum += (src_ip[2] << 8) | src_ip[3];
    sum += (dest_ip[0] << 8) | dest_ip[1];
    sum += (dest_ip[2] << 8) | dest_ip[3];
    sum += 6;
    sum += seg_len;

    const unsigned short *ptr = (const unsigned short *)tcp_seg;
    int len = seg_len;
    while (len > 1)
    {
        sum += *ptr++;
        len -= 2;
    }
    if (len > 0)
    {
        sum += (*(const unsigned char *)ptr) << 8;
    }
    while (sum >> 16)
    {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return (unsigned short)(~sum);
}

static int tcp_send_segment(unsigned char flags, const void *payload, unsigned short payload_len)
{
    unsigned char seg[1500];
    tcp_header_t *tcp = (tcp_header_t *)seg;

    tcp->src_port = swap16(conn_local_port);
    tcp->dest_port = swap16(conn_target_port);
    tcp->seq_num = swap32(local_seq);
    tcp->ack_num = swap32(remote_seq);
    tcp->data_offset = (sizeof(tcp_header_t) / 4) << 4;
    tcp->flags = flags;
    tcp->window_size = swap16(8192);
    tcp->checksum = 0;
    tcp->urgent_ptr = 0;

    if (payload && payload_len > 0)
    {
        const unsigned char *src = (const unsigned char *)payload;
        for (unsigned int i = 0; i < payload_len; i++)
        {
            seg[sizeof(tcp_header_t) + i] = src[i];
        }
    }

    unsigned short seg_len = sizeof(tcp_header_t) + payload_len;
    tcp->checksum = tcp_checksum(net_get_my_ip(), conn_target_ip, seg, seg_len);

    return net_send_ipv4(conn_target_ip, IP_PROTO_TCP, seg, seg_len);
}

void tcp_init(void)
{
    conn_state = TCP_STATE_CLOSED;
    rx_len = 0;
}

int tcp_is_connected(void)
{
    return (conn_state == TCP_STATE_ESTABLISHED);
}

int tcp_connect(const unsigned char target_ip[4], unsigned short target_port)
{
    for (int i = 0; i < 4; i++)
        conn_target_ip[i] = target_ip[i];
    conn_target_port = target_port;
    conn_local_port++;
    if (conn_local_port < 49152)
        conn_local_port = 49152;

    local_seq = 0x20000;
    remote_seq = 0;
    rx_len = 0;
    conn_state = TCP_STATE_SYN_SENT;

    tcp_send_segment(TCP_FLAG_SYN, 0, 0);
    local_seq++;

    for (int i = 0; i < 30000; i++)
    {
        net_poll();
        if (conn_state == TCP_STATE_ESTABLISHED)
        {
            return 0;
        }
    }

    conn_state = TCP_STATE_CLOSED;
    return -1;
}

int tcp_send(const void *data, unsigned int len)
{
    if (conn_state != TCP_STATE_ESTABLISHED || !data || len == 0)
        return -1;

    int res = tcp_send_segment(TCP_FLAG_ACK | TCP_FLAG_PSH, data, len);
    if (res == 0)
    {
        local_seq += len;
    }
    return res;
}

int tcp_close(void)
{
    if (conn_state == TCP_STATE_ESTABLISHED)
    {
        tcp_send_segment(TCP_FLAG_ACK | TCP_FLAG_FIN, 0, 0);
        local_seq++;
        conn_state = TCP_STATE_FIN_WAIT;
    }
    else
    {
        conn_state = TCP_STATE_CLOSED;
    }
    return 0;
}

int tcp_get_rx_data(char *out_buf, unsigned int max_len)
{
    if (!out_buf || max_len == 0)
        return 0;
    unsigned int copy_len = rx_len;
    if (copy_len >= max_len)
        copy_len = max_len - 1;

    for (unsigned int i = 0; i < copy_len; i++)
    {
        out_buf[i] = rx_buf[i];
    }
    out_buf[copy_len] = '\0';
    rx_len = 0;
    return copy_len;
}

void tcp_handle_packet(const unsigned char *ip_data, unsigned int ip_len)
{
    if (ip_len < 20)
        return;
    const ipv4_header_t *ip = (const ipv4_header_t *)ip_data;
    unsigned int ip_hdr_len = (ip->version_ihl & 0x0F) * 4;
    if (ip_len < ip_hdr_len + 20)
        return;

    const tcp_header_t *tcp = (const tcp_header_t *)(ip_data + ip_hdr_len);
    unsigned int tcp_hdr_len = ((tcp->data_offset >> 4) & 0x0F) * 4;
    if (ip_len < ip_hdr_len + tcp_hdr_len)
        return;

    unsigned short dport = swap16(tcp->dest_port);
    unsigned short sport = swap16(tcp->src_port);
    if (dport != conn_local_port || sport != conn_target_port)
        return;

    unsigned int in_seq = swap32(tcp->seq_num);
    unsigned int in_ack = swap32(tcp->ack_num);
    unsigned char flags = tcp->flags;
    unsigned int payload_len = ip_len - ip_hdr_len - tcp_hdr_len;
    const unsigned char *payload = ip_data + ip_hdr_len + tcp_hdr_len;

    if (conn_state == TCP_STATE_SYN_SENT)
    {
        if ((flags & (TCP_FLAG_SYN | TCP_FLAG_ACK)) == (TCP_FLAG_SYN | TCP_FLAG_ACK))
        {
            remote_seq = in_seq + 1;
            local_seq = in_ack;
            conn_state = TCP_STATE_ESTABLISHED;
            tcp_send_segment(TCP_FLAG_ACK, 0, 0);
        }
    }
    else if (conn_state == TCP_STATE_ESTABLISHED || conn_state == TCP_STATE_FIN_WAIT)
    {
        if (payload_len > 0)
        {
            for (unsigned int i = 0; i < payload_len && rx_len < sizeof(rx_buf) - 1; i++)
            {
                rx_buf[rx_len++] = payload[i];
            }
            rx_buf[rx_len] = '\0';
            remote_seq = in_seq + payload_len;
            tcp_send_segment(TCP_FLAG_ACK, 0, 0);
        }

        if (flags & TCP_FLAG_FIN)
        {
            remote_seq = in_seq + 1;
            tcp_send_segment(TCP_FLAG_ACK, 0, 0);
            conn_state = TCP_STATE_CLOSED;
        }
    }
}
