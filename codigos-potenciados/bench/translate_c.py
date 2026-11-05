"""Strict-subset C -> Python translator (via an F77 intermediate).

Reads an old single-file C attractor driver (main() only) and EXECUTES it:
the front-end parses the supported C subset into fixed-form F77 text, which
`translate_f77.translate` then compiles to the legacy_* Python module. The
precision declared in the C source is preserved (`float` variables and
f-suffixed literals -> np.float32, `double` and bare decimal literals ->
np.float64), so the fingerprint (lambda1, D2) comes from the LEGACY
recurrence itself; `execution.translator` is "strict_c_subset" and the
intermediate F77 is an implementation detail (not stored in reports).

Supported subset (anything else raises TranslationError and the caller
falls back to the synthetic-equivalent diagnosis):
  - `#include`/preprocessor lines, `/* ... */` and `//` comments dropped
    (line numbers preserved for error reporting)
  - one function: `int main(...)` with a brace body; `return` dropped
  - declarations `float|double|int a, b = expr;` (before the for-loop or at
    file scope; arrays/pointers/qualifiers unsupported), f-suffixed or bare
    decimal literals, integer literals
  - assignments `x = expr;` and compound `x +=|-|=|*=|/= expr;`
  - ONE `for (init; i < bound|i <= bound; i++|++i|i += 1|i = i + 1)` with a
    brace body; init may declare the counter (`for (int i = 0; ...)`); the
    loop counter must NOT be referenced in the body (the F77 do-counter is
    excluded from availability, and C sees 0-based vs F77 1-based values)
  - block `if / else if / else` with braces, single-line `if (cond) stmt;`
    (no else); relational `< > <= >= == !=`, logical `&& || !`, unary minus
  - dropped as informational: printf/puts/scanf family, `return`
Refused: while/do-while/switch/goto/labels, arrays `a[i]`, pointers,
functions, nested or repeated for-loops, statements after the loop,
brace-less else, chained assignment, unclosed braces, double literals
with e-exponents (use `f` suffix).

HONESTY: same execution driver as the F77 path (fingerprint_ns): lambda1
uses the legacy program's own loop count as transient and the registry
window afterwards; D2 uses the last 4000 recorded steps. The recorded
variables are the first `entry.dim` state floats in declaration order.

Usage:
  from translate_c import fingerprint_c_source
  cand, info = fingerprint_c_source(text, entry, h=0.01)
"""
import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from translate_f77 import TranslationError, fingerprint_ns, translate

# informational calls dropped (they do not affect the recurrence)
_IO_FUNCS = {"printf", "puts", "fprintf", "sprintf", "snprintf", "scanf",
             "sscanf", "fscanf", "getchar", "putchar", "fputs", "fgets",
             "perror", "exit"}

_DECL_KW = {"float": "real", "double": "double precision", "int": "integer"}

# numeric literal with optional f/F suffix (C), identifiers protected
_LIT_RE = re.compile(
    r"(?<![A-Za-z0-9_.])"
    r"(\d+\.\d*(?:[eE][+-]?\d+)?|\.\d+(?:[eE][+-]?\d+)?"
    r"|\d+[eE][+-]?\d+|\d+)"
    r"([fF])?(?![A-Za-z0-9_])")


def _strip_comments(text):
    """Remove /* */ and // comments; keep newlines (line numbers stable)."""
    out, i, n, in_str = [], 0, len(text), None
    while i < n:
        ch = text[i]
        if in_str:
            out.append(ch)
            if ch == "\\" and i + 1 < n:
                out.append(text[i + 1])
                i += 2
                continue
            if ch == in_str:
                in_str = None
            i += 1
            continue
        if ch in "\"'":
            in_str = ch
            out.append(ch)
            i += 1
            continue
        if ch == "/" and i + 1 < n and text[i + 1] == "/":
            while i < n and text[i] != "\n":
                i += 1
            continue
        if ch == "/" and i + 1 < n and text[i + 1] == "*":
            start = text.count("\n", 0, i) + 1
            i += 2
            while i + 1 < n and not (text[i] == "*" and text[i + 1] == "/"):
                if text[i] == "\n":
                    out.append("\n")
                i += 1
            if i + 1 >= n:
                raise TranslationError(start, "unterminated /* comment")
            i += 2
            continue
        out.append(ch)
        i += 1
    if in_str:
        raise TranslationError(text.count("\n", 0, n) + 1,
                               "unterminated string literal")
    return "".join(out)


def _strip_preproc(text):
    """Drop preprocessor lines, keeping their newlines."""
    out = []
    for line in text.splitlines(keepends=True):
        if line.lstrip(" \t").startswith("#"):
            out.append("\n" if line.endswith("\n") else "")
        else:
            out.append(line)
    return "".join(out)


