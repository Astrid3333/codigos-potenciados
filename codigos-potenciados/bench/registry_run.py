"""Load JSON registries and evaluate entries with the same invariants as Octave.

The Python runner is a second independent implementation over the same
registry JSON. It compares computed invariants against the analytic or
literature references already stored in the registry (not against Octave),
so both languages independently hit the same ground truth.
"""
from __future__ import annotations

import json
import sys
import time
from pathlib import Path

import numpy as np
from scipy.optimize import minimize_scalar

from handles import map_step, rhs
from kernels import (box_counting_dim, corr_dim_grassberger, fd_eigenvalues,
                     lyap_map, lyap_ode)

ROOT = Path(__file__).resolve().parent.parent
REG_ATTRA = ROOT / "registries" / "attractors.json"
REG_POTE = ROOT / "registries" / "potentials.json"
REG_ENERGY = ROOT / "registries" / "energy_drift.json"
REG_DSP = ROOT / "registries" / "goertzel_fft.json"
REG_ARCH = ROOT / "registries" / "archimedes_pi.json"
REG_MSW = ROOT / "registries" / "middle_square_weyl.json"
REG_HYBRID = ROOT / "registries" / "hybrid_prng.json"
OUTDIR = ROOT / "results"


def load_registry(path: Path) -> list[dict]:
    with open(path) as f:
        data = json.load(f)
    return data["entries"]


def mkcheck(quantity, computed, reference, tol, rel_err, ok):
    return {
        "quantity": quantity,
        "computed": float(computed) if computed is not None else None,
        "reference": float(reference) if reference is not None and not (
            isinstance(reference, float) and np.isnan(reference)) else None,
        "tol": float(tol) if tol is not None else None,
        "rel_err": float(rel_err) if rel_err is not None else None,
        "pass": bool(ok),
    }


def eval_map(e: dict, outdir: Path) -> dict:
    f = map_step(e["name"], e["params"])
    x = np.asarray(e["x0"], dtype=float)
    norb = e["n_orbit"]
    orbit = np.zeros((norb, e["dim"]))
    for i in range(norb):
        x = f(x)
        orbit[i, :] = x
    if not np.all(np.isfinite(orbit)):
        raise RuntimeError(f"Orbit of {e['name']} is non-finite")
    odir = outdir / "orbits"
    odir.mkdir(parents=True, exist_ok=True)
    opath = odir / f"{e['name']}_orbit.txt"
    np.savetxt(opath, orbit, fmt="%.17g")

    lam = lyap_map(f, np.asarray(e["x0"], dtype=float),
                   e["n_lyap"], e["transient"])
    checks = []
    ref = e.get("ref") or {}
    # bit-exactness: first n iterates vs frozen reference sequence
    oh = ref.get("orbit_head")
    if oh is not None:
        ref_vals = np.asarray(oh["values"], dtype=float)
        head = orbit[: oh["n"]]
        md = float(np.max(np.abs(head - ref_vals)))
        tol = float(oh["abs_tol"])
        checks.append(mkcheck("orbit_head_maxabs", md, 0.0, tol, md,
                              md <= tol))
    lyap_ref = ref.get("lyap1")
    if lyap_ref is not None:
        val = lyap_ref["value"]
        tol = lyap_ref["abs_tol"]
        rel = abs(lam - val) / max(abs(val), np.finfo(float).eps)
        checks.append(mkcheck("lyap1", lam, val, tol, rel, abs(lam - val) <= tol))
    else:
        checks.append(mkcheck("lyap1_observed", lam, None, None, None, True))
    # Grassberger-Procaccia correlation dimension (protocol fixed in registry)
    cd_ref = ref.get("corr_dim")
    if cd_ref is not None:
        proto = cd_ref["protocol"]
        x = np.asarray(e["x0"], dtype=float)
        for _ in range(proto["transient"]):
            x = f(x)
        pts = np.zeros((proto["n_points"], e["dim"]))
        for i in range(proto["n_points"]):
            x = f(x)
            pts[i, :] = x
        dim = corr_dim_grassberger(pts, proto["n_radii"],
                                   proto["c_lo"], proto["c_hi"])
        val = cd_ref["value"]
        tol = cd_ref["rel_tol"] * abs(val)
        rel = abs(dim - val) / abs(val)
        checks.append(mkcheck("corr_dim", dim, val, tol, rel,
                              rel <= cd_ref["rel_tol"]))
    return {
        "name": e["name"], "family": e["family"],
        "pass": all(c["pass"] for c in checks),
        "checks": checks,
        "orbit_path": str(opath), "lyap1_observed": float(lam),
    }


