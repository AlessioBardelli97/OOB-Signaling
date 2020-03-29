#define _POSIX_C_SOURCE 199309L

/**
 * @file supervisor.c
 * @brief Sorgente principale del supervisor.
 *
 * @author Alessio Bardelli 544270
 * 
 * Si dichiara che il contenuto di questo file è in ogni sua parte opera
 * originale dell'autore
 */

#include <utils.h>
#include <dict.h>
#include <signal.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/select.h>

static Dict_t* dict;

static volatile sig_atomic_t stop = false;
static volatile sig_atomic_t print_request = false;

static int k;
static int* pids;
static int** pfds;

static time_t lasttime = 0;
static struct sigaction intHandler;
static void sigIntHandler(int signum) {

    if (time(NULL) - lasttime <= 1)
        stop = true;

    else
		print_request = true;

	// Si noti che la funzione time è signal-safe.    
	lasttime = time(NULL);
}

static void print_table(Dict_t* dict, FILE* file)  {

    long long int key; struct value_t value;

    foreach(dict, key, value) {

        fprintf(file, "SUPERVISOR ESTIMATE %d FOR %x BASED ON %d\n", value.miglior_stima, (int)key, value.count_server); 
        fflush(file);
    }
}

int main(int argc, char** argv) {

    fd_set set, rdset; FD_ZERO(&set); int fd_max = -1; pids = NULL; pfds = NULL; dict = NULL;

    if (argc < 2) {

        fprintf(stderr, "Usage: %s <num-of-server>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    k = (int)stol(argv[1], 10);

    CALLOC(pids, k, sizeof(int), "Supervisor: main: calloc 1", return -1)
    CALLOC(pfds, k, sizeof(int*), "Supervisor: main: calloc 2", return -1)

    memset(&intHandler, 0, sizeof(intHandler));
    intHandler.sa_handler = sigIntHandler;
    sigemptyset(&intHandler.sa_mask);
    sigaction(SIGINT, &intHandler, NULL);

    dict = initDict();

    printf("SUPERVISOR STARTING %d\n", k); fflush(stdout);

    for (int i = 0; i < k; i++) {

		CALLOC(pfds[i], 2, sizeof(int), "supervisor: main: calloc 3", return -1)

        MENO1(pipe(pfds[i]), "supervisor: main: pipe", return -1)

        MENO1(pids[i] = fork(), "supervisor: main: fork", return -1)

        // figlio, server...
        if (!pids[i]) {

            char arg1[16], arg2[16];
            snprintf(arg1, 16, "%d", i);
            snprintf(arg2, 16, "%d", pfds[i][1]);

            MENO1(close(pfds[i][0]), "server (forked by supervisor): main: close", exit(EXIT_FAILURE))

            execl("bin/server", "server", arg1, arg2, NULL);

            perror("server (forked by supervisor): main: execl");
            exit(EXIT_FAILURE);
        }

        // padre, supervisor...
        close(pfds[i][1]); FD_SET(pfds[i][0], &set);

        if (pfds[i][0] > fd_max) 
            fd_max = pfds[i][0];
    }

    while (!stop) {

		rdset = set; struct timeval timeout = {0, 150};

        while (select(fd_max+1, &rdset, NULL, NULL, &timeout) == -1 && errno == EINTR);

        if (print_request) {

            print_table(dict, stderr);
            print_request = false;
        }

        for (int i = 0; i < k; i++) {

            if (FD_ISSET(pfds[i][0], &rdset)) {

                char buffer[64]; char* tmp;
                MENO1(myread(pfds[i][0], buffer, 64), "supervisor: main: myread", exit(EXIT_FAILURE))

                long long int ID = atoll(strtok_r(buffer, ",", &tmp));
                int stima_secret = atoi(strtok_r(NULL, ",", &tmp));

                printf("SUPERVISOR ESTIMATE %d FOR %x FROM %d\n", stima_secret, (int)ID, i);
				fflush(stdout);

                struct value_t value = get_value(dict, ID);
                
                if (value.miglior_stima > stima_secret)
                    value.miglior_stima = stima_secret;
                
                value.count_server += 1;

                add(dict, ID, value);
            }
        }
    }

    print_table(dict, stdout);

	for (int i = 0; i < k; i++) {

		close(pfds[i][0]); free(pfds[i]);

		kill(pids[i], SIGTERM);

		waitpid(pids[i], NULL, 0);
	}

    printf("SUPERVISOR EXITING\n");

	deleteDict(dict); free(pfds); free(pids); return 0;
}
