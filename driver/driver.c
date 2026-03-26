#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../common/config.h"
#include "../common/error.h"
#include "driver.h"

void send_sockg(int sock, char *msg) {
        int cc = write(sock, msg, sizeof(char)*SMAX);                                                
        if(cc == -1) FATAL("Send msgs write");
}

void receiv_sockd(int sock, char *msg)  {
        int cc = read(sock, msg, sizeof(char)*SMAX);
        if(cc == -1) FATAL("Receiv message");
}

void generate_token(char *buffer){
        char token[TOKEN_SIZE+1] = {'0'};
        memcpy(buffer, token, TOKEN_SIZE);
}

void increment_token(char *token){
        int value = (int)strtol(token, NULL, 16);
        value++;
        snprintf(token, sizeof(char)*TOKEN_SIZE+1, TOKEN_FMT, value);  // uppercase, zero-padded to 8 chars
}
