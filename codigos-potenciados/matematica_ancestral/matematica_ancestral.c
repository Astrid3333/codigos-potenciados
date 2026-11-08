/* matematica_ancestral.c - Implementación de algoritmos matemáticos
 * de tradiciones ancestrales de distintas culturas del mundo.
 *
 * Cada función incluye referencias históricas y comentarios sobre
 * su origen cultural.
 *
 * Referencias:
 *   1. Egipto antiguo: Papiro de Rhind (c. 1650 a.C.)
 *   2. Mesopotamia antigua: Tablillas babilónicas (c. 1800-1600 a.C.)
 *   3. India antigua: Aryabhatiya de Aryabhata (499 d.C.)
 *   4. China antigua: El método de los nueve capítulos (c. siglo I d.C.)
 *   5. Mesoamérica: Códices mayas (c. siglo II d.C. en adelante)
 *   6. China antigua: Sunzi Suanjing (c. siglo III-V d.C.)
 */

#include "matematica_ancestral.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* =========================================================================
 * 1. MULTIPLICACIÓN EGipcia (duplicación y adición)
 * =========================================================================
 *
 * Origen: Papiro de Rhind (c. 1650 a.C.), Egipto antiguo.
 *
 * El método egipcio de multiplicación descompone uno de los factores en
 * potencias de 2 y suma los múltiplos correspondientes del otro factor.
 * Es equivalente a la multiplicación binaria moderna pero documentada
 * milenios antes del sistema binario moderno.
 *
 * Ejemplo: 13 * 12
 *   1   12
 *   2   24
 *   4   48
 *   8   96
 *   Suma: 12 + 48 + 96 = 156 (porque 13 = 1 + 4 + 8)
 */

int64_t multiplicacion_egipcia(int64_t a, int64_t b) {
    int64_t resultado = 0;
    int64_t a_actual = a;
    int64_t b_actual = b;

    /* Manejar signos */
    int negativo = 0;
    if (a_actual < 0) { a_actual = -a_actual; negativo = !negativo; }
    if (b_actual < 0) { b_actual = -b_actual; negativo = !negativo; }

    while (b_actual > 0) {
        /* Si b_actual es impar, agregar a_actual al resultado */
        if (b_actual & 1) {
            resultado += a_actual;
        }
        /* Duplicar a_actual y reducir b_actual a la mitad */
        a_actual <<= 1;
        b_actual >>= 1;
    }

    return negativo ? -resultado : resultado;
}

/* ---------- 2. FRACCIONES EGipcias (algoritmo golardo) ---------- */

/*
 * Origen: Papiro de Rhind (c. 1650 a.C.), Egipto antiguo.
 *
 * Los antiguos egipcios representaban fracciones como sumas de fracciones
 * unitarias (fracciones con numerador 1), excepto 2/3 que tenían un símbolo
 * especial. El algoritmo golardo (nombrado por Fibonacci) encuentra una
 * descomposición en fracciones unitarias tomando repetidamente el mayor
 * unitario menor que el resto.
 */

int fracciones_egipcias(int numerador, int denominador,
                        int64_t denominadores[], int max_terminos) {
    int count = 0;

    if (numerador <= 0 || denominador <= 0) return 0;

    /* Los intermedios se mantienen en int64_t: numerador*m puede exceder int
     * y truncaría silenciosamente el residuo de la fracción. */
    int64_t num = numerador;
    int64_t den = denominador;

    while (num > 0 && count < max_terminos) {
        /* Encontrar el menor m tal que 1/m <= numerador/denominador */
        /* Es decir: m = ceil(denominador / numerador) */
        int64_t m = (den + num - 1) / num;

        denominadores[count++] = m;

        /* Restar 1/m del valor actual */
        /* numerador/denominador - 1/m = (numerador*m - denominador) / (denominador*m) */
        int64_t nuevo_numerador = num * m - den;
        int64_t nuevo_denominador = den * m;

        /* Simplificar por mcd */
        int64_t a = nuevo_numerador;
        int64_t b = nuevo_denominador;
        while (b != 0) {
            int64_t temp = b;
            b = a % b;
            a = temp;
        }
        num = nuevo_numerador / a;
        den = nuevo_denominador / a;
    }

    return count;
}

