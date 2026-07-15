#include "../include/ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <string.h>

struct estadisticas* inicializar_memoria_compartida() {
	int fd_shm = shm_open(NOMBRE_SHM, O_CREAT | O_RDWR, 0666);
	if (fd_shm < 0) {
		perror("Error shm_open");
		exit(EXIT_FAILURE);
	}
	if (ftruncate(fd_shm, sizeof(struct estadisticas)) == -1) {
		perror("Error ftruncate");
		exit(EXIT_FAILURE);
	}
	struct estadisticas *estadisticas_shm = mmap(0, sizeof(struct estadisticas), PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm, 0);
	if (estadisticas_shm == MAP_FAILED) {
		perror("Error mmap");
		exit(EXIT_FAILURE);
	}
	memset(estadisticas_shm, 0, sizeof(struct estadisticas));
	return estadisticas_shm;
}

sem_t* inicializar_semaforo() {
	sem_t *sem = sem_open(NOMBRE_SEM, O_CREAT, 0666, 1);
	if (sem == SEM_FAILED) {
		perror("Error sem_open");
		exit(EXIT_FAILURE);
	}
	return sem;
}
