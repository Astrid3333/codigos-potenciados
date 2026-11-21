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
#
# Enlace estatico por defecto (TESTU01_STATIC=1) si existen los archivos .a:
# asi el binario no necesita LD_LIBRARY_PATH en tiempo de ejecucion. Con
# TESTU01_STATIC=0 se enlaza contra las librerias compartidas.
TESTU01_STATIC ?= 1
TESTU01_LIBS_STATIC = $(TESTU01_HOME)/lib/libtestu01.a \
                      $(TESTU01_HOME)/lib/libprobdist.a \
                      $(TESTU01_HOME)/lib/libmylib.a
TESTU01_LIBS_SHARED = -ltestu01 -lprobdist -lmylib
HAVE_STATIC := $(wildcard $(TESTU01_HOME)/lib/libtestu01.a)
ifeq ($(TESTU01_STATIC),1)
ifneq ($(HAVE_STATIC),)
TESTU01_LIBS = $(TESTU01_LIBS_STATIC)
else
TESTU01_LIBS = $(TESTU01_LIBS_SHARED)
endif
else
TESTU01_LIBS = $(TESTU01_LIBS_SHARED)
endif
LDLIBS   += $(TESTU01_LIBS) -lgmp -lm

BIN := run_randu_plus run_prng_modules lattice_recovery

.PHONY: all clean check

all: $(BIN)

run_randu_plus: run_randu_plus.c uRanduPlus.c uRanduPlus.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ run_randu_plus.c uRanduPlus.c $(LDFLAGS) $(LDLIBS)

run_prng_modules: run_prng_modules.c uPrngModulos.c uPrngModulos.h
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ run_prng_modules.c uPrngModulos.c $(LDFLAGS) $(LDLIBS)

lattice_recovery: lattice_recovery.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ lattice_recovery.c $(LDFLAGS) $(LDLIBS)

check: lattice_recovery
	./lattice_recovery

clean:
	rm -f $(BIN)