def _chaos_game(e: dict, rng: np.random.Generator) -> np.ndarray:
    n = e["n_points"]
    bi = e["burn_in"]
    x = np.asarray(e["x0"], dtype=float)
    pts = np.zeros((n, 2))
    params = e["params"]
    if "vertices" in params:
        V = np.asarray(params["vertices"], dtype=float)
        p = np.asarray(params["p"], dtype=float)
        cp = np.cumsum(p / p.sum())
        for i in range(n + bi):
            k = np.searchsorted(cp, rng.random())
            x = (x + V[k]) / 2.0
            if i >= bi:
                pts[i - bi] = x
    elif "maps" in params:
        M = params["maps"]
        pv = np.array([m["p"] for m in M], dtype=float)
        cp = np.cumsum(pv / pv.sum())
        for i in range(n + bi):
            k = np.searchsorted(cp, rng.random())
            m = M[k]
            x = np.array([
                m["a"] * x[0] + m["b"] * x[1] + m["e"],
                m["c"] * x[0] + m["d"] * x[1] + m["f"],
            ])
            if i >= bi:
                pts[i - bi] = x
    else:
        raise ValueError(f"Unknown IFS param shape for {e['name']}")
    return pts


def eval_ifs(e: dict) -> dict:
    # fixed seed for run-to-run reproducibility in the cross-language hub
    rng = np.random.default_rng(0)
    pts = _chaos_game(e, rng)
    dim = box_counting_dim(pts, np.asarray(e["box_sizes"], dtype=float))
    checks = []
    ref = e.get("ref") or {}
    dim_ref = ref.get("dim")
    if dim_ref is not None:
        val = dim_ref["value"]
        tol = dim_ref["rel_tol"] * abs(val)
        rel = abs(dim - val) / abs(val)
        checks.append(mkcheck("box_dim", dim, val, tol, rel, rel <= dim_ref["rel_tol"]))
    else:
        checks.append(mkcheck("box_dim_observed", dim, None, None, None, True))
    return {
        "name": e["name"], "family": e["family"],
        "pass": all(c["pass"] for c in checks),
        "checks": checks, "dim_observed": float(dim),
    }


def eval_ode(e: dict) -> dict:
    f = rhs(e["name"], e["params"])
    lam = lyap_ode(f, np.asarray(e["x0"], dtype=float),
                   e["dt_lyap"], e["t_transient"], e["t_lyap"])
    checks = []
    ref = e.get("ref") or {}
    lyap_ref = ref.get("lyap1")
    if lyap_ref is not None:
        val = lyap_ref["value"]
        if "rel_tol" in lyap_ref:
            tol = lyap_ref["rel_tol"] * abs(val)
            rel = abs(lam - val) / abs(val)
            ok = rel <= lyap_ref["rel_tol"]
        else:
            tol = lyap_ref["abs_tol"]
            rel = abs(lam - val) / max(abs(val), np.finfo(float).eps)
            ok = abs(lam - val) <= tol
        checks.append(mkcheck("lyap1", lam, val, tol, rel, ok))
    else:
        checks.append(mkcheck("lyap1_observed", lam, None, None, None, True))
    return {
        "name": e["name"], "family": e["family"],
        "pass": all(c["pass"] for c in checks),
        "checks": checks, "lyap1_observed": float(lam),
    }


def eval_neural(e: dict) -> dict:
    P = np.asarray(e["params"]["patterns"], dtype=float)
    n = e["params"]["n"]
    W = (P.T @ P) / n
    W = W - np.diag(np.diag(W))
    checks = []
    for i, prow in enumerate(P):
        p = prow.reshape(-1, 1)
        s = np.sign(W @ p).ravel()
        ok = np.all(s == prow)
        checks.append(mkcheck(f"fixed_point_pattern_{i+1}",
                              float(ok), 1.0, 0.0, float(not ok), bool(ok)))
    return {
        "name": e["name"], "family": e["family"],
        "pass": all(c["pass"] for c in checks), "checks": checks,
    }


