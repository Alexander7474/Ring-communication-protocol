#include "../common/config.h"

#include <sys/un.h>

void help();    // Affiche les commandes utilisables via l'interpréteur de commandes

void emettre(const char * msg, int localsock, struct sockaddr_un * serv, const char * destinataire); // Envoyer un message à une machine destination

void diffuser(const char * msg, int localsock, struct sockaddr_un * serv);    // Diffuser un message à toutes les machines de l'anneau

void recuperer();   // Récupérer les informations concernant toutes les machines du réseau

void transferer_fichier(const char * fichier, const char * destinataire, int localsock, struct sockaddr_un * serv);  // Permet l'envoie ou la réception d'un fichier (binaire ou ASCII) entre deux machines

void commande(char * commande, int localsock, struct sockaddr_un * serv);  // Détermine la commande entrée par l'utilisateur et exécute l'action correspondante

void comm(int localsock, struct sockaddr_un * serv);    // Menu permettant à l'utilisateur d'utiliser les fonctions