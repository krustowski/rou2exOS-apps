#include "net.h"
#include "printf.h"
#include "string.h"
#include "syscall.h"

/*
 *  chat
 *
 *  TCP chat over SLIP/serial for rou2exOS.
 *  Server mode: multi-client chat room with per-client nicknames.
 *    - On connect, client is prompted for a nickname before entering.
 *    - Messages are broadcast to all room members with "[nick] " prefix.
 *    - The local r2 user is identified as "[server]".
 *  Client mode: connect to PEER_IP:CHAT_PORT (sends TCP SYN).
 *
 *  Usage:
 *    r2 server:  chat s [eth|slip]      -> room; nc 10.3.3.2 9000 to join
 *    r2 client:  chat c [peer_ip]       -> connects to room or single peer
 *    host:       nc -l -p 9000          -> single-peer server for client mode
 *
 *  krusty@vxn.dev / Apr 20, 2026
 */

#define CHAT_PORT   9000
#define LINE_CAP    128
#define PIPE_CAP    17
#define NICK_CAP    16
#define MAX_CLIENTS (MAX_SOCKETS - 1) /* one slot reserved for the listener */

/* Parse "A.B.C.D" into ip[4]; returns 1 on success. */
static uint8_t parse_ip_str(const uint8_t *s, uint8_t ip[4]) {
    for (uint8_t oct = 0; oct < 4; oct++) {
        uint16_t v = 0;
        if (*s < '0' || *s > '9') return 0;
        while (*s >= '0' && *s <= '9') v = (uint16_t)(v * 10 + *s++ - '0');
        ip[oct] = (uint8_t)v;
        if (oct < 3) { if (*s++ != '.') return 0; }
    }
    return 1;
}

/* PS/2 Set 1 make-code -> ASCII, unshifted. */
static const uint8_t sc_normal[0x3a] = {
    0,    0,   '1', '2',  '3', '4', '5', '6', /* 00-07 */
    '7',  '8', '9', '0',  '-', '=', 0,   0,   /* 08-0f */
    'q',  'w', 'e', 'r',  't', 'y', 'u', 'i', /* 10-17 */
    'o',  'p', '[', ']',  0,   0,   'a', 's', /* 18-1f */
    'd',  'f', 'g', 'h',  'j', 'k', 'l', ';', /* 20-27 */
    '\'', '`', 0,   '\\', 'z', 'x', 'c', 'v', /* 28-2f */
    'b',  'n', 'm', ',',  '.', '/', 0,   0,   /* 30-37 */
    0,    ' ',                                 /* 38-39 */
};

/* PS/2 Set 1 make-code -> ASCII, shifted. */
static const uint8_t sc_shift[0x3a] = {
    0, 0,   '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', 0,   0,
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '{', '}', 0,   0,
    'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ':', '"', '~', 0,   '|',
    'Z', 'X', 'C', 'V', 'B', 'N', 'M', '<', '>', '?', 0,   0,   0,   ' ',
};

/* Prompt prefix — set to "chat> ", "[server]> ", or "[nick]> " at runtime. */
static uint8_t g_prompt_pfx[NICK_CAP + 5];

static void reprint_prompt(const uint8_t *line, uint8_t llen) {
    static uint8_t buf[LINE_CAP + NICK_CAP + 6];
    uint8_t i = 0;
    const uint8_t *pfx = g_prompt_pfx;
    while (*pfx) buf[i++] = *pfx++;
    for (uint8_t j = 0; j < llen; j++) buf[i++] = line[j];
    buf[i] = '\0';
    print(buf);
}

