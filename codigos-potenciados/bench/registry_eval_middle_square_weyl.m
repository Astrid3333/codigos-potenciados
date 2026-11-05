function res = registry_eval_middle_square_weyl(e)
  % Middle-square de von Neumann (4 digitos) vs Weyl vs TCR.
  % Compara periodo de ciclo y fraccion de semillas que colapsan a 0.
  params = e.params;
  digits = double(params.digits);
  modulus = double(params.modulus);
  weyl_s = double(params.weyl_s);
  p1 = double(params.tcr_p1);
  p2 = double(params.tcr_p2);
  max_iter = double(params.max_iter);

  ref = e.ref;
  checks = emptychecks();

  %% ---------- Middle-square original ----------
  function y = ms_next(x)
    s = sprintf('%08d', mod(x^2, 10^8));  % 8 digitos con leading zeros
    mid = str2double(s(3:6));              % 4 digitos del medio
    y = mid;
  end

  seen = false(1, modulus);
  cycles = {};
  n_cycles = 0;
  for x0 = 0:(modulus-1)
    x = x0; tr = [];
    while ~seen(x+1)
      seen(x+1) = true;
      tr = [tr, x];
      x = ms_next(x);
    end
    idx = find(tr == x);
    if ~isempty(idx)
      n_cycles = n_cycles + 1;
      cycles{n_cycles} = tr(idx:end);
    end
  end

  zero_sink = 0;
  for x0 = 0:(modulus-1)
    x = x0;
    for i = 1:50
      x = ms_next(x);
    end
    if x == 0
      zero_sink = zero_sink + 1;
    end
  end
  zs_rel = zero_sink / modulus;

  n_cycles_ref = double(ref.middle_square_cycles.value);
  tol_cycles = double(ref.middle_square_cycles.abs_tol);
  checks(end+1) = mkcheck('middle_square_cycles', n_cycles, n_cycles_ref, ...
                          tol_cycles, abs(n_cycles - n_cycles_ref)/max(abs(n_cycles_ref),1), ...
                          abs(n_cycles - n_cycles_ref) <= tol_cycles);

  zs_ref = double(ref.middle_square_zero_sink.value);
  zs_tol = double(ref.middle_square_zero_sink.abs_tol);
  checks(end+1) = mkcheck('middle_square_zero_sink', zs_rel, zs_ref, ...
                          zs_tol, abs(zs_rel - zs_ref), ...
                          abs(zs_rel - zs_ref) <= zs_tol);

  %% ---------- Weyl ----------
  function [x, w] = weyl_next(x, w, s)
    w = mod(w + s, modulus);
    s8 = sprintf('%08d', mod(x^2 + w, 10^8));
    x = str2double(s8(3:6));
  end

  s = weyl_s; w = 0; x = 1234;
  trw = []; seenw = false(1, modulus); weyl_period = 0;
  for i = 1:max_iter
    [x, w] = weyl_next(x, w, s);
    if seenw(x+1)
      j = find(trw == x, 1);
      weyl_period = i - j;
      break;
    end
    seenw(x+1) = true;
    trw = [trw, x];
  end

  wp_val = ref.weyl_period.value;
  if ~isempty(wp_val) && ~isnan(wp_val)
    wp_ref = double(wp_val);
    wp_tol = double(ref.weyl_period.abs_tol);
    checks(end+1) = mkcheck('weyl_period', weyl_period, wp_ref, ...
                            wp_tol, abs(weyl_period - wp_ref)/max(abs(wp_ref),1), ...
                            abs(weyl_period - wp_ref) <= wp_tol);
  else
    checks(end+1) = mkcheck('weyl_period', weyl_period, NaN, NaN, NaN, true);
  end

  %% ---------- TCR ----------
  M = p1 * p2;
  w1 = 0; w2 = 0; x = 1234;
  trcrt = []; seen_crt = false(1, modulus); tcr_period = 0;
  inv_p1_mod_p2 = mod(inv_mod(p1, p2), p2);
  for i = 1:max_iter
    w1 = mod(w1 + weyl_s, p1);
    w2 = mod(w2 + weyl_s, p2);
    k = mod((w2 - w1) * inv_p1_mod_p2, p2);
    w = mod(w1 + p1 * k, M);
    s8 = sprintf('%08d', mod(x^2 + w, 10^8));
    x = str2double(s8(3:6));
    if seen_crt(x+1)
      j = find(trcrt == x, 1);
      tcr_period = i - j;
      break;
    end
    seen_crt(x+1) = true;
    trcrt = [trcrt, x];
  end

  tp_val = ref.tcr_period.value;
  if ~isempty(tp_val) && ~isnan(tp_val)
    tp_ref = double(tp_val);
    tp_tol = double(ref.tcr_period.abs_tol);
    checks(end+1) = mkcheck('tcr_period', tcr_period, tp_ref, ...
                            tp_tol, abs(tcr_period - tp_ref)/max(abs(tp_ref),1), ...
                            abs(tcr_period - tp_ref) <= tp_tol);
  else
    checks(end+1) = mkcheck('tcr_period', tcr_period, NaN, NaN, NaN, true);
  end

  %% ---------- Variant spread ----------
  st = double(ref.variant_spread.abs_tol);
  vs_val = ref.variant_spread.value;
  spread = abs(weyl_period - tcr_period) / max(abs(tcr_period), 1);
  if ~isempty(vs_val) && ~isnan(vs_val)
    vs_ref = double(vs_val);
    checks(end+1) = mkcheck('variant_spread', spread, vs_ref, st, spread, spread <= st);
  else
    checks(end+1) = mkcheck('variant_spread', spread, NaN, NaN, NaN, true);
  end

  res = struct('name', e.name, 'family', e.family, ...
               'pass', all([checks.pass]), 'checks', checks, ...
               'middle_square_cycles', n_cycles, ...
               'middle_square_zero_sink', zs_rel, ...
               'weyl_period', weyl_period, ...
               'tcr_period', tcr_period, ...
               'tcr_modulus_M', M);
end

function inv = inv_mod(a, m)
  % Inverso modular de a modulo m (a y m coprimos) via Euclides extendido.
  [g, x, ~] = extgcd(a, m);
  inv = mod(x, m);
end

function [g, x, y] = extgcd(a, b)
  if b == 0
    g = a; x = 1; y = 0;
  else
    [g1, x1, y1] = extgcd(b, mod(a, b));
    g = g1;
    x = y1;
    y = x1 - floor(a/b) * y1;
  end
end