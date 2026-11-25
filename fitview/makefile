CC      := gcc
TARGET  := fitview
BUILD   := build

# Flag di compilazione: standard C99, warning attivi, ottimizzazione e percorsi header
CFLAGS  := -std=c99 -Wall -Wextra -O2 -I. -Iinclude

# Flag del linker per Arch Linux (raylib + librerie di sistema)
LDFLAGS := -lraylib -lcurl -lGL -lm -lpthread -ldl -lrt

ifneq ($(OS),Windows_NT)
	CFLAGS += -DFIT_USE_STDINT_H
endif

# Lista dei file sorgente
SRCS := fitview.c \
        include/fit.c \
        include/fit_convert.c \
        include/fit_crc.c \
        include/fit_product.c

# Trasforma automaticamente i percorsi .c in build/.../*.o
OBJS := $(SRCS:%.c=$(BUILD)/%.o)

# File di dipendenza (.d) per ricompilare se modifichi un file .h
DEPS := $(OBJS:.o=.d)

# Regola principale
all: $(TARGET)

# Linking finale dell'eseguibile
$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

# Regola generica per compilare ogni file .c nel rispettivo .o dentro build/
$(BUILD)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

# Include le dipendenze generate automaticamente
-include $(DEPS)

# Comandi utili
run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD) $(TARGET)

.PHONY: all run clean