def _split_c(text):
    """-> [(kind, line, payload)] kind in stmt|lbrace|rbrace (string aware)."""
    text = _strip_preproc(_strip_comments(text))
    items = []
    buf, buf_ln = [], None
    depth, line, i, n = 0, 1, 0, len(text)
    in_str = None

    def has():
        return any(c.strip() for c in buf)

    def flush():
        nonlocal buf, buf_ln
        if has():
            items.append(("stmt", buf_ln, "".join(buf).strip()))
        buf, buf_ln = [], None

    while i < n:
        ch = text[i]
        if in_str:
            buf.append(ch)
            if ch == "\n":
                line += 1
            elif ch == "\\" and i + 1 < n:
                buf.append(text[i + 1])
                i += 1
            elif ch == in_str:
                in_str = None
            i += 1
            continue
        if ch in "\"'":
            if not has() and buf_ln is None:
                buf_ln = line
            in_str = ch
            buf.append(ch)
            i += 1
            continue
        if ch == "\n":
            line += 1
            if has():
                buf.append(" ")
            i += 1
            continue
        if ch == "(":
            if not has() and buf_ln is None:
                buf_ln = line
            depth += 1
        elif ch == ")":
            if depth > 0:
                depth -= 1
            if not has() and buf_ln is None:
                buf_ln = line
        elif depth == 0 and ch == ";":
            flush()
            i += 1
            continue
        elif depth == 0 and ch == "{":
            flush()
            items.append(("lbrace", line, ""))
            i += 1
            continue
        elif depth == 0 and ch == "}":
            flush()
            items.append(("rbrace", line, ""))
            i += 1
            continue
        elif not has() and buf_ln is None and not ch.isspace():
            buf_ln = line
        buf.append(ch)
        i += 1
    flush()
    return items


def _match_paren(s, open_i):
    depth = 0
    for j in range(open_i, len(s)):
        if s[j] == "(":
            depth += 1
        elif s[j] == ")":
            depth -= 1
            if depth == 0:
                return j
    return -1


def _split_top(s, sep):
    """Split on `sep` at paren depth 0."""
    parts, buf, depth = [], [], 0
    for ch in s:
        if ch == "(":
            depth += 1
        elif ch == ")":
            depth -= 1
        if ch == sep and depth == 0:
            parts.append("".join(buf))
            buf = []
        else:
            buf.append(ch)
    parts.append("".join(buf))
    return parts


def _c_expr(expr, ln):
    """Rewrite a C expression to its F77 spelling (operators + literals)."""
    s = expr.strip()
    s = s.replace("!=", "/=")
    s = s.replace("&&", " .and. ").replace("||", " .or. ")
    s = re.sub(r"!(?!=)", " .not. ", s)

    def lit(m):
        num, suf = m.group(1), m.group(2)
        if suf:                                    # 10.0f -> F77 single
            return num
        if re.fullmatch(r"\d+", num):              # integer: both truncate
            return num
        if "e" in num or "E" in num:
            raise TranslationError(ln, f"double literal {num!r} with "
                                   f"exponent unsupported (use an f suffix "
                                   f"or plain decimal digits)")
        return num + "d0"                          # bare C double -> F77 d

    return _LIT_RE.sub(lit, s)


def _assign_stmt(st, ln):
    """C assignment -> F77 assignment line, or None if not an assignment."""
    m = re.match(r"^([A-Za-z_]\w*)\s*(\+|-|\*|/)?=\s*(.+)$", st)
    if not m:
        return None
    lhs, op, rhs = m.group(1), m.group(2), m.group(3)
    if rhs.lstrip().startswith("="):
        raise TranslationError(ln, "relational == on the left of an "
                                    "assignment unsupported")
    if len(_split_top(rhs, "=")) > 1:
        raise TranslationError(ln, "chained assignment unsupported")
    code = _c_expr(rhs, ln)
    if op:
        return f"{lhs} = ({lhs}) {op} ({code})"
    return f"{lhs} = {code}"


