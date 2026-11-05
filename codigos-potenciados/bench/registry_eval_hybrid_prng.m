function res = registry_eval_hybrid_prng(e)
  % hybrid_prng: middle-square de von Neumann + msws64 (Widynski 2017)
  % Compara middle-square original (roto) vs híbrido XOR con msws64.
  params = e.params;
  seed = 1234;
  if isfield(params, 'seed'), seed = double(params.seed); end
  n_samples = 10000;
  if isfield(params, 'chi2_samples'), n_samples = double(params.chi2_samples); end
  nbins = 10;
  if isfield(params, 'chi2_bins'), nbins = double(params.chi2_bins); end

  ref = e.ref;
  checks = emptychecks();

  %% ---------- middle-square original (4 dígitos) ----------
  function y = ms_next(x)
    s = sprintf('%08d', mod(x^2, 10^8));
    mid = str2double(s(3:6));
    y = mid;
  end

  seen = false(1, 10000);
  n_cycles = 0;
  for x0 = 0:9999
    x = x0; tr = [];
    while ~seen(x+1)
      seen(x+1) = true;
      tr = [tr, x];
      x = ms_next(x);
    end
    idx = find(tr == x);
    if ~isempty(idx)
      n_cycles = n_cycles + 1;
    end
  end

  zero_sink = 0;
  for x0 = 0:9999
    x = x0;
    for i = 1:50
      x = ms_next(x);
    end
    if x == 0
      zero_sink = zero_sink + 1;
    end
  end
  zs_rel = zero_sink / 10000;

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

  %% ---------- hybrid PRNG (middle-square + msws64) ----------
  ms_state = uint64(mod(seed, 9999) + 1);
  ms_state2 = uint64(mod(bitshift(uint64(seed), -16), 9999) + 1);
  mw_state = bitor(uint64(seed), uint64(1)); % asegurar no cero

  samples = zeros(1, n_samples);
  doubles = zeros(1, n_samples);

  for i = 1:n_samples
    % middle-square paso
    sq1 = ms_state * ms_state;
    s1 = sprintf('%08d', mod(sq1, 10^8));
    ms1 = str2double(s1(3:6));

    sq2 = ms_state2 * ms_state2;
    s2 = sprintf('%08d', mod(sq2, 10^8));
    ms2 = str2double(s2(3:6));

    ms_state = uint64(ms1);
    ms_state2 = uint64(ms2);

    % msws64 paso (uint64 arithmética)
    mw_state = mw_state * uint64(6364136223846793005);
    mw_state = bitxor(mw_state, bitshift(mw_state, -17));
    mw_state = mw_state * uint64(1442695040888963407);
    mw_state = bitxor(mw_state, bitshift(mw_state, -31));
    mw_state = mw_state * uint64(3141592653589793239);
    mw_state = bitxor(mw_state, bitshift(mw_state, -27));

    ms_combined = bitxor(bitshift(uint64(ms1), 48), bitshift(uint64(ms2), 32));
    r = bitxor(mw_state, ms_combined);

    samples(i) = double(r);
    doubles(i) = double(r) / double(intmax('uint64'));
  end

  %% ---------- Chi^2 de uniformidad ----------
  counts = zeros(1, nbins);
  expected = n_samples / nbins;
  for i = 1:n_samples
    b = floor(doubles(i) * nbins) + 1;
    if b > nbins, b = nbins; end
    counts(b) = counts(b) + 1;
  end
  chi2 = sum((counts - expected).^2 ./ expected);

  mean_d = mean(doubles);

  chi2_ref = double(ref.chi2.value);
  chi2_tol = double(ref.chi2.abs_tol);
  checks(end+1) = mkcheck('chi2', chi2, chi2_ref, ...
                          chi2_tol, abs(chi2 - chi2_ref)/max(abs(chi2_ref),1), ...
                          abs(chi2 - chi2_ref) <= chi2_tol);

  bins_ref = double(ref.chi2_bins.value);
  bins_tol = double(ref.chi2_bins.abs_tol);
  checks(end+1) = mkcheck('chi2_bins', nbins, bins_ref, ...
                          bins_tol, abs(nbins - bins_ref)/max(abs(bins_ref),1), ...
                          abs(nbins - bins_ref) <= bins_tol);

  samples_ref = double(ref.chi2_samples.value);
  samples_tol = double(ref.chi2_samples.abs_tol);
  checks(end+1) = mkcheck('chi2_samples', n_samples, samples_ref, ...
                          samples_tol, abs(n_samples - samples_ref)/max(abs(samples_ref),1), ...
                          abs(n_samples - samples_ref) <= samples_tol);

  res = struct('name', e.name, 'family', e.family, ...
               'pass', all([checks.pass]), 'checks', checks, ...
               'middle_square_cycles', n_cycles, ...
               'middle_square_zero_sink', zs_rel, ...
               'chi2', chi2, ...
               'chi2_bins', nbins, ...
               'chi2_samples', n_samples, ...
               'mean_double', mean_d, ...
               'first_values', samples(1:min(10, n_samples)));
end