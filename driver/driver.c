#include <arpa/inet.h>  /* inet_addr()                        */
#include <errno.h>      /* errno, EAGAIN, EWOULDBLOCK         */
#include <ifaddrs.h>    /* getifaddrs(), freeifaddrs()        */
#include <stdio.h>      /* printf()                           */
#include <stdlib.h>     /* strtol()                           */
#include <string.h>     /* memset(), memcpy(), snprintf()     */
#include <sys/socket.h> /* socket(), connect(), recv()...     */
#include <unistd.h>     /* read(), write(), close()           */

#include "../common/error.h" /* FATAL() */
#include "driver.h"

/**
 * send_sockg - Envoie un message sur un socket
 * @sock: socket d'envoi
 * @msg: buffer de taille SMAX à envoyer
 */
void send_sockg(int sock, char *msg)
{
        int cc = write(sock, msg, sizeof(char) * SMAX);
        if (cc <= 0)
                FATAL("Send msgs write");
}

/**
 * receiv_sockd - Reçoit un message depuis un socket
 * @sock: socket de réception
 * @msg: buffer de taille SMAX à remplir
 */
void receiv_sockd(int sock, char *msg)
{
        int cc = read(sock, msg, sizeof(char) * SMAX);
        if (cc <= 0)
                FATAL("Receiv message");
}

/**
 * generate_message_buffer - Génère un buffer de token libre (flag 'f')
 * @buffer: buffer de taille SMAX à initialiser
 */
void generate_message_buffer(char *buffer)
{
        char new_buffer[SMAX];
        memset(new_buffer, '\0', sizeof(new_buffer));
        new_buffer[0] = 'f';
        memcpy(buffer, new_buffer, SMAX);
}

/**
 * increment_token - Incrémente la valeur hexadécimale du token dans le buffer
 * @buffer: buffer structuré contenant le token à incrémenter
 */
void increment_token(char *buffer)
{
        char token[TOKEN_SIZE + 1] = { '\0' };
        get_token(buffer, token);
        int value = (int)strtol(token, NULL, 16);
        value++;
        snprintf(token, sizeof(char) * TOKEN_SIZE + 1, TOKEN_FMT, value);
        memcpy(buffer + 1, token, TOKEN_SIZE);
}

/**
 * get_flag - Retourne le flag du message
 * @buffer: buffer structuré du message
 *
 * Return: caractère flag en première position du buffer
 */
char get_flag(char *buffer)
{
        return buffer[0];
}

/**
 * get_data - Copie le champ data du buffer dans @data
 * @buffer: buffer structuré du message
 * @data: buffer de taille DATA_SIZE+1 à remplir
 */
void get_data(char *buffer, char *data)
{
        memcpy(data, buffer + DATA_OFFSET, DATA_SIZE);
}

/**
 * get_addr - Retourne l'adresse de destination du message
 * @buffer: buffer structuré du message
 *
 * Return: adresse de destination au format uint32_t
 */
uint32_t get_addr(char *buffer)
{
        uint32_t addr;
        memcpy(&addr, buffer + ADDR_OFFSET, sizeof(uint32_t));
        return addr;
}

/**
 * get_src_addr - Retourne l'adresse source du message
 * @buffer: buffer structuré du message
 *
 * Return: adresse source au format uint32_t
 */
uint32_t get_src_addr(char *buffer)
{
        uint32_t addr;
        memcpy(&addr, buffer + ADDR_SRC_OFFSET, sizeof(uint32_t));
        return addr;
}

/**
 * get_token - Copie le token du buffer dans @token
 * @buffer: buffer structuré du message
 * @token: buffer de taille TOKEN_SIZE+1 à remplir
 */
void get_token(char *buffer, char *token)
{
        memcpy(token, buffer + TOKEN_OFFSET, TOKEN_SIZE);
}

/**
 * set_flag - Positionne le flag du message
 * @flag: caractère flag à positionner
 * @buffer: buffer structuré du message
 */
void set_flag(char flag, char *buffer)
{
        buffer[0] = flag;
}

/**
 * set_addr - Positionne l'adresse de destination dans le buffer
 * @addr: adresse réseau au format uint32_t
 * @buffer: buffer structuré du message
 */
