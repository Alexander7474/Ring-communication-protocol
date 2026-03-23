#include "../common/config.h"

#include <sys/un.h>

void help();    // Affiche les commandes utilisables via l'interpréteur de commandes

void envoyer_paquets(int sack, char * msg, char * token, char * destinataire);   // Construit le paquet et l'envoie au Driver pour le faire circuler dans l'anneau jusqu'au destinataire

// Fonctions à implémenter décrites dans le sujet 
void emettre(char * msg, int sack, struct sockaddr_un * pserv, char * destinataire); // Envoyer un message à une machine destination
void diffuser();    // Diffuser un message à toutes les machines de l'anneau
void recuperer();   // Récupérer les informations concernant toutes les machines du réseau
void transferer_fichier();  // Permet l'envoie ou la réception d'un fichier (binaire ou ASCII) entre deux machines

void commande(char * commande, int localsack, struct sockaddr_un * pserv);  // Détermine la commande entrée par l'utilisateur et exécute l'action correspondante

void comm();    // Menu permettant à l'utilisateur d'utiliser les fonctions