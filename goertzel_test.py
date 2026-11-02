import numpy as np, math
rng = np.random.default_rng(1)
def goertzel_K(x, w):
    K = 2*math.cos(w); s1 = s2 = 0.0
    for v in x:
        s = v + K*s1 - s2; s2 = s1; s1 = s
    s = K*s1 - s2                      # x[N] = 0
    return s - complex(math.cos(w), -math.sin(w))*s1
def goertzel_alpha(x, w):
    al = 4*math.sin(w/2)**2; s = d = 0.0
    for v in x:
        d = d + v - al*s               # d[n] = d[n-1] + x[n] - alpha*s[n-1]
        s = s + d
    d = d - al*s; s = s + d            # x[N] = 0
    s1 = s - d
    return s - complex(math.cos(w), -math.sin(w))*s1
def ref(x, w):
    n = np.arange(len(x), dtype=np.longdouble)
    return np.sum(x.astype(np.longdouble)*np.exp(-1j*np.longdouble(w)*n))
N = 2**17
x = rng.standard_normal(N)
X = np.fft.fft(x)
print(f"N = {N}, entrada ruido gaussiano. Error relativo en magnitud vs suma directa en long double\n")
print(f"{'bin k':>7} {'omega':>10} {'K=2cos':>10} {'alpha':>10} {'fft':>10}")
for k in [1, 3, 10, 100, 1000, N//8, N//4, N//2-1]:
    w = 2*math.pi*k/N; R = ref(x, w)
    eK  = abs(abs(goertzel_K(x, w))     - abs(R))/abs(R)
    eA  = abs(abs(goertzel_alpha(x, w)) - abs(R))/abs(R)
    eF  = abs(abs(X[k])                 - abs(R))/abs(R)
    print(f"{k:7d} {w:10.2e} {eK:10.1e} {eA:10.1e} {eF:10.1e}")

def goertzel_beta(x, w):
    # espejo para w > pi/2: beta = 4cos^2(w/2) = 2 + K ; d[n] = s[n] + s[n-1]
    be = 4*math.cos(w/2)**2; s = d = 0.0
    for v in x:
        d = v + be*s - d
        s = d - s
    d = be*s - d; s = d - s            # x[N] = 0
    s1 = d - s
    return s - complex(math.cos(w), -math.sin(w))*s1
def goertzel_hybrid(x, w):
    return goertzel_alpha(x, w) if w < math.pi/2 else goertzel_beta(x, w)

print("\nVariante hibrida (alpha si w<pi/2, beta si w>pi/2):")
print(f"{'bin k':>7} {'omega':>10} {'K=2cos':>10} {'hibrida':>10}")
for k in [1, 10, 1000, N//4-1, N//4+1, N//2-1000, N//2-10, N//2-1]:
    w = 2*math.pi*k/N; R = ref(x, w)
    eK = abs(abs(goertzel_K(x, w))      - abs(R))/abs(R)
    eH = abs(abs(goertzel_hybrid(x, w)) - abs(R))/abs(R)
    print(f"{k:7d} {w:10.2e} {eK:10.1e} {eH:10.1e}")