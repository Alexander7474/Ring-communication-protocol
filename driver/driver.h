#ifndef DRIVER_H
#define DRIVER_H

#include "../common/config.h"

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
set_addr(unsigned long addr, char *buffer);

void
set_src_addr(unsigned long addr, char *buffer);

unsigned long
get_addr(char *buffer);

unsigned long
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
send_connection_message(int sockg, unsigned long dest_addr,
                        unsigned long new_host_addr, char *buffer);

int
is_own_addr(unsigned long addr);

unsigned long
get_sockaddr(int sock);

void
connect_sock(unsigned long addr, int sock);

#endif // !DRIVER_H
