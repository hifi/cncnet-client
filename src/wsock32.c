/*
 * Copyright (c) 2010, 2011 Toni Spets <toni.spets@iki.fi>
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

#include "sockets.h"
#include <windows.h>
#include <wsipx.h>
#include <assert.h>
#include <stdio.h>

struct sockaddr_in my_addr;
struct sockaddr_in my_bcast;
unsigned short my_port = 8055;

void ipx2in(struct sockaddr_ipx *from, struct sockaddr_in *to)
{
    to->sin_family = AF_INET;
    memcpy(&to->sin_addr.s_addr, from->sa_nodenum, 4);
    memcpy(&to->sin_port, from->sa_nodenum + 4, 2);
}

void in2ipx(struct sockaddr_in *from, struct sockaddr_ipx *to)
{
    to->sa_family = AF_IPX;
    *(DWORD *)&to->sa_netnum = 1;
    memcpy(to->sa_nodenum, &from->sin_addr.s_addr, 4);
    memcpy(to->sa_nodenum + 4, &from->sin_port, 2);
    to->sa_socket = from->sin_port;
}

int is_ipx_broadcast(struct sockaddr_ipx *addr)
{
    unsigned char ff[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
    if (memcmp(addr->sa_netnum, ff, 4) == 0 || memcmp(addr->sa_nodenum, ff, 6) == 0)
        return TRUE;
    else
        return FALSE;
}

SOCKET WINAPI fake_socket(int af, int type, int protocol)
{
    printf("socket(af=%08X, type=%08X, protocol=%08X)\n", af, type, protocol);

    if (af == AF_IPX)
    {
        SOCKET s = net_socket();
        net_address_ex(&my_addr, INADDR_ANY, my_port);
        net_address_ex(&my_bcast, INADDR_BROADCAST, my_port);
        net_broadcast(s);
        bind(s, (const struct sockaddr *)&my_addr, sizeof(struct sockaddr_in));
        return s;
    }

    return socket(af, type, protocol);
}

int WINAPI fake_bind(SOCKET s, const struct sockaddr *name, int namelen)
{
    printf("bind(s=%d, name=%p, namelen=%d)\n", s, name, namelen);

    if (((struct sockaddr_ipx *)name)->sa_family == AF_IPX)
    {
        return 0;
    }

    return bind(s, name, namelen);
}

int WINAPI fake_recvfrom(SOCKET s, char *buf, int len, int flags, struct sockaddr *from, int *fromlen)
{
    struct sockaddr_in from_in;

    int ret = net_recv(s, buf, len, &from_in);

    if(ret > 0)
    {
        in2ipx(&from_in, (struct sockaddr_ipx *)from);
    }

#ifdef _DEBUG
    printf("recvfrom(s=%d, buf=%p, len=%d, flags=%08X, from=%p, fromlen=%p (%d) -> %d (err: %d)\n", s, buf, len, flags, from, fromlen, *fromlen, ret, WSAGetLastError());
#endif

    return ret;
}

int WINAPI fake_sendto(SOCKET s, const char *buf, int len, int flags, const struct sockaddr *to, int tolen)
{
#ifdef _DEBUG
    printf("sendto(s=%d, buf=%p, len=%d, flags=%08X, to=%p, tolen=%d\n", s, buf, len, flags, to, tolen);
#endif

    if (to->sa_family == AF_IPX)
    {
        struct sockaddr_in to_in;

        ipx2in((struct sockaddr_ipx *)to, &to_in);

        /* check if it's a broadcast */
        if (is_ipx_broadcast((struct sockaddr_ipx *)to))
        {
            net_send(s, buf, len, &my_bcast);
            return len;
        }

        net_send(s, buf, len, &to_in);
        return len;
    }

    return sendto(s, buf, len, flags, to, tolen);
}

int WINAPI fake_getsockopt(SOCKET s, int level, int optname, char *optval, int *optlen)
{
    if (level == 0x3E8)
    {
        *optval = 1;
        *optlen = 1;
        return 0;
    }

    if (level == 0xFFFF)
    {
        *optval = 1;
        *optlen = 1;
        return 0;
    }

    return getsockopt(s, level, optname, optval, optlen);
}

int WINAPI fake_setsockopt(SOCKET s, int level, int optname, const char *optval, int optlen)
{
    printf("setsockopt(s=%d, level=%08X, optname=%08X, optval=%p, optlen=%d)\n", s, level, optname, optval, optlen);

    if (level == 0x3E8)
    {
        return 0;
    }
    if (level == 0xFFFF)
    {
        return 0;
    }

    return setsockopt(s, level, optname, optval, optlen);
}

int WINAPI fake_getsockname(SOCKET s, struct sockaddr *name, int *namelen)
{
    struct sockaddr_in name_in;
    int name_in_len = sizeof(struct sockaddr_in);

    printf("getsockname(s=%d, name=%p, namelen=%p (%d)\n", s, name, namelen, *namelen);

    int ret = getsockname(s, (struct sockaddr *)&name_in, &name_in_len);

    if (ret == 0)
    {
#if 0
        /* this doesn't work, we have binded to 0.0.0.0 */
        printf("getsockname: local ip: %s\n", inet_ntoa(name_in.sin_addr));
        in2ipx(&name_in, (struct sockaddr_ipx *)name);
#else
        char hostname[256];
        struct hostent *he;

        gethostname(hostname, 256);
        he = gethostbyname(hostname);

        printf("getsockname: local hostname: %s\n", hostname);

        if (he)
        {
            printf("getsockname: local ip: %s\n", inet_ntoa(*(struct in_addr *)(he->h_addr_list[0])));
            name_in.sin_addr = *(struct in_addr *)(he->h_addr_list[0]);
            in2ipx(&name_in, (struct sockaddr_ipx *)name);
        }
#endif
    }
    return ret;
}