/* ---------- 3. RAIZ CUADRADA BABILONIA (método de Heron) ---------- */

/*
 * Origen: Tablillas babilónicas (c. 1800-1600 a.C.), Mesopotamia.
 *
 * Las tablillas babilónicas (como YBC 7289) muestran que conocían
 * el método para calcular raíces cuadradas mediante iteraciones:
 *   x_{n+1} = 0.5 * (x_n + S/x_n)
 *
 * Este es el mismo método que se le atribuye a Heron de Alejandría
 * (siglo I d.C.), pero los babilonios lo usaban casi 2000 años antes.
 */

double raiz_cuadrada_babilonia(double x, int iteraciones) {
    if (x < 0.0) return -1.0;  /* No definido para negativos */
    if (x == 0.0) return 0.0;

    /* Valor inicial: x/2 es una aproximación razonable */
    double valor = x / 2.0;
    if (valor == 0.0) valor = 1.0;

    for (int i = 0; i < iteraciones; i++) {
        valor = 0.5 * (valor + x / valor);
    }

    return valor;
}

/* ---------- 4. SENO DE ARYABHATA (India, 499 d.C.) ---------- */

/*
 * Origen: Aryabhatiya de Aryabhata (499 d.C.), India.
 *
 * Aryabhata describió una recurrencia para calcular tablas de senos
 * usando diferencias segundas. La fórmula es:
 *   s_{n+1} = 2*s_n - s_{n-1} - (s_n * k)
 * donde k es una constante relacionada con el paso angular.
 *
 * Esto permite calcular senos de forma recursiva sin funciones
 * trigonométricas, usando solo sumas y multiplicaciones.
 */

double seno_aryabhata(double theta, int pasos) {
    /* Normalizar theta a [0, 2*pi] */
    while (theta < 0.0) theta += 2.0 * M_PI;
    while (theta > 2.0 * M_PI) theta -= 2.0 * M_PI;

    /* Para pasos pequeños, usamos la aproximación directa */
    /* aryabhata_sin(theta) ≈ theta * R / (R + theta^2/6)  (aproximación) */
    /* Pero usamos la recurrencia de diferencias para ser más precisos */

    /* Número de pasos para cubrir el círculo */
    int n_pasos = pasos > 0 ? pasos : 360;
    double paso = 2.0 * M_PI / n_pasos;

    /* Encontrar el índice más cercano a theta */
    int indice = (int)(theta / paso);
    if (indice < 0) indice = 0;
    if (indice > n_pasos) indice = n_pasos;

    /* Calcular senos recursivamente usando la fórmula de diferencias
     * segundos de Aryabhata:
     *   s_{n+1} = 2*s_n - s_{n-1} - k*s_n
     * donde k = 2*(1 - cos(paso)) ≈ paso^2 para pasos pequeños
     *
     * Empezamos con s_0 = 0, s_1 = sin(paso) ≈ paso (para pasos pequeños)
     * Pero para ser más precisos, calculamos s_1 usando la serie de Taylor
     * o asumimos un valor inicial pequeño.
     */

    double k = 2.0 * (1.0 - cos(paso));  /* ≈ paso^2 para pasos pequeños */

    /* Valores iniciales */
    double s_prev = 0.0;                    /* sin(0) */
    double s_actual = sin(paso);            /* sin(paso) - usamos libm solo para iniciar */
    /* Nota: en una implementación puramente aritmética, s_1 se calcularía
     * mediante serie de Taylor u otro método */

    if (indice == 0) return 0.0;
    if (indice == 1) return s_actual;

    double s_siguiente;
    for (int n = 1; n < indice; n++) {
        s_siguiente = 2.0 * s_actual - s_prev - k * s_actual;
        s_prev = s_actual;
        s_actual = s_siguiente;
    }

    return s_actual;
}

/* ---------- 5. TEOREMA CHINO DEL RESTO ---------- */

/*
 * Origen: Sunzi Suanjing (c. siglo III-V d.C.), China antigua.
 * También documentado por Qin Jiushao (siglo XIII d.C.).
 *
 * Resuelve sistemas de congruencias lineales simultáneas.
 */

