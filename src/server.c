#define _POSIX_C_SOURCE 199309L

/**
 * @file server.c
 * @brief Sorgente principale del server.
 *
 * @author Alessio Bardelli 544270
 * 
 * Si dichiara che il contenuto di questo file è in ogni sua parte opera
 * originale dell'autore
 */

#include <sys/time.h>
#include <sys/select.h>
#include <signal.h>
#include <connection.h>
#include <threadpool.h>
#include <netinet/in.h>

#define MAX_CONNECTION 20 // Numero massimo di connessioni.
#define QUEUE_SIZE 20 // Dimensione della coda del thread pool. 

static threadpool_t* tp = NULL; // Thread pool per gestire le connessioni con i client.

// Variabile utilizzata per interrompere il ciclo del server.
static volatile sig_atomic_t stop = false; 

// Id del server, file descriptor della pipe con 
// il supervisor e file descriptor della socket.
static int server_id, pfd, fd_skt;

static Address_t addr; // Indirizzo del server.

// Gestione dei segnali:
//   alla ricezione di SIGTERM si esce dal ciclo del server;
//   SIGINT viene ignorato.
static struct sigaction intHandler, termHandlar;

/**
 * @function sigTermHandler
 * @brief Funzione per la gestione di SIGTERM.
 * 
 * NOTA: Si noti che tale funzione è signal-safe.
 *       Difatti, tale funzione, si limita ad aggiornare
 *       lo stato interno.
 */
static void sigTermHandler(int signum) { stop = true; }

/**
 * @function timeval_sub
 * @brief Utilizzata per effettuare la differenza tra due oggetti 
 *        di tipo struct timeval.
 * @param result Viene scritto il risultato della differenza tra x e y.
 */
static int timeval_sub(struct timeval* result, struct timeval* x, struct timeval* y) {

    /* Perform the carry for the later subtraction by updating y. */
    if (x->tv_usec < y->tv_usec) {
    
        int nsec = (y->tv_usec - x->tv_usec) / 1000000 + 1;
        y->tv_usec -= 1000000 * nsec;
        y->tv_sec += nsec;
    }

    if (x->tv_usec - y->tv_usec > 1000000) {
    
        int nsec = (y->tv_usec - x->tv_usec) / 1000000;
        y->tv_usec += 1000000 * nsec;
        y->tv_sec -= nsec;
    }

    /* Compute the time remaining to wait.
       tv_usec is certainly positive. */
    result->tv_sec = x->tv_sec - y->tv_sec;
    result->tv_usec = x->tv_usec - y->tv_usec;

    /* Return 1 if result is negative. */
    return x->tv_sec < y->tv_sec;
}

/**
 * @function task
 * @brief Task che viene eseguito da un thread del pool,
 *        gestisce tutta la connessione con il client e 
 *        stima il secret in base ai messaggi che il client gli invia.
 *        Quando il client termina la connessione procede con l'invio
 *        della sua stima del secret al supervisor.
 * @param arg Array di 3 interi:
 *        arg[0] -> file descriptor della connessione con il client;
 *        arg[1] -> file descriptor della pipe con il supervisor;
 *        arg[2] -> identificatore del server.
 */
static void task(void* arg) {

    char msg[64]; struct timeval lst_message, prec_message, estimate;
    prec_message.tv_usec = INT_MAX; prec_message.tv_sec = LONG_MAX;
    int stima_secret = INT_MAX;

    int fd_c = ((int*)arg)[0];
    int pfd = ((int*)arg)[1];
    int server_id = ((int*)arg)[2];

	long long int ID = -1;

	while (myread(fd_c, msg, 64) != 0) {

        gettimeofday(&lst_message, NULL);

        ID = ntohl(atol(msg)); memset(msg, '\0', 64);

        printf("SERVER %d INCOMING FROM %x @ %ld.%d\n", server_id, (int)ID, lst_message.tv_sec, (int)lst_message.tv_usec/1000);
        fflush(stdout);

        timeval_sub(&estimate, &lst_message, &prec_message);

        char est[32]; long min_est;

        snprintf(est, 32, "%ld%03d", labs(estimate.tv_sec), abs(estimate.tv_usec/1000));

		if ((min_est = strtoll(est, NULL, 10)) == 0) {
            fprintf(stderr, "server: task: strtol"); exit(EXIT_FAILURE); }

        if (stima_secret > min_est)
            stima_secret = min_est;
        
        prec_message = lst_message;
    }

    close(fd_c); memset(msg, '\0', 64);

	if (ID != -1 && stima_secret != INT_MAX) {
	    snprintf(msg, 64, "%lld,%d", ID, stima_secret); mywrite(pfd, msg); }
    
    printf("SERVER %d CLOSING %x ESTIMATES %d\n", server_id, (int)ID, stima_secret);
    fflush(stdout);
}

