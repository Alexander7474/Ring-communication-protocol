#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "../common/config.h"
#include "../common/error.h"
#include "init.h"

/**
 * init_inet_listenner - Initialise et met en écoute un socket TCP
 *
 * Crée un socket TCP sur INADDR_ANY:PORT avec l'option SO_REUSEADDR,
 * l'attache et le met en écoute.
 *
 * @servd: structure sockaddr_in à initialiser
 *
 * Return: descripteur du socket d'écoute
 */
int
init_inet_listenner(struct sockaddr_in *servd)
{
        servd->sin_family = AF_INET;
        servd->sin_port = htons(PORT);
        servd->sin_addr.s_addr = htonl(INADDR_ANY);
        int newsockd = socket(AF_INET, SOCK_STREAM, 0);
        int opt = 1;
        setsockopt(newsockd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        int cc = bind(newsockd, (struct sockaddr *)servd, sizeof(*servd));
        if (cc == -1)
                FATAL("bind network");
        cc = listen(newsockd, 5);
        if (cc == -1)
                FATAL("listen network");
        return newsockd;
}

/**
 * init_unix_listenner - Initialise et met en écoute un socket Unix
 *
 * Supprime l'éventuel socket Unix résiduel, crée un socket AF_UNIX,
 * l'attache et le met en écoute. Le chemin du socket est lu depuis
 * la variable d'environnement DRIVER_SOCKET_PATH si définie, sinon
 * UNIX_SOCKET_PATH est utilisé.
 *
 * @servcomm: structure sockaddr_un à initialiser
 *
 * Return: descripteur du socket d'écoute
 */
int
init_unix_listenner(struct sockaddr_un *servcomm)
{
        char cmd[64] = "rm ";
        strcat(cmd, UNIX_SOCKET_PATH);
        int cc = system(cmd);
        if (cc < 0)
                printf("Error while deleting localsock.sock");
        servcomm->sun_family = AF_UNIX;
        const char *socket_path = getenv("DRIVER_SOCKET_PATH");
        if (!socket_path)
                socket_path = UNIX_SOCKET_PATH;
        strncpy(servcomm->sun_path, socket_path,
                sizeof(servcomm->sun_path) - 1);
        int newsockcomm = socket(AF_UNIX, SOCK_STREAM, 0);
        cc = bind(newsockcomm, (struct sockaddr *)servcomm, sizeof(*servcomm));
        if (cc == -1)
                FATAL("bind unix");
        cc = listen(newsockcomm, 5);
        if (cc == -1)
                FATAL("listen unix");
        return newsockcomm;
}
