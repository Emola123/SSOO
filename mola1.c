#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/time.h>

#define NHILOS 3
#define LINEAS 80

pthread_t *hilos;
int *ids;

char **buffer1;
char **buffer2;
char **buffer3;
char **buffer_validados;

int cont2 = 0, cont3 = 0, cont_validado = 0;

int listo_etapa1 = 0;
int listo_etapa2 = 0;

pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex3 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_val = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t cond2 = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond3 = PTHREAD_COND_INITIALIZER;

void* etapa1(void* arg);
void* etapa2(void* arg);
void* etapa3(void* arg);

int main() {
    struct timeval inicio, fin;
    gettimeofday(&inicio, NULL);

    int cantidad;
    FILE *file = fopen("input.txt", "r");
    if (!file) {
        perror("Error al abrir el archivo\n");
        exit(EXIT_FAILURE);
    }

    if (fscanf(file, "%d", &cantidad) != 1) {
        perror("Error al leer la variable cantidad\n");
        exit(EXIT_FAILURE);
    }

    buffer1 = calloc(cantidad, sizeof(char *));
    buffer2 = calloc(cantidad, sizeof(char *));
    buffer3 = calloc(cantidad, sizeof(char *));
    buffer_validados = calloc(cantidad, sizeof(char *));

    for (int i = 0; i < cantidad; i++) {
        buffer1[i] = calloc(LINEAS, sizeof(char));
        buffer2[i] = calloc(LINEAS, sizeof(char));
        buffer3[i] = calloc(LINEAS, sizeof(char));
        buffer_validados[i] = calloc(LINEAS, sizeof(char));
    }

    fgets(buffer1[0], LINEAS, file);
    for (int i = 0; i < cantidad; i++) {
        if (fgets(buffer1[i], LINEAS, file) == NULL) {
            perror("Error al copiar datos a buffer1\n");
            exit(EXIT_FAILURE);
        }
    }
    fclose(file);

    hilos = calloc(NHILOS, sizeof(pthread_t));
    ids = calloc(1, sizeof(int));
    *ids = cantidad;

    pthread_create(&hilos[0], NULL, etapa1, ids);
    pthread_create(&hilos[1], NULL, etapa2, ids);
    pthread_create(&hilos[2], NULL, etapa3, ids);

    for (int i = 0; i < NHILOS; i++) {
        pthread_join(hilos[i], NULL);
    }

    printf("\nTransacciones validadas completamente:\n");
    for (int i = 0; i < cont_validado; i++) {
        printf("%s", buffer_validados[i]);
    }

    gettimeofday(&fin, NULL);
    double tiempo = (fin.tv_sec - inicio.tv_sec) * 1000.0 + (fin.tv_usec - inicio.tv_usec) / 1000.0;
    printf("\nTiempo de ejecucion: %.2f ms\n", tiempo);

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
    free(hilos);
    free(ids);

    return EXIT_SUCCESS;
}

void* etapa1(void* arg) {
    int cantidad = *(int*)arg;

    for (int i = 0; i < cantidad; i++) {
        pthread_mutex_lock(&mutex1);
        if (strncmp(buffer1[i], "TRX:", 4) == 0 && strstr(buffer1[i], "->") && strstr(buffer1[i], ":")) {
            strcpy(buffer2[cont2], buffer1[i]);
            cont2++;
        }
        pthread_mutex_unlock(&mutex1);
    }

    pthread_mutex_lock(&mutex2);
    listo_etapa1 = 1;
    pthread_cond_signal(&cond2); 
    pthread_mutex_unlock(&mutex2);

    pthread_exit(NULL);
}

void* etapa2(void* arg) {
    int cantidad = *(int*)arg;

    pthread_mutex_lock(&mutex2);
    while (!listo_etapa1) {
        pthread_cond_wait(&cond2, &mutex2); 
    }
    pthread_mutex_unlock(&mutex2);

    for (int i = 0; i < cont2; i++) {
        pthread_mutex_lock(&mutex2);
        if (buffer2[i][0] == '\0') {
            pthread_mutex_unlock(&mutex2);
            continue;
        }

        char linea[LINEAS];
        strcpy(linea, buffer2[i]);

        char* monto = strrchr(linea, ':');
        if (!monto || strlen(monto) <= 1) {
            pthread_mutex_unlock(&mutex2);
            continue;
        }

        double monto1 = atof(monto + 1);
        if (monto1 > 0 && monto1 <= 10000) {
            strcpy(buffer3[cont3], buffer2[i]);
            cont3++;
        }

        pthread_mutex_unlock(&mutex2);
    }

    pthread_mutex_lock(&mutex3);
    listo_etapa2 = 1;
    pthread_cond_signal(&cond3); 
    pthread_mutex_unlock(&mutex3);

    pthread_exit(NULL);
}

void* etapa3(void* arg) {
    int cantidad = *(int*)arg;

    pthread_mutex_lock(&mutex3);
    while (!listo_etapa2) {
        pthread_cond_wait(&cond3, &mutex3);
    }
    pthread_mutex_unlock(&mutex3);

    for (int i = 0; i < cont3; i++) {
        pthread_mutex_lock(&mutex3);

        char* linea = buffer3[i];

        if (strstr(linea, "hacker") || strstr(linea, "cashout") || strstr(linea, "vault")) {
        } else {
            pthread_mutex_lock(&mutex_val);
            strcpy(buffer_validados[cont_validado++], linea);
            pthread_mutex_unlock(&mutex_val);
        }

        pthread_mutex_unlock(&mutex3);
    }

    pthread_exit(NULL);
}
