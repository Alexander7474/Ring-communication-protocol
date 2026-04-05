#include <arpa/inet.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "../common/config.h"
#include "../common/error.h"
#include "../common/ring_buffer.h"
#include "driver.h"

// server qui se parle a lui meme avec son fils (bizarre)
int
main(int argc, char *argv[])
{
	if (argc != 2)
		FATAL("Nombre d'arguments incorrect.\nUsage: ./driver address");

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

	newsockd = socket(AF_INET, SOCK_STREAM, 0); // Création de la socket

	int opt = 1;
	setsockopt(newsockd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	cc = bind(newsockd, (struct sockaddr *)&servd, sizeof(servd));
	if (cc == -1)
		FATAL("bind"); // Erreur à l'attachement

	cc = listen(newsockd, 5);
	if (cc == -1)
		FATAL("listen");

	fd_set readfds;
	while (1) {
		int data_recv = 0;

		FD_ZERO(&readfds);
		FD_SET(newsockd, &readfds);
		max_sd = newsockd;

		if (sockd > 0)
			FD_SET(sockd, &readfds);

		if (sockd > newsockd)
			max_sd = sockd;

		struct timeval timeout = { 1, 0 };

		int activity =
		        select(max_sd + 1, &readfds, NULL, NULL, &timeout);

		if (activity < 0)
			FATAL("activity");

		if (FD_ISSET(newsockd, &readfds)) {
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

		if (FD_ISSET(sockd, &readfds)) {
			receiv_sockd(sockd, recv_buffer);
			data_recv = 1;
			clock_gettime(CLOCK_MONOTONIC, &last_recv);
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

		// connection de sockg
		struct hostent *hp;
		struct sockaddr_in servg;

		// connection de sockg avec address en argument
		if (sockg <= 0) {
			hp = gethostbyname(argv[1]);
			if (hp == NULL)
				FATAL("gethostbyname"); // Toujours tester pour
				                        // éviter d'accumuler
				                        // les erreurs

			servg.sin_family = AF_INET;
			servg.sin_port = htons(PORT);
			bcopy(hp->h_addr, (char *)&servg.sin_addr,
			      hp->h_length);

			sockg = socket(AF_INET, SOCK_STREAM, 0);

			if (connect(sockg, (struct sockaddr *)&servg,
			            sizeof(servg)) == -1) {
				FATAL("Connect socket");
			}
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

		if (!data_recv)
			continue; // Aucune données recu

			// traitement des données reçu
			// si token libre -> Check new hosts FILE -> sinon check
			// besoin du comm -> sinon faire passer

#ifdef DEBUG
		dump_message(recv_buffer);
#endif
		char flag = get_flag(recv_buffer);
		if (flag == 'f') {
			if (!is_rg_buff_empty(&waiting_hosts)) {
				if (is_own_addr(get_sockaddr(sockg))) {
					close(sockd);
					close(sockg);

					pop_rg_buff(&waiting_hosts,
					            &sockd); // recup de premier
					                     // host de la file
					unsigned long nsockd_addr =
					        get_sockaddr(sockd);
					sockg = socket(AF_INET, SOCK_STREAM, 0);
					connect_sock(nsockd_addr, sockg);
					continue;
				}

				unsigned long old_sockd_addr =
				        get_sockaddr(sockd);

				shutdown(sockd, SHUT_WR);
				close(sockd);
				pop_rg_buff(&waiting_hosts,
				            &sockd); // recup de premier host de
				                     // la file
				unsigned long new_sockd_addr =
				        get_sockaddr(sockd);
				// envoie du message 'c'
				send_connection_message(sockg, old_sockd_addr,
				                        new_sockd_addr,
				                        recv_buffer);
			} else {
				int cc = skip_buffer(sockg, recv_buffer);
				if (cc <= 0)
					FATAL("skip_buffer");
			}

			continue;
		}

		// si packet non destiné à la machine
		if (!is_own_addr(get_addr(recv_buffer))) {
			int cc = skip_buffer(sockg, recv_buffer);
			if (cc <= 0)
				FATAL("skip_buffer");
			continue;
		}

		// si detiné à la machine tester les flag et agir
		switch (flag) {
		case 'c':
			// copy de l'addresse de connection
			unsigned long addr;
			memcpy(&addr, recv_buffer + DATA_OFFSET,
			       sizeof(unsigned long));

			close(sockg);
			sockg = socket(AF_INET, SOCK_STREAM, 0);
			connect_sock(addr, sockg);

			break;
		default:
			FATAL("Unknow flag wtf\n");
		}
	}

	close(sockd);
	close(sockg);

	exit(0);
	return 0;
}
