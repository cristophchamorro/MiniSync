#ifndef WORKER_H
#define WORKER_H
#include "ipc.h"

void proceso_worker(int pipe_lectura, struct estadisticas *shm, sem_t *sem, const char *dir_backup);
long copiarArchivo(const char *origen, const char *destino);

#endif

