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
