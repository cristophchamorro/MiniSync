#include "../include/logger.h"
#include "../include/ipc.h"
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <time.h>
#include <string.h>

void proceso_logger() {
	int fd = open(RUTA_FIFO, O_RDONLY);
	if (fd < 0) exit(EXIT_FAILURE);
	char buffer[512];
	int posicion = 0;
	char caracter;
	while (read(fd, &caracter, 1) > 0) {
		if (caracter == '\n') {
			buffer[posicion] = '\0';			
			time_t tiempo = time(NULL);
			struct tm *tm = localtime(&tiempo);
			char cadena_tiempo[64];
			strftime(cadena_tiempo, sizeof(cadena_tiempo), "%Y-%m-%d %H:%M:%S", tm);
			FILE *archivo_log = fopen("/tmp/minisync.log", "a");
			if (archivo_log) {
				fprintf(archivo_log, "[%s] %s\n", cadena_tiempo, buffer);
				fclose(archivo_log);
			}
			posicion = 0;
		} else {
			if (posicion < (int)(sizeof(buffer) - 1)) {
			buffer[posicion++] = caracter;
			}
		}
	}
}
