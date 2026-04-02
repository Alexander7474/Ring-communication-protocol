#ifndef DRIVER_H
#define DRIVER_H

#include "../common/config.h"

void send_sockg(int sock, char *msg);

void receiv_sockd(int sock, char *msg);

void generate_message_buffer(char *buffer);

void increment_token(char *buffer);

char get_flag(char *buffer);

void set_flag(char flag, char *buffer);

void get_addr(char *buffer, char *addr);

void get_token(char *buffer, char *token);

void get_data(char *buffer, char *data);

void dump_message(char* buffer);

int skip_buffer(int sock, char *buffer);

int send_connection_message(int sockg, int sockd, char *buffer);

int is_loopback_sock(int sock);

#endif // !DRIVER_H
