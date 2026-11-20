# Makefile - pruebas TestU01 de randu_plus
#
# Requiere TestU01 (L'Ecuyer & Simard) instalado. Indica el prefijo de
# instalacion con TESTU01_HOME, por ejemplo:
#
#   make TESTU01_HOME=/ruta/a/testu01
#
# Las cabeceras de randu_plus.h estan en el directorio padre
# (codigos-potenciados/randu_plus.h).

TESTU01_HOME ?= /usr/local
CODIGOS_DIR  := ..

CC      ?= cc
CFLAGS  ?= -O2 -Wall -Wextra -std=c99
CPPFLAGS += -I$(TESTU01_HOME)/include -I$(CODIGOS_DIR)
LDFLAGS  += -L$(TESTU01_HOME)/lib
# TestU01 se instala en tres librerias: testu01 (tests), mylib (util, num,
# chrono...) y probdist (distribuciones). El orden importa: los simbolos de
# testu01 dependen de mylib y probdist.
LDLIBS   += -ltestu01 -lprobdist -lmylib -lgmp -lm

BIN := run_randu_plus lattice_recovery

.PHONY: all clean check

all: $(BIN)

run_randu_plus: run_randu_plus.c uRanduPlus.c uRanduPlus.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ run_randu_plus.c uRanduPlus.c $(LDFLAGS) $(LDLIBS)

lattice_recovery: lattice_recovery.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ lattice_recovery.c $(LDFLAGS) $(LDLIBS)

check: lattice_recovery
	./lattice_recovery

clean:
	rm -f $(BIN)
