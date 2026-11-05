"""Cross-language hub: compare Octave, Python and Fortran runs over one registry.

All three languages independently evaluate the same registry JSON and compare
computed invariants against the same analytic/literature references (not
against each other). This hub then diffs the three result sets entry by entry
to report agreement between languages.
"""
from __future__ import annotations

import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
RESULTS = ROOT / "results"
LANGS = ("octave", "python", "fortran")


def latest_run(lang: str) -> Path | None:
    if lang == "python":
        cands = sorted(RESULTS.glob("registry_run_py_*.json"))
    elif lang == "fortran":
        cands = sorted(RESULTS.glob("registry_run_f_*.json"))
    else:
        cands = sorted(p for p in RESULTS.glob("registry_run_*.json")
                       if "_py_" not in p.name and "_f_" not in p.name)
    return cands[-1] if cands else None


def index_runs(run: dict) -> dict[str, dict]:
    return {r["name"]: r for r in run["runs"]}


def _checks_as_list(checks) -> list[dict]:
    # Octave jsonencode serializes a 1-element struct array as a single dict.
    if isinstance(checks, dict):
        return [checks]
    return checks


def _observed(checks) -> float | None:
    for c in _checks_as_list(checks):
        if str(c.get("quantity", "")).endswith("_observed"):
            return c.get("computed")
    return None


def compare(paths: dict[str, Path]) -> dict:
    idx = {}
    for lang in LANGS:
        with open(paths[lang]) as f:
            idx[lang] = index_runs(json.load(f))

    names = sorted(set().union(*(set(i) for i in idx.values())))
    rows = []
    all_pass = 0
    for name in names:
        present = {lang: name in idx[lang] for lang in LANGS}
        if not all(present.values()):
            rows.append({"name": name, "status": "MISSING", "present": present})
            continue
        passes = {lang: idx[lang][name]["pass"] for lang in LANGS}
        obs = {lang: _observed(idx[lang][name]["checks"]) for lang in LANGS}
        obs_vals = [v for v in obs.values() if v is not None]
        agree = None
        if len(obs_vals) >= 2:
            den = max(max(abs(v) for v in obs_vals), 1e-15)
            agree = (max(obs_vals) - min(obs_vals)) / den
        if all(passes.values()):
            status = "ALL_PASS"
            all_pass += 1
        elif any(passes.values()):
            status = "PARTIAL"
        else:
            status = "ALL_FAIL"
        rows.append({
            "name": name, "status": status, "passes": passes,
            "obs_rel_diff": agree, "obs": obs,
        })

    summary = {
        "files": {lang: paths[lang].name for lang in LANGS},
        "n_entries": len(names), "all_pass": all_pass,
        "rows": rows,
    }
    out_path = RESULTS / "hub_compare.json"
    with open(out_path, "w") as f:
        json.dump(summary, f, indent=2)
    return summary


def main():
    paths = {lang: latest_run(lang) for lang in LANGS}
    missing = [lang for lang in LANGS if paths[lang] is None]
    if missing:
        print("Missing registry_run result in results/ for: "
              + ", ".join(missing), file=sys.stderr)
        sys.exit(1)
    for lang in LANGS:
        print(f"{lang.capitalize()}: {paths[lang].name}")
    s = compare(paths)
    print(f"\nEntries: {s['n_entries']}  All-pass entries: {s['all_pass']}")
    for r in s["rows"]:
        if r["status"] == "MISSING":
            gone = ", ".join(l for l in LANGS if not r["present"][l])
            print(f"  {r['name']:16s} MISSING in {gone}")
            continue
        extra = ""
        if r.get("obs_rel_diff") is not None:
            extra = f"  obs_rel_diff={r['obs_rel_diff']:.3g}"
        bits = "".join("P" if r["passes"][l] else "F" for l in LANGS)
        print(f"  {r['name']:16s} {r['status']} [{bits}]{extra}")
    print(f"\nSaved: {RESULTS / 'hub_compare.json'}")


if __name__ == "__main__":
    main()
