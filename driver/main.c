#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <time.h>

#include "driver.h"
#include "../common/config.h"
#include "../common/ring_buffer.h"
#include "../common/error.h"

// server qui se parle a lui meme avec son fils (bizarre)
int main (int argc, char *argv[])
{
        if(argc != 2) FATAL("Nombre d'arguments incorrect.\nUsage: ./driver address");
 
        char recv_buffer[SMAX];
        char send_buffer[SMAX];
        int cc, newsockd, max_sd;
        struct sockaddr_in servd;
        
        int sockd = 0;
        int sockg = 0;

        struct ring_buffer waiting_hosts;

        // gestion du temps
        struct timespec last_recv;
        struct timespec actual_time;
        clock_gettime(CLOCK_MONOTONIC, &last_recv);

        servd.sin_family = AF_INET;  // On nous demandait d'utiliser le domaine Internet
        servd.sin_port = htons(PORT);    // htons(PORT) pour convertir le numéro de port
        // hp->h_addr = hp->h_addr_list[0]
        servd.sin_addr.s_addr = htonl(INADDR_ANY);

        newsockd = socket(AF_INET, SOCK_STREAM, 0);  // Création de la socket

        int opt = 1;
        setsockopt(newsockd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        cc = bind(newsockd, (struct sockaddr *) &servd, sizeof(servd));
        if(cc == -1) FATAL("bind"); // Erreur à l'attachement

        cc = listen(newsockd, 5);
        if(cc == -1) FATAL("listen");

        fd_set readfds;
        while(1){
                int data_recv = 0;

                FD_ZERO(&readfds);
                FD_SET(newsockd, &readfds);
                max_sd = newsockd;

                if(sockd>0)
                        FD_SET(sockd, &readfds);
                else 
                        printf("Aucun client connécté\n");

                if(sockd>newsockd)
                        max_sd = sockd;

                struct timeval timeout = {0, 100000};

                int activity = select(max_sd+1, &readfds, NULL, NULL, &timeout);

                if(activity < 0)
                        FATAL("activity");

                if(FD_ISSET(newsockd, &readfds)){
                        printf("Sockd nouvelle connection !\n");
                        int lenpservd = sizeof(servd);

                        if(sockd > 0){
                                int tmp_socket = accept(newsockd, (struct sockaddr *)&servd, (socklen_t *) &lenpservd);
                                push_rg_buff(&waiting_hosts, tmp_socket);
                        }else{
                                sockd = accept(newsockd, (struct sockaddr *)&servd, (socklen_t *) &lenpservd);
                        }
                }


                if(FD_ISSET(sockd, &readfds)){
                        receiv_sockd(sockd, recv_buffer);
                        printf("Message recu: %s\n", recv_buffer);
                        data_recv = 1;
                        clock_gettime(CLOCK_MONOTONIC, &last_recv);
                }

                clock_gettime(CLOCK_MONOTONIC, &actual_time);
                if(last_recv.tv_sec < actual_time.tv_sec - MAX_WAIT){ 
                        generate_token(send_buffer);
                        clock_gettime(CLOCK_MONOTONIC, &last_recv);
                        printf("Token regénéré\n");
                        send_sockg(sockg, send_buffer);
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

                // traitement des données reçu 
                // si token libre -> Check new hosts FILE -> sinon check besoin du comm -> sinon faire passer
                if(data_recv){
                        char token[TOKEN_SIZE+1];
                        memcpy(token, recv_buffer, TOKEN_SIZE);
                        token[TOKEN_SIZE] = '\0';
                        increment_token(&token);
                        memcpy(send_buffer, token, TOKEN_SIZE); 
                        printf("Envoie: %s\n", send_buffer);
                        send_sockg(sockg, send_buffer);
                } 
        }
        
        close(sockd);
        close(sockg);

        exit(0);
        return 0;
}
