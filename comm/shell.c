#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../common/config.h"
#include "../common/error.h"

#include "shell.h"
#include "commandes.h"
#include "fichier.h"
#include "paquet.h"

// Affiche les commandes utilisables via l'interpréteur de commandes 
void help() {

    printf("- help : affiche les commandes utilisables via l'interpréteur de commandes.\n");
    printf("- exit : ferme le programme.\n");
    printf("- quit : ferme le programme.\n");
    printf("- echo [MESSAGE] : envoie un message à toutes les machines connectées à l'anneau si seul le message est fourni.\n");
    printf("- echo [ADRESSE IP] [MESSAGE] : envoie un message à une machine connectée à l'anneau à partir de son adresse IP.\n");
    printf("- file [ADRESSE IP] [FICHIER] : envoie un fichier à une machine connectée à l'anneau à partir de son adresse IP.\n");
    printf("- hosts : affiche les informations (hostname et adresse IP) concernant toutes les machines connectées à l'anneau.\n");

}

// Fonction static car elle est appelée depuis commande()
static void echo(int localsock) {

    struct in_addr test;

    // Récupération des paramètres de la commande
    char * premier = strtok(NULL, " "); // Adresse IP OU début du message si on est dans le cas d'une diffusion
    char * reste = strtok(NULL, "");    // Message OU le reste du message

    // Vérifie la syntaxe de la commande
    if(premier == NULL) {
        printf("Usage: echo [MESSAGE] | echo [ADRESSE IP] [MESSAGE]\n");
        return;
    }

    // Vérifie si le premier argument est une adresse IP valide
    if(inet_pton(AF_INET, premier, &test) == 1) {
        if(reste == NULL) {
            printf("Usage: echo [ADRESSE IP] [MESSAGE]\n");
            return;
        }
        emettre(reste, localsock, premier);
    } 
        
    else {
        // Le premier argument n'est pas une adresse IP donc c'est une diffusion et on concatène tout le message
        if(reste != NULL) *(reste - 1) = ' ';
        diffuser(premier, localsock);
    }

}

// Fonction static car elle est appelée depuis commande()
static void file(int localsock) {

    // Récupération des paramètres de la commande
    char * destinataire = strtok(NULL, " ");
    char * fichier = strtok(NULL, " ");

    if(fichier == NULL || destinataire == NULL) { 
        printf("Usage: file [ADRESSE IP | HOSTNAME] [FICHIER]\n"); 
        return; 
    }

    transferer_fichier(fichier, destinataire, localsock);

}

// Détermine la commande entrée par l'utilisateur et exécute l'action correspondante
void commande(char * c, int localsock) {

    char * commande_utilisateur = strtok(c, " ");    // La variable ne peut pas s'appeller "commande" car c'est le nom de la fonction

    // Arrête le programme via une commande sans utiliser CTRL + C
    if(strcmp(commande_utilisateur, "exit") == 0 || strcmp(commande_utilisateur, "quit") == 0) exit(0);

    // Affiche les commandes utilisables via l'interpréteur de commandes
    else if(strcmp(commande_utilisateur, "help") == 0) help();   // Simplification possible en mettant cette condition dans le else ?

    // Envoi d'un message à une machine spécifique de l'anneau OU diffuse le message à toutes les machines connectées à l'anneau
    else if(strcmp(commande_utilisateur, "echo") == 0) echo(localsock);

    // Transfère un fichier à une machine spécifique de l'anneau
    else if(strcmp(commande_utilisateur, "file") == 0) file(localsock);

    // Récupère les informations (IP + Hostname) de toutes les machines connectées à l'anneau
    else if(strcmp(commande_utilisateur, "hosts") == 0) recuperer(localsock);

    // Affiche les commandes utilisables via l'interpréteur de commandes si la commande n'est pas reconnue
    else help();

}  

// Traite une commande entrée par l'utilisateur
void traiter_commande(int localsock) {

    char input[PACKET_SIZE];

    // Stockage de la commande entrée par l'utilisateur
    if(fgets(input, PACKET_SIZE, stdin) == NULL) return;
    input[strcspn(input, "\n")] = '\0';  // Retire le \n à la fin de la commande

    // Détermine la commande entrée par l'utilisateur et exécute l'action correspondante
    commande(input, localsock);

    // Réaffiche le prompt après le traitement de la commande
    printf("comm> ");
    fflush(stdout);

}

// Affiche l'interpréteur de commandes utilisables pour communiquer dans l'anneau
void comm(int localsock) {

    fd_set readfds;
    int cc;

    // Affiche le tout premier prompt
    printf("comm> ");
    fflush(stdout); // Nécessaire car sinon le prompt "comm> " n'est pas affiché directement

    // Tant que l'utilisateur ne s'est pas déconnecté = tant que le programme n'a pas été volontairement arrêté
    while(1) {

        FD_ZERO(&readfds);
        FD_SET(STDIN_FILENO, &readfds); // Surveille les commandes entrées par l'utilisateur
        FD_SET(localsock, &readfds);    // Surveille les transmissions envoyées par le Driver au Comm

        // Mise en place du select
        cc = select(localsock + 1, &readfds, NULL, NULL, NULL);
        if(cc == -1) FATAL("select");

        // Traitement d'une commande entrée par l'utilisateur
        if(FD_ISSET(STDIN_FILENO, &readfds)) traiter_commande(localsock);

        // Réception d'un paquet envoyé par le Driver au Comm
        else if(FD_ISSET(localsock, &readfds)) recevoir_paquet(localsock); 

    }

}