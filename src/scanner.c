#include "../include/scanner.h"
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <time.h>

/*
 * Sincronizacion incremental:
 * Para cada archivo del directorio origen, se compara su tamaio y fecha de modificación
 * directamente con el archivo ya existente en el directorio de backup
 */
void escanear_directorio_sync(const char *ruta_dir, const char *dir_backup, int *pipes_trabajadores, int *trabajador_actual, int num_trabajadores) {
	DIR *dir = opendir(ruta_dir);
	if (!dir) return;
	struct dirent *entrada;
	struct stat estado_origen, estado_destino;
	char ruta[PATH_MAX];
	char ruta_destino[PATH_MAX];
	while ((entrada = readdir(dir)) != NULL) {
		if (strcmp(entrada->d_name, ".") == 0 || strcmp(entrada->d_name, "..") == 0)
			continue;
		snprintf(ruta, sizeof(ruta), "%s/%s", ruta_dir, entrada->d_name);
		if (lstat(ruta, &estado_origen) == -1) continue;
		if (S_ISDIR(estado_origen.st_mode)) {
			escanear_directorio_sync(ruta, dir_backup, pipes_trabajadores, trabajador_actual, num_trabajadores);
		} else if (S_ISREG(estado_origen.st_mode)) {
			/* mismo esquema de nombre que usa worker.c: dir_backup/basename */
			snprintf(ruta_destino, sizeof(ruta_destino), "%s/%s", dir_backup, entrada->d_name);
			int necesita_copia = 1;
			if (stat(ruta_destino, &estado_destino) == 0) {
				if (estado_origen.st_size == estado_destino.st_size &&	estado_origen.st_mtime <= estado_destino.st_mtime) {
					necesita_copia = 0;
				}
			}
			if (necesita_copia) {
				char msj[PATH_MAX + 20];
				snprintf(msj, sizeof(msj), "COPIAR %s\n", ruta);
				if (write(pipes_trabajadores[*trabajador_actual], msj, strlen(msj)) < 0) {
					perror("Error al escribir en pipe del worker");
				}
				*trabajador_actual = (*trabajador_actual + 1) % num_trabajadores;
			}
		}
	}
	closedir(dir);
}
