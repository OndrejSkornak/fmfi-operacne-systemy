#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#define BUF_SIZE (1024*650)

char buffer[BUF_SIZE];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t writer_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t reader_cond = PTHREAD_COND_INITIALIZER;
int readed = 0;
int done = 0; // Flag to indicate completion

void *writer(void *data) {
    while (1) {
        pthread_mutex_lock(&mutex);
        while (readed == 0) {
            if (done) {
                pthread_mutex_unlock(&mutex);
                pthread_exit(NULL);
            }
            pthread_cond_wait(&writer_cond, &mutex);
        }

        int tmp = 0;
        while(tmp < readed) {
            int w = write(STDOUT_FILENO, buffer + tmp, readed - tmp);
            if (w < 0) {
                perror("Error writing");
                exit(EXIT_FAILURE);
            }
            tmp += w;
        }

        readed = 0;
        pthread_cond_signal(&reader_cond);
        pthread_mutex_unlock(&mutex);
    }
}

void *reader(void *data) {
    while (1) {
        pthread_mutex_lock(&mutex);
        while (readed > 0) {
            pthread_cond_wait(&reader_cond, &mutex);
        }

        int r = read(STDIN_FILENO, buffer, BUF_SIZE);
        if (r < 0) {
            perror("Error reading");
            exit(EXIT_FAILURE);
        } else if (r == 0) {
            done = 1; // Set done flag if end-of-file is reached
            pthread_cond_signal(&writer_cond);
            pthread_mutex_unlock(&mutex);
            pthread_exit(NULL);
        }
        readed = r;
        pthread_cond_signal(&writer_cond);
        pthread_mutex_unlock(&mutex);
    }
}

int main() {
    pthread_t in, out;

    if (pthread_create(&in, NULL, reader, NULL) != 0) {
        perror("Error creating reader thread");
        return EXIT_FAILURE;
    }

    if (pthread_create(&out, NULL, writer, NULL) != 0) {
        perror("Error creating writer thread");
        return EXIT_FAILURE;
    }

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