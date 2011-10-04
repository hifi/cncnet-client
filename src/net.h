/*
 * Copyright (c) 2011 Toni Spets <toni.spets@iki.fi>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <stdint.h>

#ifdef WIN32
    #include <winsock.h>
    #include <winsock2.h>
    #include <wsipx.h>

    void ipx2in(struct sockaddr_ipx *from, struct sockaddr_in *to);
    void in2ipx(struct sockaddr_in *from, struct sockaddr_ipx *to);
    uint8_t ipx2id(struct sockaddr_ipx *from);
    void id2ipx(uint8_t peer_id, struct sockaddr_ipx *to);
    int is_ipx_broadcast(struct sockaddr_ipx *addr);

#else
    #include <sys/types.h>
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <netdb.h>

    #define TRUE 1
    #define FALSE 0
#endif

#include <unistd.h>

#define NET_BUF_SIZE 2048

enum
{
    /* to dedicated from clients and control sources */
    /* 0x00 - 0xFD = clients */
    CMD_CONTROL     = 0xFE,
    CMD_BROADCAST   = 0xFF
};

enum
{
    /* to dedicated from anywhere */
    CTL_PING        = 0x00,
    CTL_QUERY       = 0x01,
    CTL_RESET       = 0x02,
    CTL_DISCONNECT  = 0x03
};

int net_reuse(uint16_t sock);
int net_address(struct sockaddr_in *addr, const char *host, uint16_t port);
void net_address_ex(struct sockaddr_in *addr, uint32_t ip, uint16_t port);

int net_init();
void net_free();

int net_bind(const char *ip, int port);

int8_t net_read_int8();
int16_t net_read_int16();
int32_t net_read_int32();
int64_t net_read_int64();
int net_read_data(void *, size_t);
int net_read_string(char *);

int net_write_int8(int8_t);
int net_write_int16(int16_t);
int net_write_int32(int32_t);
int net_write_int64(int64_t);
int net_write_data(void *, size_t);
int net_write_string(char *);

int net_recv(struct sockaddr_in *);
int net_send(struct sockaddr_in *);
int net_send_noflush(struct sockaddr_in *dst);
void net_send_discard();
void net_peer_add_by_host(const char *host, int16_t port);
int net_peer_add(struct sockaddr_in *peer);
struct sockaddr_in *net_peer_get(int i);
int net_peer_ok(struct sockaddr_in *peer);
int net_broadcast();

extern int net_socket;
extern int net_open;
