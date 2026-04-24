#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

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

unsigned long
get_addr(char *buffer)
{
    uint32_t addr = 0;
    memcpy(&addr, buffer + ADDR_OFFSET, ADDR_SIZE);
    return (unsigned long) addr;
}

unsigned long
get_src_addr(char *buffer)
{
    uint32_t addr = 0;
    memcpy(&addr, buffer + ADDR_SRC_OFFSET, ADDR_SIZE);
    return (unsigned long) addr;
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
set_addr(unsigned long addr, char *buffer)
{
    uint32_t addr32 = (uint32_t) addr;
    memcpy(buffer + ADDR_OFFSET, &addr32, ADDR_SIZE);
}

void
set_src_addr(unsigned long addr, char *buffer)
{
    uint32_t addr32 = (uint32_t) addr;
    memcpy(buffer + ADDR_SRC_OFFSET, &addr32, ADDR_SIZE);
}

void
dump_message(char *buffer)
{
	char token[TOKEN_SIZE + 1], data[DATA_SIZE + 1];
	get_token(buffer, token);
	unsigned long addr = get_addr(buffer);
	unsigned long src_addr = get_src_addr(buffer);
	get_data(buffer, data);
	token[TOKEN_SIZE] = '\0';
	data[DATA_SIZE] = '\0';
	printf("Message dump start ----------------\n");
	printf("Message: %s\n", buffer);
	printf("Flag: %c\n", get_flag(buffer));
	printf("Token: %s\n", token);
	printf("Source address: %lu\n", src_addr);
	printf("Destination address: %lu\n", addr);
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
is_own_addr(unsigned long addr)
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
send_connection_message(int sockg, unsigned long dest_addr,
                        unsigned long new_host_addr, char *buffer)
{
    if (get_flag(buffer) != 'f')
        return -1;

    char send_buffer[SMAX];
    memcpy(send_buffer, buffer, SMAX);

    uint32_t new_host_addr32 = (uint32_t) new_host_addr;
    memcpy(send_buffer + DATA_OFFSET, &new_host_addr32, ADDR_SIZE);  // ← ADDR_SIZE au lieu de sizeof(unsigned long)
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
unsigned long
get_sockaddr(int sock)
{
	struct sockaddr_in addr;
	socklen_t len = sizeof(addr);

	if (getpeername(sock, (struct sockaddr *)&addr, &len) == -1) {
		FATAL("getpeername");
	}

	return addr.sin_addr.s_addr;
}

/**
 * connect_sock - Connect un socket à une addresse
 * @addr : addresse au format réseau little endian
 * @sock : socket
 */
void
connect_sock(unsigned long addr, int sock)
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
is_diffusion_addr(unsigned long addr)
{
  if(inet_addr(BROADCAST_ADDR) == addr)
    return 1;
  else 
    return 0;
}
