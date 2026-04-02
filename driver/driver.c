#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ifaddrs.h>

#include "../common/config.h"
#include "../common/error.h"
#include "driver.h"

void send_sockg(int sock, char *msg) {
        int cc = write(sock, msg, sizeof(char)*SMAX);                                                
        if(cc <= 0) FATAL("Send msgs write");
}

void receiv_sockd(int sock, char *msg)  {
        int cc = read(sock, msg, sizeof(char)*SMAX);
        if(cc <= 0) FATAL("Receiv message");
}

void generate_message_buffer(char *buffer){
        char new_buffer[SMAX];
        memset(new_buffer, '#', sizeof(new_buffer));
        new_buffer[0] = 'f';
        memcpy(buffer, new_buffer, SMAX);
}

void increment_token(char *buffer){
        char token[TOKEN_SIZE+1] = {'\0'};
        get_token(buffer, token);
        int value = (int)strtol(token, NULL, 16);
        value++;
        snprintf(token, sizeof(char)*TOKEN_SIZE+1, TOKEN_FMT, value);  // uppercase, zero-padded to 8 chars
        memcpy(buffer+1, token, TOKEN_SIZE); 
}

char get_flag(char *buffer){
        return buffer[0];
}

void get_data(char *buffer, char *data){
        memcpy(data, buffer+FLAG_SIZE+TOKEN_SIZE+ADDR_SIZE, DATA_SIZE);
}

void get_addr(char *buffer, char *addr){
        memcpy(addr, buffer+FLAG_SIZE+TOKEN_SIZE, ADDR_SIZE);
}

void get_token(char *buffer, char *token){
        memcpy(token, buffer+FLAG_SIZE, TOKEN_SIZE);
}

void set_flag(char flag, char *buffer){
        buffer[0] = flag;
}

void dump_message(char *buffer){
        char token[TOKEN_SIZE+1], addr[ADDR_SIZE+1], data[DATA_SIZE+1];
        get_token(buffer, token);
        get_addr(buffer, addr);
        get_data(buffer, data);
        token[TOKEN_SIZE] = '\0';          
        addr[ADDR_SIZE] = '\0';    
        data[DATA_SIZE] = '\0';
        printf("Message dump start ----------------\n");
        printf("Message: %s\n", buffer);
        printf("Flag: %c\n", get_flag(buffer));
        printf("Token: %s\n", token);
        printf("Address: %s\n", addr);
        printf("Data: %s\n", data);
        printf("Message dump end ------------------\n");
}

int skip_buffer(int sock, char *buffer){
        char send_buffer[SMAX];

        memcpy(send_buffer, buffer, SMAX); 
        increment_token(send_buffer);
        return write(sock, send_buffer, sizeof(send_buffer));                                                
}

int is_loopback_sock(int sock) {
    struct sockaddr_storage addr;
    socklen_t len = sizeof(addr);

    // get the peer IP (remote endpoint)
    if (getpeername(sock, (struct sockaddr *)&addr, &len) == -1) {
        perror("getpeername");
        return 0;
    }

    if (addr.ss_family != AF_INET) {
        // Only handle IPv4 here
        return 0;
    }

    struct sockaddr_in *peer = (struct sockaddr_in *)&addr;

    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        return 0;
    }

    int found = 0;
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (!ifa->ifa_addr) continue;
        if (ifa->ifa_addr->sa_family != AF_INET) continue;

        struct sockaddr_in *local = (struct sockaddr_in *)ifa->ifa_addr;

        // Compare IPs
        if (peer->sin_addr.s_addr == local->sin_addr.s_addr) {
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
 * notre sockd
 * @sockg : socket gauche pour l'envoie
 * @sockd : socket pour récupérer l'addresse à envoyer dans le message 
 * de connection
 * @buffer : buffer structuré recu contenant un flag free et le token
 *
 * Return : 1 si ok, 0 > si non ok
 */
int send_connection_message(int sockg, int sockd, char *buffer){
        if(get_flag(buffer) != 'f')
                return -1;

        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);

        if (getpeername(sockd, (struct sockaddr *)&addr, &len) == -1) {
                perror("getpeername");
                return -1;
        }

        char *ip = inet_ntoa(addr.sin_addr);
        int port = ntohs(addr.sin_port);

        printf("Peer IP: %s\n", ip);
        printf("Peer port: %d\n", port);

        char send_buffer[SMAX];
        memcpy(send_buffer, buffer, SMAX); 
        increment_token(send_buffer);
        set_flag('c', send_buffer);
        return write(sockg, send_buffer, sizeof(send_buffer));                                                
}
