function c = mkcheck(q, comp, refv, tol, rel, pass)
  % Uniform check record for registry results.
  c = struct('quantity', q, 'computed', comp, 'reference', refv, ...
             'tol', tol, 'rel_err', rel, 'pass', pass);
end
