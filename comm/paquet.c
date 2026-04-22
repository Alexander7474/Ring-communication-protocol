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

#include "utils.h"
#include "paquet.h"
#include "commandes.h"
#include "fichier.h"

bool emission_en_cours = false;  // Permet de savoir si on doit libérer le token ou pas

// Mémorise le nom du fichier à recevoir à partir du contenu d'un paquet ayant le flag 's'
static void recevoir_nom_fichier(const char * contenu, char * filename) {

    // Stockage du fichier reçu dans le répertoire où l'utilisateur a lancé le programme comm
    const char * base = strrchr(contenu, '/');  // Extraction uniquement du nom du fichier (pas du chemin entier)
    if(base) base = base + 1;
    else base = contenu;

    // Met dans la variable filename le nom du fichier
    strncpy(filename, base, CONTENT_SIZE - 1);
    filename[CONTENT_SIZE - 1] = '\0';

}

// Écrit le bloc de contenu reçu dans le fichier crée
static void recevoir_bloc_fichier(const char * contenu, char urgent, char * filename, struct in_addr source) {

    // Création du fichier
    FILE * file = fopen(filename, "ab");
    if(file == NULL) FATAL("fopen");

    fwrite(contenu, 1, CONTENT_SIZE, file);
    fclose(file);

    // Affiche un message d'information à l'utilisateur pour lui confirmer la réception s'il s'agissait du dernier paquet avec le flag 'e'
    if(urgent == 'e') {
        printf("\r%s vous a envoyé le fichier : %s\ncomm>", inet_ntoa(source), filename);
        fflush(stdout);
        filename[0] = '\0';  // Remise à zéro pour fin de traitement de ce fichier pour laisser la place à une nouvelle réception de fichier
    }

}

// Affiche les informations sur une machine connectée à l'anneau
static void afficher_informations_machines(const char * contenu) {

    struct in_addr addr;    // Adresse IP de la machine
    char hostname[5];  // Hostname de la machine
    char ip_str[INET_ADDRSTRLEN];

    memcpy(&addr.s_addr, contenu, ADDR_SIZE);   // 4 octets pour l'adresse IP 

    // Récupération du hostname
    memcpy(hostname, contenu + ADDR_SIZE, 4);  // 4 octets pour le hostname
    hostname[4] = '\0';    // Fin des informations récupérées

    inet_ntop(AF_INET, &addr, ip_str, sizeof(ip_str));
    printf("\r  %-16s %s\n", ip_str, hostname); // Affichage propre et régulier

}

// Lit le paquet envoyé et stocke les données importantes à traiter
void lire_paquet(int localsock, char * urgent, char ** contenu, struct in_addr * source, struct in_addr * dest, char * token) {

    static char paquet[PACKET_SIZE];
    int cc;

    // Réception du paquet envoyé envoyé par le Driver au Comm
    cc = recv(localsock, paquet, PACKET_SIZE, 0);
    if(cc == -1) FATAL("recv");

    // Extraction du flag
    *urgent = paquet[0];

    // Extraction du token
    memcpy(token, paquet + TOKEN_OFFSET, TOKEN_SIZE);

    // Extraction de l'IP source
    memcpy(&source->s_addr, paquet + ADDR_SRC_OFFSET, ADDR_SIZE);

    // Extraction de l'IP de destination
    memcpy(&dest->s_addr, paquet + ADDR_OFFSET, ADDR_SIZE);

    // Extraction du contenu du paquet
    *contenu = paquet + URGENT_SIZE + TOKEN_SIZE + ADDR_SIZE + ADDR_SIZE;

}

// Construit le paquet contenant le caractère urgent, le token, l'adresse IP du destinataire et le contenu
void construire_paquet(char * paquet, char urgent, const char * token, struct in_addr source, struct in_addr dest, const char * contenu, int nb_octets) {
    
    int current_octet = 0;

    // Ajoute le caractère urgent au paquet
    paquet[current_octet] = urgent;
    current_octet += URGENT_SIZE;

    // Ajoute le token au paquet
    memcpy(paquet + current_octet, token, TOKEN_SIZE);
    current_octet += TOKEN_SIZE;

    // Ajoute l'adresse IP de la source au paquet
    uint32_t source_long = source.s_addr;
    memcpy(paquet + current_octet, &source_long, ADDR_SIZE);
    current_octet += ADDR_SIZE;

    // Ajoute l'adresse IP du destinataire au paquet
    uint32_t dest_long = dest.s_addr;
    memcpy(paquet + current_octet, &dest_long, ADDR_SIZE);
    current_octet += ADDR_SIZE;

    // Ajoute le contenu au paquet
    memset(paquet + current_octet, 0, CONTENT_SIZE);
    memcpy(paquet + current_octet, contenu, nb_octets);

}

