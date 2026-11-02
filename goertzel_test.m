% Goertzel: forma K=2cos(w) vs forma alpha=4sin^2(w/2), referencia: fft
N = 2^17; rand('state',1); randn('state',1); x = randn(1,N);
X = fft(x);
function y = g_K(x,w)
  K = 2*cos(w); s1 = 0; s2 = 0;
  for v = x, s = v + K*s1 - s2; s2 = s1; s1 = s; end
  s = K*s1 - s2; y = s - exp(-1i*w)*s1;
end
function y = g_A(x,w)
  al = 4*sin(w/2)^2; s = 0; d = 0;
  for v = x, d = d + v - al*s; s = s + d; end
  d = d - al*s; s = s + d; y = s - exp(-1i*w)*(s - d);
end
function y = g_B(x,w)   % espejo para w > pi/2: beta = 4cos^2(w/2)
  be = 4*cos(w/2)^2; s = 0; d = 0;
  for v = x, d = v + be*s - d; s = d - s; end
  d = be*s - d; s = d - s; y = s - exp(-1i*w)*(d - s);
end
function y = g_H(x,w), if w < pi/2, y = g_A(x,w); else y = g_B(x,w); end, end
printf('%7s %10s %10s %10s %10s\n','k','omega','K=2cos','alpha','hibrida');
for k = [1 3 10 100 1000 N/8 N/4 N/2-1]
  w = 2*pi*k/N; R = X(k+1);
  printf('%7d %10.2e %10.1e %10.1e %10.1e\n', k, w, abs(abs(g_K(x,w))-abs(R))/abs(R), abs(abs(g_A(x,w))-abs(R))/abs(R), abs(abs(g_H(x,w))-abs(R))/abs(R));
end