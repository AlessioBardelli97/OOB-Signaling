/**
 * @file utils.c
 * @brief Implementazione delle funzioni definite nella
 *        rispettiva interfaccia.
 *
 * @author Alessio Bardelli 544270
 * 
 * Si dichiara che il contenuto di questo file è in ogni sua parte opera
 * originale dell'autore
 */

#include <utils.h>

long stol(const char* str, int base) {

    char *endptr; long val; errno = 0;
    
    val = strtol(str, &endptr, base);

    if ((errno == ERANGE && (val == LONG_MAX || val == LONG_MIN)) || (errno != 0 && val == 0)) {

        perror("stol");
        return -1;
    }

    if (endptr == str) {

        fprintf(stderr, "stol: No digits were found\n");
        return -1;
    }

    return val;
}

int mywrite(int fd, const char* buf) { return write(fd, buf, strlen(buf)); }

int myread(int fd, char* buf, const int size) { int __n__ = read(fd, buf, size); buf[__n__] = '\0'; return __n__; }
