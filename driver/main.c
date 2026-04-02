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
        rg_buff_set(&waiting_hosts);

        // gestion du temps
        struct timespec last_recv;
        struct timespec actual_time;
        clock_gettime(CLOCK_MONOTONIC, &last_recv);

        // socket d'écoute
        servd.sin_family = AF_INET;  
        servd.sin_port = htons(PORT);     
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

                struct timeval timeout = {1, 0};

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
                                printf("Première reception de sockd\n");
                                sockd = accept(newsockd, (struct sockaddr *)&servd, (socklen_t *) &lenpservd);
                        }
                }


                if(FD_ISSET(sockd, &readfds)){
                        receiv_sockd(sockd, recv_buffer);
                        data_recv = 1;
                        clock_gettime(CLOCK_MONOTONIC, &last_recv);
                }

                clock_gettime(CLOCK_MONOTONIC, &actual_time);
                if(last_recv.tv_sec < actual_time.tv_sec - MAX_WAIT){ 
                        generate_message_buffer(send_buffer);
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

#ifdef DEBUG
                printf("Debug start ----------------\n");
                printf("Taille ring_buffer: %d\n", rg_buff_size(&waiting_hosts));
                printf("Debug end ------------------\n");
#endif
#ifdef SLOW_MODE
                sleep(1);
#endif

                if(!data_recv)
                        continue; // Aucune données recu
                
                // traitement des données reçu 
                // si token libre -> Check new hosts FILE -> sinon check besoin du comm -> sinon faire passer

                dump_message(recv_buffer);
                char flag = get_flag(recv_buffer);
                // Do things
                switch(flag){
                case 'f':
                        if(!is_rg_buff_empty(&waiting_hosts)){
                                if(is_loopback_sock(sockg)){
                                        close(sockd);
                                        close(sockg);

                                        // TODO -- change cette procedure de merde
                                        pop_rg_buff(&waiting_hosts, &sockd); // recup de premier host de la file
                                        struct sockaddr_storage peer_addr;
                                        socklen_t len = sizeof(peer_addr);
                                        getpeername(sockd, (struct sockaddr *)&peer_addr, &len);
                                        struct sockaddr_in *peer_in = (struct sockaddr_in *)&peer_addr;
                                        char ip_str[INET_ADDRSTRLEN];
                                        inet_ntop(AF_INET, &peer_in->sin_addr, ip_str, sizeof(ip_str));
                                        struct sockaddr_in new_addr;
                                        memset(&new_addr, 0, sizeof(new_addr));
                                        new_addr.sin_family = AF_INET;
                                        new_addr.sin_port = htons(PORT); // same port as the original connection
                                        inet_pton(AF_INET, ip_str, &new_addr.sin_addr);
                                        sockg = socket(AF_INET, SOCK_STREAM, 0);
                                        if (connect(sockg, (struct sockaddr *)&new_addr, sizeof(new_addr)) == -1) {
                                                FATAL("Reconnect socket");
                                        }else{
                                                printf("Sockg reconnecté\n");
                                        }
 
                                        break;
                                }

                                shutdown(sockd, SHUT_WR);
                                close(sockd);
                                pop_rg_buff(&waiting_hosts, &sockd); // recup de premier host de la file
                                // envoie du message 'c'
                                send_connection_message(sockg, sockd, recv_buffer);
                        }else{
                                int cc = skip_buffer(sockg, recv_buffer);
                                if(cc <= 0) FATAL("skip_buffer");
                        }
                        break;
                case 'c':
                        // test si sockg est fermé (oui = changement de sockg)
                        char buf;
                        ssize_t n = recv(sockg, &buf, 1, MSG_PEEK | MSG_DONTWAIT);
                        if (n == 0) {

                        }else{
                                int cc = skip_buffer(sockg, recv_buffer);
                                if(cc <= 0) FATAL("skip_buffer");
                        }
                default:
                        FATAL("Unknow flag wtf\n");
                }

        }
        
        close(sockd);
        close(sockg);

        exit(0);
        return 0;
}
