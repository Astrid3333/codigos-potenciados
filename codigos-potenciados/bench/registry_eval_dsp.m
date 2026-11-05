function res = registry_eval_dsp(e)
  % Goertzel single-bin DFT (variants K, alpha, beta) vs analytic |X[k]| = N*A/2.
  N = e.N;
  nn = (0:N-1).';
  k = e.tone_k(:);
  A = e.tone_A(:);
  ph = e.tone_phi(:);
  x = zeros(N, 1);
  for t = 1:numel(k)
    x = x + A(t) * cos(2 * pi * k(t) * nn / N + ph(t));
  end
  names = {'K', 'alpha', 'beta'};
  mags = zeros(3, numel(k));
  for t = 1:numel(k)
    w = 2 * pi * k(t) / N;
    c = 2 * cos(w);
    s1 = 0; s2 = 0;
    for i = 1:N
      s0 = x(i) + c * s1 - s2; s2 = s1; s1 = s0;
    end
    mags(1, t) = sqrt(s1^2 + s2^2 - c * s1 * s2);
    s = 0; d = 0; a = c - 2;
    for i = 1:N
      d = d + x(i) + a * s; s = s + d;
    end
    sN = s; sP = s - d;
    mags(2, t) = sqrt(sN^2 + sP^2 - c * sN * sP);
    s = 0; ee = 0; b = c + 2;
    for i = 1:N
      ee = x(i) + b * s - ee; s = ee - s;
    end
    sN = s; sP = ee - s;
    mags(3, t) = sqrt(sN^2 + sP^2 - c * sN * sP);
  end
  checks = emptychecks();
  rt = e.ref.magnitude.rel_tol;
  spread = 0;
  for t = 1:numel(k)
    refv = N * A(t) / 2;
    for v = 1:3
      rel = abs(mags(v, t) - refv) / refv;
      checks(end + 1) = mkcheck(sprintf('%s_k%d', names{v}, k(t)), ...
                                mags(v, t), refv, rt * refv, rel, rel <= rt);
    end
    spread = max([spread, abs(mags(2, t) - mags(1, t)) / refv, ...
                  abs(mags(3, t) - mags(1, t)) / refv]);
  end
  st = e.ref.variant_spread.abs_tol;
  checks(end + 1) = mkcheck('variant_spread', spread, 0, st, spread, spread <= st);
  res = struct('name', e.name, 'family', e.family, ...
               'pass', all([checks.pass]), 'checks', checks);
end