static int64_t mcd_extendido(int64_t a, int64_t b, int64_t *x, int64_t *y) {
    if (a == 0) {
        *x = 0;
        *y = 1;
        return b;
    }
    int64_t x1, y1;
    int64_t gcd = mcd_extendido(b % a, a, &x1, &y1);
    *x = y1 - (b / a) * x1;
    *y = x1;
    return gcd;
}

int64_t inverso_modular(int64_t a, int64_t m) {
    int64_t x, y;
    int64_t g = mcd_extendido((a % m + m) % m, m, &x, &y);
    if (g != 1) return -1;  /* No tiene inverso */
    return ((x % m) + m) % m;
}

int64_t teorema_chino_resto(const int64_t residuos[],
                            const int64_t modulos[],
                            int n,
                            int64_t *M_result) {
    if (n <= 0) return 0;

    int64_t M = 1;
    for (int i = 0; i < n; i++) {
        M *= modulos[i];
    }

    int64_t resultado = 0;
    for (int i = 0; i < n; i++) {
        int64_t Mi = M / modulos[i];
        int64_t inv = inverso_modular(Mi % modulos[i], modulos[i]);
        if (inv == -1) return -1;  /* No son coprimos */
        resultado += residuos[i] * Mi * inv;
    }

    int64_t resultado_final = ((resultado % M) + M) % M;

    if (M_result) *M_result = M;
    return resultado_final;
}

/* ---------- 6. SISTEMA VIGESIMAL MAYA ------------------------------ */

/*
 * Origen: Civilización maya (c. siglo II d.C. en adelante), Mesoamérica.
 *
 * Los mayas usaban un sistema vigesimal (base 20) con un concepto de cero
 * (representado por un glifo de concha). Sus números se escribían con
 * puntos (1) y barras (5).
 *
 * Este sistema les permitió calcular fechas astronómicas con gran
 * precisión y desarrollar el calendario maya.
 */

int sistema_vigesimal_maya(uint64_t valor, int digitos[], int max_digitos) {
    int count = 0;
    uint64_t v = valor;

    if (v == 0) {
        if (max_digitos > 0) {
            digitos[0] = 0;
            return 1;
        }
        return 0;
    }

    while (v > 0 && count < max_digitos) {
        digitos[count++] = (int)(v % 20);
        v /= 20;
    }

    return count;
}

/* ---------- 7. ELIMINACIÓN CHINA -------------------------------- */

/*
 * Origen: El método de los nueve capítulos (c. siglo I d.C.), China antigua.
 *
 * El método de eliminación para resolver sistemas de ecuaciones lineales
 * aparece en "Los nueve capítulos sobre el arte matemático" (Jiuzhang
 * Suanshu), siglos antes que Gauss. El método usa reducción por filas
 * para convertir el sistema en forma triangular y luego resolver por
 * sustitución hacia atrás.
 */

int eliminacion_china(double **A, double *b, double *x, int n) {
    /* Eliminación hacia adelante */
    for (int col = 0; col < n; col++) {
        /* Buscar pivote máximo */
        int pivote = col;
        double max_val = fabs(A[col][col]);
        for (int fila = col + 1; fila < n; fila++) {
            if (fabs(A[fila][col]) > max_val) {
                max_val = fabs(A[fila][col]);
                pivote = fila;
            }
        }

        if (max_val < 1e-14) return -1;  /* Matriz singular */

        /* Intercambiar filas */
        if (pivote != col) {
            double *temp = A[col];
            A[col] = A[pivote];
            A[pivote] = temp;
            double temp_b = b[col];
            b[col] = b[pivote];
            b[pivote] = temp_b;
        }

        /* Eliminación */
        for (int fila = col + 1; fila < n; fila++) {
            double factor = A[fila][col] / A[col][col];
            for (int k = col; k < n; k++) {
                A[fila][k] -= factor * A[col][k];
            }
            b[fila] -= factor * b[col];
        }
    }

    /* Sustitución hacia atrás */
    for (int i = n - 1; i >= 0; i--) {
        x[i] = b[i];
        for (int j = i + 1; j < n; j++) {
            x[i] -= A[i][j] * x[j];
        }
        x[i] /= A[i][i];
    }

    return 0;
}