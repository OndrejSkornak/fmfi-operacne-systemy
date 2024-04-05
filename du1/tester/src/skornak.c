#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

// Veľkosť buffra pre čítanie a zápis
#define BUF_SIZE (1024*650)

// Globálny buffer a mutex pre synchronizáciu
char buffer[BUF_SIZE];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
// Podmienkové premenné pre signalizáciu medzi vláknami
pthread_cond_t writer_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t reader_cond = PTHREAD_COND_INITIALIZER;
// Premenné pre sledovanie stavu čítania a ukončenia
int readed = 0;
int done = 0;

// Funkcia pre vlákno zapisovača
void *writer(void *data) {
    while (1) {
        pthread_mutex_lock(&mutex);
        // Čakanie, kým nie je čo čítať
        while (readed == 0) {
            if (done) {
                pthread_mutex_unlock(&mutex);
                pthread_exit(NULL);
            }
            pthread_cond_wait(&writer_cond, &mutex);
        }

        // Zápis dát z buffra na štandardný výstup
        int tmp = 0;
        while(tmp < readed) {
            int w = write(STDOUT_FILENO, buffer + tmp, readed - tmp);
            if (w < 0) {
                perror("Error writing");
                exit(EXIT_FAILURE);
            }
            tmp += w;
        }

        // Resetovanie buffra a signalizácia čítaču
        readed = 0;
        pthread_cond_signal(&reader_cond);
        pthread_mutex_unlock(&mutex);
    }
}

// Funkcia pre vlákno čítača
void *reader(void *data) {
    while (1) {
        pthread_mutex_lock(&mutex);
        // Čakanie, kým nie je buffer plný
        while (readed > 0) {
            pthread_cond_wait(&reader_cond, &mutex);
        }

        // Čítanie dát do buffra zo štandardného vstupu
        int r = read(STDIN_FILENO, buffer, BUF_SIZE);
        if (r < 0) {
            perror("Error reading");
            exit(EXIT_FAILURE);
        } else if (r == 0) {
            // Ukončenie, ak nie sú žiadne dáta na čítanie
            done = 1;
            pthread_cond_signal(&writer_cond);
            pthread_mutex_unlock(&mutex);
            pthread_exit(NULL);
        }
        readed = r;
        // Signalizácia zapisovaču
        pthread_cond_signal(&writer_cond);
        pthread_mutex_unlock(&mutex);
    }
}

int main() {
    pthread_t in, out;

    // Vytvorenie vlákien
    if (pthread_create(&in, NULL, reader, NULL) != 0) {
        perror("Error creating reader thread");
        return EXIT_FAILURE;
    }

    if (pthread_create(&out, NULL, writer, NULL) != 0) {
        perror("Error creating writer thread");
        return EXIT_FAILURE;
    }

    // Čakanie na ukončenie vlákien
    if (pthread_join(in, NULL) != 0) {
        perror("Error joining reader thread");
        return EXIT_FAILURE;
    }

    if (pthread_join(out, NULL) != 0) {
        perror("Error joining writer thread");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}