## Mini Sistema de Sincronizacion de Archivos

Este proyecto es una version simple de un servicio de sincronizacion tipo Dropbox, 
que es hecho en C. La lógica del proyecto es que un proceso monitor está 
atento a un directorio, detecta que archivos han cambiado y para copiarlos 
reparte este trabajo entre algunos procesos worker, usando pipes, 
memoria compartida, semáforos y un proceso logger independiente.

## ESTRUCTURA DEL PROYECTO

Dentro de la carpeta MiniSync se encuentran dos subcarpetas:

- include: Dentro de esta carpeta se guardan todos los archivos .h. Estos archivos se usan para declarar estructuras y métodos necesarios para la implementación 
- scanner.h
- ipc.h
- logger.h
- worker.h 

- src: Dentro de esta carpeta se guardan todos los archivos .c. Estos archivos llaman a los .h para usar los métodos o estructuras ya declaradas

- scanner.c - archivo que implementa la lógica para el recorrido recursivo y la sincronización incremental
- ipc.c - archivo que implementa la lógica de memoria compartida y semáforos
- logger.c - es el proceso independiente llamado Logger, es elque lee del FIFO
- worker.c - es el proceso worker, el que copia los archivos 
- scan.c - logica del comando  scan
- monitor. - es el  proceso principal, es el daemon

Aparte se tiene el Makefile, donde están todos los comandos para compilar y ejecutar el proyectoy ese README

## REQUISITOS
Para este proyectoes necesario:
- Linux
- gcc
- Librerias -lrt -lpthread (para la memoria compartida POSIX y semaforos)

## COMPILAR

Ejecutar en la terminal el comando: make

Al ejecutar este comando se generan dos ejecutables:

1. scan – es el escaner recursivo de directorios (que es un comando independiente)
2. minisync – el proceso daemon (monitor + workers + logger)

NOTA: Para limpiar los binarios y los archivos temporales que genera el programa puede usar:

make clean

## COMO USARLO

### 1. Comando scan

Recorre un directorio recursivamente e imprime nombre, número de i-nodo,
tamaño, permisos y fecha de modificación de cada archivo (usando readdir(),
closedir(), stat() y lstat()).

./scan directorio

### 2. Daemon minisync

./minisync <directorio_origen> <directorio_backup> [num_workers]

- Hay que aclarar que para el directorio_backup, se debe usar mkdir para crearlo o asignar el directorio que se quiera
- directorio_origen: es la carpeta que se quiere sincronizar
- directorio_backup: es la carpeta destino donde se copian los archivos
- num_workers: es la cantidad de procesos worker, por defecto se usan 2

El programa se convierte en daemon (para esto se usa fork() + setsid() + chdir("/")),
asi que después de ejecutarlo, el proceso sigue corriendo en segundo plano.

## FUNCIONAMIENTO

- El monitor escanea el directorio origen cada 5 segundos, y va comparando
  tamaño y fecha de modificacion contra el archivo correspondiente en el
  backup. Si el archivo no existe en el backup, hubo cambio de tamaño, o el 
  origen es más nuevo, se manda a copiar.
- Las tareas se reparten entre los workers mediante pipes
  (usando pipe() + fork()), con mensajes tipo COPIAR ruta.
- Cada worker copia el archivo usando open(), read() y write()
  (mediante la función copiarArchivo()), actualiza una estructura de estadisticas en
  memoria compartida (para esto se usa shm_open() + mmap()), la cúal está protegida 
  con un semáforo POSIX (donde se usa sem_open(), sem_wait(), sem_post()), y le avisa
  al logger a traves de un FIFO con nombre (usando mkfifo()).
- El logger es un proceso aparte que lee del FIFO y escribe cada evento
  con fecha y hora en el archivo /tmp/minisync.log.
- El monitor tambien escribe un resumen de las estadisticas compartidas
  (archivos copiados, bytes copiados, errores) en el archivo
  /tmp/minisync_stats.log en cada ciclo.

## Archivos generados en /tmp y /dev/shm

 Archivo:

- /tmp/sync_logger_fifo -> FIFO usado por monitor/workers -> logger 
- /tmp/minisync.log -> log de eventos (archivos copiados) 
- /tmp/minisync_stats.log -> estadisticas actuales (archivos, bytes, errores) 
- /dev/shm/sync_shm_stats -> memoria compartida con la estructura de estadisticas 

## COMO DETENER DAEMON

Ya que minisync corre como daemon, no se detiene con Ctrl+C desde otra
terminal. Hay que buscar el PID y mandarle la señal:

 - pgrep minisync
 - kill pid

Al recibir SIGTERM o SIGINT, el monitor mata a los workers y al logger,
libera la memoria compartida, cierra el semaforo y borra el FIFO