def eval_potential(e: dict) -> dict:
    name = e["name"].lower()
    ref = e.get("ref") or {}
    checks = []
    # scalar minimization: use Brent with tight xatol, same spirit as fminbnd
    def _minimize(f, lo, hi):
        res = minimize_scalar(f, bounds=(lo, hi), method="bounded",
                              options={"xatol": 1e-12})
        return float(res.x), float(res.fun)

    if name in ("lennard_jones", "morse", "double_well"):
        if name == "lennard_jones":
            ep, sg = e["params"]["eps"], e["params"]["sigma"]
            V = lambda r: 4.0 * ep * ((sg / r) ** 12 - (sg / r) ** 6)
            lo, hi = e["r_range"]
        elif name == "morse":
            De, a, re_ = e["params"]["De"], e["params"]["a"], e["params"]["re"]
            V = lambda r: De * (1.0 - np.exp(-a * (r - re_))) ** 2 - De
            lo, hi = e["r_range"]
        else:
            a, b = e["params"]["a"], e["params"]["b"]
            V = lambda x: a * x ** 4 - b * x ** 2
            lo, hi = 0.0, e["r_range"][1]
        xmin, vmin = _minimize(V, lo, hi)
        rmin = ref.get("r_min")
        if rmin is not None:
            val, tol = rmin["value"], rmin["abs_tol"]
            rel = abs(xmin - val) / abs(val)
            checks.append(mkcheck("r_min", xmin, val, tol, rel, abs(xmin - val) <= tol))
        rmin_abs = ref.get("r_min_abs")
        if rmin_abs is not None:
            val, tol = rmin_abs["value"], rmin_abs["abs_tol"]
            rel = abs(abs(xmin) - val) / abs(val)
            checks.append(mkcheck("r_min_abs", abs(xmin), val, tol, rel,
                                  abs(abs(xmin) - val) <= tol))
        vmin_ref = ref.get("V_min")
        if vmin_ref is not None:
            val, tol = vmin_ref["value"], vmin_ref["abs_tol"]
            rel = abs(vmin - val) / max(abs(val), np.finfo(float).eps)
            checks.append(mkcheck("V_min", vmin, val, tol, rel, abs(vmin - val) <= tol))

    elif name in ("harmonic", "hydrogen", "poschl_teller"):
        if name == "harmonic":
            g = np.linspace(e["x_grid"]["xmin"], e["x_grid"]["xmax"], e["x_grid"]["n"])
            Vg = 0.5 * e["params"]["k"] * g ** 2
            sigma = 0.0
        elif name == "hydrogen":
            n = e["x_grid"]["n"]
            rmax = e["x_grid"]["xmax"]
            h = rmax / (n + 1)
            g = h * np.arange(1, n + 1, dtype=float)
            Vg = -1.0 / g
            sigma = -1.0
        else:  # poschl_teller: full-line smooth well, no cusp
            g = np.linspace(e["x_grid"]["xmin"], e["x_grid"]["xmax"], e["x_grid"]["n"])
            lam = e["params"]["lambda"]
            Vg = -0.5 * lam * (lam + 1.0) / np.cosh(g) ** 2
            # bound states E_n = -(lambda-n)^2/2 < 0; continuum starts ~0.
            # Shift by -depth-ish so eigsh gets lowest bound states first.
            sigma = -0.5 * lam * (lam + 1.0) - 1.0
        k = len(e["eigen_n"])
        ev = fd_eigenvalues(g, Vg, k, sigma)
        eig_ref = ref["eigenvalues"]
        for j, en in enumerate(e["eigen_n"]):
            val = eig_ref["value"][j]
            comp = ev[j]
            rel = abs(comp - val) / abs(val)
            tol = eig_ref["rel_tol"] * abs(val)
            checks.append(mkcheck(f"E_{en}", comp, val, tol, rel,
                                  rel <= eig_ref["rel_tol"]))
        if "omega" in ref and ref["omega"] is not None:
            w = np.sqrt(e["params"]["k"] / e["params"]["m"])
            val, tol = ref["omega"]["value"], ref["omega"]["abs_tol"]
            checks.append(mkcheck("omega", w, val, tol, abs(w - val) / abs(val),
                                  abs(w - val) <= tol))

    elif name == "yukawa":
        A, kappa = e["params"]["A"], e["params"]["kappa"]
        Vh = lambda r: A * np.exp(-kappa * r) / r
        Fh = lambda r: A * np.exp(-kappa * r) * (1.0 / r ** 2 + kappa / r)
        hh = 1e-5
        for r in e["r_check"]:
            Fnum = -(Vh(r + hh) - Vh(r - hh)) / (2.0 * hh)
            Fan = Fh(r)
            val = Fan
            tol = ref["force"]["abs_tol"]
            rel = abs(Fnum - val) / abs(val)
            checks.append(mkcheck(f"force_r_{r:g}", Fnum, val, tol, rel,
                                  abs(Fnum - val) <= tol))
        F1 = -(Vh(1.0 + hh) - Vh(1.0 - hh)) / (2.0 * hh)
        val = ref["force"]["value_at_r1"]
        tol = ref["force"]["abs_tol"]
        checks.append(mkcheck("force_r_1_ref", F1, val, tol,
                              abs(F1 - val) / abs(val), abs(F1 - val) <= tol))
    else:
        raise ValueError(f"Unknown potential: {e['name']}")

    if not checks:
        raise RuntimeError(f"No checks produced for {e['name']}")
    return {
        "name": e["name"], "family": e["family"],
        "pass": all(c["pass"] for c in checks), "checks": checks,
    }


