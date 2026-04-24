#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../common/config.h"
#include "../common/error.h"

#include "paquet.h"
#include "fichier.h"
#include "utils.h"

// Envoyer un message à une machine destination
void emettre(const char * msg, int localsock, const char * destinataire) {

    envoyer(localsock, msg, destinataire);
    printf("Le message a bien été envoyé à %s !\n", destinataire);

}

// Diffuser un message à toutes les machines de l'anneau
void diffuser(const char * msg, int localsock) {

    envoyer(localsock, msg, BROADCAST_ADDR);
    printf("Le message %s a bien été diffusé à toutes les machines connectées à l'anneau !\n", msg);

}

// Récupérer les informations concernant toutes les machines du réseau
void recuperer(int localsock) {

    int cc;
    char token[TOKEN_SIZE];
    struct in_addr source, broadcast;

    recuperer_token(localsock, token);

    // Envoi un paquet contenant le caractère urgent 'h' au Driver pour récupérer les données de toutes les machines connectées à l'anneau
    char paquet[PACKET_SIZE];
    source = resoudre_source();
    inet_pton(AF_INET, BROADCAST_ADDR, &broadcast);

    construire_paquet(paquet, 'h', token, source, broadcast, "", 0);

    cc = send(localsock, paquet, PACKET_SIZE, 0);
    if(cc == -1) FATAL("send");

}