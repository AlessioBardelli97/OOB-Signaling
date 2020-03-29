/**
 * @file utils.h
 * @brief Contiene macro di utilità e il prototipo 
 *        di funzioni di utilità.
 *
 * @author Alessio Bardelli 544270
 * 
 * Si dichiara che il contenuto di questo file è in ogni sua parte opera
 * originale dell'autore
 */

#ifndef _UTILS_H_
#define _UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>

#define true 1
#define false 0

typedef int boolean;

int __err__;

#define CALLOC(buf, nmemb, size, messg, comand)				\
	if (((buf) = calloc((nmemb), (size))) == NULL) {		\
		perror(messg);										\
		comand;												\
	}

#define REALLOC(buf, newsize, messg, comand)				\
	if (((buf) = realloc((buf), (newsize))) == NULL) {		\
		perror(messg);										\
		comand;												\
	}

#define MENO1(expr,messg,comand)	\
	if((expr) == -1) {				\
		perror(messg);			    \
		comand;}

#define NULL_ERR(expr, messg, comand)   \
	if((expr) == NULL) {				\
			perror(messg);		        \
			comand;}

#define THREAD_ERR(tid,m,c)	        \
	if((__err__ = (tid)) != 0) {	\
		errno = __err__;			\
		perror(m);				    \
		c;}

/**
 * @function stol
 * @brief Esegue la funzione @strtol e controlla che non si siano 
 * 		  verificati errori durante la conversione.
 * @return -1 se c'è stato un errore, il risultato 
 *         della conversione altrimenti.
 */
long stol(const char* str, int base);

/**
 * @function mywrite
 */
int mywrite(int fd, const char* buf);

/**
 * @function myread
 * @brief Legge dal file descriptor @fd, memorizzando
 *        quello che legge in @buffer per poi aggiunger il carattere
 *        '\0' alla fine.
 */
int myread(int fd, char* buf, const int size);

#endif // _UTILS_H_