void set_addr(uint32_t addr, char *buffer)
{
        memcpy(buffer + ADDR_OFFSET, &addr, sizeof(uint32_t));
}

/**
 * set_src_addr - Positionne l'adresse source dans le buffer
 * @addr: adresse réseau au format uint32_t
 * @buffer: buffer structuré du message
 */
void set_src_addr(uint32_t addr, char *buffer)
{
        memcpy(buffer + ADDR_SRC_OFFSET, &addr, sizeof(uint32_t));
}

/**
 * dump_message - Affiche le contenu d'un message sur la sortie standard
 * @buffer: buffer structuré du message à afficher
 */
void dump_message(char *buffer)
{
        char token[TOKEN_SIZE + 1], data[DATA_SIZE + 1];
        get_token(buffer, token);
        uint32_t addr = get_addr(buffer);
        uint32_t src_addr = get_src_addr(buffer);
        get_data(buffer, data);
        token[TOKEN_SIZE] = '\0';
        data[DATA_SIZE] = '\0';
        printf("Message dump start ----------------\n");
        printf("Message: %s\n", buffer);
        printf("Flag: %c\n", get_flag(buffer));
        printf("Token: %s\n", token);
        printf("Source address: %u\n", src_addr);
        printf("Destination address: %u\n", addr);
        printf("Data: %s\n", data);
        printf("Message dump end ------------------\n");
}

/**
 * skip_buffer - Incrémente le token et fait suivre le message sur @sock
 * @sock: socket sur lequel renvoyer le message
 * @buffer: buffer structuré du message reçu
 *
 * Return: valeur retournée par write(), <= 0 en cas d'erreur
 */
int skip_buffer(int sock, char *buffer)
{
        char send_buffer[SMAX];

        memcpy(send_buffer, buffer, SMAX);
        increment_token(send_buffer);
        return write(sock, send_buffer, sizeof(send_buffer));
}

/**
 * is_own_addr - Teste si @addr correspond à une adresse locale de la machine
 * @addr: adresse réseau au format uint32_t à tester
 *
 * Return: 1 si @addr est une adresse locale, 0 sinon
 */
int is_own_addr(uint32_t addr)
{
        struct ifaddrs *ifaddr, *ifa;
        if (getifaddrs(&ifaddr) == -1)
                FATAL("getifaddrs");

        int found = 0;
        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
                if (!ifa->ifa_addr)
                        continue;
                if (ifa->ifa_addr->sa_family != AF_INET)
                        continue;

                struct sockaddr_in *local = (struct sockaddr_in *)ifa->ifa_addr;

                if (addr == local->sin_addr.s_addr) {
                        found = 1;
                        break;
                }
        }

        freeifaddrs(ifaddr);
        return found;
}

/**
 * send_connection_message - Envoie un message de connexion marqué 'c'
 *
 * Envoie un message à @dest_addr pour qu'il connecte son sockg à
 * @new_host_addr. Le buffer doit contenir un token libre (flag 'f').
 *
 * @sockg: socket gauche pour l'envoi
 * @dest_addr: adresse de destination
 * @new_host_addr: adresse du nouvel host à intégrer dans l'anneau
 * @buffer: buffer structuré contenant un flag free et le token
 *
 * Return: valeur retournée par write(), -1 si le flag n'est pas 'f'
 */
int send_connection_message(int sockg, uint32_t dest_addr,
                            uint32_t new_host_addr, char *buffer)
{
        if (get_flag(buffer) != 'f')
                return -1;

        char send_buffer[SMAX];
        memcpy(send_buffer, buffer, SMAX);

        memcpy(send_buffer + DATA_OFFSET, &new_host_addr, sizeof(uint32_t));
        increment_token(send_buffer);
        set_flag('c', send_buffer);
        set_addr(dest_addr, send_buffer);

        return write(sockg, send_buffer, sizeof(send_buffer));
}

/**
 * get_sock_remote_addr - Retourne l'adresse de la machine connectée au socket
 * @sock: socket dont on veut l'adresse distante
 *
 * Return: adresse distante au format sockaddr_in.sin_addr.s_addr
 */
uint32_t get_sock_remote_addr(int sock)
{
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);

        if (getpeername(sock, (struct sockaddr *)&addr, &len) == -1)
                FATAL("getpeername");

        return addr.sin_addr.s_addr;
}

