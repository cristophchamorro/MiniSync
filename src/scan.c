#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <limits.h>

void imprimir_permisos(mode_t modo) {
	printf("%c%c%c%c%c%c%c%c%c%c",
			S_ISDIR(modo) ? 'd' : S_ISLNK(modo) ? 'l' : '-',
			(modo & S_IRUSR) ? 'r' : '-', (modo & S_IWUSR) ? 'w' : '-', (modo & S_IXUSR) ? 'x' : '-',
			(modo & S_IRGRP) ? 'r' : '-', (modo & S_IWGRP) ? 'w' : '-', (modo & S_IXGRP) ? 'x' : '-',
			(modo & S_IROTH) ? 'r' : '-', (modo & S_IWOTH) ? 'w' : '-', (modo & S_IXOTH) ? 'x' : '-');
}

void escanear_dir(const char *ruta_dir) {
    DIR *dir = opendir(ruta_dir);
    if (!dir) return;
    struct dirent *entrada;
    struct stat estado_archivo;
    struct stat estado_destino;
    char ruta[PATH_MAX];
    while ((entrada = readdir(dir)) != NULL) {
        if (strcmp(entrada->d_name, ".") == 0 || strcmp(entrada->d_name, "..") == 0) continue;
        snprintf(ruta, sizeof(ruta), "%s/%s", ruta_dir, entrada->d_name);
        if (lstat(ruta, &estado_archivo) == -1) continue;
        printf("Archivo: %s\n", ruta);
        printf("  I-nodo: %lu\n", (unsigned long)estado_archivo.st_ino);
        printf("  Tamano: %lld bytes\n", (long long)estado_archivo.st_size);
        printf("  Permisos: ");
        imprimir_permisos(estado_archivo.st_mode);
        printf("\n");

        char buffer_tiempo[64];
        strftime(buffer_tiempo, sizeof(buffer_tiempo), "%Y-%m-%d %H:%M:%S", localtime(&estado_archivo.st_mtime));
        printf("  Modificacion: %s\n", buffer_tiempo);
        if (S_ISLNK(estado_archivo.st_mode)) {
            if (stat(ruta, &estado_destino) != -1) {
                printf("  -> Apunta a i-nodo: %lu (Tamano real: %lld bytes)\n",
                       (unsigned long)estado_destino.st_ino, (long long)estado_destino.st_size);
            }
        }
        printf("\n");
        if (S_ISDIR(estado_archivo.st_mode)) {
            escanear_dir(ruta);
        }
    }
    closedir(dir);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Uso: ./scan <directorio>\n");
        return EXIT_FAILURE;
    }
    escanear_dir(argv[1]);
    return EXIT_SUCCESS;
}
