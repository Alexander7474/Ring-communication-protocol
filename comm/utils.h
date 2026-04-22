#ifndef UTILS_H
#define UTILS_H

#include "../common/config.h"

// Retourne l'adresse IP de la machine source pour pouvoir savoir d'où vient le paquet et surtout permettre de s'arrêter à la bonne machine dans le cadre d'une diffusion
struct in_addr resoudre_source();

// Retourne l'adresse IP du destinataire pour gérer le cas où l'utilisateur renseigne le hostname de la machine destinataire
struct in_addr resoudre_destinataire(const char * destinataire);

// Envoie un ACK au Driver pour confirmer la réception d'un paquet
void envoyer_ack(int localsock);

// Libère le token en envoyant 'f' au Driver
void liberer_token(int localsock);

#endif