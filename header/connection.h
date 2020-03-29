/**
 * @file connection.h
 * @brief Interfaccia per gestire gli indirizzi
 *        delle socket.
 *
 * @author Alessio Bardelli 544270
 * 
 * Si dichiara che il contenuto di questo file è in ogni sua parte opera
 * originale dell'autore
 */

#ifndef CONNECTION_H_
#define CONNECTION_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#define UNIX_PATH_MAX 108
#define MAX_RETRY 3
#define SLEEP_TIME 2

typedef struct sockaddr_un Address_t;

#define ADDRESS_INIT(addr, sockname)                    \
    strncpy(addr.sun_path, sockname, UNIX_PATH_MAX);    \
    addr.sun_family = AF_UNIX

#endif //CONNECTION_H_
