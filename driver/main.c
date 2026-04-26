#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include "../common/config.h"
#include "../common/error.h"
#include "../common/ring_buffer.h"
#include "driver.h"
#include "init.h"

int
main(int argc, char *argv[])
{
        if (argc != 2)
                FATAL("Nombre d'arguments incorrect.\nUsage: ./driver address");

        char recv_buffer[SMAX];
        char read_buffer[SMAX];
        char send_buffer[SMAX];
        int cc, newsockd, newsockcomm, max_sd;
        struct sockaddr_in servd;
        struct sockaddr_un servcomm;

        int comm_request = 0;

        int sockd = 0;
        int sockg = 0;
        int sockcomm = 0;

        struct ring_buffer waiting_hosts;
        rg_buff_set(&waiting_hosts);

        // gestion du temps
        struct timespec last_recv;
        struct timespec actual_time;
        clock_gettime(CLOCK_MONOTONIC, &last_recv);

        // socket d'ecoute unix comm
        newsockcomm = init_unix_listenner(&servcomm);

        // socket d'écoute réseau
        newsockd = init_inet_listenner(&servd);

        fd_set readfds;
        while (1) {
                int data_recv = 0;
                int data_read = 0;

                FD_ZERO(&readfds);
                FD_SET(newsockd, &readfds);
                FD_SET(newsockcomm, &readfds);

                if (sockd > 0)
                        FD_SET(sockd, &readfds);

                if (sockcomm > 0)
                        FD_SET(sockcomm, &readfds);

                max_sd = newsockd;
                if (sockd > max_sd)
                        max_sd = sockd;
                if (sockcomm > max_sd)
                        max_sd = sockcomm;
                if (newsockcomm > max_sd)
                        max_sd = newsockcomm;

                struct timeval timeout = { 1, 0 };

                int activity =
                        select(max_sd + 1, &readfds, NULL, NULL, &timeout);

                if (activity < 0)
                        FATAL("activity");

                if(sockcomm < 0)
                        comm_request = 0;

                //
                // Gestion des nouveaux arrivants sur comm et d
                //

                if (newsockd > 0 && FD_ISSET(newsockd, &readfds)) {
#ifdef DEBUG
                        printf("Sockd nouvelle connection !\n");
#endif
                        int lenpservd = sizeof(servd);

                        if (sockd > 0) {
                                int tmp_socket = accept(
                                        newsockd, (struct sockaddr *)&servd,
                                        (socklen_t *)&lenpservd);
                                push_rg_buff(&waiting_hosts, tmp_socket);
                        } else {
#ifdef DEBUG
                                printf("Première reception de sockd\n");
#endif
                                sockd = accept(newsockd,
                                               (struct sockaddr *)&servd,
                                               (socklen_t *)&lenpservd);
                        }
                }

                if (newsockcomm > 0 && sockcomm <= 0 && FD_ISSET(newsockcomm, &readfds)) {
#ifdef DEBUG
                        printf("Sockcomm nouvelle connection !\n");
#endif
                        int lenpservcomm = sizeof(servcomm);
                        sockcomm = accept(newsockcomm,
                                          (struct sockaddr *)&servcomm,
                                          (socklen_t *)&lenpservcomm);
                }

                //
                // Reception des données sur les sockets d et comm
                //

                if (sockd > 0 && FD_ISSET(sockd, &readfds)) {
                        cc = recv(sockd, recv_buffer, sizeof(char) * SMAX, 0);
                        if(cc <= 0){
                                // envoie demande de connection en broadcast
                                send_connection_message(sockg, inet_addr(BROADCAST_ADDR),
                                                        get_sock_own_addr(sockd), recv_buffer);
                                close(sockd);
                                sockd = -1;
                        }
                        else{
                                data_recv = 1;
                                clock_gettime(CLOCK_MONOTONIC, &last_recv);
                        }
                }

                if (sockcomm > 0 && FD_ISSET(sockcomm, &readfds)) {
                        cc = read(sockcomm, read_buffer, sizeof(char) * SMAX);
                        if (cc <= 0){
                                close(sockcomm);
                                sockcomm = -1;
                        }else{
                                data_read = 1;
                        }
                }

                clock_gettime(CLOCK_MONOTONIC, &actual_time);
                if (last_recv.tv_sec < actual_time.tv_sec - MAX_WAIT) {
                        generate_message_buffer(send_buffer);
                        clock_gettime(CLOCK_MONOTONIC, &last_recv);
#ifdef DEBUG
                        printf("Token regénéré\n");
#endif
                        send_sockg(sockg, send_buffer);
                }

                //
                // Connection de sockg
                //

                if (sockg <= 0) {
                        struct hostent *hp;
                        hp = gethostbyname(argv[1]);
                        if (hp == NULL)
                                FATAL("gethostbyname"); 

                        uint32_t addr;
                        bcopy(hp->h_addr, (uint32_t *)&addr,
                              hp->h_length);

                        sockg = socket(AF_INET, SOCK_STREAM, 0);
                        connect_sock(addr, sockg);
#ifdef DEBUG
                        printf("Client prêt !\n");
#endif
                }

#ifdef DEBUG
                printf("Debug start ----------------\n");
                printf("Taille ring_buffer: %d\n",
                       rg_buff_size(&waiting_hosts));
                printf("Debug end ------------------\n");
#endif
#ifdef SLOW_MODE
                sleep(1);
#endif

                //
                // Traitement des données recu dans sockd et sockcomm
                //
                // traitement des données reçu
                // si token libre -> Check new hosts FILE -> sinon check
                // besoin du comm -> sinon faire passer

                char flag;
                if (!data_read)
                        goto recv_buffer_process; // Aucune données recu dans
                                                  // read_buffer

#ifdef DEBUG
                dump_message(read_buffer);
#endif
                flag = get_flag(read_buffer);
                if (flag == 'n')
                        comm_request++;

        recv_buffer_process:
                if (!data_recv)
                        continue; // Aucune données recu dans recv_buffer

#ifdef DEBUG
                dump_message(recv_buffer);
#endif
                flag = get_flag(recv_buffer);
                if (flag != 'f')
                        goto flag_process;

                if (!is_rg_buff_empty(&waiting_hosts)) {
                        if (is_own_addr(get_sock_remote_addr(sockg))) {
                                close(sockd);
                                close(sockg);

                                pop_rg_buff(&waiting_hosts,
                                            &sockd); // recup de premier
                                                     // host de la file
                                uint32_t nsockd_addr = get_sock_remote_addr(sockd);
                                sockg = socket(AF_INET, SOCK_STREAM, 0);
                                connect_sock(nsockd_addr, sockg);
                                continue;
                        }

                        uint32_t old_sockd_addr = get_sock_remote_addr(sockd);

                        shutdown(sockd, SHUT_WR);
                        close(sockd);
                        pop_rg_buff(&waiting_hosts,
                                    &sockd); // recup de premier host de
                                             // la file
                        uint32_t new_sockd_addr = get_sock_remote_addr(sockd);
                        // envoie du message 'c'
                        send_connection_message(sockg, old_sockd_addr,
                                                new_sockd_addr, recv_buffer);
                } else if (comm_request >= 1 && sockcomm > 0) {
                        send_sockg(sockcomm, recv_buffer);
                        receiv_sockd(sockcomm, recv_buffer);
                        skip_buffer(sockg, recv_buffer);
                        comm_request--;
                } else {
                        int cc = skip_buffer(sockg, recv_buffer);
                        if (cc <= 0)
                                FATAL("skip_buffer");
                }

                continue;

        flag_process:
                // si packet non destiné à la machine
                if (!is_own_addr(get_addr(recv_buffer)) &&
                    !is_diffusion_addr(get_addr(recv_buffer))) {
                        // si packet envoyer par le driver lui même
                        if (is_own_addr(get_src_addr(recv_buffer))) {
                                generate_message_buffer(send_buffer);
                                clock_gettime(CLOCK_MONOTONIC, &last_recv);
#ifdef DEBUG
                                printf("Token regénéré\n");
#endif
                                send_sockg(sockg, send_buffer);
                                continue;
                        }

                        int cc = skip_buffer(sockg, recv_buffer);
                        if (cc <= 0)
                                FATAL("skip_buffer");
                        continue;
                }

                // si detiné à la machine tester les flag et agir
                switch (flag) {
                case 'c':
                        // Si le message de connexion est une diffusion je vérifie 
                        // que sockg est toujours actif.
                        // Le message de connexion avec address de diffusion fait 
                        // partie de la procedure de reconnexion de l'anneau en 
                        // cas de perte d'un host.
                        // Si sockg en vie -> c'est au prochain driver de vérifier
                        // si son sockg a crash.
                        if(is_diffusion_addr(get_addr(recv_buffer)) && inet_socket_healthcheck(sockg))
                                break;

                        // Connexion de sockg à la nouvelle address 
                        uint32_t addr;
                        memcpy(&addr, recv_buffer + DATA_OFFSET,
                               sizeof(uint32_t));

                        close(sockg);
                        sockg = socket(AF_INET, SOCK_STREAM, 0);
                        connect_sock(addr, sockg);

                        break;
                case 'u':
                case 'a':
                case 'e':
                case 'i':
                case 'h':
                case 's':
                        if(sockcomm > 0){
                                send_sockg(sockcomm, recv_buffer);
                                receiv_sockd(sockcomm, recv_buffer);
                                skip_buffer(sockg, recv_buffer);
                        }
                
                        break;
                default:
                        FATAL("Unknow flag wtf\n");
                        break;
                }
        }

        close(sockd);
        close(sockg);
        close(sockcomm);

        exit(0);
        return 0;
}
