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

int main(int argc, char *argv[])
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
        struct timespec last_recv, actual_time;
        clock_gettime(CLOCK_MONOTONIC, &last_recv);

        // socket d'ecoute unix comm
        newsockcomm = init_unix_listenner(&servcomm);

        // socket d'écoute réseau
        newsockd = init_inet_listenner(&servd);

        fd_set readfds;
        while (1) {
                int data_recv = 0;
                int data_read = 0;

                setup_fdset(&readfds, &max_sd, newsockd, newsockcomm, sockd,
                            sockcomm);

                struct timeval timeout = { 1, 0 };
                int activity =
                        select(max_sd + 1, &readfds, NULL, NULL, &timeout);
                if (activity < 0)
                        FATAL("activity");

                if (sockcomm < 0)
                        comm_request = 0;

                //
                // Gestion des nouveaux arrivants sur comm et d
                //

                if (newsockd > 0 && FD_ISSET(newsockd, &readfds)) {
#ifdef DEBUG
                        printf("Sockd nouvelle connection !\n");
#endif
                        handle_new_inet_conn(newsockd, &servd, &sockd,
                                             &waiting_hosts);
                }

                if (newsockcomm > 0 && sockcomm <= 0 &&
                    FD_ISSET(newsockcomm, &readfds)) {
#ifdef DEBUG
                        printf("Sockcomm nouvelle connection !\n");
#endif
                        handle_new_unix_conn(newsockcomm, &servcomm, &sockcomm);
                }

                //
                // Reception des données sur les sockets d et comm
                //

                if (sockd > 0 && FD_ISSET(sockd, &readfds))
                        data_recv = handle_recv_sockd(&sockd, sockg,
                                                      recv_buffer, &last_recv);

                if (sockcomm > 0 && FD_ISSET(sockcomm, &readfds))
                        data_read =
                                handle_read_sockcomm(&sockcomm, read_buffer);

                clock_gettime(CLOCK_MONOTONIC, &actual_time);
                check_token_timeout(sockg, send_buffer, &last_recv,
                                    &actual_time);

                //
                // Connection de sockg
                //

                if (sockg <= 0) {
                        struct hostent *hp = gethostbyname(argv[1]);
                        if (hp == NULL)
                                FATAL("gethostbyname");

                        uint32_t addr;
                        bcopy(hp->h_addr, (uint32_t *)&addr, hp->h_length);
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

                if (flag == 'f') {
                        if (handle_flag_free(&sockd, &sockg, &sockcomm,
                                             &comm_request, &waiting_hosts,
                                             recv_buffer, send_buffer))
                                continue;
                        continue;
                }

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
                        cc = skip_buffer(sockg, recv_buffer);
                        if (cc <= 0)
                                FATAL("skip_buffer");
                        continue;
                }

                // si detiné à la machine tester les flag et agir
                handle_flag_local(flag, &sockg, &sockcomm, recv_buffer,
                                  send_buffer);
        }

        close(sockd);
        close(sockg);
        close(sockcomm);
        exit(0);
        return 0;
}
