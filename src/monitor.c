#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <limits.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "../include/ipc.h"
#include "../include/worker.h"
#include "../include/escaner.h"
#include "../include/logger.h"

struct estadisticas *g_estadisticas_shm;
sem_t *g_sem;
pid_t g_pids[32];
int g_num_pids = 0;

void limpiar_y_salir(int sig) {
	(void)sig;
	for (int i = 0; i < g_num_pids; i++) {
		if (g_pids[i] > 0) kill(g_pids[i], SIGTERM);
	}
	munmap(g_estadisticas_shm, sizeof(struct estadisticas));
	shm_unlink(NOMBRE_SHM);
	sem_close(g_sem);
	sem_unlink(NOMBRE_SEM);
	unlink(RUTA_FIFO);
	exit(EXIT_SUCCESS);
}

void manejador_sigchld(int sig) {
	(void)sig;
	while (waitpid(-1, NULL, WNOHANG) > 0);
}

void demonizar() {
	pid_t pid = fork();
	if (pid < 0) exit(EXIT_FAILURE);
	if (pid > 0) exit(EXIT_SUCCESS);
	if (setsid() < 0) exit(EXIT_FAILURE);
	pid = fork();
	if (pid < 0) exit(EXIT_FAILURE);
	if (pid > 0) exit(EXIT_SUCCESS);
	if (chdir("/") < 0) exit(EXIT_FAILURE);
	int fd_nulo = open("/dev/null", O_RDWR);
	if (fd_nulo >= 0) {
		dup2(fd_nulo, STDIN_FILENO);
		dup2(fd_nulo, STDOUT_FILENO);
		dup2(fd_nulo, STDERR_FILENO);
		if (fd_nulo > STDERR_FILENO) close(fd_nulo);
	}
}

int main(int argc, char *argv[]) {
	if (argc < 3 || argc > 4) {
		exit(EXIT_FAILURE); 
	}
	        
	int num_trabajadores = (argc == 4) ? atoi(argv[3]) : 2;
	if (num_trabajadores < 1 || num_trabajadores > 10) num_trabajadores = 2;
	char dir_origen[PATH_MAX], dir_backup[PATH_MAX];
	if (!realpath(argv[1], dir_origen) || !realpath(argv[2], dir_backup)) {
		exit(EXIT_FAILURE);
	}
	demonizar();
	signal(SIGCHLD, manejador_sigchld);
	signal(SIGTERM, limpiar_y_salir);
	signal(SIGINT, limpiar_y_salir);
	unlink(RUTA_FIFO);
	if (mkfifo(RUTA_FIFO, 0666) == -1) exit(EXIT_FAILURE);
	g_estadisticas_shm = inicializar_memoria_compartida();
	g_sem = inicializar_semaforo();
	int fds_escritura_trabajadores[num_trabajadores];
	int pipes[num_trabajadores][2];
	for (int i = 0; i < num_trabajadores; i++) {
		if (pipe(pipes[i]) == -1) exit(EXIT_FAILURE);
	}
	pid_t pid_logger = fork();
	if (pid_logger == 0) {
		for (int j = 0; j < num_trabajadores; j++) {
			close(pipes[j][0]);
			close(pipes[j][1]);
		}
		proceso_logger();
		exit(0);
	}
	g_pids[g_num_pids++] = pid_logger;
	for (int i = 0; i < num_trabajadores; i++) {
		pid_t pid_worker = fork();
		if (pid_worker == 0) {
			for (int j = 0; j < num_trabajadores; j++) {
				close(pipes[j][1]); 
				if (j != i) close(pipes[j][0]); 
			}
			proceso_worker(pipes[i][0], g_estadisticas_shm, g_sem, dir_backup);
			exit(0);
		}
		close(pipes[i][0]); 
		fds_escritura_trabajadores[i] = pipes[i][1]; 
		g_pids[g_num_pids++] = pid_worker;
	}

	int trabajador_actual = 0;
	while (1) {
		escanear_directorio_sync(dir_origen, dir_backup, fds_escritura_trabajadores, &trabajador_actual, num_trabajadores);
		FILE *archivo_est = fopen("/tmp/minisync_stats.log", "w");
		if (archivo_est) {
			sem_wait(g_sem);
			fprintf(archivo_est, "--- ESTADÍSTICAS ---\n");
			fprintf(archivo_est, "Workers activos: %d\n", num_trabajadores);
			fprintf(archivo_est, "Archivos copiados: %ld\n", g_estadisticas_shm->archivos_copiados);
			fprintf(archivo_est, "Bytes copiados: %ld\n", g_estadisticas_shm->bytes_copiados);
			fprintf(archivo_est, "Errores: %ld\n", g_estadisticas_shm->errores);
			sem_post(g_sem);
			fclose(archivo_est);
		}
		sleep(5);
	}
	return 0;
}