/**
 * get_sock_own_addr - Retourne l'adresse locale utilisée sur le socket
 * @sock: socket dont on veut l'adresse locale
 *
 * Return: adresse locale au format sockaddr_in.sin_addr.s_addr
 */
uint32_t get_sock_own_addr(int sock)
{
        struct sockaddr_in addr;
        socklen_t len = sizeof(addr);

        if (getsockname(sock, (struct sockaddr *)&addr, &len) == -1)
                FATAL("getsockname");

        return addr.sin_addr.s_addr;
}

/**
 * connect_sock - Connecte un socket à une adresse sur le port configuré
 * @addr: adresse réseau au format little endian uint32_t
 * @sock: socket à connecter
 */
void connect_sock(uint32_t addr, int sock)
{
        struct sockaddr_in new_addr;
        memset(&new_addr, 0, sizeof(new_addr));
        new_addr.sin_family = AF_INET;
        new_addr.sin_port = htons(PORT);
        new_addr.sin_addr.s_addr = addr;
        connect(sock, (struct sockaddr *)&new_addr, sizeof(new_addr));
}

/**
 * is_diffusion_addr - Teste si @addr est l'adresse de diffusion configurée
 * @addr: adresse réseau au format uint32_t à tester
 *
 * Return: 1 si @addr est l'adresse de diffusion, 0 sinon
 */
int is_diffusion_addr(uint32_t addr)
{
        if (inet_addr(BROADCAST_ADDR) == addr)
                return 1;
        else
                return 0;
}

/**
 * inet_socket_healthcheck - Vérifie si un socket TCP est toujours actif
 * @sock: socket à tester
 *
 * Utilise MSG_PEEK | MSG_DONTWAIT pour tester l'état de la connexion
 * sans consommer les données disponibles.
 *
 * Return: 1 si le socket est en vie, 0 si la connexion est rompue
 */
int inet_socket_healthcheck(int sock)
{
        char buf;
        ssize_t n = recv(sock, &buf, 1, MSG_PEEK | MSG_DONTWAIT);

        if (n > 0) {
                return 1;
        } else if (n == 0) {
                return 0;
        } else {
                if (errno == EAGAIN || errno == EWOULDBLOCK)
                        return 1;
                else
                        return 0;
        }
}

/**
 * setup_fdset - Initialise le fd_set et calcule max_sd
 * @readfds: fd_set à initialiser
 * @max_sd: pointeur vers le descripteur maximum, mis à jour par la fonction
 * @newsockd: socket d'écoute réseau
 * @newsockcomm: socket d'écoute Unix
 * @sockd: socket de données réseau actif, ignoré si <= 0
 * @sockcomm: socket de communication Unix actif, ignoré si <= 0
 */
void setup_fdset(fd_set *readfds, int *max_sd, int newsockd, int newsockcomm,
                 int sockd, int sockcomm)
{
        FD_ZERO(readfds);
        FD_SET(newsockd, readfds);
        FD_SET(newsockcomm, readfds);

        if (sockd > 0)
                FD_SET(sockd, readfds);
        if (sockcomm > 0)
                FD_SET(sockcomm, readfds);

        *max_sd = newsockd;
        if (sockd > *max_sd)
                *max_sd = sockd;
        if (sockcomm > *max_sd)
                *max_sd = sockcomm;
        if (newsockcomm > *max_sd)
                *max_sd = newsockcomm;
}

/**
 * handle_new_inet_conn - Accepte une nouvelle connexion réseau
 *
 * Si @sockd est déjà occupé, le nouveau socket est mis en file d'attente
 * dans @waiting_hosts. Sinon il devient le @sockd courant.
 *
 * @newsockd: socket d'écoute réseau
 * @servd: structure sockaddr_in du socket d'écoute
 * @sockd: pointeur vers le socket de données courant
 * @waiting_hosts: file d'attente des hosts en attente de connexion
 */
void handle_new_inet_conn(int newsockd, struct sockaddr_in *servd, int *sockd,
                          struct ring_buffer *waiting_hosts)
{
        int lenpservd = sizeof(*servd);