// Envoi une demande au Driver pour récupérer le Token et le retourne
void recuperer_token(int localsock, char * token) {

    char requete[PACKET_SIZE];
    int cc;
    char paquet[PACKET_SIZE];

    // Envoit un paquet avec le flag 'n' seulement et le reste à null ?
    memset(requete, 0, PACKET_SIZE);
    requete[0] = 'n';

    // Comm demande le token au Driver en envoyant le flag 'n'
    cc = send(localsock, requete, PACKET_SIZE, 0);
    if(cc == -1) FATAL("send");

    // Réception du paquet contenant le token envoyé par le Driver
    cc = recv(localsock, paquet, PACKET_SIZE, 0);
    if(cc == -1) FATAL("recv");

    memcpy(token, paquet + TOKEN_OFFSET, TOKEN_SIZE);

}

// Construit le paquet et l'envoie au Driver pour le faire circuler dans l'anneau jusqu'au destinataire
void envoyer_paquets(int localsock, const char * msg, const char * token, const char * destinataire) {

    int cc;
    int msg_len = strlen(msg);
    int nb_octets_envoyes = 0;
    struct in_addr source, addr;

    source = resoudre_source();
    addr = resoudre_destinataire(destinataire);

    // Envoyer des paquets tant qu'il reste du contenu à envoyer car il faut gérer le cas où la taille du contenu est supérieur à 256
    while(nb_octets_envoyes < msg_len) {

        int nb_octets_a_envoyer;
        bool dernier_paquet;
        char urgent;
        char paquet[PACKET_SIZE];   // 1 + 4 + 8 + 8 + 32 = 53 octets (taille d'un paquet)

        // Calcul du nombre de bits à envoyer dans ce paquet
        nb_octets_a_envoyer = msg_len - nb_octets_envoyes;
        if(nb_octets_a_envoyer > CONTENT_SIZE) nb_octets_a_envoyer = CONTENT_SIZE;  // S'assure que la partie message du paquet ne dépasse pas 256 bits

        // Détermine si ce paquet sera le dernier envoyé ou pas
        if(nb_octets_envoyes + nb_octets_a_envoyer == msg_len) dernier_paquet = true;
        else dernier_paquet = false;

        // Détermine le caractère urgent à utiliser pour ce paquet
        if(dernier_paquet) urgent = 'e';
        else urgent = 'u';

        // On est en cours d'émission tant qu'on n'a pas envoyé 'e'
        emission_en_cours = !dernier_paquet;

        construire_paquet(paquet, urgent, token, source, addr, msg + nb_octets_envoyes, nb_octets_a_envoyer);

        // Envoi le paquet
        cc = send(localsock, paquet, PACKET_SIZE, 0);
        if(cc == -1) FATAL("send");
        nb_octets_envoyes += nb_octets_a_envoyer;

    }

}

// Demande le token au Driver et le récupère, puis envoie le ou les paquet(s) en fonction de la taille du message
void envoyer(int localsock, const char * msg, const char * destinataire) {

    char token[TOKEN_SIZE];

    recuperer_token(localsock, token);    // Envoi une demande au Driver pour récupérer le Token qui le retourne

    // Fabrication des paquets et les envoie au Driver avec le caractère urgent 'u'
    // Le dernier paquet envoyé aura lui le caractère urgent 'e' pour spécifier la fin de l'émission
    envoyer_paquets(localsock, msg, token, destinataire);

}

