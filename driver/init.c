#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "init.h"
#include "../common/config.h"
#include "../common/error.h"

int 
init_inet_listenner(struct sockaddr_in *servd)
{
        servd->sin_family = AF_INET;
        servd->sin_port = htons(PORT);
        servd->sin_addr.s_addr = htonl(INADDR_ANY);

        int newsockd = socket(AF_INET, SOCK_STREAM, 0); // Création de la socket

        int opt = 1;
        setsockopt(newsockd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        int cc = bind(newsockd, (struct sockaddr *)servd, sizeof(*servd));
        if (cc == -1)
                FATAL("bind network"); // Erreur à l'attachement

        cc = listen(newsockd, 5);
        if (cc == -1)
                FATAL("listen network");

        return newsockd;
}

int 
init_unix_listenner(struct sockaddr_un *servcomm)
{
        char cmd[64] = "rm ";
        strcat(cmd, UNIX_SOCKET_PATH);
        int cc = system(cmd);
        if(cc < 0)
                printf("Error while deleting localsock.sock");
        servcomm->sun_family = AF_UNIX;
        const char *socket_path = getenv("DRIVER_SOCKET_PATH");
        if (!socket_path)
                socket_path = UNIX_SOCKET_PATH;
        strncpy(servcomm->sun_path, socket_path, sizeof(servcomm->sun_path) - 1);

        int newsockcomm = socket(AF_UNIX, SOCK_STREAM, 0); // Création de la socket
        cc = bind(newsockcomm, (struct sockaddr *)servcomm, sizeof(*servcomm));
        if (cc == -1)
                FATAL("bind unix"); // Erreur à l'attachement

        cc = listen(newsockcomm, 5);
        if (cc == -1)
                FATAL("listen unix");

        return newsockcomm;
}