int main(int argc, char** argv) {

    char sockname[UNIX_PATH_MAX]; int fd_c;
    fd_set set, rdset; FD_ZERO(&set); FD_ZERO(&rdset);

    // Parso dagli argomenti del main l'identificatore del server,
    // e il file descriptor della pipe con il supervisor.
    server_id = stol(argv[1], 10);
    pfd = stol(argv[2], 10);
    
    // Istallazione dei gestori dei segnali.
    memset(&intHandler, 0, sizeof(intHandler));
    memset(&termHandlar, 0, sizeof(termHandlar));
    intHandler.sa_handler = SIG_IGN;
    termHandlar.sa_handler = sigTermHandler;
    sigaction(SIGINT, &intHandler, NULL);
    sigaction(SIGTERM, &termHandlar, NULL);

    unlink(sockname); // Elimino eventuali socket rimaste da esecuzioni precedenti.

    // Creo la socket del server e inizializzo l'indirizzo del server.
    snprintf(sockname, UNIX_PATH_MAX, "OOB-server-%d", server_id); ADDRESS_INIT(addr, sockname);
    MENO1(fd_skt = socket(AF_UNIX, SOCK_STREAM, 0), "server: main: socket", exit(EXIT_FAILURE))

    // Faccio il bind tra la socket e l'indirizzo del server e mi preparo per accettare connessioni.
    MENO1(bind(fd_skt, (struct sockaddr*) &addr, sizeof(addr)), "server: main: bind", exit(EXIT_FAILURE))
    MENO1(listen(fd_skt, MAX_CONNECTION), "server: main: listen", exit(EXIT_FAILURE)) FD_SET(fd_skt, &set);

    // Inizializzazione del thread pool. 
	NULL_ERR (
        tp = threadpool_create(MAX_CONNECTION, QUEUE_SIZE), 
        "server: main: threadpool_create", 
        close(fd_skt); exit(EXIT_FAILURE)
    )

    // Stampa del messaggio di avvio.
	printf("SERVER %d ACTIVE\n", server_id); fflush(stdout);

    // Fin tanto che non ricevo SIGTERM...
    while (!stop) {

        rdset = set; struct timeval timeout = {0, 150};

        // Seleziono i canali che sono pronti per operazioni di I/O. 
        // Se vengo interrotto durante la SC la riattivo esplicitamente. 
        while (select(fd_skt+1, &rdset, NULL, NULL, &timeout) == -1 && errno == EINTR);

        // Se la socket del server è pronta per operazioni di I/O...
        if (FD_ISSET(fd_skt, &rdset)) {

            // Accetto la nuova conessione da parte del client.
            MENO1(fd_c = accept(fd_skt, NULL, 0), "server: main: accept", goto err)
            printf("SERVER %d CONNECT FROM CLIENT\n", server_id); fflush(stdout);

            // Metto in coda un nuovo task.
            int arg[] = {fd_c, pfd, server_id};
		    threadpool_add(tp, &task, (void*)arg);
        }
    }

    // Libero la memoria e chiudo i descrittori di file.
	threadpool_destroy(tp, threadpool_graceful);
	close(fd_skt); unlink(sockname); close(pfd);

	return 0;

    err: {

        threadpool_destroy(tp, threadpool_immediate);
	    close(fd_skt); unlink(sockname); close(pfd);
        exit(EXIT_FAILURE);
    }
}
