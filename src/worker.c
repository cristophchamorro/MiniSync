#include "../include/worker.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <limits.h>

long copiarArchivo(const char *origen, const char *destino) {
	int fd_entrada = open(origen, O_RDONLY);
	if (fd_entrada < 0) return -1;
	int fd_salida = open(destino, O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (fd_salida < 0) {
		close(fd_entrada);
		return -1;
	}
	char buffer[4096];
	ssize_t bytes_leidos;
	long total_bytes = 0;
	while ((bytes_leidos = read(fd_entrada, buffer, sizeof(buffer))) > 0) {
		ssize_t bytes_escritos = write(fd_salida, buffer, bytes_leidos);
		if (bytes_escritos != bytes_leidos) {
			close(fd_entrada);
			close(fd_salida);
			return -1;
		}
		total_bytes += bytes_escritos;	
	}

	fsync(fd_salida);
	close(fd_entrada);
	close(fd_salida);
	return total_bytes;
}

void proceso_worker(int pipe_lectura, struct estadisticas *shm, sem_t *sem, const char *dir_backup) {
	int fd_fifo = open(RUTA_FIFO, O_WRONLY);
	char buffer[PATH_MAX + 20];
	int posicion = 0;
	char caracter;
	while (read(pipe_lectura, &caracter, 1) > 0) {
		if (caracter == '\n') {
			buffer[posicion] = '\0'; 
			if (strncmp(buffer, "COPIAR ", 7) == 0) {
				char *ruta_archivo = buffer + 7;
				char *nombre_archivo = strrchr(ruta_archivo, '/');
				nombre_archivo = (nombre_archivo) ? nombre_archivo + 1 : ruta_archivo;
				char ruta_destino[PATH_MAX];
				snprintf(ruta_destino, sizeof(ruta_destino), "%s/%s", dir_backup, nombre_archivo);
				long copiado = copiarArchivo(ruta_archivo, ruta_destino);
				sem_wait(sem);
				if (copiado >= 0) {
					shm->archivos_copiados++;
					shm->bytes_copiados += copiado;
					char msj_log[PATH_MAX + 32];
					snprintf(msj_log, sizeof(msj_log), "copiado %s\n", nombre_archivo);
					if (fd_fifo >= 0) {					
						if (write(fd_fifo, msj_log, strlen(msj_log)) < 0) {
							perror("Error al escribir en FIFO del logger");
						}
					}
				} else {
					shm->errores++;
				}
				sem_post(sem);
			}
			posicion = 0; 
		} else {
			if (posicion < (int)(sizeof(buffer) - 1)) {
				buffer[posicion++] = caracter;
			}
		}
	}
}

