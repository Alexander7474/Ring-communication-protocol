#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

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
void envoyer_paquets(int sack, char * msg, char * token, char * destinataire) {

    // 1. Envoyer le caractère urgent 'u' sauf pour le dernier paquet envoyé ou ce sera le caractère urgent 'e' qui sera utilisé

    // 2. Envoyer le token

    // 3. Envoyer l'adresse IP du destinataire 
    // La récupérer via la fonction gethostbyname() si le nom de l'hôte a été fourni à la place

    // 4. Envoyer le contenu

}

// Envoyer un message à une machine destination
void emettre(char * msg, int sack, struct sockaddr_un * pserv, char * destinataire) {

    printf("Émission d'un message\n");    // Debugging pour tester les commandes via le shell

    int cc;
    char token[TOKEN_SIZE];
    socklen_t lg;

    // Comm demande le token au Driver en envoyant avec le caractère urgent 'n'
    cc = send(sack, "n", 1, 0);
    if(cc == -1) FATAL("send");

    // Réception du token envoyé par le Driver
    lg = sizeof(* pserv);
    cc = recvfrom(sack, token, TOKEN_SIZE, 0, (struct sockaddr *) pserv, &lg);  // Attend la réponse = le token
    if(cc == -1) FATAL("recvfrom");

    // Fabrication des paquets et les envoie au Driver avec le caractère urgent 'u'
    // Le dernier paquet envoyé aura lui le caractère urgent 'e' pour spécifier la fin de l'émission
    envoyer_paquets(sack, msg, token, destinataire);

}

// Diffuser un message à toutes les machines de l'anneau
void diffuser() {

    printf("Diffusion d'un message\n");    // Debugging pour tester les commandes via le shell

}

// Permet l'envoie ou la réception d'un fichier (binaire ou ASCII) entre deux machines
void transferer_fichier() {

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
        char * destinataire = strtok(NULL, " ");    // Adresse IP / Nom de la machine si l'utilisateur / Sinon broadcast à tout le réseau

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