        if (*sockd > 0) {
                int tmp_socket = accept(newsockd, (struct sockaddr *)servd,
                                        (socklen_t *)&lenpservd);
                push_rg_buff(waiting_hosts, tmp_socket);
        } else {
#ifdef DEBUG
                printf("Première reception de sockd\n");
#endif
                *sockd = accept(newsockd, (struct sockaddr *)servd,
                                (socklen_t *)&lenpservd);
        }
}

/**
 * handle_new_unix_conn - Accepte une nouvelle connexion Unix comm
 *
 * N'accepte qu'une seule connexion à la fois ; à n'appeler que si
 * @sockcomm est inactif (<= 0).
 *
 * @newsockcomm: socket d'écoute Unix
 * @servcomm: structure sockaddr_un du socket d'écoute
 * @sockcomm: pointeur vers le socket comm courant
 */
void handle_new_unix_conn(int newsockcomm, struct sockaddr_un *servcomm,
                          int *sockcomm)
{
        int lenpservcomm = sizeof(*servcomm);
        *sockcomm = accept(newsockcomm, (struct sockaddr *)servcomm,
                           (socklen_t *)&lenpservcomm);
}

/**
 * handle_recv_sockd - Reçoit les données depuis sockd
 *
 * En cas de déconnexion ou d'erreur, ferme et invalide @sockd puis envoie
 * un message de reconnexion en broadcast.
 *
 * @sockd: pointeur vers le socket de données, mis à -1 si fermé
 * @sockg: socket gauche utilisé pour le broadcast de reconnexion
 * @recv_buffer: buffer de taille SMAX à remplir
 * @last_recv: horodatage mis à jour à chaque réception réussie
 *
 * Return: 1 si des données ont été reçues, 0 sinon
 */
int handle_recv_sockd(int *sockd, int sockg, char *recv_buffer,
                      struct timespec *last_recv)
{
        int cc = recv(*sockd, recv_buffer, sizeof(char) * SMAX, 0);

        if (cc <= 0) {
                // envoie demande de connection en broadcast
                send_connection_message(sockg, inet_addr(BROADCAST_ADDR),
                                        get_sock_own_addr(*sockd), recv_buffer);
                close(*sockd);
                *sockd = -1;
                return 0;
        }

        clock_gettime(CLOCK_MONOTONIC, last_recv);
        return 1;
}

/**
 * handle_read_sockcomm - Lit les données depuis sockcomm
 *
 * En cas de déconnexion ou d'erreur, ferme et invalide @sockcomm.
 *
 * @sockcomm: pointeur vers le socket comm, mis à -1 si fermé
 * @read_buffer: buffer de taille SMAX à remplir
 *
 * Return: 1 si des données ont été lues, 0 sinon
 */
int handle_read_sockcomm(int *sockcomm, char *read_buffer)
{
        int cc = read(*sockcomm, read_buffer, sizeof(char) * SMAX);

        if (cc <= 0) {
                close(*sockcomm);
                *sockcomm = -1;
                return 0;
        }

        return 1;
}

/**
 * check_token_timeout - Régénère le token si le délai MAX_WAIT est dépassé
 *
 * Compare @last_recv et @actual_time ; si la différence dépasse MAX_WAIT,
 * génère un nouveau token libre et le transmet sur @sockg.
 *
 * @sockg: socket gauche sur lequel envoyer le nouveau token
 * @send_buffer: buffer de taille SMAX utilisé pour la génération
 * @last_recv: horodatage de la dernière réception, mis à jour si timeout
 * @actual_time: horodatage courant
 */
void check_token_timeout(int sockg, char *send_buffer,
                         struct timespec *last_recv,
                         const struct timespec *actual_time)
{
        if (last_recv->tv_sec >= actual_time->tv_sec - MAX_WAIT)
                return;

        generate_message_buffer(send_buffer);
        clock_gettime(CLOCK_MONOTONIC, last_recv);
#ifdef DEBUG
        printf("Token regénéré\n");
#endif
        send_sockg(sockg, send_buffer);
}