def eval_energy_drift(e: dict) -> dict:
    """Double-well trajectory: velocity Verlet vs RK4, relative energy drift."""
    a, b = e["params"]["a"], e["params"]["b"]
    mass = float(e["mass"])
    h = float(e["h"])
    T = float(e["T"])
    n = int(round(T / h))
    x = float(e["x0"][0])
    v = float(e["v0"][0])

    def V(q):
        return a * q ** 4 - b * q ** 2

    def F(q):
        return -4.0 * a * q ** 3 + 2.0 * b * q

    E0 = 0.5 * mass * v * v + V(x)
    # velocity Verlet
    xv, vv = x, v
    max_rel = 0.0
    for _ in range(n):
        acc = F(xv) / mass
        xv = xv + vv * h + 0.5 * acc * h * h
        acc_new = F(xv) / mass
        vv = vv + 0.5 * (acc + acc_new) * h
        E = 0.5 * mass * vv * vv + V(xv)
        rel = abs(E - E0) / abs(E0)
        if rel > max_rel:
            max_rel = rel
    verlet_final = rel
    verlet_max = max_rel
    # RK4 on (x, v)
    def f_y(y):
        return np.array([y[1], F(y[0]) / mass])
    y = np.array([x, v])
    max_rel = 0.0
    for _ in range(n):
        k1 = f_y(y)
        k2 = f_y(y + h / 2.0 * k1)
        k3 = f_y(y + h / 2.0 * k2)
        k4 = f_y(y + h * k3)
        y = y + h / 6.0 * (k1 + 2.0 * k2 + 2.0 * k3 + k4)
        E = 0.5 * mass * y[1] ** 2 + V(y[0])
        rel = abs(E - E0) / abs(E0)
        if rel > max_rel:
            max_rel = rel
    rk4_final = rel
    rk4_max = max_rel

    ref = e.get("ref") or {}
    checks = []
    for key, computed in [("E0", E0),
                          ("verlet_final_rel_drift", verlet_final),
                          ("verlet_max_rel_drift", verlet_max),
                          ("rk4_final_rel_drift", rk4_final),
                          ("rk4_max_rel_drift", rk4_max)]:
        r = ref.get(key)
        if r is None:
            checks.append(mkcheck(f"{key}_observed", computed, None, None,
                                  None, True))
            continue
        val = r["value"]
        tol = r.get("abs_tol", r.get("rel_tol", 0.0) * abs(val))
        rel = abs(computed - val) / max(abs(val), np.finfo(float).eps)
        checks.append(mkcheck(key, computed, val, tol, rel,
                              abs(computed - val) <= tol))
    return {
        "name": e["name"], "family": e["family"],
        "pass": all(c["pass"] for c in checks), "checks": checks,
    }


def eval_dsp_bin(e: dict) -> dict:
    """Goertzel single-bin DFT (K, alpha, beta) vs analytic |X[k]| = N*A/2."""
    N = int(e["N"])
    nn = np.arange(N, dtype=float)
    ks, As, phs = e["tone_k"], e["tone_A"], e["tone_phi"]
    x = np.zeros(N)
    for k, A, ph in zip(ks, As, phs):
        x = x + A * np.cos(2 * np.pi * k * nn / N + ph)
    xs = x.tolist()
    names = ["K", "alpha", "beta"]
    checks = []
    rt = e["ref"]["magnitude"]["rel_tol"]
    spread = 0.0
    for k, A in zip(ks, As):
        w = 2 * np.pi * k / N
        c = 2 * np.cos(w)
        s1 = s2 = 0.0
        for v in xs:
            s0 = v + c * s1 - s2
            s2 = s1
            s1 = s0
        mK = np.sqrt(s1 * s1 + s2 * s2 - c * s1 * s2)
        s = d = 0.0
        a = c - 2
        for v in xs:
            d = d + v + a * s
            s = s + d
        sN, sP = s, s - d
        mA = np.sqrt(sN * sN + sP * sP - c * sN * sP)
        s = ee = 0.0
        b = c + 2
        for v in xs:
            ee = v + b * s - ee
            s = ee - s
        sN, sP = s, ee - s
        mB = np.sqrt(sN * sN + sP * sP - c * sN * sP)
        refv = N * A / 2
        for nm, m in zip(names, (mK, mA, mB)):
            rel = abs(m - refv) / refv
            checks.append(mkcheck(f"{nm}_k{k}", m, refv, rt * refv, rel, rel <= rt))
        spread = max(spread, abs(mA - mK) / refv, abs(mB - mK) / refv)
    st = e["ref"]["variant_spread"]["abs_tol"]
    checks.append(mkcheck("variant_spread", spread, 0.0, st, spread, spread <= st))
    return {
        "name": e["name"], "family": e["family"],
        "pass": all(c["pass"] for c in checks), "checks": checks,
    }


def _mid4(x: int, digits: int = 4, modulus: int = 10000) -> int:
    """Extrae `digits` dígitos del medio de x^2 (middle-square de von Neumann)."""
    s = f"{x * x:0{digits * 2}d}"
    start = (len(s) - digits) // 2
    return int(s[start:start + digits])


