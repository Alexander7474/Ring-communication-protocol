#ifndef CONFIG_H 
#define CONFIG_H

/**
 * Taille max d'un message 
*/
#define SMAX 64

/**
 * Taille du caractère urgent (en octet)
*/
#define URGENT_SIZE 1   // 8 bits

/**
 * Taille du token (en octets)
*/
#define TOKEN_SIZE 4    // 32 bits

/**
 * Taille du token (en octets)
*/
#define ADDR_SIZE 4     // 32 bits

/**
 * Taille du contenu (en octets)
*/
#define CONTENT_SIZE 32 // 256 bits, donc 32 octets car 256 / 8 (car 1 octet = 8 bytes) = 32

/**
 * Taille d'un paquet (en octets)
*/
#define PACKET_SIZE (URGENT_SIZE + TOKEN_SIZE + ADDR_SIZE + CONTENT_SIZE)   // 1 + 4 + 4 + 32 = 41 octets (taille d'un paquet)

/**
* Taille de la file d'attente d'hosts
 */
#define WAITING_HOST_MAX 32

/**
 * Taille du token (en bytes)
 */
#define TOKEN_SIZE 4

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
