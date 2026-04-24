#ifndef SHELL_H
#define SHELL_H

#include "../common/config.h"

// Affiche les commandes utilisables via l'interpréteur de commandes 
void help();

// Détermine la commande entrée par l'utilisateur et exécute l'action correspondante
void commande(char * c, int localsock);

// Traite une commande entrée par l'utilisateur
void traiter_commande(int localsock);

// Affiche l'interpréteur de commandes utilisables pour communiquer dans l'anneau
void comm(int localsock);

#endif