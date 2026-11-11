#include "net.h"
#include "rtl8139.h"

static unsigned char my_ip[4] = {10, 0, 2, 15};
static unsigned char gateway_ip[4] = {10, 0, 2, 2};
static unsigned char dns_server_ip[4] = {10, 0, 2, 3};
static unsigned char gateway_mac[6] = {0x52, 0x54, 0x00, 0x12, 0x34, 0x56};
static int has_gateway_mac = 0;
static int ping_reply_received = 0;
static unsigned short ping_reply_seq = 0;
static unsigned char ping_reply_ttl = 64;

static unsigned char dns_result_ip[4] = {0};
static int dns_reply_received = 0;

static unsigned short swap16(unsigned short val)
{
    return (val << 8) | (val >> 8);
}

static unsigned short checksum(const void *buf, int len)
{
    unsigned int sum = 0;
    const unsigned short *ptr = (const unsigned short *)buf;
    while (len > 1)
    {
        sum += *ptr++;
        len -= 2;
    }
    if (len > 0)
    {
        sum += *(const unsigned char *)ptr;
    }
    while (sum >> 16)
    {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }
    return (unsigned short)(~sum);
}

void net_init(void)
{
    ping_reply_received = 0;
    dns_reply_received = 0;
}

const unsigned char *net_get_my_ip(void)
{
    return my_ip;
}

const unsigned char *net_get_gateway_ip(void)
{
    return gateway_ip;
}

const unsigned char *net_get_dns_server(void)
{
    return dns_server_ip;
}

int net_has_ping_reply(void)
{
    return ping_reply_received;
}

void net_clear_ping_reply(void)
{
    ping_reply_received = 0;
}

int net_has_dns_reply(void)
{
    return dns_reply_received;
}

void net_clear_dns_reply(void)
{
    dns_reply_received = 0;
}

void net_get_dns_result(unsigned char out_ip[4])
{
    for (int i = 0; i < 4; i++)
    {
        out_ip[i] = dns_result_ip[i];
    }
}

int net_send_arp(const unsigned char target_ip[4])
{
    unsigned char frame[sizeof(eth_header_t) + sizeof(arp_packet_t)];
    eth_header_t *eth = (eth_header_t *)frame;
    arp_packet_t *arp = (arp_packet_t *)(frame + sizeof(eth_header_t));

    unsigned char my_mac[6];
    rtl8139_get_mac(my_mac);

    for (int i = 0; i < 6; i++)
    {
        eth->dest_mac[i] = 0xFF;
        eth->src_mac[i] = my_mac[i];
    }
    eth->ethertype = swap16(ETH_TYPE_ARP);

    arp->hw_type = swap16(1);
    arp->proto_type = swap16(ETH_TYPE_IPV4);
    arp->hw_len = 6;
    arp->proto_len = 4;
    arp->opcode = swap16(1);

    for (int i = 0; i < 6; i++)
    {
        arp->sender_mac[i] = my_mac[i];
        arp->target_mac[i] = 0x00;
    }
    for (int i = 0; i < 4; i++)
    {
        arp->sender_ip[i] = my_ip[i];
        arp->target_ip[i] = target_ip[i];
    }

    return rtl8139_send_packet(frame, sizeof(frame));
}

unsigned short net_get_ping_reply_seq(void)
{
    return ping_reply_seq;
}

unsigned char net_get_ping_reply_ttl(void)
{
    return ping_reply_ttl;
}

int net_send_ping_seq(const unsigned char target_ip[4], unsigned short seq)
{
    unsigned int total_len = sizeof(eth_header_t) + sizeof(ipv4_header_t) + sizeof(icmp_header_t) + 32;
    unsigned char frame[128];

    eth_header_t *eth = (eth_header_t *)frame;
    ipv4_header_t *ip = (ipv4_header_t *)(frame + sizeof(eth_header_t));
    icmp_header_t *icmp = (icmp_header_t *)(frame + sizeof(eth_header_t) + sizeof(ipv4_header_t));
    unsigned char *payload = frame + sizeof(eth_header_t) + sizeof(ipv4_header_t) + sizeof(icmp_header_t);

    unsigned char my_mac[6];
    rtl8139_get_mac(my_mac);

    for (int i = 0; i < 6; i++)
    {
        eth->dest_mac[i] = has_gateway_mac ? gateway_mac[i] : 0xFF;
        eth->src_mac[i] = my_mac[i];
    }
    eth->ethertype = swap16(ETH_TYPE_IPV4);

    ip->version_ihl = 0x45;
    ip->tos = 0;
    ip->total_length = swap16(sizeof(ipv4_header_t) + sizeof(icmp_header_t) + 32);
    static unsigned short ping_ip_id = 0x1337;
    ip->id = swap16(ping_ip_id++);
    ip->flags_fragment = swap16(0x4000);
    ip->ttl = 64;
    ip->protocol = IP_PROTO_ICMP;
    ip->checksum = 0;
    for (int i = 0; i < 4; i++)
    {
        ip->src_ip[i] = my_ip[i];
        ip->dest_ip[i] = target_ip[i];
    }
    ip->checksum = checksum(ip, sizeof(ipv4_header_t));

    icmp->type = 8;
    icmp->code = 0;
    icmp->checksum = 0;
    icmp->id = swap16(0xBEEF);
    icmp->sequence = swap16(seq);

    for (int i = 0; i < 32; i++)
    {
        payload[i] = 'A' + (i % 26);
    }
    icmp->checksum = checksum(icmp, sizeof(icmp_header_t) + 32);

    ping_reply_received = 0;
    return rtl8139_send_packet(frame, total_len);
}

