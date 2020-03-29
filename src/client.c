#define _POSIX_C_SOURCE 199309L

/**
 * @file client.c
 * @brief Sorgente principale del client.
 *
 * @author Alessio Bardelli 544270
 * 
 * Si dichiara che il contenuto di questo file è in ogni sua parte opera
 * originale dell'autore
 */

#include <arpa/inet.h>
#include <connection.h>
#include <utils.h>
#include <time.h>

static int P, K, W, secret; // Secret del client.
static long long int ID; // Id del client.

static int *indexs, *sockets, *choices;

static struct timespec timeout;

static char msg[16];

/**
 * @function Connect
 * @brief Connette il client al server, riprovando per @MAX_RETRY volte si il
 *        tentativo fallisce, ciascuna volta attendendo per @SLEEP_TIME secondi.
 * @param skt File descriptor della socket del server.
 * @param addr Indirizzo della socket del server.
 */
static void Connect(int skt, Address_t* addr) {

    int i = 0, res = -1;
    while (res == -1 && i < MAX_RETRY) {

        printf("Tentativo di connessione n° %d al server %s.\n", ++i, addr->sun_path);
        res = connect(skt, (struct sockaddr*)addr, sizeof(*addr));

        if (res == -1 && errno == ENOENT) {

            printf("Tentativo di connessione non riuscito, nuovo tentativo tra %d secondi.\n", SLEEP_TIME);
            sleep(SLEEP_TIME);

        } else if (res == -1 && errno != ENOENT) {

            printf("Connessione al server fallita.\n");
            exit(EXIT_FAILURE);

        } else if (res == 0) {

            printf("Connessione al server riuscita...\n");
            return;
        }
    }

    printf("Connessione al server fallita.\n");
    exit(EXIT_FAILURE);
}

/**
 * @functiom mix
 * @brief Funzione utilizzata per inizializzare il generatore pseudo-casuale.
 */ 
static unsigned long mix(unsigned long a, unsigned long b, unsigned long c) {
    
    a = a-b;  a = a-c;  a = a^(c >> 13);
    b = b-c;  b = b-a;  b = b^(a << 8);
    c = c-a;  c = c-b;  c = c^(b >> 13);
    a = a-b;  a = a-c;  a = a^(c >> 12);
    b = b-c;  b = b-a;  b = b^(a << 16);
    c = c-a;  c = c-b;  c = c^(b >> 5);
    a = a-b;  a = a-c;  a = a^(c >> 3);
    b = b-c;  b = b-a;  b = b^(a << 10);
    c = c-a;  c = c-b;  c = c^(b >> 15);
    
    return c;
}

/**
 * @function usage
 * @brief Stampa il messaggio di usage.
 * @param prog Nome del programma.
 */
static void usage(char* prog) {

    fprintf(stderr, "  Usage: %s P K W\n", prog);
    fprintf(stderr, "    Dove: 1 <= P < K  e  W > 3P\n");
    fprintf(stderr, "    P:int = # di server a cui connettersi\n");
    fprintf(stderr, "    K:int = # di server totali avviati\n");
    fprintf(stderr, "    W:int = # di messaggi da inviare\n");
    exit(EXIT_FAILURE);
}

/**
 * @function isin
 * @param indexs Array di interi.
 * @param idx Intero da cercare all'interno dell'array.
 * @param P Lunghezza dell'array.
 * @return true se @idx è presente nell'array @indexs, false altrimenti.
 */
static boolean isin(int* indexs, int idx, int P) {

    for (int i = 0; i < P; i++)
        if (indexs[i] == idx)
            return true;

    return false;
}

/**
 * @function clean_up
 * @brief Funzione che viene chiamata alla distruzione del processo,
 *        libera la memoria dinamica allocata e chiude le connessioni
 *        verso i server.
 */
static void clean_up() {

    if (indexs) free(indexs);

    if (choices) free(choices);

    if (sockets) {
        
        for (int i = 0; i < P; i++)
            close(sockets[i]);

        free(sockets);
    }
}

int main(int argc, char** argv) {

    int idx = -1; char sockname[UNIX_PATH_MAX];
    choices = indexs = sockets = NULL;

    if (argc < 4)
        usage(argv[0]);

    // Parso i parametri passati al main.
    P = (int)stol(argv[1], 10); K = (int)stol(argv[2], 10); W = (int)stol(argv[3], 10);
    
    // Controllo che i parametri passati al main siano coretti.
    if (P == -1 || K == -1 || W == -1 || P < 1 || P > K || !(W > (3*P)))
        usage(argv[0]);

    // Inizializzazione di secret e ID.
    srand(mix(clock(), time(NULL), getpid())); 
    ID = rand(); secret = (rand() % 3000) + 1;

    // Struttura dati che verrà utilizzata nella
	// nanosleep per effettuare l'attesa.
	timeout.tv_sec = (int)(secret/1000);
    timeout.tv_nsec = (secret % 1000) * 1e6;

    // Registro una funzione di clean up,
    // che sarà chiamata alla distruzione del processo.
    atexit(clean_up);

    // allocazione della memoria necessaria. 
    CALLOC(indexs, P, sizeof(int), "client: main: calloc 1", exit(EXIT_FAILURE))
    CALLOC(sockets, P, sizeof(int), "client: main: calloc 2", exit(EXIT_FAILURE))
    CALLOC(choices, W, sizeof(int), "client: main: calloc 3", exit(EXIT_FAILURE))
	memset(indexs, -1, P*sizeof(int));

	// Stampa del messaggio di avvio.
    printf("CLIENT %x SECRET %d\n", (int)ID, secret);

    // Scelta casuale dei server a cui connettersi e connessione.
    for (int i = 0; i < P; i++) {

        do { idx = rand() % K; }
        while (isin(indexs, idx, P));

        indexs[i] = idx; // Memorizzo in un array gli indici dei server a cui il client si collega.

        // Creo l'indirizzo del server.
        snprintf(sockname, UNIX_PATH_MAX, "OOB-server-%d", idx);
        Address_t addr; ADDRESS_INIT(addr, sockname);

        // Connetto il client al server. 
        MENO1(sockets[i] = socket(AF_UNIX, SOCK_STREAM, 0), "client: main: socket", exit(EXIT_FAILURE))
        Connect(sockets[i], &addr);
    }

	// Scelgo, per ogni messaggio, il server a cui inviarlo.
	// I server a cui inviare i messaggi vengono scelti 
	// preventivamente per rendere il più efficiente possibile 
	// la fase di invio dei messaggi ai suddetti server, 
	// garantendo così una stima più accurate del secret 
	// da parte dei server stessi.
    for (int i = 0; i < W; i++)
        choices[i] = rand() % P;

	// Genero il messaggio che dovrà 
	// essere inviato ai server.
    snprintf(msg, 16, "%ud", htonl(ID));

    // Fase di invio dei messaggi ai server
	//  e attesa tramite nanosleep.
    for (int i = 0; i < W; i++) {

        mywrite(sockets[choices[i]], msg);
        nanosleep(&timeout, NULL);
    }

	// Stampa del messaggio di terminazione.
    printf("CLIENT %x DONE\n", (int)ID);

	// Processo client terminato con successo.
    exit(EXIT_SUCCESS);
}
