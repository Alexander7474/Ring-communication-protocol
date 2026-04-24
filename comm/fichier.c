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
#include "utils.h"

// Permet l'envoie ou la réception d'un fichier (binaire ou ASCII) entre deux machines
void transferer_fichier(const char * fichier, const char * destinataire, int localsock) {

    int cc, nb_octets_lus;
    char token[TOKEN_SIZE], filename[PACKET_SIZE], contenu[CONTENT_SIZE];
    FILE * file;
    struct in_addr source, addr;

    // Ouverture du fichier en mode lecture
    file = fopen(fichier, "rb");    // "Envoit des fichiers binaires ou ASCII"
    if(file == NULL) FATAL("fopen");

    source = resoudre_source();    // Récupère l'adresse IP de la source
    addr = resoudre_destinataire(destinataire);    // Récupère l'adresse IP du destinataire pour gérer le cas où l'utilisateur renseigne le hostname de la machine destinataire  

    recuperer_token(localsock, token);    // Envoi une demande au Driver pour récupérer le Token qui le retourne

    // Envoi d'un paquet contenant le caractère urgent 's' spécifiant le nom du fichier
    construire_paquet(filename, 's', token, source, addr, fichier, strlen(fichier) + 1);
    cc = send(localsock, filename, PACKET_SIZE, 0);
    if(cc == -1) FATAL("send");

    recuperer_token(localsock, token); // Récupère le nouveau token pour envoyer le fichier

    // Lecture du fichier par tranche de 256 octets 
    while((nb_octets_lus = fread(contenu, 1, CONTENT_SIZE, file)) > 0) {

        bool dernier_paquet;
        char urgent;
        char paquet[PACKET_SIZE];

        // Détermine si ce paquet sera le dernier envoyé ou pas car nous avons atteint la fin du fichier
        if(nb_octets_lus < CONTENT_SIZE) dernier_paquet = true;
        else dernier_paquet = false;

        // Détermine le caractère urgent à utiliser pour ce paquet
        if(dernier_paquet) urgent = 'e';
        else urgent = 'u';

        construire_paquet(paquet, urgent, token, source, addr, contenu, nb_octets_lus);

        // Envoi le paquet
        cc = send(localsock, paquet, PACKET_SIZE, 0);
        if(cc == -1) FATAL("send");

    }

    fclose(file);
    printf("Le fichier %s a bien été envoyé à %s !\n", fichier, destinataire);

}