int net_send_ping(const unsigned char target_ip[4])
{
    return net_send_ping_seq(target_ip, 1);
}

extern void tcp_handle_packet(const unsigned char *ip_data, unsigned int ip_len);

int net_send_ipv4(const unsigned char target_ip[4], unsigned char protocol, const void *payload, unsigned short payload_len)
{
    unsigned int total_len = sizeof(eth_header_t) + sizeof(ipv4_header_t) + payload_len;
    if (total_len > 1514)
        return -1;

    unsigned char frame[1514];
    eth_header_t *eth = (eth_header_t *)frame;
    ipv4_header_t *ip = (ipv4_header_t *)(frame + sizeof(eth_header_t));
    unsigned char *data = frame + sizeof(eth_header_t) + sizeof(ipv4_header_t);

    unsigned char my_mac[6];
    rtl8139_get_mac(my_mac);

    for (int i = 0; i < 6; i++)
    {
        eth->dest_mac[i] = has_gateway_mac ? gateway_mac[i] : 0xFF;
        eth->src_mac[i] = my_mac[i];
    }
    eth->ethertype = swap16(ETH_TYPE_IPV4);

    ip->version_ihl = 0x45;
    ip->tos = 0;
    ip->total_length = swap16(sizeof(ipv4_header_t) + payload_len);
    static unsigned short ip_id_counter = 0x3000;
    ip->id = swap16(ip_id_counter++);
    ip->flags_fragment = swap16(0x4000);
    ip->ttl = 64;
    ip->protocol = protocol;
    ip->checksum = 0;
    for (int i = 0; i < 4; i++)
    {
        ip->src_ip[i] = my_ip[i];
        ip->dest_ip[i] = target_ip[i];
    }
    ip->checksum = checksum(ip, sizeof(ipv4_header_t));

    const unsigned char *src_data = (const unsigned char *)payload;
    for (unsigned int i = 0; i < payload_len; i++)
    {
        data[i] = src_data[i];
    }

    return rtl8139_send_packet(frame, total_len);
}

int net_send_udp(const unsigned char target_ip[4], unsigned short src_port, unsigned short dest_port, const void *payload, unsigned short payload_len)
{
    unsigned int total_len = sizeof(eth_header_t) + sizeof(ipv4_header_t) + sizeof(udp_header_t) + payload_len;
    if (total_len > 1514)
        return -1;

    unsigned char frame[1514];
    eth_header_t *eth = (eth_header_t *)frame;
    ipv4_header_t *ip = (ipv4_header_t *)(frame + sizeof(eth_header_t));
    udp_header_t *udp = (udp_header_t *)(frame + sizeof(eth_header_t) + sizeof(ipv4_header_t));
    unsigned char *data = frame + sizeof(eth_header_t) + sizeof(ipv4_header_t) + sizeof(udp_header_t);

    unsigned char my_mac[6];
    rtl8139_get_mac(my_mac);

    for (int i = 0; i < 6; i++)
    {
        eth->dest_mac[i] = has_gateway_mac ? gateway_mac[i] : 0xFF;
        eth->src_mac[i] = my_mac[i];
    }
    eth->ethertype = swap16(ETH_TYPE_IPV4);

    ip->version_ihl = 0x45;
    ip->tos = 0;
    ip->total_length = swap16(sizeof(ipv4_header_t) + sizeof(udp_header_t) + payload_len);
    ip->id = swap16(0x2468);
    ip->flags_fragment = swap16(0x4000);
    ip->ttl = 64;
    ip->protocol = IP_PROTO_UDP;
    ip->checksum = 0;
    for (int i = 0; i < 4; i++)
    {
        ip->src_ip[i] = my_ip[i];
        ip->dest_ip[i] = target_ip[i];
    }
    ip->checksum = checksum(ip, sizeof(ipv4_header_t));

    udp->src_port = swap16(src_port);
    udp->dest_port = swap16(dest_port);
    udp->length = swap16(sizeof(udp_header_t) + payload_len);
    udp->checksum = 0;

    const unsigned char *src_data = (const unsigned char *)payload;
    for (unsigned int i = 0; i < payload_len; i++)
    {
        data[i] = src_data[i];
    }

    return rtl8139_send_packet(frame, total_len);
}

