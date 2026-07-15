C = gcc
CFLAGS = -Wall -Wextra -O2 -I./include
LDFLAGS = -lrt -lpthread
TARGET_DAEMON = minisync
TARGET_SCAN = scan
TARGET_DAEMON = minisync
TARGET_SCAN = scan

SRC_DAEMON = src/monitor.c src/worker.c src/scanner.c src/logger.c src/ipc.c
OBJ_DAEMON = $(SRC_DAEMON:.c=.o)

SRC_SCAN = src/scan.c
OBJ_SCAN = $(SRC_SCAN:.c=.o)

all: $(TARGET_DAEMON) $(TARGET_SCAN)

$(TARGET_DAEMON): $(OBJ_DAEMON)
		$(CC) $(CFLAGS) -o $@ $(OBJ_DAEMON) $(LDFLAGS)

$(TARGET_SCAN): $(OBJ_SCAN)
		$(CC) $(CFLAGS) -o $@ $(OBJ_SCAN)

src/%.o: src/%.c include/*.h
		$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET_DAEMON) $(TARGET_SCAN) src/*.o
	rm -f /tmp/sync_logger_fifo /tmp/minisync.log /tmp/minisync_stats.log
	rm -f /dev/shm/sync_shm_stats 2>/dev/null || true

.PHONY: all clean

