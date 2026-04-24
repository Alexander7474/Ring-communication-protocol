#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <ifaddrs.h>

#include "../common/config.h"
#include "../common/error.h"

// Retourne l'adresse IP de la machine source pour pouvoir savoir d'où vient le paquet et surtout permettre de s'arrêter à la bonne machine dans le cadre d'une diffusion
struct in_addr resoudre_source() {

    struct in_addr addr;
    const char * env_ip = getenv("COMM_IP");

    // Cas où on simule des drivers sur la même machine
    if(env_ip != NULL) {
        if(inet_pton(AF_INET, env_ip, &addr) != 1) FATAL("COMM_IP invalide");
        return addr;
    }

    // Cas où chaque driver est sur une machine indépendante
    struct ifaddrs *ifaddr, *ifa;
    addr.s_addr = 0;

    if(getifaddrs(&ifaddr) == -1) FATAL("getifaddrs");

    for(ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if(ifa->ifa_addr == NULL) continue;
        if(ifa->ifa_addr->sa_family == AF_INET && strcmp(ifa->ifa_name, "lo") != 0) {
            struct sockaddr_in * sin = (struct sockaddr_in *) ifa->ifa_addr;
            addr = sin->sin_addr;
            break;
        }
    }

    freeifaddrs(ifaddr);

    if(addr.s_addr == 0) FATAL("Aucune interface réseau trouvée !");
    return addr;

}

// Retourne l'adresse IP du destinataire pour gérer le cas où l'utilisateur renseigne le hostname de la machine destinataire
struct in_addr resoudre_destinataire(const char * destinataire) {

    struct in_addr addr;

    // Récupération de l'adresse IP de la machine destinataire si le hostname a été passé en paramètre à la place de l'adresse IP
    // inet_pton retourne 1 si la conversion a réussi, sinon on tente gethostbyname
    if(inet_pton(AF_INET, destinataire, &addr) != 1) {
        struct hostent * hp = gethostbyname(destinataire);
        if(hp == NULL) FATAL("gethostbyname");
        memcpy(&addr, hp->h_addr, hp->h_length);
        // hp->h_addr = hp->h_addr_list[0]
    }

    return addr;

}

// Envoie un ACK au Driver pour confirmer la réception d'un paquet
void envoyer_ack(int localsock) {

    char buffer[PACKET_SIZE];
    memset(buffer, 0, PACKET_SIZE);
    buffer[0] = 'a';

    int cc = send(localsock, buffer, PACKET_SIZE, 0);
    if(cc == -1) FATAL("send ack");

}

// Libère le token en envoyant 'f' au Driver
void liberer_token(int localsock) {

    char buffer[PACKET_SIZE];
    memset(buffer, 0, PACKET_SIZE);
    buffer[0] = 'f';

    int cc = send(localsock, buffer, PACKET_SIZE, 0);
    if(cc == -1) FATAL("send free");

}