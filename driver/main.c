#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include "driver.h"
#include "../common/config.h"
#include "../common/error.h"

// server qui se parle a lui meme avec son fils (bizarre)
int main (int argc, char *argv[])
{
        if(argc != 2) FATAL("Nombre d'arguments incorrect.\n Usage: ./driver address");
 
        char recv_buffer[SMAX];
        char send_buffer[SMAX];
        int cc, sockd, max_sd;
        int newsockd = 0;
        int sockg = 0;
        struct sockaddr_in servd;

        servd.sin_family = AF_INET;  // On nous demandait d'utiliser le domaine Internet
        servd.sin_port = htons(PORT);    // htons(PORT) pour convertir le numéro de port
        // hp->h_addr = hp->h_addr_list[0]
        servd.sin_addr.s_addr = htonl(INADDR_ANY);

        sockd = socket(AF_INET, SOCK_STREAM, 0);  // Création de la socket

        int opt = 1;
        setsockopt(sockd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        cc = bind(sockd, (struct sockaddr *) &servd, sizeof(servd));
        if(cc == -1) FATAL("bind"); // Erreur à l'attachement

        cc = listen(sockd, 5);
        if(cc == -1) FATAL("listen");

        fd_set readfds;
        while(1){
                int data_recv = 0;

                FD_ZERO(&readfds);
                FD_SET(sockd, &readfds);
                max_sd = sockd;

                if(newsockd>0)
                        FD_SET(newsockd, &readfds);
                else 
                        printf("Aucun client connécté\n");

                if(newsockd>sockd)
                        max_sd = newsockd;

                struct timeval timeout;
                timeout.tv_sec = 1;
                timeout.tv_usec = 0;

                int activity = select(max_sd+1, &readfds, NULL, NULL, &timeout);

                if(activity < 0)
                        FATAL("activity");

                if(FD_ISSET(sockd, &readfds)){
                        int lenpservd = sizeof(servd);
                        newsockd = accept(sockd, (struct sockaddr *)&servd, (socklen_t *) &lenpservd);
                        printf("Sockd nouvelle connection !\n");
                }


                if(FD_ISSET(newsockd, &readfds)){
                        receiv_sockd(newsockd, recv_buffer);
                        printf("Message recu: %s\n", recv_buffer);
                        data_recv = 1;
                }else{ // si l'on ne recoit rien depuis MAX_WAIT, on regénére un token
                        generate_token(send_buffer);
                }

                // connection de sockg
                struct hostent * hp;
                struct sockaddr_in servg;

                // connection de sockg avec address en argument
                if(sockg <= 0){
                        hp = gethostbyname(argv[1]);
                        if(hp == NULL) FATAL("gethostbyname");  // Toujours tester pour éviter d'accumuler les erreurs

                        servg.sin_family = AF_INET;
                        servg.sin_port = htons(PORT);
                        bcopy(hp->h_addr, (char *) & servg.sin_addr, hp->h_length);

                        sockg = socket(AF_INET, SOCK_STREAM, 0);

                        if(connect(sockg, (struct sockaddr*) &servg, sizeof(servg)) == -1){
                          FATAL("Connect socket");
                        }
                        printf("Client prêt !\n");
                }

                if(data_recv){
                        char token[TOKEN_SIZE+1];
                        memcpy(token, recv_buffer, TOKEN_SIZE); // extraction du token dans le message
                        int value = (int)strtol(token, NULL, 16);
                        value++;
                        snprintf(token, sizeof(token), "%08X", value);  // uppercase, zero-padded to 8 chars
                        memcpy(send_buffer, token, TOKEN_SIZE); 
                } 
                
                printf("Envoie: %s\n", send_buffer);
                send_sockg(sockg, send_buffer);
        }
        
        close(sockd);
        close(sockg);

        exit(0);
        return 0;
}
