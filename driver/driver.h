#ifndef DRIVER_H
#define DRIVER_H

#include <netinet/in.h> /* struct sockaddr_in */
#include <stdint.h>     /* uint32_t           */
#include <sys/select.h> /* fd_set             */
#include <sys/un.h>     /* struct sockaddr_un */
#include <time.h>       /* struct timespec    */

#include "../common/config.h"      /* SMAX, PORT, ...        */
#include "../common/ring_buffer.h" /* struct ring_buffer     */

void send_sockg(int sock, char *msg);
void receiv_sockd(int sock, char *msg);
void generate_message_buffer(char *buffer);
void increment_token(char *buffer);
char get_flag(char *buffer);
void set_flag(char flag, char *buffer);
void set_addr(uint32_t addr, char *buffer);
void set_src_addr(uint32_t addr, char *buffer);
uint32_t get_addr(char *buffer);
uint32_t get_src_addr(char *buffer);
void get_token(char *buffer, char *token);
void get_data(char *buffer, char *data);
void dump_message(char *buffer);
int skip_buffer(int sock, char *buffer);
int send_connection_message(int sockg, uint32_t dest_addr,
                            uint32_t new_host_addr, char *buffer);
int is_own_addr(uint32_t addr);
int is_diffusion_addr(uint32_t addr);
uint32_t get_sock_remote_addr(int sock);
uint32_t get_sock_own_addr(int sock);
void connect_sock(uint32_t addr, int sock);
void repair_ring(int sockg, int sockd);
int inet_socket_healthcheck(int sock);
void setup_fdset(fd_set *readfds, int *max_sd, int newsockd, int newsockcomm,
                 int sockd, int sockcomm);
void handle_new_inet_conn(int newsockd, struct sockaddr_in *servd, int *sockd,
                          struct ring_buffer *waiting_hosts);
void handle_new_unix_conn(int newsockcomm, struct sockaddr_un *servcomm,
                          int *sockcomm);
int handle_recv_sockd(int *sockd, int sockg, char *recv_buffer,
                      struct timespec *last_recv);
int handle_read_sockcomm(int *sockcomm, char *read_buffer);
void check_token_timeout(int sockg, char *send_buffer,
                         struct timespec *last_recv,
                         const struct timespec *actual_time);
int handle_flag_free(int *sockd, int *sockg, int *sockcomm, int *comm_request,
                     struct ring_buffer *waiting_hosts, char *recv_buffer,
                     char *send_buffer);
void handle_flag_local(char flag, int *sockg, int *sockcomm, char *recv_buffer,
                       char *send_buffer);

#endif /* DRIVER_H */