def eval_middle_square_weyl(e: dict) -> dict:
    """Compara middle-square original vs Weyl vs TCR (Teorema Chino del Resto)."""
    digits = int(e.get("params", {}).get("digits", 4))
    modulus = int(e.get("params", {}).get("modulus", 10 ** digits))
    weyl_s = int(e.get("params", {}).get("weyl_s", 6329))
    p1 = int(e.get("params", {}).get("tcr_p1", 9973))
    p2 = int(e.get("params", {}).get("tcr_p2", 9967))
    max_iter = int(e.get("params", {}).get("max_iter", 1000000))

    ref = e.get("ref") or {}
    checks = []

    # ---------- Middle-square original ----------
    def ms_next(x):
        return _mid4(x, digits, modulus)

    seen = [False] * modulus
    cycles = []
    for x0 in range(modulus):
        x = x0
        tr = []
        while not seen[x]:
            seen[x] = True
            tr.append(x)
            x = ms_next(x)
        if x in tr:
            idx = tr.index(x)
            cycles.append(tr[idx:])

    n_cycles = len(cycles)

    zero_sink = 0
    for x0 in range(modulus):
        x = x0
        for _ in range(50):
            x = ms_next(x)
        if x == 0:
            zero_sink += 1

    n_cycles_ref = ref.get("middle_square_cycles", {}).get("value", 0)
    tol_cycles = ref.get("middle_square_cycles", {}).get("abs_tol", 0)
    checks.append(mkcheck("middle_square_cycles", n_cycles, n_cycles_ref,
                          tol_cycles, abs(n_cycles - n_cycles_ref) / max(abs(n_cycles_ref), 1),
                          abs(n_cycles - n_cycles_ref) <= tol_cycles))

    zs_ref = ref.get("middle_square_zero_sink", {}).get("value", 0.0)
    zs_tol = ref.get("middle_square_zero_sink", {}).get("abs_tol", 0.0)
    zs_rel = zero_sink / modulus
    checks.append(mkcheck("middle_square_zero_sink", zs_rel, zs_ref, zs_tol,
                          abs(zs_rel - zs_ref), abs(zs_rel - zs_ref) <= zs_tol))

    # ---------- Weyl ----------
    def weyl_next(x, w, s):
        w = (w + s) % modulus
        x = _mid4(x * x + w, digits, modulus)
        return x, w

    s = weyl_s
    w = 0
    x = 1234
    trw = []
    seenw = [False] * modulus
    weyl_period = 0
    for i in range(1, max_iter + 1):
        x, w = weyl_next(x, w, s)
        if seenw[x]:
            j = trw.index(x)
            weyl_period = i - j
            break
        seenw[x] = True
        trw.append(x)

    wp_ref = ref.get("weyl_period", {}).get("value", None)
    wp_tol = ref.get("weyl_period", {}).get("abs_tol", 0)
    if wp_ref is None:
        checks.append(mkcheck("weyl_period", weyl_period, None, None, None, True))
    else:
        checks.append(mkcheck("weyl_period", weyl_period, wp_ref, wp_tol,
                              abs(weyl_period - wp_ref) / max(abs(wp_ref), 1),
                              abs(weyl_period - wp_ref) <= wp_tol))

    # ---------- TCR ----------
    M = p1 * p2
    w1 = 0
    w2 = 0
    x = 1234
    trcrt = []
    seen_crt = [False] * modulus
    tcr_period = 0
    for i in range(1, max_iter + 1):
        w1 = (w1 + weyl_s) % p1
        w2 = (w2 + weyl_s) % p2
        # reconstruir w via TCR: w ≡ w1 (mod p1), w ≡ w2 (mod p2)
        inv_p1_mod_p2 = pow(p1 % p2, -1, p2)
        k = ((w2 - w1) * inv_p1_mod_p2) % p2
        w = (w1 + p1 * k) % M
        x = _mid4(x * x + w, digits, modulus)
        if seen_crt[x]:
            j = trcrt.index(x)
            tcr_period = i - j
            break
        seen_crt[x] = True
        trcrt.append(x)

    tp_ref = ref.get("tcr_period", {}).get("value", None)
    tp_tol = ref.get("tcr_period", {}).get("abs_tol", 0)
    if tp_ref is None:
        checks.append(mkcheck("tcr_period", tcr_period, None, None, None, True))
    else:
        checks.append(mkcheck("tcr_period", tcr_period, tp_ref, tp_tol,
                              abs(tcr_period - tp_ref) / max(abs(tp_ref), 1),
                              abs(tcr_period - tp_ref) <= tp_tol))

    # ---------- Variant spread ----------
    st = ref.get("variant_spread", {}).get("abs_tol", 1e-6)
    vs_ref = ref.get("variant_spread", {}).get("value", None)
    spread = abs(weyl_period - tcr_period) / max(abs(tcr_period), 1)
    if vs_ref is None:
        checks.append(mkcheck("variant_spread", spread, None, None, None, True))
    else:
        checks.append(mkcheck("variant_spread", spread, vs_ref, st, spread, spread <= st))

    return {
        "name": e["name"], "family": e["family"],
        "pass": all(c["pass"] for c in checks), "checks": checks,
        "middle_square_cycles": n_cycles,
        "middle_square_zero_sink": zs_rel,
        "weyl_period": weyl_period,
        "tcr_period": tcr_period,
        "tcr_modulus_M": M,
    }