def c_to_f77(text):
    """Parse the C subset -> (fixed-form F77 source, origin line per line)."""
    items = _split_c(text)
    out, origin = [], []
    stack = []                       # (marker, line): main|for|if|else
    pending_if, pending_ln = False, 0
    main_seen = main_closed = False
    for_done = prev_single_if = False
    loop_var = None
    n = len(items)
    idx = 0

    def has(marker):
        return any(k == marker for k, _ in stack)

    def emit(f77, ln):
        out.append("      " + f77)
        origin.append(ln)

    def flush_pending():
        nonlocal pending_if
        if pending_if:
            emit("end if", pending_ln)
            pending_if = False

    def need_lbrace(ln, what):
        nonlocal idx
        nxt = items[idx] if idx < n else None
        if not nxt or nxt[0] != "lbrace":
            raise TranslationError(ln, f"{what} must be followed by a "
                                       f"brace block")
        idx += 1

    while idx < n:
        kind, ln, st = items[idx]
        idx += 1
        is_else = kind == "stmt" and bool(re.match(r"^else\b", st))
        if not is_else:
            flush_pending()
        if kind == "lbrace":
            raise TranslationError(ln, "unexpected '{' (missing header or "
                                       "bare block unsupported)")
        if kind == "rbrace":
            if not stack:
                raise TranslationError(ln, "unmatched '}'")
            top, _ = stack.pop()
            if top == "if":
                pending_if, pending_ln = True, ln
            elif top == "else":
                emit("end if", ln)
            elif top == "for":
                emit("end do", ln)
                for_done = True
            else:                                    # main
                emit("end", ln)
                main_closed = True
            continue

        # ---- statements ----
        if main_closed:
            raise TranslationError(ln, "statement/function after main()")
        if prev_single_if and re.match(r"^else\b", st):
            raise TranslationError(ln, "else after brace-less if "
                                       "unsupported (use braces)")
        prev_single_if = False
        if re.match(r"^return\b", st):
            continue
        fm = re.match(r"^([A-Za-z_]\w*)\s*\(", st)
        if fm and fm.group(1) in _IO_FUNCS:
            continue                                 # informational only
        if loop_var is not None and has("for") and \
                re.search(rf"\b{re.escape(loop_var)}\b", st):
            raise TranslationError(ln, f"C loop variable {loop_var!r} "
                                       f"referenced in the for body "
                                       f"unsupported (counter excluded)")
        if re.match(r"^(?:int|void)\s+main\s*\(", st):
            if main_seen:
                raise TranslationError(ln, "only one function (main) "
                                            "supported")
            o = st.index("(")
            c = _match_paren(st, o)
            if c < 0 or st[c + 1:].strip():
                raise TranslationError(ln, "unsupported main signature")
            need_lbrace(ln, "main body")
            main_seen = True
            stack.append(("main", ln))
            continue
        if re.match(r"^for\s*\(", st):
            if has("for"):
                raise TranslationError(ln, "nested for-loop unsupported")
            if for_done:
                raise TranslationError(ln, "only one for-loop supported "
                                            "(single-loop subset)")
            o = st.index("(")
            c = _match_paren(st, o)
            if c < 0:
                raise TranslationError(ln, "unbalanced parentheses in for")
            if st[c + 1:].strip():
                raise TranslationError(ln, "for-loop without braces "
                                            "unsupported")
            head = st[o + 1:c]
            need_lbrace(ln, "for-loop body")
            parts = [p.strip() for p in _split_top(head, ";")]
            if len(parts) != 3:
                raise TranslationError(ln, "for header must be "
                                            "(init; condition; increment)")
            init_s, cond_s, incr_s = parts
            mi = re.match(r"^(?:(int|float|double)\s+)?([A-Za-z_]\w*)"
                          r"\s*=\s*(.+)$", init_s)
            if not mi:
                raise TranslationError(ln, f"unsupported for-init "
                                           f"{init_s!r}")
            if mi.group(1):
                emit(f"{_DECL_KW[mi.group(1)]} {mi.group(2)}", ln)
            var = mi.group(2)
            init_rhs = _c_expr(mi.group(3), ln)
            mc = re.match(r"^([A-Za-z_]\w*)\s*(<=|<)\s*(.+)$", cond_s)
            if not mc:
                raise TranslationError(ln, f"unsupported for condition "
                                           f"{cond_s!r} (loop variable "
                                           f"compared with < or <= on the "
                                           f"left)")
            if mc.group(1) != var:
                raise TranslationError(ln, "for-init and for-condition use "
                                           "different variables")
            bound = _c_expr(mc.group(3), ln)
            inc_var = None
            for pat in (r"^([A-Za-z_]\w*)\s*\+\+$",
                        r"^\+\+\s*([A-Za-z_]\w*)$",
                        r"^([A-Za-z_]\w*)\s*\+=\s*1$",
                        r"^([A-Za-z_]\w*)\s*=\s*\1\s*\+\s*1$"):
                mm = re.match(pat, incr_s)
                if mm:
                    inc_var = mm.group(1)
                    break
            if inc_var is None:
                raise TranslationError(ln, f"unsupported for increment "
                                           f"{incr_s!r} (only +1 steps)")
            if inc_var != var:
                raise TranslationError(ln, "for-init and for-increment use "
                                           "different variables")
            end = bound if mc.group(2) == "<" else f"({bound}) + 1"
            emit(f"do {var} = ({init_rhs}) + 1, {end}", ln)
            loop_var = var
            stack.append(("for", ln))
            continue
        if is_else:
            if st != "else" and not re.match(r"^else\s+if\s*\(", st):
                raise TranslationError(ln, "else without braces unsupported")
            if not pending_if:
                raise TranslationError(ln, "'else' without a matching 'if'")
            pending_if = False                      # chain continues here
            if st == "else":
                need_lbrace(ln, "else branch")
                emit("else", ln)
                stack.append(("else", ln))
                continue
            rest = st[4:].strip()
            o = rest.find("(")
            c = _match_paren(rest, o) if o >= 0 else -1
            if c < 0:
                raise TranslationError(ln, "unbalanced parentheses in "
                                            "else-if")
            if rest[c + 1:].strip():
                raise TranslationError(ln, "else-if without braces "
                                            "unsupported")
            emit(f"else if ({_c_expr(rest[o + 1:c], ln)}) then", ln)
            need_lbrace(ln, "else-if body")
            stack.append(("if", ln))
            continue
        if re.match(r"^if\s*\(", st):
            o = st.index("(")
            c = _match_paren(st, o)
            if c < 0:
                raise TranslationError(ln, "unbalanced parentheses in if")
            cond = _c_expr(st[o + 1:c], ln)
            tail = st[c + 1:].strip()
            if not tail:
                need_lbrace(ln, "if body")
                emit(f"if ({cond}) then", ln)
                stack.append(("if", ln))
                continue
            if re.match(r"^(goto|break|continue|return)\b", tail):
                raise TranslationError(ln, f"unsupported single-line if "
                                           f"body {tail!r}")
            a = _assign_stmt(tail, ln)
            if a is None:
                raise TranslationError(ln, f"unsupported single-line if "
                                           f"body {tail!r}")
            emit(f"if ({cond}) {a}", ln)
            prev_single_if = True
            continue
        md = re.match(r"^(float|double|int)\b(.*)$", st)
        if md:
            if has("for"):
                raise TranslationError(ln, "C declaration inside the "
                                            "for-loop body unsupported")
            if not md.group(2).strip():
                raise TranslationError(ln, "empty declaration")
            names, inits = [], []
            for part in _split_top(md.group(2), ","):
                part = part.strip()
                ma = re.match(r"^([A-Za-z_]\w*)\s*(?:\[(\d+)\])?"
                              r"\s*(?:=\s*(.+))?$", part)
                if not ma or not part:
                    raise TranslationError(ln, f"unsupported declaration "
                                               f"item {part!r} (only name "
                                               f"or name = expr)")
                if ma.group(2):
                    raise TranslationError(ln, "C arrays unsupported")
                names.append(ma.group(1))
                if ma.group(3) is not None:
                    inits.append((ma.group(1), ma.group(3)))
            emit(f"{_DECL_KW[md.group(1)]} {', '.join(names)}", ln)
            for nm, expr in inits:
                emit(f"{nm} = {_c_expr(expr, ln)}", ln)
            continue
        if re.match(r"^(while|do|switch|goto|break|continue|case)\b", st):
            raise TranslationError(ln, f"unsupported C construct "
                                       f"{st.split()[0]!r}")
        if re.match(r"^[A-Za-z_]\w*\s*:", st):
            raise TranslationError(ln, "labels unsupported in C subset")
        if re.match(r"^[A-Za-z_]\w*\[", st):
            raise TranslationError(ln, "C arrays unsupported")
        a = _assign_stmt(st, ln)
        if a is not None:
            if for_done:
                raise TranslationError(ln, "statement after the for-loop "
                                           "unsupported (single-loop subset)")
            emit(a, ln)
            continue
        if fm:
            raise TranslationError(ln, f"function call {fm.group(1)!r} "
                                       f"unsupported")
        raise TranslationError(ln, f"unsupported C statement: {st!r}")

    flush_pending()
    if stack:
        marker, open_ln = stack[-1]
        raise TranslationError(open_ln, f"unclosed '{marker}' block "
                                        f"(missing closing brace)")
    if not out:
        raise TranslationError(0, "empty C translation")
    return "\n".join(out) + "\n", origin


def load_c(text, dim=None):
    """Translate the C subset and exec it -> ns with legacy_* + __source."""
    f77, line_map = c_to_f77(text)
    try:
        src = translate(f77, dim)
    except TranslationError as e:
        if 0 < e.line <= len(line_map):
            msg = re.sub(r"^line \d+: ", "", str(e))
            raise TranslationError(line_map[e.line - 1], msg)
        raise
    ns = {}
    exec(compile(src, "<legacy_c>", "exec"), ns)
    ns["__source"] = src
    ns["__f77"] = f77
    return ns


def fingerprint_c_source(text, entry, h=None):
    """Run the legacy C source -> (candidate_fingerprint, exec_info)."""
    ns = load_c(text, entry["dim"])
    return fingerprint_ns(ns, entry, h, translator="strict_c_subset")
