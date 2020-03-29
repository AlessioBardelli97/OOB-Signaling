/**
 * @file threadpool.h
 * @brief Interfaccia per la struttura dati threadpool.
 *
 * @author Alessio Bardelli 544270
 * 
 * Si dichiara che il contenuto di questo file è in ogni sua parte opera
 * originale dell'autore
 */

#ifndef THREADPOOL_H_
#define THREADPOOL_H_

#include <pthread.h>
#include <unistd.h>
#include <utils.h>

#define MAX_THREADS 64
#define MAX_QUEUE 65536

typedef struct threadpool_t threadpool_t;

/**
 * @enum threadpool_destroy_flags_t
 * @brief Flag per specificare come si vuole 
 *        terminare il threadpool.
 */
typedef enum {

    threadpool_immediate = 0,
    threadpool_graceful = 1

} threadpool_destroy_flags_t;

/**
 * @function threadpool_create
 * @brief Crea e restituisce un threadpool correttamente inizializzato.
 * @param thread_count Numero di thread worker.
 * @param queue_size   Dimensione della coda.
 */
threadpool_t* threadpool_create(int thread_count, int queue_size);

/**
 * @function threadpool_add
 * @brief Aggiunge un nuovo task nella coda del threadpool.
 * @param pool     Threadpool a cui si aggiunge il task.
 * @param function Funzione che eseguirà il thread.
 * @param argument Argomento passato alla funzione @function.
 * @return 0 successo, -1 altrimenti.
 */
int threadpool_add(threadpool_t* pool, void (*routine) (void*), void *arg);

/**
 * @function threadpool_destroy
 * @brief Termina e distrugge il threadpool.
 * @param pool  Threadpool da distruggere.
 * @param flags Flags per la terminazione.
 * @return 0 successo, -1 altrimenti.
 *
 * NOTE: valori accettabili per flag sono quelli definiti dalla
 *       enumerazione @threadpool_destroy_flags_t. In ogni caso
 *       il threadpool non accetterà nuovi task. Quando il flag vale 
 *       threadpool_graceful il threadpool processera tutti i task pendenti
 *       prima di terminare.
 */
int threadpool_destroy(threadpool_t* pool, int flags);

#endif // THREADPOOL_H_