def eval_hybrid_prng(e: dict) -> dict:
    """Evaluador de hybrid_prng: middle-square (roto) vs msws64 híbrido."""
    import numpy as np

    params = e.get("params", {})
    seed = int(params.get("seed", 1234))
    n_samples = int(params.get("n_samples", 10000))
    nbins = int(params.get("nbins", 10))

    ref = e.get("ref") or {}
    checks = []

    # ---------- middle-square original (4 dígitos) ----------
    def ms_next_4(x):
        sq = x * x
        s = f"{sq:08d}"
        return int(s[2:6])

    seen = [False] * 10000
    n_cycles = 0
    for x0 in range(10000):
        x = x0
        tr = []
        while not seen[x]:
            seen[x] = True
            tr.append(x)
            x = ms_next_4(x)
        if x in tr:
            n_cycles += 1

    zero_sink = 0
    for x0 in range(10000):
        x = x0
        for _ in range(50):
            x = ms_next_4(x)
        if x == 0:
            zero_sink += 1

    zs_rel = zero_sink / 10000

    n_cycles_ref = ref.get("middle_square_cycles", {}).get("value", 0)
    tol_cycles = ref.get("middle_square_cycles", {}).get("abs_tol", 0)
    checks.append(mkcheck("middle_square_cycles", n_cycles, n_cycles_ref,
                          tol_cycles, abs(n_cycles - n_cycles_ref) / max(abs(n_cycles_ref), 1),
                          abs(n_cycles - n_cycles_ref) <= tol_cycles))

    zs_ref = ref.get("middle_square_zero_sink", {}).get("value", 0.0)
    zs_tol = ref.get("middle_square_zero_sink", {}).get("abs_tol", 0.0)
    checks.append(mkcheck("middle_square_zero_sink", zs_rel, zs_ref, zs_tol,
                          abs(zs_rel - zs_ref), abs(zs_rel - zs_ref) <= zs_tol))

    # ---------- msws64 híbrido ----------
    state = seed | 1  # asegurar no cero

    def ms_next_hybrid(x):
        sq = x * x
        d1 = (sq // 1000) % 10
        d2 = (sq // 100) % 10
        d3 = (sq // 10) % 10
        d4 = sq % 10
        return d1 * 1000 + d2 * 100 + d3 * 10 + d4

    def msws_next(state):
        state = (state * 6364136223846793005) & 0xFFFFFFFFFFFFFFFF
        state ^= state >> 17
        state &= 0xFFFFFFFFFFFFFFFF
        state = (state * 1442695040888963407) & 0xFFFFFFFFFFFFFFFF
        state ^= state >> 31
        state &= 0xFFFFFFFFFFFFFFFF
        state = (state * 3141592653589793239) & 0xFFFFFFFFFFFFFFFF
        state ^= state >> 27
        return state & 0xFFFFFFFFFFFFFFFF & 0xFFFFFFFFFFFFFFFF

    ms_state = seed % 9999 + 1
    ms_state2 = (seed >> 16) % 9999 + 1
    mw_state = seed | 1

    samples = []
    doubles = []
    for _ in range(n_samples):
        ms1 = ms_next_hybrid(ms_state)
        ms2 = ms_next_hybrid(ms_state2)
        ms_state = ms1
        ms_state2 = ms2

        mw_state = msws_next(mw_state)
        mw = mw_state

        ms_combined = ((ms1 << 48) | (ms2 << 32)) & 0xFFFFFFFFFFFFFFFF
        r = mw ^ ms_combined
        samples.append(r)
        doubles.append(r / 0xFFFFFFFFFFFFFFFF)

    # Chi² de uniformidad
    counts = [0] * nbins
    expected = n_samples / nbins
    for r in samples:
        b = int((r / 0xFFFFFFFFFFFFFFFF) * nbins)
        if b >= nbins:
            b = nbins - 1
        counts[b] += 1
    chi2 = sum((c - expected) ** 2 / expected for c in counts)

    mean_d = sum(doubles) / n_samples

    chi2_ref = ref.get("chi2", {}).get("value", None)
    if chi2_ref is not None:
        chi2_tol = ref.get("chi2", {}).get("abs_tol", 1e-6)
        checks.append(mkcheck("chi2", chi2, chi2_ref, chi2_tol,
                              abs(chi2 - chi2_ref) / max(abs(chi2_ref), 1),
                              abs(chi2 - chi2_ref) <= chi2_tol))
    else:
        checks.append(mkcheck("chi2", chi2, None, None, None, True))

    bins_ref = ref.get("chi2_bins", {}).get("value", None)
    if bins_ref is not None:
        bins_tol = ref.get("chi2_bins", {}).get("abs_tol", 0)
        checks.append(mkcheck("chi2_bins", nbins, bins_ref, bins_tol,
                              abs(nbins - bins_ref) / max(abs(bins_ref), 1),
                              abs(nbins - bins_ref) <= bins_tol))
    else:
        checks.append(mkcheck("chi2_bins", nbins, None, None, None, True))

    samples_ref = ref.get("chi2_samples", {}).get("value", None)
    if samples_ref is not None:
        samples_tol = ref.get("chi2_samples", {}).get("abs_tol", 0)
        checks.append(mkcheck("chi2_samples", n_samples, samples_ref, samples_tol,
                              abs(n_samples - samples_ref) / max(abs(samples_ref), 1),
                              abs(n_samples - samples_ref) <= samples_tol))
    else:
        checks.append(mkcheck("chi2_samples", n_samples, None, None, None, True))

    return {
        "name": e["name"], "family": e["family"],
        "pass": all(c["pass"] for c in checks), "checks": checks,
        "middle_square_cycles": n_cycles,
        "middle_square_zero_sink": zs_rel,
        "chi2": chi2,
        "chi2_bins": nbins,
        "chi2_samples": n_samples,
        "mean_double": mean_d,
        "first_values": samples[:10],
    }


def eval_archimedes_pi(e: dict) -> dict:
    """Pi de Arquimedes: polígono circunscrito + extrapolación Liu Hui/Romberg.
    Compara forma estable t/(sqrt(1+t^2)+1) vs ingenua (sqrt(1+t^2)-1)/t."""
    import math

    params = e.get("params", {})
    iterations = int(params.get("iterations", 10))
    levels = int(params.get("levels", 3))
    ref = e.get("ref") or {}
    pi_ref = ref.get("pi", {}).get("value", math.pi)
    tol = ref.get("pi", {}).get("abs_tol", 1e-12)

    checks = []

    for variant in ("stable", "naive"):
        t = 1.0 / math.sqrt(3.0)
        n = 6
        A = []
        for _ in range(iterations + 1):
            A.append(n * t)
            if variant == "stable":
                t = t / (math.sqrt(1.0 + t * t) + 1.0)
            else:
                t = (math.sqrt(1.0 + t * t) - 1.0) / t
            n = 2 * n

        # Extrapolación Romberg/Liu Hui
        R = list(A)
        for j in range(1, levels + 1):
            denom = 4.0 ** j - 1.0
            R = [R[i + 1] + (R[i + 1] - R[i]) / denom for i in range(len(R) - 1)]

        pi_est = R[-1]
        rel_err = abs(pi_est - pi_ref) / abs(pi_ref)
        ok = abs(pi_est - pi_ref) <= tol
        checks.append(
            mkcheck(
                f"pi_{variant}",
                pi_est,
                pi_ref,
                tol,
                rel_err,
                ok,
            )
        )

    # Spread between variants
    # Re-run both to capture values cleanly
    def _run(variant):
        t = 1.0 / math.sqrt(3.0)
        n = 6
        A = []
        for _ in range(iterations + 1):
            A.append(n * t)
            if variant == "stable":
                t = t / (math.sqrt(1.0 + t * t) + 1.0)
            else:
                t = (math.sqrt(1.0 + t * t) - 1.0) / t
            n = 2 * n
        R = list(A)
        for j in range(1, levels + 1):
            denom = 4.0 ** j - 1.0
            R = [R[i + 1] + (R[i + 1] - R[i]) / denom for i in range(len(R) - 1)]
        return R[-1]

    pi_stable = _run("stable")
    pi_naive = _run("naive")
    spread = abs(pi_stable - pi_naive) / abs(pi_ref)
    spread_tol = ref.get("variant_spread", {}).get("abs_tol", 1e-8)
    checks.append(
        mkcheck(
            "variant_spread",
            spread,
            0.0,
            spread_tol,
            spread,
            spread <= spread_tol,
        )
    )

    return {
        "name": e["name"],
        "family": e["family"],
        "pass": all(c["pass"] for c in checks),
        "checks": checks,
        "pi_stable": pi_stable,
        "pi_naive": pi_naive,
    }
    if sel == "all":
        return True
    if sel == "attractors":
        return e["family"] != "potential"
    if sel == "potentials":
        return e["family"] == "potential"
    return sel == e["name"] or sel == e["family"]


def selected(e: dict, sel: str) -> bool:
    if sel == "all":
        return True
    if sel == "attractors":
        return e["family"] != "potential"
    if sel == "potentials":
        return e["family"] == "potential"
    return sel == e["name"] or sel == e["family"]


EVALUATORS = {
    "map": eval_map,
    "ifs": eval_ifs,
    "ode": eval_ode,
    "neural": eval_neural,
    "potential": eval_potential,
    "energy_drift": eval_energy_drift,
    "dsp_bin": eval_dsp_bin,
    "archimedes_pi": eval_archimedes_pi,
    "prng_bin": eval_middle_square_weyl,
    "hybrid_prng": eval_hybrid_prng,
}


def printrun(r: dict) -> None:
    if "error" in r:
        print(f"  ERROR: {r['error']}")
        print("  => FAIL")
        return
    for c in r["checks"]:
        tag = "PASS" if c["pass"] else "FAIL"
        if c["reference"] is None:
            print(f"  {tag} {c['quantity']} (observed) = {c['computed']:.12g}")
        else:
            print(f"  {tag} {c['quantity']}: computed={c['computed']:.12g} "
                  f"ref={c['reference']:.12g} rel_err={c['rel_err']:.3g} "
                  f"tol={c['tol']:.3g}")
    print("  => PASS" if r["pass"] else "  => FAIL")


def run(selector: str = "all") -> dict:
    att = load_registry(REG_ATTRA)
    pot = load_registry(REG_POTE)
    OUTDIR.mkdir(parents=True, exist_ok=True)

    runs = []
    for e in att:
        if not selected(e, selector):
            continue
        print(f"[{e['family']}] {e['name']}")
        try:
            r = EVALUATORS[e["family"]](e, OUTDIR) if e["family"] == "map" \
                else EVALUATORS[e["family"]](e)
        except Exception as exc:  # noqa: BLE001 — report per-entry, continue suite
            r = {"name": e["name"], "family": e["family"], "pass": False,
                 "checks": [mkcheck("error", None, None, None, None, False)],
                 "error": str(exc)}
        printrun(r)
        runs.append(r)
    for e in pot:
        if not selected(e, selector):
            continue
        print(f"[{e['family']}] {e['name']}")
        try:
            r = EVALUATORS["potential"](e)
        except Exception as exc:  # noqa: BLE001
            r = {"name": e["name"], "family": e["family"], "pass": False,
                 "checks": [mkcheck("error", None, None, None, None, False)],
                 "error": str(exc)}
        printrun(r)
        runs.append(r)
    for e in load_registry(REG_ENERGY):
        if not selected(e, selector):
            continue
        print(f"[{e['family']}] {e['name']}")
        try:
            r = EVALUATORS["energy_drift"](e)
        except Exception as exc:  # noqa: BLE001
            r = {"name": e["name"], "family": e["family"], "pass": False,
                 "checks": [mkcheck("error", None, None, None, None, False)],
                 "error": str(exc)}
        printrun(r)
        runs.append(r)

    for e in load_registry(REG_DSP):
        if not selected(e, selector):
            continue
        print(f"[{e['family']}] {e['name']}")
        try:
            r = EVALUATORS["dsp_bin"](e)
        except Exception as exc:  # noqa: BLE001
            r = {"name": e["name"], "family": e["family"], "pass": False,
                 "checks": [mkcheck("error", None, None, None, None, False)],
                 "error": str(exc)}
        printrun(r)
        runs.append(r)

    for e in load_registry(REG_ARCH):
        if not selected(e, selector):
            continue
        print(f"[{e['family']}] {e['name']}")
        try:
            r = EVALUATORS["archimedes_pi"](e)
        except Exception as exc:  # noqa: BLE001
            r = {"name": e["name"], "family": e["family"], "pass": False,
                 "checks": [mkcheck("error", None, None, None, None, False)],
                 "error": str(exc)}
        printrun(r)
        runs.append(r)

    for e in load_registry(REG_MSW):
        if not selected(e, selector):
            continue
        print(f"[{e['family']}] {e['name']}")
        try:
            r = EVALUATORS["prng_bin"](e)
        except Exception as exc:  # noqa: BLE001
            r = {"name": e["name"], "family": e["family"], "pass": False,
                 "checks": [mkcheck("error", None, None, None, None, False)],
                 "error": str(exc)}
        printrun(r)
        runs.append(r)

    for e in load_registry(REG_HYBRID):
        if not selected(e, selector):
            continue
        print(f"[{e['family']}] {e['name']}")
        try:
            r = EVALUATORS["hybrid_prng"](e)
        except Exception as exc:  # noqa: BLE001
            r = {"name": e["name"], "family": e["family"], "pass": False,
                 "checks": [mkcheck("error", None, None, None, None, False)],
                 "error": str(exc)}
        printrun(r)
        runs.append(r)

    npass = sum(1 for r in runs if r["pass"])
    print(f"\nSummary: {npass}/{len(runs)} entries PASS")

    out = {
        "timestamp": time.strftime("%Y%m%dT%H%M%S"),
        "selector": selector,
        "language": "python",
        "n_pass": npass, "n_total": len(runs), "runs": runs,
    }
    jpath = OUTDIR / f"registry_run_py_{out['timestamp']}.json"
    with open(jpath, "w") as f:
        json.dump(out, f, indent=2)
    print(f"Saved: {jpath}")
    return out


if __name__ == "__main__":
    sel = sys.argv[1] if len(sys.argv) > 1 else "all"
    run(sel)
