#!/usr/bin/env python3
"""Captura el contrato real de tools de cada servidor MCP del ecosistema.

Usa MCP Inspector (`inspector --cli --method tools/list`) contra cada
servidor declarado en el template del repo y en la configuracion desplegada
de Claude Desktop, y escribe la evidencia en mcp_inspector_tools.json.

Reglas:
- Las rutas absolutas del template que apuntan a la copia obsoleta
  (/home/astrid/mcp-ecosystem-chiloe) se remapean a la copia canonica del
  workspace, y el remape queda registrado en la evidencia.
- Un servidor que no responde en INSPECTOR_TIMEOUT se marca como timeout,
  no como vacio: son categorias distintas.
"""
from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
import time
from concurrent.futures import ThreadPoolExecutor

INSPECTOR_TIMEOUT = 90  # segundos por servidor
STALE_ROOT = "/home/astrid/mcp-ecosystem-chiloe"
HERE = os.path.dirname(os.path.abspath(__file__))
WORKSPACE = os.path.dirname(os.path.dirname(HERE))
MCP_REPO = os.path.join(WORKSPACE, "mcp-ecosystem-chiloe")
TEMPLATE = os.path.join(MCP_REPO, "claude_desktop_config.template.json")
DEPLOYED = os.path.expanduser("~/.config/Claude/claude_desktop_config.json")
OUT = os.path.join(HERE, "mcp_inspector_tools.json")


def parse_inspector_output(raw: str) -> dict | None:
    """Extrae el primer objeto JSON-RPC completo de la salida del Inspector.

    La salida puede traer texto extra alrededor, asi que se barre con
    raw_decode en vez de hacer json.loads del bloque entero.
    """
    dec = json.JSONDecoder()
    for i, ch in enumerate(raw):
        if ch not in "{[":
            continue
        try:
            obj, _ = dec.raw_decode(raw[i:])
        except ValueError:
            continue
        if isinstance(obj, dict) and (
            "tools" in obj or "result" in obj or "error" in obj
        ):
            return obj
    return None


def remap(cfg: dict) -> tuple[dict, str | None]:
    """Apunta al repo canonico las rutas que apuntan a la copia obsoleta."""
    note = None
    args = list(cfg.get("args") or [])
    new_args = []
    for a in args:
        if isinstance(a, str) and a.startswith(STALE_ROOT):
            repl = a.replace(STALE_ROOT, MCP_REPO, 1)
            note = f"ruta remapeada: {a} -> {repl}"
            new_args.append(repl)
        else:
            new_args.append(a)
    out = dict(cfg)
    out["args"] = new_args
    return out, note


def probe(name: str, cfg: dict, source: str) -> dict:
    cmd = cfg.get("command")
    args = cfg.get("args") or []
    if not cmd:
        return {
            "servidor": name,
            "fuente": source,
            "estado": "sin_command",
            "n_tools": 0,
        }
    argv = [cmd] + [str(a) for a in args]
    t0 = time.time()
    try:
        # cwd neutral: el Inspector resuelve el target relativo a su cwd y
        # duplica el prefijo si el cwd coincide con el directorio del script.
        p = subprocess.run(
            ["inspector", "--cli", "--method", "tools/list"] + argv,
            capture_output=True,
            text=True,
            timeout=INSPECTOR_TIMEOUT,
            cwd="/tmp",
        )
        elapsed = round(time.time() - t0, 2)
    except subprocess.TimeoutExpired:
        return {
            "servidor": name,
            "fuente": source,
            "estado": "timeout",
            "n_tools": 0,
            "argv": argv,
            "segundos": INSPECTOR_TIMEOUT,
        }
    obj = parse_inspector_output(p.stdout)
    if obj is None:
        return {
            "servidor": name,
            "fuente": source,
            "estado": "sin_respuesta",
            "n_tools": 0,
            "argv": argv,
            "segundos": elapsed,
            "stderr": (p.stderr or "").strip()[-300:],
        }
    if "error" in obj:
        return {
            "servidor": name,
            "fuente": source,
            "estado": "error_inspector",
            "n_tools": 0,
            "argv": argv,
            "segundos": elapsed,
            "error": obj["error"],
        }
    tools = obj.get("tools") or (obj.get("result") or {}).get("tools") or []
    return {
        "servidor": name,
        "fuente": source,
        "estado": "ok",
        "n_tools": len(tools),
        "argv": argv,
        "segundos": elapsed,
        "tools": [
            {
                "name": t.get("name"),
                "description": (t.get("description") or "")[:300],
                "inputSchema": t.get("inputSchema") or {},
            }
            for t in tools
        ],
    }


def load_servers(path: str) -> dict:
    with open(path, encoding="utf-8") as fh:
        data = json.load(fh)
    return data.get("mcpServers") or {}


def main() -> int:
    if not shutil.which("inspector"):
        print("MCP Inspector no esta en el PATH", file=sys.stderr)
        return 2
    ver = subprocess.run(
        ["inspector", "--version"], capture_output=True, text=True, timeout=30
    ).stdout.strip()

    blocks = {}
    for label, path in (("template", TEMPLATE), ("desplegado", DEPLOYED)):
        servers = load_servers(path)
        jobs = []
        for name, cfg in servers.items():
            cfg2, note = remap(cfg)
            job = {"servidor": name, "_cfg": cfg2, "_nota": note}
            jobs.append(job)
        # Secuencial a proposito: varias instancias del Inspector en paralelo
        # compiten por el catalogo compartido y devuelven stdout vacio.
        out_jobs = []
        for j in jobs:
            res = probe(j["servidor"], j["_cfg"], label)
            if j["_nota"]:
                res["nota"] = j["_nota"]
            out_jobs.append(res)
        jobs = out_jobs
        blocks[label] = {
            "config": path,
            "n_servidores": len(servers),
            "servidores": sorted(jobs, key=lambda j: j["servidor"]),
        }

    ev = {
        "generado": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
        "mcp_inspector": ver,
        "timeout_por_servidor_s": INSPECTOR_TIMEOUT,
        "repo_canonico": MCP_REPO,
        "copia_obsoleta": STALE_ROOT,
        "bloques": blocks,
    }
    with open(OUT, "w", encoding="utf-8") as fh:
        json.dump(ev, fh, indent=2, ensure_ascii=False)

    for label, blk in blocks.items():
        ok = [s for s in blk["servidores"] if s["estado"] == "ok"]
        print(
            f"{label}: {len(ok)}/{blk['n_servidores']} responden, "
            f"{sum(s['n_tools'] for s in ok)} tools declaradas"
        )
        for s in blk["servidores"]:
            if s["estado"] != "ok":
                print(f"   - {s['servidor']}: {s['estado']}")
    print("evidencia ->", OUT)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
