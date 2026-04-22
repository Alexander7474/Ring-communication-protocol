#ifndef PAQUET_H
#define PAQUET_H

#include "../common/config.h"
#include <stdbool.h>

extern bool emission_en_cours;  // Permet de savoir si on doit libérer le token ou pas

// Réceptionne un paquet envoyé par le Driver au Comm
void recevoir_paquet(int localsock);

// Lit le paquet envoyé et stocke les données importantes à traiter
void lire_paquet(int localsock, char * urgent, char ** contenu, struct in_addr * source, struct in_addr * dest, char * token);

// Construit le paquet contenant le caractère urgent, le token, l'adresse IP du destinataire et le contenu
void construire_paquet(char * paquet, char urgent, const char * token, struct in_addr source, struct in_addr dest, const char * contenu, int nb_octets);

// Construit le paquet et l'envoie au Driver pour le faire circuler dans l'anneau jusqu'au destinataire
void envoyer_paquets(int localsock, const char * msg, const char * token, const char * destinataire);

// Envoi une demande au Driver pour récupérer le Token et le retourne
void recuperer_token(int localsock, char * token);

// Demande le token au Driver et le récupère, puis envoie le ou les paquet(s) en fonction de la taille du message
void envoyer(int localsock, const char * msg, const char * destinataire);

#endif