/**
 * handle_flag_free - Gère la réception d'un token libre (flag 'f')
 *
 * Trois cas par ordre de priorité :
 *   1. Des hosts attendent dans @waiting_hosts -> intégration dans l'anneau
 *   2. Une requête comm est en attente         -> transmission au processus
 * local
 *   3. Aucun besoin local                      -> simple passage du token
 *
 * @sockd: pointeur vers le socket de données courant
 * @sockg: pointeur vers le socket gauche courant
 * @sockcomm: pointeur vers le socket comm courant
 * @comm_request: compteur de requêtes comm en attente
 * @waiting_hosts: file d'attente des hosts en attente de connexion
 * @recv_buffer: buffer contenant le message reçu
 * @send_buffer: buffer de taille SMAX pour les messages sortants
 *
 * Return: 1 si la boucle principale doit effectuer un continue, 0 sinon
 */
int handle_flag_free(int *sockd, int *sockg, int *sockcomm, int *comm_request,
                     struct ring_buffer *waiting_hosts, char *recv_buffer,
                     char *send_buffer)
{
        if (!is_rg_buff_empty(waiting_hosts)) {
                if (is_own_addr(get_sock_remote_addr(*sockg))) {
                        close(*sockd);
                        close(*sockg);

                        pop_rg_buff(waiting_hosts,
                                    sockd); // recup de premier host de la file
                        uint32_t nsockd_addr = get_sock_remote_addr(*sockd);
                        *sockg = socket(AF_INET, SOCK_STREAM, 0);
                        connect_sock(nsockd_addr, *sockg);
                        return 1;
                }

                uint32_t old_sockd_addr = get_sock_remote_addr(*sockd);

                shutdown(*sockd, SHUT_WR);
                close(*sockd);
                pop_rg_buff(waiting_hosts,
                            sockd); // recup de premier host de la file
                uint32_t new_sockd_addr = get_sock_remote_addr(*sockd);
                // envoie du message 'c'
                send_connection_message(*sockg, old_sockd_addr, new_sockd_addr,
                                        recv_buffer);

        } else if (*comm_request >= 1 && *sockcomm > 0 && inet_socket_healthcheck(*sockcomm)) {
                send_sockg(*sockcomm, recv_buffer);
                receiv_sockd(*sockcomm, recv_buffer);
                skip_buffer(*sockg, recv_buffer);
                (*comm_request)--;
        } else {
                int cc = skip_buffer(*sockg, recv_buffer);
                if (cc <= 0)
                        FATAL("skip_buffer");
        }

        return 0;
}

/**
 * handle_flag_local - Gère un paquet destiné à la machine locale
 *
 * Traite les flags suivants :
 *   'c' - reconnexion de sockg à une nouvelle adresse
 *   'u', 'a', 'e', 'i', 'h', 's' - transmission au processus local via comm
 *
 * À n'appeler que si le paquet est bien destiné à cette machine.
 *
 * @flag: flag du message reçu
 * @sockg: pointeur vers le socket gauche courant
 * @sockcomm: pointeur vers le socket comm courant
 * @recv_buffer: buffer contenant le message reçu
 * @send_buffer: buffer de taille SMAX pour les messages sortants
 */
void handle_flag_local(char flag, int *sockg, int *sockcomm, char *recv_buffer,
                       char *send_buffer)
{
        switch (flag) {
        case 'c':
                // Si le message de connexion est une diffusion je vérifie
                // que sockg est toujours actif.
                // Le message de connexion avec address de diffusion fait
                // partie de la procedure de reconnexion de l'anneau en
                // cas de perte d'un host.
                // Si sockg en vie -> c'est au prochain driver de vérifier
                // si son sockg a crash.
                if (is_diffusion_addr(get_addr(recv_buffer)) &&
                    inet_socket_healthcheck(*sockg))
                        break;

                // Connexion de sockg à la nouvelle address
                uint32_t addr;
                memcpy(&addr, recv_buffer + DATA_OFFSET, sizeof(uint32_t));
                close(*sockg);
                *sockg = socket(AF_INET, SOCK_STREAM, 0);
                connect_sock(addr, *sockg);
                break;
        case 'u':
        case 'a':
        case 'e':
        case 'i':
        case 'h':
        case 's':
                if (*sockcomm > 0 && inet_socket_healthcheck(*sockcomm)) {
                        send_sockg(*sockcomm, recv_buffer);
                        receiv_sockd(*sockcomm, recv_buffer);
                        skip_buffer(*sockg, recv_buffer);
                }else{
                        skip_buffer(*sockg, recv_buffer);
                }
                break;
        default:
                FATAL("Unknow flag wtf\n");
                break;
        }
}
