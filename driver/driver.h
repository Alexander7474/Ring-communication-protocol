#ifndef DRIVER_H
#define DRIVER_H

#include "../common/config.h"
#include <stdint.h>

void
send_sockg(int sock, char *msg);

void
receiv_sockd(int sock, char *msg);

void
generate_message_buffer(char *buffer);

void
increment_token(char *buffer);

char
get_flag(char *buffer);

void
set_flag(char flag, char *buffer);

void
set_addr(uint32_t addr, char *buffer);

void
set_src_addr(uint32_t addr, char *buffer);

uint32_t
get_addr(char *buffer);

uint32_t
get_src_addr(char *buffer);

void
get_token(char *buffer, char *token);

void
get_data(char *buffer, char *data);

void
dump_message(char *buffer);

int
skip_buffer(int sock, char *buffer);

int
send_connection_message(int sockg, uint32_t dest_addr, uint32_t new_host_addr,
                        char *buffer);

int
is_own_addr(uint32_t addr);

int
is_diffusion_addr(uint32_t addr);

uint32_t
get_sockaddr(int sock);

void
connect_sock(uint32_t addr, int sock);

int 
unix_socket_healthcheck(int sock);

int 
inet_socket_healthcheck(int sock);

#endif // !DRIVER_H
