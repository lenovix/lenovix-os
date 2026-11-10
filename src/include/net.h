#ifndef NET_H
#define NET_H

#define ETH_TYPE_ARP 0x0806
#define ETH_TYPE_IPV4 0x0800
#define IP_PROTO_ICMP 1
#define IP_PROTO_UDP 17

typedef struct
{
    unsigned char dest_mac[6];
    unsigned char src_mac[6];
    unsigned short ethertype;
} __attribute__((packed)) eth_header_t;

typedef struct
{
    unsigned short hw_type;
    unsigned short proto_type;
    unsigned char hw_len;
    unsigned char proto_len;
    unsigned short opcode;
    unsigned char sender_mac[6];
    unsigned char sender_ip[4];
    unsigned char target_mac[6];
    unsigned char target_ip[4];
} __attribute__((packed)) arp_packet_t;

typedef struct
{
    unsigned char version_ihl;
    unsigned char tos;
    unsigned short total_length;
    unsigned short id;
    unsigned short flags_fragment;
    unsigned char ttl;
    unsigned char protocol;
    unsigned short checksum;
    unsigned char src_ip[4];
    unsigned char dest_ip[4];
} __attribute__((packed)) ipv4_header_t;

typedef struct
{
    unsigned char type;
    unsigned char code;
    unsigned short checksum;
    unsigned short id;
    unsigned short sequence;
} __attribute__((packed)) icmp_header_t;

typedef struct
{
    unsigned short src_port;
    unsigned short dest_port;
    unsigned short length;
    unsigned short checksum;
} __attribute__((packed)) udp_header_t;

typedef struct
{
    unsigned short id;
    unsigned short flags;
    unsigned short qdcount;
    unsigned short ancount;
    unsigned short nscount;
    unsigned short arcount;
} __attribute__((packed)) dns_header_t;

void net_init(void);
void net_poll(void);
int net_send_arp(const unsigned char target_ip[4]);
int net_send_ping(const unsigned char target_ip[4]);
int net_has_ping_reply(void);
void net_clear_ping_reply(void);

int net_send_udp(const unsigned char target_ip[4], unsigned short src_port, unsigned short dest_port, const void *payload, unsigned short payload_len);
int net_send_dns_query(const char *domain);
int net_has_dns_reply(void);
void net_clear_dns_reply(void);
void net_get_dns_result(unsigned char out_ip[4]);

const unsigned char *net_get_my_ip(void);
const unsigned char *net_get_gateway_ip(void);
const unsigned char *net_get_dns_server(void);

#endif
