#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>

#define MAX_LINEA 256
#define NHILOS 3

char **buffer1, **buffer2, **buffer3, **buffer_validados;
int cantidad, cont2 = 0, cont3 = 0, cont_validado = 0;

pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex3 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_val = PTHREAD_MUTEX_INITIALIZER;

pthread_barrier_t barrera1, barrera2;

void* hilo_func(void* arg) {
    int id = *(int*)arg;

    int base = cantidad / NHILOS;
    int extra = cantidad % NHILOS;
    int inicio = id * base + (id < extra ? id : extra);
    int fin = inicio + base + (id < extra ? 1 : 0);

    for (int i = inicio; i < fin; i++) {
        if (strncmp(buffer1[i], "TRX:", 4) == 0 && strstr(buffer1[i], "->") && strstr(buffer1[i], ":")) {
            pthread_mutex_lock(&mutex2);
            strcpy(buffer2[cont2++], buffer1[i]);
            pthread_mutex_unlock(&mutex2);
        }
    }

    pthread_barrier_wait(&barrera1);

    int limite2;
    pthread_mutex_lock(&mutex2);
    limite2 = cont2;
    pthread_mutex_unlock(&mutex2);

    for (int i = 0; i < limite2; i++) {
        char linea[MAX_LINEA];
        strcpy(linea, buffer2[i]);

        char* token = strrchr(linea, ':');
        if (!token || strlen(token) <= 1) continue;

        double monto = atof(token + 1);
        if (monto > 0 && monto <= 10000) {
            pthread_mutex_lock(&mutex3);
            strcpy(buffer3[cont3++], buffer2[i]);
            pthread_mutex_unlock(&mutex3);
        }
    }

    pthread_barrier_wait(&barrera2);

    int limite3;
    pthread_mutex_lock(&mutex3);
    limite3 = cont3;
    pthread_mutex_unlock(&mutex3);

    for (int i = 0; i < limite3; i++) {
        char* linea = buffer3[i];
        if (strstr(linea, "hacker") || strstr(linea, "cashout") || strstr(linea, "vault")) continue;

        pthread_mutex_lock(&mutex_val);
        strcpy(buffer_validados[cont_validado++], linea);
        pthread_mutex_unlock(&mutex_val);
    }

    pthread_exit(NULL);
}

int main() {
    struct timeval inicio, fin;
    gettimeofday(&inicio, NULL);

    FILE* file = fopen("input.txt", "r");
    if (!file) {
        perror("Error al abrir input.txt");
        exit(EXIT_FAILURE);
    }

    if (fscanf(file, "%d\n", &cantidad) != 1) {
        perror("Error al leer la cantidad");
        fclose(file);
        exit(EXIT_FAILURE);
    }

    buffer1 = (char **)calloc(cantidad, sizeof(char*));
    buffer2 = (char **)calloc(cantidad, sizeof(char*));
    buffer3 = (char **)calloc(cantidad, sizeof(char*));
    buffer_validados = (char **)calloc(cantidad, sizeof(char*));

    for (int i = 0; i < cantidad; i++) {
        buffer1[i] = (char *)calloc(MAX_LINEA, sizeof(char));
        buffer2[i] = (char *)calloc(MAX_LINEA, sizeof(char));
        buffer3[i] = (char *)calloc(MAX_LINEA, sizeof(char));
        buffer_validados[i] = (char *)calloc(MAX_LINEA, sizeof(char));
        if (fgets(buffer1[i], MAX_LINEA, file) == NULL) {
            buffer1[i][0] = '\0';
        }
    }
    fclose(file);

    pthread_barrier_init(&barrera1, NULL, NHILOS);
    pthread_barrier_init(&barrera2, NULL, NHILOS);

    pthread_t hilos[NHILOS];
    int ids[NHILOS];
    for (int i = 0; i < NHILOS; i++) {
        ids[i] = i;
        pthread_create(&hilos[i], NULL, hilo_func, &ids[i]);
    }

    for (int i = 0; i < NHILOS; i++) {
        pthread_join(hilos[i], NULL);
    }

    gettimeofday(&fin, NULL);
    double tiempo = (fin.tv_sec - inicio.tv_sec) * 1000.0 + (fin.tv_usec - inicio.tv_usec) / 1000.0;

    printf("\nTransacciones validadas completamente:\n");
    for (int i = 0; i < cont_validado; i++) {
        printf("%s", buffer_validados[i]);
    }

    printf("\nTiempo de ejecución: %.2f ms\n", tiempo);

    for (int i = 0; i < cantidad; i++) {
        free(buffer1[i]);
        free(buffer2[i]);
        free(buffer3[i]);
        free(buffer_validados[i]);
    }
    free(buffer1);
    free(buffer2);
    free(buffer3);
    free(buffer_validados);

    pthread_barrier_destroy(&barrera1);
    pthread_barrier_destroy(&barrera2);

    return 0;
}
