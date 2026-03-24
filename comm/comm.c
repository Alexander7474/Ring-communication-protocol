#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <stdbool.h>

#include "../common/config.h"
#include "../common/error.h"
#include "comm.h"

// Affiche les commandes utilisables via l'interpréteur de commandes 
void help() {

    printf("- help : affiche les commandes utilisables via l'interpréteur de commandes.\n");
    printf("- exit : ferme le programme.\n");
    printf("- quit : ferme le programme.\n");
    printf("- echo [MESSAGE] : envoie un message à toutes les machines connectés à l'anneau si seul le message est fourni.\n");
    printf("- echo [MESSAGE] [ADRESSE IP | HOSTNAME] : envoie un message à une machine connectée à l'anneau à partir de son adresse IP ou de son nom d'hôte.\n");
    printf("- file [FICHIER] [ADRESSE IP | HOSTNAME] : envoie un fichier à une machine connectée à l'anneau à partir de son adresse IP ou de son nom d'hôte.\n");
    printf("- hosts : affiche les informations concernant toutes les machines du réseau.\n");

}

// Construit le paquet et l'envoie au Driver pour le faire circuler dans l'anneau jusqu'au destinataire
void envoyer_paquets(int localsack, char * msg, char * token, char * destinataire) {

    int cc;
    int msg_len = strlen(msg);
    int nb_bits_envoyes = 0;
    struct in_addr addr;

    // Récupération de l'adresse IP de la machine destinaire si le hostname a été passé en paramètre à la place de l'adresse IP
    // inet_pton retourne 1 si la conversion a réussi, sinon on tente gethostbyname
    if(inet_pton(AF_INET, destinataire, &addr) != 1) {
        struct hostent * hp = gethostbyname(destinataire);
        if(hp == NULL) FATAL("gethostbyname");
        memcpy(&addr, hp->h_addr, hp->h_length);
        // hp->h_addr = hp->h_addr_list[0]
    }

    // Envoyer des paquets tant qu'il reste du contenu à envoyer car il faut gérer le cas où la taille du contenu est supérieur à 256
    while(nb_bits_envoyes < msg_len) {

        int nb_bits_a_envoyer, current_octet;
        bool dernier_paquet;
        char urgent;
        char paquet[URGENT_SIZE + TOKEN_SIZE + ADDR_SIZE + CONTENT_SIZE];   // 1 + 4 + 4 + 32 = 41 octets (taille d'un paquet)

        // Calcul du nombre de bits à envoyer dans ce paquet
        nb_bits_a_envoyer = msg_len - nb_bits_envoyes;
        if(nb_bits_a_envoyer > CONTENT_SIZE) nb_bits_a_envoyer = CONTENT_SIZE;  // S'assure que la partie message du paquet ne dépasse pas 256 bits

        // Détermine si ce paquet sera le dernier envoyé ou pas
        if(nb_bits_envoyes + nb_bits_a_envoyer == msg_len) dernier_paquet = true;
        else dernier_paquet = false;

        // Détermine le caractère urgent à utiliser pour ce paquet
        if(dernier_paquet) urgent = 'e';
        else urgent = 'u';

        // Ajoute le caractère urgent au paquet
        current_octet = 0;
        paquet[current_octet] = urgent;
        current_octet += URGENT_SIZE;

        // Ajoute le token au paquet
        memcpy(paquet + current_octet, token, TOKEN_SIZE);
        current_octet += TOKEN_SIZE;

        // Ajoute l'adresse IP du destinataire au paquet
        memcpy(paquet + current_octet, &addr.s_addr, ADDR_SIZE);
        current_octet += ADDR_SIZE;

        // Ajoute le contenu au paquet
        memset(paquet + current_octet, 0, CONTENT_SIZE);
        memcpy(paquet + current_octet, msg + nb_bits_envoyes, nb_bits_a_envoyer);
        current_octet += CONTENT_SIZE;

        // Envoi le paquet
        cc = send(localsack, paquet, current_octet, 0);
        if(cc == -1) FATAL("send");
        nb_bits_envoyes += nb_bits_a_envoyer;

    }

}

