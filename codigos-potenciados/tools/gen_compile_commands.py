#!/usr/bin/env python3
"""Genera compile_commands.json para el analysis estatico de codigos-potenciados.

Uso:
    python3 codigos-potenciados/tools/gen_compile_commands.py

Salida:
    compile_commands.json en la raiz del repositorio (raiz de git).

El archivo generado esta en .gitignore: se regenera cuando cambian las fuentes.
"""

from __future__ import annotations

import json
import os
import sys

MODULES = [
    "cordic",
    "goertzel",
    "hastings",
    "hybrid_prng",
    "lcg_lehmer",
    "msws64",
    "matematica_ancestral",
]

ROOT_FILES = ["randu_plus.c", "sine_fixed.c"]

FLAGS = [
    "-std=c99",
    "-Wall",
    "-Wextra",
    "-Wpedantic",
    "-Wshadow",
    "-Wconversion",
    "-Wsign-conversion",
    "-Wcast-qual",
    "-Wstrict-prototypes",
    "-Wmissing-prototypes",
]

CC = "/usr/bin/gcc"


def repo_root() -> str:
    return os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))


def entries_for(directory: str, sources: list[str], includes: list[str]) -> list[dict]:
    result = []
    for source in sources:
        absolute = os.path.join(directory, source)
        if not os.path.isfile(absolute):
            continue
        command = " ".join(
            [CC, *FLAGS, *[f"-I{inc}" for inc in includes], "-c", absolute, "-o", os.path.join(directory, os.path.basename(source) + ".o")]
        )
        result.append({"directory": directory, "command": command, "file": absolute})
    return result


def main() -> int:
    root = repo_root()
    base = os.path.join(root, "codigos-potenciados")
    if not os.path.isdir(base):
        print(f"ERROR: no existe {base}", file=sys.stderr)
        return 1

    entries: list[dict] = []
    for module in MODULES:
        directory = os.path.join(base, module)
        if not os.path.isdir(directory):
            continue
        sources = sorted(f for f in os.listdir(directory) if f.endswith(".c"))
        includes = [directory]
        entries.extend(entries_for(directory, sources, includes))

    entries.extend(entries_for(base, ROOT_FILES, [base]))

    output = os.path.join(root, "compile_commands.json")
    with open(output, "w", encoding="utf-8") as handle:
        json.dump(entries, handle, indent=2)
        handle.write("\n")

    print(f"OK: {len(entries)} entradas -> {output}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