int net_send_dns_query(const char *domain)
{
    unsigned char dns_payload[256];
    dns_header_t *dns = (dns_header_t *)dns_payload;

    dns->id = swap16(0xABCD);
    dns->flags = swap16(0x0100);
    dns->qdcount = swap16(1);
    dns->ancount = 0;
    dns->nscount = 0;
    dns->arcount = 0;

    unsigned char *qname = dns_payload + sizeof(dns_header_t);
    int qi = 0;
    int dot_pos = 0;

    int i = 0;
    while (domain[i])
    {
        if (domain[i] == '.')
        {
            qname[dot_pos] = qi - dot_pos - 1;
            dot_pos = qi;
        }
        else
        {
            qname[qi] = domain[i];
        }
        qi++;
        i++;
    }
    qname[dot_pos] = qi - dot_pos - 1;
    qname[qi++] = 0;

    qname[qi++] = 0x00;
    qname[qi++] = 0x01;
    qname[qi++] = 0x00;
    qname[qi++] = 0x01;

    dns_reply_received = 0;
    return net_send_udp(dns_server_ip, 5353, 53, dns_payload, sizeof(dns_header_t) + qi);
}

static void handle_dns(const unsigned char *data, unsigned int len)
{
    if (len < sizeof(dns_header_t))
        return;
    const dns_header_t *dns = (const dns_header_t *)data;

    unsigned short answers = swap16(dns->ancount);
    if (answers == 0)
        return;

    unsigned int offset = sizeof(dns_header_t);
    while (offset < len && data[offset] != 0)
    {
        offset += data[offset] + 1;
    }
    offset += 5;

    for (int a = 0; a < answers && offset < len; a++)
    {
        if ((data[offset] & 0xC0) == 0xC0)
        {
            offset += 2;
        }
        else
        {
            while (offset < len && data[offset] != 0)
                offset += data[offset] + 1;
            offset++;
        }

        if (offset + 10 > len)
            break;

        unsigned short type = (data[offset] << 8) | data[offset + 1];
        unsigned short rdlength = (data[offset + 8] << 8) | data[offset + 9];
        offset += 10;

        if (type == 1 && rdlength == 4 && offset + 4 <= len)
        {
            for (int k = 0; k < 4; k++)
            {
                dns_result_ip[k] = data[offset + k];
            }
            dns_reply_received = 1;
            return;
        }
        offset += rdlength;
    }
}

static void handle_arp(const arp_packet_t *arp)
{
    if (swap16(arp->opcode) == 2)
    {
        for (int i = 0; i < 6; i++)
        {
            gateway_mac[i] = arp->sender_mac[i];
        }
        has_gateway_mac = 1;
    }
}

static void handle_ipv4(const unsigned char *data, unsigned int len)
{
    if (len < sizeof(ipv4_header_t))
        return;
    const ipv4_header_t *ip = (const ipv4_header_t *)data;
    unsigned int ip_hdr_len = (ip->version_ihl & 0x0F) * 4;

    if (ip->protocol == IP_PROTO_ICMP)
    {
        if (len < ip_hdr_len + sizeof(icmp_header_t))
            return;

        const icmp_header_t *icmp = (const icmp_header_t *)(data + ip_hdr_len);
        if (icmp->type == 0)
        {
            ping_reply_received = 1;
            ping_reply_seq = swap16(icmp->sequence);
            ping_reply_ttl = ip->ttl;
        }
    }
    else if (ip->protocol == IP_PROTO_UDP)
    {
        if (len < ip_hdr_len + sizeof(udp_header_t))
            return;

        const udp_header_t *udp = (const udp_header_t *)(data + ip_hdr_len);
        if (swap16(udp->src_port) == 53)
        {
            handle_dns(data + ip_hdr_len + sizeof(udp_header_t), len - ip_hdr_len - sizeof(udp_header_t));
        }
    }
    else if (ip->protocol == IP_PROTO_TCP)
    {
        tcp_handle_packet(data, len);
    }
}

void net_poll(void)
{
    static unsigned char rx_buf[1536];
    int len = rtl8139_receive_packet(rx_buf, sizeof(rx_buf));
    if (len < (int)sizeof(eth_header_t))
        return;

    const eth_header_t *eth = (const eth_header_t *)rx_buf;
    unsigned short type = swap16(eth->ethertype);

    if (type == ETH_TYPE_ARP && len >= (int)(sizeof(eth_header_t) + sizeof(arp_packet_t)))
    {
        handle_arp((const arp_packet_t *)(rx_buf + sizeof(eth_header_t)));
    }
    else if (type == ETH_TYPE_IPV4)
    {
        handle_ipv4(rx_buf + sizeof(eth_header_t), len - sizeof(eth_header_t));
    }
}
