#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

#include "../common/config.h"
#include "../common/error.h"
#include "driver.h"

void
send_sockg(int sock, char *msg)
{
        int cc = write(sock, msg, sizeof(char) * SMAX);
        if (cc <= 0)
                FATAL("Send msgs write");
}

void
receiv_sockd(int sock, char *msg)
{
        int cc = read(sock, msg, sizeof(char) * SMAX);
        if (cc <= 0)
                FATAL("Receiv message");
}

void
generate_message_buffer(char *buffer)
{
        char new_buffer[SMAX];
        memset(new_buffer, '\0', sizeof(new_buffer));
        new_buffer[0] = 'f';
        memcpy(buffer, new_buffer, SMAX);
}

void
increment_token(char *buffer)
{
        char token[TOKEN_SIZE + 1] = { '\0' };
        get_token(buffer, token);
        int value = (int)strtol(token, NULL, 16);
        value++;
        snprintf(token, sizeof(char) * TOKEN_SIZE + 1, TOKEN_FMT,
                 value); // uppercase, zero-padded to 8 chars
        memcpy(buffer + 1, token, TOKEN_SIZE);
}

char
get_flag(char *buffer)
{
        return buffer[0];
}

void
get_data(char *buffer, char *data)
{
        memcpy(data, buffer + DATA_OFFSET, DATA_SIZE);
}

uint32_t
get_addr(char *buffer)
{
        uint32_t addr;
        memcpy(&addr, buffer + ADDR_OFFSET, sizeof(uint32_t));
        return addr;
}

uint32_t
get_src_addr(char *buffer)
{
        uint32_t addr;
        memcpy(&addr, buffer + ADDR_SRC_OFFSET, sizeof(uint32_t));
        return addr;
}

void
get_token(char *buffer, char *token)
{
        memcpy(token, buffer + TOKEN_OFFSET, TOKEN_SIZE);
}

void
set_flag(char flag, char *buffer)
{
        buffer[0] = flag;
}

void
set_addr(uint32_t addr, char *buffer)
{
        memcpy(buffer + ADDR_OFFSET, &addr, sizeof(uint32_t));
}

void
set_src_addr(uint32_t addr, char *buffer)
{
        memcpy(buffer + ADDR_SRC_OFFSET, &addr, sizeof(uint32_t));
}

void
dump_message(char *buffer)
{
        char token[TOKEN_SIZE + 1], data[DATA_SIZE + 1];
        get_token(buffer, token);
        uint32_t addr = get_addr(buffer);
        uint32_t src_addr = get_src_addr(buffer);
        get_data(buffer, data);
        token[TOKEN_SIZE] = '\0';
        data[DATA_SIZE] = '\0';
        printf("Message dump start ----------------\n");
        printf("Message: %s\n", buffer);
        printf("Flag: %c\n", get_flag(buffer));
        printf("Token: %s\n", token);
        printf("Source address: %u\n", src_addr);
        printf("Destination address: %u\n", addr);
        printf("Data: %s\n", data);
        printf("Message dump end ------------------\n");
}

int
skip_buffer(int sock, char *buffer)
{
        char send_buffer[SMAX];

        memcpy(send_buffer, buffer, SMAX);
        increment_token(send_buffer);
        return write(sock, send_buffer, sizeof(send_buffer));
}

int
is_own_addr(uint32_t addr)
{
        struct ifaddrs *ifaddr, *ifa;
        if (getifaddrs(&ifaddr) == -1) {
                FATAL("getifaddrs");
        }

        int found = 0;
        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
                if (!ifa->ifa_addr)
                        continue;
                if (ifa->ifa_addr->sa_family != AF_INET)
                        continue;

                struct sockaddr_in *local = (struct sockaddr_in *)ifa->ifa_addr;

                // Compare IPs
                if (addr == local->sin_addr.s_addr) {
                        found = 1;
                        break;
                }
        }

        freeifaddrs(ifaddr);
        return found;
}

/**
 * send_connection_message - Envoie un message de connection marqué
 * 'c' au dernier host de l'anneau pour qu'il connect son sockg à
 * notre nouveau sockd
 * @sockg : socket gauche pour l'envoie
 * @dest_addr : addresse de destination
 * @new_host_addr : addresse du nouvelle host
 * @buffer : buffer structuré recu contenant un flag free et le token
 *
 * Return : 1 si ok, 0 > si non ok
 */
int
send_connection_message(int sockg, uint32_t dest_addr, uint32_t new_host_addr,
                        char *buffer)
{
        if (get_flag(buffer) != 'f')
                return -1;

        char send_buffer[SMAX];
        memcpy(send_buffer, buffer, SMAX);

        memcpy(send_buffer + DATA_OFFSET, &new_host_addr, sizeof(uint32_t));
        increment_token(send_buffer);
        set_flag('c', send_buffer);
        set_addr(dest_addr, send_buffer);

        return write(sockg, send_buffer, sizeof(send_buffer));
}

/**
 * get_sockaddr - Renvoie l'addresse de la machine connecté à un socket
 * @sock : socket ou chercher l'addresse
 *
 * Return : Adresse dans sockaddr_in.sin_addr.s_addr
 */
uint32_t
get_sock_remote_addr(int sock)
{
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);

        if (getpeername(sock, (struct sockaddr *)&addr, &len) == -1) {
                FATAL("getpeername");
        }

        return addr.sin_addr.s_addr;
}

/**
 * get_sockaddr - Renvoie l'addresse local utilisé sur un socket
 * @sock : socket ou chercher l'addresse
 *
 * Return : Adresse dans sockaddr_in.sin_addr.s_addr
 */
uint32_t
get_sock_own_addr(int sock)
{
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);

        if (getsockname(sock, (struct sockaddr *)&addr, &len) == -1) {
                FATAL("getsockname");
        }

        return addr.sin_addr.s_addr;
}

/**
 * connect_sock - Connect un socket à une addresse
 * @addr : addresse au format réseau little endian
 * @sock : socket
 */
void
connect_sock(uint32_t addr, int sock)
{
        struct sockaddr_in new_addr;
        memset(&new_addr, 0, sizeof(new_addr));
        new_addr.sin_family = AF_INET;
        new_addr.sin_port = htons(PORT); // same port as the original connection
        new_addr.sin_addr.s_addr = addr;
        connect(sock, (struct sockaddr *)&new_addr, sizeof(new_addr));
}

/**
 * is_diffusion_addr - Retourne si oui ou non @addr
 * est une address de diffusion
 * @addr : Adresse à tester
 * Return : 1 si @addr est une addresse de diffusion, 0 si non
 */
int
is_diffusion_addr(uint32_t addr)
{
        if (inet_addr(BROADCAST_ADDR) == addr)
                return 1;
        else
                return 0;
}

/**
 * repair_ring - Procedure de reconnexion si @sockd 
 * tombe en panne
 * @sockg : 
 * @sockd : 
 */
void 
repair_ring(int sockg, int sockd)
{
       
}

int 
inet_socket_healthcheck(int sock)
{
        char buf;
        ssize_t n = recv(sock, &buf, 1, MSG_PEEK | MSG_DONTWAIT);

        if (n > 0) {
                return 1;// Data is available, connection is alive (and you didn’t consume it)
        } else if (n == 0) {
                return 0;// Peer has closed the connection
        } else {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                        return 1;// No data available right now, but socket is still alive
                } else {
                        return 0;// Real error → connection likely broken
                }
        }
}