// Envoyer un message à une machine destination
void emettre(char * msg, int localsack, struct sockaddr_un * pserv, char * destinataire) {

    printf("Émission d'un message\n");    // Debugging pour tester les commandes via le shell

    int cc;
    char token[TOKEN_SIZE];

    // Comm demande le token au Driver en envoyant avec le caractère urgent 'n'
    cc = send(localsack, "n", 1, 0);
    if(cc == -1) FATAL("send");

    // Réception du token envoyé par le Driver
    cc = recv(localsack, token, TOKEN_SIZE, 0);
    if(cc == -1) FATAL("recv");

    // Fabrication des paquets et les envoie au Driver avec le caractère urgent 'u'
    // Le dernier paquet envoyé aura lui le caractère urgent 'e' pour spécifier la fin de l'émission
    envoyer_paquets(localsack, msg, token, destinataire);

}

// Diffuser un message à toutes les machines de l'anneau
void diffuser(char * msg, int localsack, struct sockaddr_un * pserv) {

    printf("Diffusion d'un message\n");    // Debugging pour tester les commandes via le shell

}

// Permet l'envoie ou la réception d'un fichier (binaire ou ASCII) entre deux machines
void transferer_fichier(char * fichier, char * destinataire, int localsack, struct sockaddr_un * pserv) {

    printf("Transfert de fichier\n");    // Debugging pour tester les commandes via le shell

}

// Récupérer les informations concernant toutes les machines du réseau
void recuperer() {

    printf("Récupérer des informations sur les machines du réseau\n");    // Debugging pour tester les commandes via le shell

}

// Détermine la commande entrée par l'utilisateur et exécute l'action correspondante
void commande(char * commande, int localsack, struct sockaddr_un * pserv) {

    char * command = strtok(commande, " ");

    if(strcmp(command, "exit") == 0 || strcmp(command, "quit") == 0) exit(0); // Arrêter le programme via une commande sans utiliser CTRL + C

    else if(strcmp(command, "help") == 0) help();

    else if(strcmp(command, "echo") == 0) {

        // Récupération des paramètres de la commande
        char * msg = strtok(NULL, " ");  // Message à envoyer
        char * destinataire = strtok(NULL, " ");    // Adresse IP ou nom de la machine si l'utilisateur / Sinon broadcast à tout le réseau

        // Vérifie la syntaxe de la commande
        if(msg == NULL) { 
            printf("Usage: echo [MESSAGE] [ADRESSE IP | HOSTNAME]\n"); 
            return;
        }

        if(destinataire != NULL) emettre(msg, localsack, pserv, destinataire);    // Envoyer le message à une machine du réseau
        else diffuser(msg, localsack, pserv); // Envoyer le message à toutes les machines du réseau

    }

    else if(strcmp(command, "file") == 0) {

        // Récupération des paramètres de la commande
        char * fichier = strtok(NULL, " ");
        char * destinataire = strtok(NULL, " ");

        if(fichier == NULL || destinataire == NULL) { 
            printf("Usage: file [FICHIER] [ADRESSE IP | HOSTNAME]\n"); 
            return; 
        }

        transferer_fichier(fichier, destinataire, localsack, pserv);

    }

    else if(strcmp(command, "hosts") == 0) recuperer();

    else printf("Commande introuvable\n");

}  

// Menu permettant à l'utilisateur d'utiliser les fonctions
void comm(int localsack, struct sockaddr_un * pserv) {

    char msg[SMAX];

    while(1) {  // Tant que l'utilisateur ne s'est pas déconnecté = tant que le programme n'a pas été volontairement arrêté

        printf("comm> ");

        // Stockage de la commande entrée par l'utilisateur
        if(fgets(msg, SMAX, stdin) == NULL) break;
        msg[strcspn(msg, "\n")] = '\0';  // Retire le \n à la fin de la commande

        commande(msg, localsack, pserv);    // Détermine la commande entrée par l'utilisateur et exécute l'action correspondante

    }

}