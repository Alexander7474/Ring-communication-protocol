#ifndef CONFIG_H 
#define CONFIG_H

// TODO -- suppremier les doublons et arranger le code qui les utilisent

/**
 * Taille du caractère urgent (en octet)
*/
#define URGENT_SIZE 1   // 8 bits
#define FLAG_SIZE 1

/**
 * Taille du token (en octets)
*/
#define TOKEN_SIZE 4    // 32 bits
#define TOKEN_OFFSET (FLAG_SIZE)

/**
 * Taille du token (en octets)
*/
#define ADDR_SIZE 8     // 32 bits
#define ADDR_OFFSET (FLAG_SIZE+TOKEN_SIZE)

/**
 * Taille du contenu (en octets)
*/
#define CONTENT_SIZE 32 // 256 bits, donc 32 octets car 256 / 8 (car 1 octet = 8 bytes) = 32
#define DATA_SIZE 32 // 256 bits, donc 32 octets car 256 / 8 (car 1 octet = 8 bytes) = 32
#define DATA_OFFSET (FLAG_SIZE+TOKEN_SIZE+ADDR_SIZE)

/**
 * Taille d'un paquet (en octets)
*/
#define PACKET_SIZE (FLAG_SIZE + TOKEN_SIZE + ADDR_SIZE + CONTENT_SIZE)   // 1 + 4 + 4 + 32 = 41 octets (taille d'un paquet)
#define SMAX (FLAG_SIZE + TOKEN_SIZE + ADDR_SIZE + CONTENT_SIZE)   // 1 + 4 + 4 + 32 = 41 octets (taille d'un paquet)

/**
* Taille de la file d'attente d'hosts
 */
#define WAITING_HOST_MAX 32

/* builds "%08X" from TOKEN_SIZE at compile time */
#define TOKEN_FMT "%0" STRINGIFY(TOKEN_SIZE) "X"
#define STRINGIFY(x) STR(x)
#define STR(x) #x

/**
 * Port utilisé par le protocole
*/
#define PORT 4444

/**
 * Adresse de diffusion
*/
#define BROADCAST_ADDR "0.0.0.0"

/**
 * Temps maximal d'attente avant de regénérer le token
*/
#define MAX_WAIT 5

/**
 * Chemin où stocker la socket Unix qui est un fichier local
*/
#define UNIX_SOCKET_PATH "/tmp/localsock.sock"

#endif
