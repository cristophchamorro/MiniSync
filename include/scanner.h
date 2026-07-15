#ifndef SCANNER_H
#define SCANNER_H

void escanear_directorio_sync(const char *ruta_dir, const char *dir_backup, int *pipes_trabajadores, int *trabajador_actual, int num_trabajadores);

#endif

