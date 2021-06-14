#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;
int ready = 0;
int data;

void *worker(void *p) {
        pthread_cond_wait(&cv, &mutex);
    printf("%d\n", data);
    return NULL;
}

int main() {
    pthread_t thread;
    pthread_create(&thread, NULL, worker, NULL);
    pthread_mutex_lock(&mutex);
    sleep(1);
    data = 1234;
    pthread_cond_signal(&cv);
    pthread_mutex_unlock(&mutex);
    pthread_join(thread, NULL);
    return 0;
}