/* Build "[A.B.C.D]" into tag (>=20 bytes). */
static uint8_t ip_tag(const uint8_t ip[4], uint8_t *tag) {
    uint8_t ti = 0;
    tag[ti++] = '[';
    for (uint8_t oct = 0; oct < 4; oct++) {
        uint8_t v = ip[oct];
        if (v >= 100) tag[ti++] = (uint8_t)('0' + v / 100);
        if (v >= 10)  tag[ti++] = (uint8_t)('0' + (v / 10) % 10);
        tag[ti++] = (uint8_t)('0' + v % 10);
        if (oct < 3) tag[ti++] = '.';
    }
    tag[ti++] = ']';
    tag[ti] = '\0';
    return ti;
}

/* Build "[nick]" into tag (>= NICK_CAP+3 bytes). */
static uint8_t nick_tag(const uint8_t *nick, uint8_t *tag) {
    uint8_t ti = 0;
    tag[ti++] = '[';
    uint8_t ni = 0;
    while (nick[ni] && ti < (uint8_t)(NICK_CAP + 1)) tag[ti++] = nick[ni++];
    tag[ti++] = ']';
    tag[ti] = '\0';
    return ti;
}

/* Print a received message above the current input line.
 * tag: e.g. "[>]", "[alice]", "[10.3.3.5]". */
static void show_received(const uint8_t *tag, const uint8_t *data, uint32_t len,
                          const uint8_t *line, uint8_t llen) {
    static uint8_t buf[1024 + 32];
    uint32_t i = 0;
    buf[i++] = '\r'; buf[i++] = '\n';
    const uint8_t *t = tag;
    while (*t && i < sizeof(buf) - 4) buf[i++] = *t++;
    buf[i++] = ' ';
    for (uint32_t j = 0; j < len && i < sizeof(buf) - 2; j++) {
        if (data[j] == '\r') continue;
        if (data[j] == '\n') break;
        buf[i++] = data[j];
    }
    buf[i++] = '\n';
    buf[i] = '\0';
    print(buf);
    reprint_prompt(line, llen);
}

/*
 * Broadcast buf to all nick_ready clients except skip (NULL = send to all).
 * Clients that haven't confirmed a nickname yet are silently skipped.
 */
static void broadcast(TcpSocket_T **clients, const uint8_t *nick_ready,
                      TcpSocket_T *skip, const uint8_t *buf, uint32_t len) {
    for (uint8_t i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i] || clients[i] == skip) continue;
        if (!nick_ready[i]) continue;
        if (clients[i]->state == SOCKET_ESTABLISHED)
            write(clients[i], buf, len);
    }
}

/*
 * Forward a chat message to all nick_ready clients except the sender.
 * Sends two TCP segments: "[nick] " header then the raw payload.
 * Avoids a large combined buffer on the stack.
 */
static void forward_msg(TcpSocket_T **clients, const uint8_t *nick_ready,
                        TcpSocket_T *sender, const uint8_t *nick,
                        const uint8_t *payload, uint32_t len) {
    uint8_t hdr[NICK_CAP + 3];
    uint8_t hi = 0;
    hdr[hi++] = '[';
    uint8_t ni = 0;
    while (nick[ni] && hi < (uint8_t)(NICK_CAP + 1)) hdr[hi++] = nick[ni++];
    hdr[hi++] = ']';
    hdr[hi++] = ' ';

    for (uint8_t i = 0; i < MAX_CLIENTS; i++) {
        if (!clients[i] || clients[i] == sender) continue;
        if (!nick_ready[i]) continue;
        if (clients[i]->state != SOCKET_ESTABLISHED) continue;
        write(clients[i], hdr, hi);
        write(clients[i], payload, len);
    }
}

