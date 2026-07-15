#ifndef IPC_H
#define IPC_H
#include <semaphore.h>

#define RUTA_FIFO "/tmp/sync_logger_fifo"
#define NOMBRE_SHM "/sync_shm_stats"
#define NOMBRE_SEM "/sync_sem_stats"

struct estadisticas {
	long archivos_copiados;
	long bytes_copiados;
	long errores;
};

struct estadisticas* inicializar_memoria_compartida();
sem_t* inicializar_semaforo();

#endif

