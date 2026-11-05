function res = registry_eval_archimedes(e)
  % Pi de Arquimedes: polígono circunscrito + extrapolación Liu Hui/Romberg.
  % Compara forma estable t/(sqrt(1+t^2)+1) vs ingenua (sqrt(1+t^2)-1)/t.
  params = e.params;
  iterations = str2double(params.iterations);
  levels = str2double(params.levels);
  ref = e.ref;
  pi_ref = str2double(ref.pi.value);
  tol = str2double(ref.pi.abs_tol);

  checks = emptychecks();

  for variant = {'stable', 'naive'}
    t = 1/sqrt(3);
    n = 6;
    A = zeros(1, iterations + 1);
    for k = 0:iterations
      A(k+1) = n * t;
      if strcmp(variant{1}, 'stable')
        t = t / (sqrt(1 + t^2) + 1);
      else
        t = (sqrt(1 + t^2) - 1) / t;
      end
      n = 2 * n;
    end
    % Extrapolación Romberg / Liu Hui
    R = A;
    for j = 1:levels
      R = R(2:end) + (R(2:end) - R(1:end-1)) / (4^j - 1);
    end
    pi_est = R(end);
    rel_err = abs(pi_est - pi_ref) / abs(pi_ref);
    ok = abs(pi_est - pi_ref) <= tol;
    checks(end + 1) = mkcheck(sprintf('pi_%s', variant{1}), ...
                              pi_est, pi_ref, tol, rel_err, ok);
  end

  % Spread entre variantes
  function pi_est = run_variant(variant)
    t = 1/sqrt(3);
    n = 6;
    A = zeros(1, iterations + 1);
    for k = 0:iterations
      A(k+1) = n * t;
      if strcmp(variant, 'stable')
        t = t / (sqrt(1 + t^2) + 1);
      else
        t = (sqrt(1 + t^2) - 1) / t;
      end
      n = 2 * n;
    end
    R = A;
    for j = 1:levels
      R = R(2:end) + (R(2:end) - R(1:end-1)) / (4^j - 1);
    end
    pi_est = R(end);
  end

  pi_stable = run_variant('stable');
  pi_naive = run_variant('naive');
  spread = abs(pi_stable - pi_naive) / abs(pi_ref);
  spread_tol = str2double(ref.variant_spread.abs_tol);
  checks(end + 1) = mkcheck('variant_spread', spread, 0, ...
                            spread_tol, spread, spread <= spread_tol);

  res = struct('name', e.name, 'family', e.family, ...
               'pass', all([checks.pass]), 'checks', checks);
end