int main(int argc, char **argv) {
    uint8_t pipe[PIPE_CAP];
    uint8_t line[LINE_CAP];
    uint8_t llen = 0;
    uint8_t shift = 0;

    uint8_t packet_buf[2048];
    uint8_t tcp_packet[1500];
    uint8_t rx_buf[1024];
    static TcpSocket_T sockets[MAX_SOCKETS];

    Ipv4Header_T ipv4_header;
    uint16_t ipv4_header_len = 0;
    TcpHeader_T tcp_header;

    /* Client mode: single connection. */
    TcpSocket_T *chat_sock = 0;
    uint8_t connected = 0;
    uint8_t have_nick = 0; /* set after first line sent (= the nick) */

    /* Server/room mode. */
    TcpSocket_T *server = 0;
    TcpSocket_T *clients[MAX_CLIENTS];
    uint8_t nick_ready[MAX_CLIENTS]; /* 1 once nick confirmed */
    uint8_t nicks[MAX_CLIENTS][NICK_CAP];
    uint8_t num_clients = 0;

    for (uint8_t i = 0; i < MAX_CLIENTS; i++) {
        clients[i]  = 0;
        nick_ready[i] = 0;
        nicks[i][0] = '\0';
    }

    uint8_t mode = 0; /* 's' or 'c' */

    uint8_t my_ip[4]   = {10, 3, 3, 2};
    uint8_t peer_ip[4] = {10, 3, 3, 1};
    uint8_t net_arg[8] = {'s','l','i','p','\0'};

    {
        SysInfo_T si;
        if (read_sysinfo(&si) && (si.ip_addr[0] || si.ip_addr[1] || si.ip_addr[2] || si.ip_addr[3]))
            memcpy(my_ip, si.ip_addr, 4);
    }

    for (int i = 1; i < argc; i++) {
        const uint8_t *a = (const uint8_t *)argv[i];
        if ((a[0] == 's' && a[1] == '\0') || memcmp(a, (const uint8_t *)"server", 7) == 0) {
            mode = 's';
        } else if ((a[0] == 'c' && a[1] == '\0') || memcmp(a, (const uint8_t *)"client", 7) == 0) {
            mode = 'c';
        } else if (memcmp(a, (const uint8_t *)"eth",  4) == 0) {
            net_arg[0]='e'; net_arg[1]='t'; net_arg[2]='h'; net_arg[3]='\0';
        } else if (memcmp(a, (const uint8_t *)"slip", 5) == 0) {
            net_arg[0]='s'; net_arg[1]='l'; net_arg[2]='i'; net_arg[3]='p'; net_arg[4]='\0';
        } else if (memcmp(a, (const uint8_t *)"--net", 6) == 0 && i + 1 < argc) {
            const uint8_t *n = (const uint8_t *)argv[++i];
            uint8_t k = 0;
            while (n[k] && k < 7) { net_arg[k] = n[k]; k++; }
            net_arg[k] = '\0';
        } else if (memcmp(a, (const uint8_t *)"--ip", 5) == 0 && i + 1 < argc) {
            parse_ip_str((const uint8_t *)argv[++i], my_ip);
        } else {
            parse_ip_str(a, peer_ip);
        }
    }

    for (uint8_t i = 0; i < PIPE_CAP; i++) pipe[i] = 0;

    /* Default prompt; overridden by mode below. */
    {
        const uint8_t *dp = (const uint8_t *)"chat> ";
        uint8_t pi = 0;
        while (*dp) g_prompt_pfx[pi++] = *dp++;
        g_prompt_pfx[pi] = '\0';
    }

    printf((const uint8_t *)"\nchat - TCP chat (%s)\n", net_arg);

    if (!pipe_subscribe(pipe)) {
        print((const uint8_t *)"chat: pipe subscribe failed\n");
        return 1;
    }

    if (!mode) {
        print((const uint8_t *)"Select mode: [s] room-server  [c] client\n");
        while (!mode) {
            uint8_t sc = pipe[0];
            if (!sc) continue;
            pipe[0] = 0;
            if (sc & 0x80) continue;
            if (sc == 0x1f) mode = 's';
            if (sc == 0x2e) mode = 'c';
        }
    }

    if (net_driver_bind_port(net_arg, CHAT_PORT) < 0) {
        print((const uint8_t *)"chat: net driver init failed\n");
        return 1;
    }

    if (net_arg[0] == 'e')
        net_get_local_ip(my_ip);

    if (mode == 's') {
        const uint8_t *sp = (const uint8_t *)"[server]> ";
        uint8_t pi = 0;
        while (*sp) g_prompt_pfx[pi++] = *sp++;
        g_prompt_pfx[pi] = '\0';
        print((const uint8_t *)"[room] listening on port 9000...\n");
        server = socket_tcp4(sockets);
        bind(server, CHAT_PORT);
        listen(server);
    } else {
        print((const uint8_t *)"[client] connecting to ");
        printf((const uint8_t *)"%d.%d.%d.%d", peer_ip[0], peer_ip[1], peer_ip[2], peer_ip[3]);
        print((const uint8_t *)"...\n");
        chat_sock = tcp_connect(sockets, peer_ip, CHAT_PORT, CHAT_PORT, my_ip);
        if (!chat_sock) {
            print((const uint8_t *)"chat: socket alloc failed\n");
            return 1;
        }
    }

    for (;;) {
        /* ---- keyboard ---- */
        uint8_t sc = pipe[0];
        if (sc) {
            pipe[0] = 0;

            if (sc & 0x80) {
                uint8_t mk = sc & 0x7f;
                if (mk == 0x2a || mk == 0x36) shift = 0;
            } else if (sc == 0x2a || sc == 0x36) {
                shift = 1;
            } else if (sc == 0x1c) {
                /* Enter -- send line */
                print((const uint8_t *)"\n");
                if (llen > 0) {
                    uint8_t me_buf[LINE_CAP + 8];
                    uint8_t mi = 0;
                    const uint8_t *mpfx = (const uint8_t *)"[me] ";
                    while (*mpfx) me_buf[mi++] = *mpfx++;
                    for (uint8_t i = 0; i < llen; i++) me_buf[mi++] = line[i];
                    me_buf[mi++] = '\n';
                    me_buf[mi] = '\0';
                    print(me_buf);
                    line[llen++] = '\n';
                    if (mode == 's') {
                        /* Two broadcasts: "[server] " header then the line. */
                        static const uint8_t srv_hdr[] = "[server] ";
                        broadcast(clients, nick_ready, 0, srv_hdr, sizeof(srv_hdr) - 1);
                        broadcast(clients, nick_ready, 0, line, llen);
                    } else if (chat_sock && chat_sock->state == SOCKET_ESTABLISHED) {
                        /* First sent line is the nick — lock it into the prompt. */
                        if (!have_nick) {
                            have_nick = 1;
                            uint8_t pi = 0;
                            g_prompt_pfx[pi++] = '[';
                            for (uint8_t k = 0; k < llen - 1 && pi < (uint8_t)(NICK_CAP + 1); k++)
                                g_prompt_pfx[pi++] = line[k];
                            g_prompt_pfx[pi++] = ']';
                            g_prompt_pfx[pi++] = '>';
                            g_prompt_pfx[pi++] = ' ';
                            g_prompt_pfx[pi] = '\0';
                        }
                        write(chat_sock, line, llen);
                    }
                }
                llen = 0;
                reprint_prompt(line, llen);
            } else if (sc == 0x01) {
                /* Escape -- quit */
                print((const uint8_t *)"\r\nchat: bye\n");
                pipe_unsubscribe(pipe);
                return 0;
            } else if (sc == 0x0e) {
                /* Backspace */
                if (llen > 0) {
                    llen--;
                    print((const uint8_t *)"\b \b");
                }
            } else if (sc < 0x3a) {
                uint8_t ch = shift ? sc_shift[sc] : sc_normal[sc];
                if (ch && llen < LINE_CAP - 2) {
                    line[llen++] = ch;
                    uint8_t buf[2] = {ch, '\0'};
                    print(buf);
                }
            }
        }

        /* ---- network ---- */
        int64_t net_len = net_drv.recv(packet_buf, sizeof(packet_buf));
        if (net_len <= 0) continue;

        ipv4_header_len = parse_ipv4_packet(packet_buf, &ipv4_header);
        if (!ipv4_header_len || ipv4_header.protocol != 6) continue;

        memcpy(tcp_packet, packet_buf + ipv4_header_len, (uint32_t)net_len - ipv4_header_len);
        parse_tcp_packet(tcp_packet, &tcp_header);
        on_tcp_packet(ipv4_header.source_addr, ipv4_header.destination_addr,
                      &tcp_header, tcp_packet, (uint32_t)net_len - ipv4_header_len, sockets);

        if (mode == 's') {
            /* -- Discover newly ESTABLISHED peers not yet in our table. --
             * Scan sockets[] directly: accept() requires rx_len > 0 and always
             * returns the first match, making it unusable for multi-client tracking. */
            for (uint8_t si = 0; si < MAX_SOCKETS; si++) {
                TcpSocket_T *s = &sockets[si];
                if (s == server) continue;
                if (!s->used || s->state != SOCKET_ESTABLISHED) continue;
                if (s->local_port != CHAT_PORT) continue;

                uint8_t known = 0;
                for (uint8_t j = 0; j < MAX_CLIENTS; j++)
                    if (clients[j] == s) { known = 1; break; }
                if (known) continue;

                uint8_t placed = 0;
                for (uint8_t j = 0; j < MAX_CLIENTS; j++) {
                    if (clients[j]) continue;
                    clients[j]    = s;
                    nick_ready[j] = 0;
                    nicks[j][0]   = '\0';
                    num_clients++;
                    placed = 1;
                    static const uint8_t prompt[] = "\r\nnick: ";
                    write(s, prompt, sizeof(prompt) - 1);
                    uint8_t itag[20];
                    ip_tag(s->remote_ip, itag);
                    printf((const uint8_t *)"\n[~] %s connecting...\n", itag);
                    reprint_prompt(line, llen);
                    break;
                }
                if (!placed) {
                    static const uint8_t full[] = "\r\n[room] full, try later\r\n";
                    write(s, full, sizeof(full) - 1);
                    close(s);
                }
            }

            /* -- Read from each peer. -- */
            for (uint8_t i = 0; i < MAX_CLIENTS; i++) {
                if (!clients[i] || clients[i]->state != SOCKET_ESTABLISHED) continue;
                uint32_t n = read(clients[i], rx_buf, sizeof(rx_buf) - 1);
                if (n == 0) continue;
                rx_buf[n] = '\0';

                if (!nick_ready[i]) {
                    /* Accumulate nick bytes until newline arrives. */
                    uint8_t ni = 0;
                    while (nicks[i][ni]) ni++;
                    uint8_t got_eol = 0;
                    for (uint32_t k = 0; k < n; k++) {
                        uint8_t c = rx_buf[k];
                        if (c == '\n') { got_eol = 1; break; }
                        if (c == '\r') continue;
                        if (ni < NICK_CAP - 1) { nicks[i][ni++] = c; nicks[i][ni] = '\0'; }
                    }
                    if (!got_eol) continue;

                    if (ni == 0) {
                        static const uint8_t bad[] = "nick cannot be empty\r\nnick: ";
                        write(clients[i], bad, sizeof(bad) - 1);
                        continue;
                    }

                    nick_ready[i] = 1;

                    /* Welcome the new member. */
                    {
                        uint8_t wbuf[NICK_CAP + 32];
                        uint32_t wlen = 0;
                        const uint8_t *w = (const uint8_t *)"[room] welcome, ";
                        while (*w) wbuf[wlen++] = *w++;
                        uint8_t k2 = 0;
                        while (nicks[i][k2]) wbuf[wlen++] = nicks[i][k2++];
                        const uint8_t *w2 = (const uint8_t *)"!\r\n";
                        while (*w2) wbuf[wlen++] = *w2++;
                        write(clients[i], wbuf, wlen);
                    }

                    /* Announce to all other nick_ready clients. */
                    {
                        uint8_t abuf[NICK_CAP + 24];
                        uint32_t alen = 0;
                        const uint8_t *a = (const uint8_t *)"[room] ";
                        while (*a) abuf[alen++] = *a++;
                        uint8_t k2 = 0;
                        while (nicks[i][k2]) abuf[alen++] = nicks[i][k2++];
                        const uint8_t *a2 = (const uint8_t *)" joined\r\n";
                        while (*a2) abuf[alen++] = *a2++;
                        broadcast(clients, nick_ready, clients[i], abuf, alen);
                    }

                    printf((const uint8_t *)"\n[+] %s is in the room (%u total)\n",
                           nicks[i], (uint32_t)num_clients);
                    reprint_prompt(line, llen);

                } else {
                    /* Confirmed member: display locally, forward with nick prefix. */
                    uint8_t tag[NICK_CAP + 3];
                    nick_tag(nicks[i], tag);
                    show_received(tag, rx_buf, n, line, llen);
                    forward_msg(clients, nick_ready, clients[i], nicks[i], rx_buf, n);
                }
            }

            /* -- Detect disconnected peers. -- */
            for (uint8_t i = 0; i < MAX_CLIENTS; i++) {
                if (!clients[i]) continue;
                if (clients[i]->used && clients[i]->state != SOCKET_CLOSED) continue;

                /* Save nick/IP before clearing the slot. */
                uint8_t was_ready = nick_ready[i];
                uint8_t saved_nick[NICK_CAP];
                uint8_t saved_ip[4];
                uint8_t k2 = 0;
                while (nicks[i][k2]) { saved_nick[k2] = nicks[i][k2]; k2++; }
                saved_nick[k2] = '\0';
                memcpy(saved_ip, clients[i]->remote_ip, 4);

                num_clients--;
                clients[i]    = 0;
                nick_ready[i] = 0;
                nicks[i][0]   = '\0';

                if (was_ready) {
                    uint8_t dbuf[NICK_CAP + 24];
                    uint32_t dlen = 0;
                    const uint8_t *d = (const uint8_t *)"[room] ";
                    while (*d) dbuf[dlen++] = *d++;
                    k2 = 0;
                    while (saved_nick[k2]) dbuf[dlen++] = saved_nick[k2++];
                    const uint8_t *d2 = (const uint8_t *)" left\r\n";
                    while (*d2) dbuf[dlen++] = *d2++;
                    broadcast(clients, nick_ready, 0, dbuf, dlen);
                    printf((const uint8_t *)"\n[-] %s left (%u in room)\n",
                           saved_nick, (uint32_t)num_clients);
                } else {
                    uint8_t itag[20];
                    ip_tag(saved_ip, itag);
                    printf((const uint8_t *)"\n[-] %s disconnected before choosing nick\n", itag);
                }
                reprint_prompt(line, llen);
            }

        } else {
            /* -- Client mode: single connection. -- */
            if (chat_sock && !connected && chat_sock->state == SOCKET_ESTABLISHED) {
                connected = 1;
                print((const uint8_t *)"\n[connected]\n");
                reprint_prompt(line, llen);
            }

            if (chat_sock && chat_sock->state == SOCKET_ESTABLISHED) {
                uint32_t n = read(chat_sock, rx_buf, sizeof(rx_buf) - 1);
                if (n > 0) {
                    rx_buf[n] = '\0';
                    show_received((const uint8_t *)"[>]", rx_buf, n, line, llen);
                }
            }

            if (connected && chat_sock && chat_sock->state == SOCKET_CLOSED) {
                print((const uint8_t *)"\r\n[disconnected]\r\n");
                pipe_unsubscribe(pipe);
                return 0;
            }
        }
    }

    return 0;
}
