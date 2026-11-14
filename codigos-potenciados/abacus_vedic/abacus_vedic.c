/* abacus_vedic.c - Suanpan y multiplicacion vedic implementados sobre las
 * cuentas (no sobre aritmetica moderna de enteros).
 *
 * Compilar: gcc -std=c11 -O2 -Wall -Wextra abacus_vedic.c abacus_vedic_test.c -lm -o prueba
 */

#include "abacus_vedic.h"
#include <limits.h>

/* Valor decimal de un poste; los postes fuera del rango valido valen 0 */
static int valor_poste(const suanpan_estado_t *estado, int indice) {
    if (indice < 0 || indice >= estado->digitos) {
        return 0;
    }
    return 5 * estado->cielo[indice] + estado->tierra[indice];
}

/* Vuelca digitos decimales (0..9) al estado de cuentas */
static void escribir_digitos(suanpan_estado_t *estado, const int digitos[], int n) {
    int i;
    estado->digitos = n;
    for (i = 0; i < n; ++i) {
        estado->cielo[i] = (digitos[i] >= 5) ? 1 : 0;
        estado->tierra[i] = digitos[i] % 5;
    }
}

/* Desplaza el abaco `postes` posiciones hacia la izquierda (x 10^postes) */
static int desplazar(suanpan_estado_t *destino, const suanpan_estado_t *origen,
                     int postes) {
    int i;
    if (postes < 0) {
        return ABACUS_ERROR_ARGUMENTO;
    }
    if (origen->digitos + postes > SUANPAN_MAX_DIGITOS) {
        return ABACUS_ERROR_DESBORDE;
    }
    destino->digitos = origen->digitos + postes;
    for (i = 0; i < postes; ++i) {
        destino->cielo[i] = 0;
        destino->tierra[i] = 0;
    }
    for (i = 0; i < origen->digitos; ++i) {
        destino->cielo[i + postes] = origen->cielo[i];
        destino->tierra[i + postes] = origen->tierra[i];
    }
    return ABACUS_OK;
}

int suanpan_desde_decimal(suanpan_estado_t *estado, unsigned long long valor) {
    int i = 0;
    if (!estado) {
        return ABACUS_ERROR_ARGUMENTO;
    }
    do {
        int digito = (int)(valor % 10ULL);
        estado->cielo[i] = (digito >= 5) ? 1 : 0;
        estado->tierra[i] = digito % 5;
        i++;
        valor /= 10ULL;
    } while (valor > 0ULL && i < SUANPAN_MAX_DIGITOS);

    if (valor > 0ULL) {
        return ABACUS_ERROR_DESBORDE;
    }
    estado->digitos = i;
    return ABACUS_OK;
}

unsigned long long suanpan_a_decimal(const suanpan_estado_t *estado) {
    unsigned long long valor = 0ULL;
    unsigned long long potencia = 1ULL;
    int i;
    if (!estado) {
        return 0ULL;
    }
    for (i = 0; i < estado->digitos && i < SUANPAN_MAX_DIGITOS; ++i) {
        unsigned long long digito = (unsigned long long)valor_poste(estado, i);
        valor += digito * potencia;
        if (i + 1 < estado->digitos) {
            potencia *= 10ULL;
        }
    }
    return valor;
}

int suanpan_a_cadenas(const suanpan_estado_t *estado, int *cielo, int *tierra,
                      size_t capacidad) {
    int i;
    if (!estado || !cielo || !tierra) {
        return ABACUS_ERROR_ARGUMENTO;
    }
    if ((size_t)estado->digitos > capacidad) {
        return ABACUS_ERROR_CAPACIDAD;
    }
    for (i = 0; i < estado->digitos; ++i) {
        int posicion = estado->digitos - 1 - i; /* indice 0 = mas significativo */
        cielo[posicion] = estado->cielo[i];
        tierra[posicion] = estado->tierra[i];
    }
    return ABACUS_OK;
}

int suanpan_desde_cadenas(suanpan_estado_t *estado, const int *cielo,
                          const int *tierra, size_t n) {
    int i;
    if (!estado || !cielo || !tierra) {
        return ABACUS_ERROR_ARGUMENTO;
    }
    if (n > (size_t)SUANPAN_MAX_DIGITOS) {
        return ABACUS_ERROR_CAPACIDAD;
    }
    estado->digitos = (int)n;
    for (i = 0; i < estado->digitos; ++i) {
        int posicion = (int)n - 1 - i;
        estado->cielo[i] = cielo[posicion];
        estado->tierra[i] = tierra[posicion];
    }
    return ABACUS_OK;
}