// Réceptionne un paquet envoyé par le Driver au Comm
void recevoir_paquet(int localsock) {

    char urgent;
    char * contenu;
    struct in_addr source;
    struct in_addr dest;
    char token[TOKEN_SIZE];

    // recevoir_paquet() est une fonction static car elle contient des variables static pour mémoriser le nom du fichier traité entre les appels à recevoir_paquet()
    static char filename[CONTENT_SIZE] = {0};   // Initialise un tableau avec CONTENT_SIZE zéros
    static char message[4096] = {0};    // Initialise un buffer suffisament grand pour reconstituer le message
    static int len = 0;

    lire_paquet(localsock, &urgent, &contenu, &source, &dest, token);

    // Réception du nom du fichier à recevoir dans le premier paquet
    if(urgent == 's') {
        recevoir_nom_fichier(contenu, filename);
        envoyer_ack(localsock);
    }

    // Réception d'un ACK = notre émission a bien été reçue
    if(urgent == 'a') {
        if(!emission_en_cours) liberer_token(localsock);   // On libère le token si on a reçu le 'a' final après notre 'e'
        // Si emission_en_cours == true, on attend encore des acks intermédiaires
        envoyer_ack(localsock);
    }

    // Une commande "hosts" a été lancée sur un comm d'une machine de l'anneau et le paquet vient d'être reçu pour récupérer des infos sur cette machine
    // Réception d'un paquet 'h' (hosts) en diffusion
    if(urgent == 'h') {

        struct in_addr ma_source = resoudre_source();

        // Si je suis la source, le paquet a fait le tour → afficher les infos et libérer le token
        if(source.s_addr == ma_source.s_addr) {

            printf("\r  %-16s %-28s\n", "Adresse IP", "Hostname");
            printf("  ─────────────────────────────────────────────\n");

            // Lire les infos de chaque machine dans le contenu (8 octets par machine)
            int nb_machines = CONTENT_SIZE / (ADDR_SIZE + 4);
            for(int i = 0; i < nb_machines; i++) {

                uint32_t addr_check = 0;
                memcpy(&addr_check, contenu + i * (ADDR_SIZE + 4), ADDR_SIZE);
                if(addr_check == 0) break;
                afficher_informations_machines(contenu + i * (ADDR_SIZE + 4));

            }

            printf("comm> ");
            fflush(stdout);
            liberer_token(localsock);
            return;

        }

        // Sinon ajouter mes infos dans le contenu et faire circuler le paquet
        char hostname[5] = {0};
        gethostname(hostname, 4);  // 4 octets max
        hostname[4] = '\0';

        // Trouver le premier emplacement libre (IP à 0)
        int nb_machines_max = CONTENT_SIZE / (ADDR_SIZE + 4);
        for(int i = 0; i < nb_machines_max; i++) {

            uint32_t addr_check = 0;
            memcpy(&addr_check, contenu + i * (ADDR_SIZE + 4), ADDR_SIZE);

            if(addr_check == 0) {
                // Emplacement libre trouvé
                memcpy(contenu + i * (ADDR_SIZE + 4), &ma_source.s_addr, ADDR_SIZE);
                memcpy(contenu + i * (ADDR_SIZE + 4) + ADDR_SIZE, hostname, 4);
                break;
            }

        }

        // Faire circuler le paquet avec les infos ajoutées
        char buffer[PACKET_SIZE];
        memset(buffer, 0, PACKET_SIZE);
        construire_paquet(buffer, 'h', token, source, dest, contenu, CONTENT_SIZE);
        int cc = send(localsock, buffer, PACKET_SIZE, 0);
        if(cc == -1) FATAL("send hosts");

    }

    // Réception d'un paquet de données pour un message texte ou un bloc de fichier
    if(urgent == 'u' || urgent == 'e') {
        
        // Détermine si c'est un fichier ou un texte qu'on reçoit dans le paquet de données
        if(filename[0] != '\0') recevoir_bloc_fichier(contenu, urgent, filename, source);

        // Cas où c'est un texte
        else {

            // On reconstitue le message texte
            int len_paquet = strnlen(contenu, CONTENT_SIZE);
            if(len + len_paquet < (int)sizeof(message) - 1) {
                memcpy(message + len, contenu, len_paquet);
                len += len_paquet;
            }

            if(urgent == 'e') {
                message[len] = '\0';
                printf("\r%s vous a envoyé un message : %s\ncomm>", inet_ntoa(source), message);
                fflush(stdout);

                // Remise à zéro du buffer pour le prochain message reçu
                len = 0;
                message[0] = '\0';
            }

        }

        // Dans le cas d'une diffusion, on ne renvoie pas de ACK, on renvoie le paquet au driver pour continuer la circulation jusqu'à ce qu'il retourne à sa source (à lui même)
        struct in_addr broadcast;
        inet_pton(AF_INET, BROADCAST_ADDR, &broadcast);

        // Renvoyer le paquet inchangé au driver pour le faire circuler
        if(dest.s_addr == broadcast.s_addr) {

            struct in_addr source_diffusion = resoudre_source();

            // Cas où le paquet d'une diffusion a fait le tour de l'anneau et me revient alors je libère le token
            if(source.s_addr == source_diffusion.s_addr) {
                liberer_token(localsock);
                return;
            }
            
            // Renvoyer le paquet pour continuer la circulation
            char buffer[PACKET_SIZE];
            memset(buffer, 0, PACKET_SIZE);
            construire_paquet(buffer, urgent, token, source, dest, contenu, strnlen(contenu, CONTENT_SIZE));
            int cc = send(localsock, buffer, PACKET_SIZE, 0);
            if(cc == -1) FATAL("send diffusion");

        } 

        else envoyer_ack(localsock);

    }

}