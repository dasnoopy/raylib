CC      = gcc
TARGET  = gitmonitor
SRC     = gitmonitor.c

# Flag base C99 + avvisi + include locale (utile per raygui.h)
CFLAGS  = -std=c99 -Wall -Wextra -I.
LDFLAGS = -lraylib -lgit2 -lGL -lm -lpthread -ldl -lrt

# Target predefinito: compilazione standard
all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(SRC) -o $(TARGET) $(CFLAGS) -O2 $(LDFLAGS)

# Compilazione con simboli di debug (per gdb/valgrind)
debug: $(SRC)
	$(CC) $(SRC) -o $(TARGET) $(CFLAGS) -g -O0 $(LDFLAGS)

# Compila ed esegue subito
run: $(TARGET)
	./$(TARGET)

# Rimuove l'eseguibile
clean:
	rm -f $(TARGET)

# Rimuove eseguibile, stato salvato e cache dei repository
distclean: clean
	rm -f repo_state.txt
	rm -rf $(XDG_CACHE_HOME)/$(TARGET)/

.PHONY: all debug run clean distclean