int suanpan_suma(suanpan_estado_t *resultado, const suanpan_estado_t *a,
                 const suanpan_estado_t *b) {
    int digitos[SUANPAN_MAX_DIGITOS + 1];
    int n, i, carry = 0, total;
    if (!resultado || !a || !b) {
        return ABACUS_ERROR_ARGUMENTO;
    }
    n = (a->digitos > b->digitos) ? a->digitos : b->digitos;
    for (i = 0; i < n; ++i) {
        int suma = valor_poste(a, i) + valor_poste(b, i) + carry;
        digitos[i] = suma % 10;
        carry = suma / 10;
    }
    total = n;
    while (carry > 0) {
        if (total > SUANPAN_MAX_DIGITOS) {
            return ABACUS_ERROR_DESBORDE;
        }
        digitos[total] = carry % 10;
        carry /= 10;
        total++;
    }
    escribir_digitos(resultado, digitos, total);
    return ABACUS_OK;
}

int suanpan_resta(suanpan_estado_t *resultado, const suanpan_estado_t *a,
                  const suanpan_estado_t *b) {
    int digitos[SUANPAN_MAX_DIGITOS];
    int i, borrow = 0;
    if (!resultado || !a || !b) {
        return ABACUS_ERROR_ARGUMENTO;
    }
    if (suanpan_a_decimal(a) < suanpan_a_decimal(b)) {
        return ABACUS_ERROR_ARGUMENTO;
    }
    for (i = 0; i < a->digitos; ++i) {
        int diferencia = valor_poste(a, i) - valor_poste(b, i) - borrow;
        if (diferencia < 0) {
            diferencia += 10;
            borrow = 1;
        } else {
            borrow = 0;
        }
        digitos[i] = diferencia;
    }
    escribir_digitos(resultado, digitos, a->digitos);
    return ABACUS_OK;
}

int suanpan_multiplica_digito(suanpan_estado_t *resultado,
                              const suanpan_estado_t *a, int digito) {
    int digitos[SUANPAN_MAX_DIGITOS + 1];
    int i, carry = 0, total;
    if (!resultado || !a) {
        return ABACUS_ERROR_ARGUMENTO;
    }
    if (digito < 0 || digito > 9) {
        return ABACUS_ERROR_ARGUMENTO;
    }
    for (i = 0; i < a->digitos; ++i) {
        int producto = valor_poste(a, i) * digito + carry;
        digitos[i] = producto % 10;
        carry = producto / 10;
    }
    total = a->digitos;
    while (carry > 0) {
        if (total > SUANPAN_MAX_DIGITOS) {
            return ABACUS_ERROR_DESBORDE;
        }
        digitos[total] = carry % 10;
        carry /= 10;
        total++;
    }
    escribir_digitos(resultado, digitos, total);
    return ABACUS_OK;
}

int vedic_multiplicar(suanpan_estado_t *resultado, const suanpan_estado_t *a,
                      const suanpan_estado_t *b) {
    suanpan_estado_t acumulado;
    suanpan_estado_t parcial;
    suanpan_estado_t desplazado;
    int i, codigo;
    if (!resultado || !a || !b) {
        return ABACUS_ERROR_ARGUMENTO;
    }
    acumulado.digitos = 0;
    for (i = 0; i < b->digitos; ++i) {
        int digito = valor_poste(b, i);
        if (digito == 0) {
            continue;
        }
        codigo = suanpan_multiplica_digito(&parcial, a, digito);
        if (codigo != ABACUS_OK) {
            return codigo;
        }
        codigo = desplazar(&desplazado, &parcial, i);
        if (codigo != ABACUS_OK) {
            return codigo;
        }
        codigo = suanpan_suma(&acumulado, &acumulado, &desplazado);
        if (codigo != ABACUS_OK) {
            return codigo;
        }
    }
    *resultado = acumulado;
    return ABACUS_OK;
}

int vedic_cruces(const suanpan_estado_t *a, const suanpan_estado_t *b,
                 unsigned long long cruces[], size_t capacidad) {
    size_t n, m, i, j;
    if (!a || !b || !cruces) {
        return ABACUS_ERROR_ARGUMENTO;
    }
    if (a->digitos <= 0 || b->digitos <= 0) {
        return ABACUS_ERROR_ARGUMENTO;
    }
    n = (size_t)a->digitos;
    m = (size_t)b->digitos;
    if (n + m - 1ULL > capacidad) {
        return ABACUS_ERROR_CAPACIDAD;
    }
    for (i = 0; i < n + m - 1ULL; ++i) {
        cruces[i] = 0ULL;
    }
    for (i = 0; i < n; ++i) {
        for (j = 0; j < m; ++j) {
            unsigned long long izq = (unsigned long long)valor_poste(a, (int)i);
            unsigned long long der = (unsigned long long)valor_poste(b, (int)j);
            cruces[i + j] += izq * der;
        }
    }
    return ABACUS_OK;
}

int vedic_tabla(unsigned long long multiplicando, unsigned long long filas[9]) {
    int d;
    if (!filas) {
        return ABACUS_ERROR_ARGUMENTO;
    }
    if (multiplicando > ULLONG_MAX / 9ULL) {
        return ABACUS_ERROR_DESBORDE;
    }
    for (d = 1; d <= 9; ++d) {
        filas[d - 1] = multiplicando * (unsigned long long)d;
    }
    return ABACUS_OK;
}
