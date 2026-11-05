"""Strict-subset Fortran 77 -> Python translator: EXECUTE legacy code.

Translates an old fixed-form F77 attractor driver into a Python step
function and runs it under the precision declared in the source (REAL ->
np.float32, REAL*8/DOUBLE PRECISION -> np.float64, literals without a D
exponent -> single, with -> double), so the fingerprint (lambda1, D2) comes
from the LEGACY recurrence itself instead of a synthetic equivalent variant.

Supported subset (anything else raises TranslationError and the caller falls
back to the synthetic-equivalent diagnosis):
  - fixed form: col-1 comments c/C/*, inline `!`, col-6 `&` continuation,
    statement labels in cols 1-5
  - program/end/implicit none, integer/real/real*4/real*8/double precision
    declarations; scalars and 1-D arrays with a LITERAL extent `name(N)`
    (arrays are zero-initialised: documented assumption)
  - parameter(name = <numeric literal>)
  - scalar and array-element assignments: + - * / ** ( ), unary minus,
    intrinsics abs/sqrt/exp/sin/cos/mod/min/max (+ d-aliases dabs, dsqrt,
    dexp, dsin, dcos, dmod); array indices are integer expressions
    (literal indices bounds-checked, converted to 0-based)
  - block IF / ELSE IF / ELSE / END IF and single-line IF with relational
    (.eq. .ne. .lt. .gt. .le. .ge. or == /= < > <= >=) and logical
    (.and. .or. .not.) operators; a conditional assignment does not count
    as an assignment for later read-before-write checks (conservative)
  - GOTO / GO TO to a statement label. Any GOTO compiles the loop body to
    a program-counter dispatch with a per-step block budget (guards cyclic
    gotos); without GOTO the body is emitted as straight structured Python.
  - goto OUT of the loop: target label must sit on a statement AFTER
    `end do` (continue/write/end); semantics = stop updating this step.
    The real program would halt there; with a state-only guard both record
    the same trajectory (guard re-evaluated on the unchanged state).
  - do var = a, b ... end do, nested to any depth; loop counters MAY be
    read in the body (e.g. as array indices) but must NOT be assigned to;
    classic labelled `do L var = a, b ... L continue` is rewritten to
    do/end do (terminator must be CONTINUE, no GOTO to it); a GOTO anywhere
    in the source together with a nested do-loop is unsupported
  - write(*,*) name|name(i)|name, ...  (evaluated at the end, informational)
The translated source is exposed as ns["__source"]; ns["MODE"] is
"structured" or "goto_dispatch".

HONESTY: lambda1 uses the legacy program's own loop count as transient and
the registry window afterwards; D2 uses the last 4000 recorded steps of the
legacy run (same per-step sampling density as the oracle). The recorded
variables are the first `entry.dim` state floats in declaration order
(array elements count as `name[j]` keys in declaration order).

Usage:
  from translate_f77 import fingerprint_source
  cand, info = fingerprint_source(text, entry, h=0.01)
"""
import math
import re
import sys
from pathlib import Path

import numpy as np

sys.path.insert(0, str(Path(__file__).resolve().parent))
from diagnose_attractor import N_POINTS_D2, fingerprint_d2

_TOKEN = re.compile(
    r"""(?P<d>(?:[0-9]+\.[0-9]*|\.[0-9]+|[0-9]+)[dD][+-]?[0-9]+)
      | (?P<f>(?:[0-9]+\.[0-9]*|\.[0-9]+)(?:[eE][+-]?[0-9]+)?
             |[0-9]+[eE][+-]?[0-9]+)
      | (?P<i>[0-9]+)
      | (?P<name>[A-Za-z][A-Za-z0-9_]*)
      | (?P<log>\.(?:and|or|not)\.)
      | (?P<rel>\.(?:eq|ne|lt|gt|le|ge)\.)
      | (?P<rel2>==|/=|<=|>=|<|>)
      | (?P<pow>\*\*)
      | (?P<comma>,)
      | (?P<op>[-+*/()])""",
    re.X)

# intrinsic spellings -> family (one/two argument emitters below)
_INTRIN1 = {"abs": "abs", "sqrt": "sqrt", "exp": "exp", "sin": "sin",
            "cos": "cos", "dabs": "abs", "dsqrt": "sqrt", "dexp": "exp",
            "dsin": "sin", "dcos": "cos"}
_INTRIN2 = {"mod": "mod", "min": "min", "max": "max", "dmod": "mod"}

_BLOCK_BUDGET = 100000   # max dispatched blocks per step (cyclic-goto guard)

_REL_PY = {".eq.": "==", ".ne.": "!=", ".lt.": "<", ".gt.": ">",
           ".le.": "<=", ".ge.": ">=", "==": "==", "/=": "!=",
           "<=": "<=", ">=": ">=", "<": "<", ">": ">"}


class TranslationError(Exception):
    def __init__(self, line, msg):
        self.line = line
        super().__init__(f"line {line}: {msg}")


def logical_lines(text):
    """Fixed-form pass: comments, labels (cols 1-5), continuations, '!'.
    -> [(ln, label_or_None, stmt)]"""
    out, cur, cur_ln, cur_lab = [], None, None, None
    for i, raw in enumerate(text.splitlines(), 1):
        if raw[:1] in ("", "c", "C", "*", "!"):
            continue
        piece = raw.split("!", 1)[0].rstrip()
        if len(raw) > 5 and raw[5] == "&":
            if cur is None:
                raise TranslationError(i, "continuation without a statement")
            cur += piece[6:]
            continue
        if not piece.strip():
            continue
        if cur is not None:
            out.append((cur_ln, cur_lab, cur))
        label = None
        if len(piece) > 5 and piece[5] == " ":
            head = piece[:5].strip()
            if head.isdigit() and len(head) <= 5:
                label = int(head)
                piece = piece[6:]
        cur, cur_ln, cur_lab = piece, i, label
    if cur is not None:
        out.append((cur_ln, cur_lab, cur))
    return out


def _promote(a, b):
    if a == "i" and b == "i":
        return "i"
    if a == "f" and b == "f":
        return "f"
    if a == "d" or b == "d":
        return "d"
    return "f"


class _ExprParser:
    def __init__(self, tokens, types, lineno, arrays=None):
        self.toks, self.i = tokens, 0
        self.types, self.ln = types, lineno
        self.arrays = arrays or {}
        self.used = set()

    def peek(self):
        return self.toks[self.i] if self.i < len(self.toks) else (None, None)

    def next(self):
        tok = self.peek()
        self.i += 1
        return tok

    def parse(self):
        v = self.expr(1)
        if self.i != len(self.toks):
            raise TranslationError(self.ln, f"junk after expression: "
                                    f"{self.toks[self.i][1]!r}")
        return v

    @staticmethod
    def _prec(kind, text):
        # or=1 < and=2 < relational=3 < +- =4 < */ =5 < ** =6 (tighter)
        if kind == "log":
            if text == ".or.":
                return 1
            if text == ".and.":
                return 2
            return None                      # .not. is handled in unary()
        if kind in ("rel", "rel2"):
            return 3
        if kind == "pow":
            return 6
        if kind == "op":
            if text in "+-":
                return 4
            if text in "*/":
                return 5
        return None

    def expr(self, min_prec):
        left = self.unary()
        seen_rel = False
        while True:
            kind, text = self.peek()
            p = self._prec(kind, text)
            if p is None or p < min_prec:
                break
            self.next()
            if p == 6:                                   # ** (right assoc)
                right = self.expr(6)
                left = (f"({left[0]} ** {right[0]})",
                        _promote(left[1], right[1]))
            elif p == 3:                                 # relational
                if seen_rel:
                    raise TranslationError(self.ln, "chained relational "
                                            "operators (add parentheses)")
                seen_rel = True
                right = self.expr(4)
                if left[1] == "l" or right[1] == "l":
                    raise TranslationError(self.ln, "relational operator "
                                            "needs non-logical operands")
                left = (f"({left[0]} {_REL_PY[text]} {right[0]})", "l")
            elif p in (1, 2):                            # .or. / .and.
                if left[1] != "l":
                    raise TranslationError(self.ln, f"{text} needs logical "
                                            "operands")
                right = self.expr(p + 1)
                if right[1] != "l":
                    raise TranslationError(self.ln, f"{text} needs logical "
                                            "operands")
                op = "and" if p == 2 else "or"
                left = (f"({left[0]} {op} {right[0]})", "l")
            else:
                right = self.expr(p + 1)
                left = self.binary(text, left, right)
        return left

    def binary(self, op, L, R):
        (py_a, ta), (py_b, tb) = L, R
        if op == "/" and ta == "i" and tb == "i":
            return (f"({py_a} // {py_b})", "i")
        return (f"({py_a} {op} {py_b})", _promote(ta, tb))

    def _expect(self, ch):
        k2, t2 = self.next()
        if k2 != "op" or t2 != ch:
            raise TranslationError(self.ln, f"missing {ch!r} in expression")

    def unary(self):
        kind, text = self.peek()
        if kind == "log" and text == ".not.":
            self.next()
            py, t = self.expr(3)
            if t != "l":
                raise TranslationError(self.ln, ".not. needs a logical "
                                        "operand")
            return (f"(not {py})", "l")
        if kind == "op" and text == "-":
            self.next()
            py, t = self.unary()
            return (f"(-{py})", t)
        if kind == "op" and text == "+":
            self.next()
            return self.unary()
        if kind == "op" and text == "(":
            self.next()
            v = self.expr(1)
            self._expect(")")
            return v
        if kind in ("d", "f", "i"):
            self.next()
            if kind == "d":
                val = float(text.replace("d", "e").replace("D", "e"))
                return (f"np.float64({val!r})", "d")
            if kind == "f":
                return (f"np.float32({float(text)!r})", "f")
            return (str(int(text)), "i")
        if kind == "name":
            self.next()
            nm = text.lower()
            if self.peek() == ("op", "("):
                self.next()                              # consume '('
                if nm in self.arrays:
                    return self._array_ref(nm)
                if nm in _INTRIN1:
                    return self._intrinsic1(nm)
                if nm in _INTRIN2:
                    return self._intrinsic2(nm)
                raise TranslationError(self.ln, f"unknown function or "
                                        f"array {nm!r}")
            if nm not in self.types:
                raise TranslationError(self.ln, f"undeclared variable {nm!r}")
            self.used.add(nm)
            return (nm, self.types[nm])
        raise TranslationError(self.ln,
                               f"unexpected token {text!r} in expression")

    def _index(self, nm):
        idx = self.expr(1)
        self._expect(")")
        if idx[1] != "i":
            raise TranslationError(self.ln, f"array index of {nm!r} must be "
                                   "integer")
        size = self.arrays[nm]
        if re.fullmatch(r"-?[0-9]+", idx[0]):
            k = int(idx[0])
            if not 1 <= k <= size:
                raise TranslationError(self.ln, f"index {k} out of bounds "
                                       f"for {nm}({size})")
            return str(k - 1)
        return f"({idx[0]}) - 1"

    def _array_ref(self, nm):
        py_i = self._index(nm)
        return (f"{nm}[{py_i}]", self.types[nm])

    def _intrinsic1(self, nm):
        a = self.expr(1)
        self._expect(")")
        fam = _INTRIN1[nm]
        if fam == "abs":
            return (f"abs({a[0]})", a[1])
        t = "f" if a[1] in ("i", "f") else "d"
        return (f"np.{fam}({a[0]})", t)

    def _intrinsic2(self, nm):
        a = self.expr(1)
        k2, _ = self.next()
        if k2 != "comma":
            raise TranslationError(self.ln, f"{nm}() takes two arguments")
        b = self.expr(1)
        self._expect(")")
        fam = _INTRIN2[nm]
        t = _promote(a[1], b[1])
        if fam == "mod":
            # Fortran MOD: trunc toward zero -> sign of the first argument
            return (f"(({a[0]}) - ({b[0]}) * int(({a[0]}) / ({b[0]})))", t)
        return (f"np.{fam}({a[0]}, {b[0]})", t)


def _tokenize(expr, ln):
    toks, pos = [], 0
    for m in _TOKEN.finditer(expr):
        if expr[pos:m.start()].strip():
            raise TranslationError(ln, f"bad expression near "
                                    f"{expr[pos:m.start()]!r}")
        pos = m.end()
        kind = m.lastgroup
        toks.append((kind, m.group(kind)))
    if expr[pos:].strip():
        raise TranslationError(ln, f"bad expression near {expr[pos:]!r}")
    if not toks:
        raise TranslationError(ln, "empty expression")
    return toks


def _parse_expr(expr, types, ln, arrays=None):
    p = _ExprParser(_tokenize(expr, ln), types, ln, arrays)
    py, typ = p.parse()
    return py, typ, p.used


def _lhs(lhs_raw, types, arrays, ln):
    """Parse an assignment target: `x` or `y(i)` -> (py, type, used)."""
    s = lhs_raw.strip().lower()
    m = re.fullmatch(r"([a-z][a-z0-9_]*)\s*\(\s*([^()]*?)\s*\)", s)
    if m:
        nm = m.group(1)
        if nm not in arrays:
            if nm in types:
                raise TranslationError(ln, f"{nm!r} is a scalar; cannot be "
                                       "indexed")
            raise TranslationError(ln, f"undeclared variable {nm!r}")
        ipy, ityp, iused = _parse_expr(m.group(2), types, ln, arrays)
        if ityp != "i":
            raise TranslationError(ln, "array index must be integer")
        if re.fullmatch(r"-?[0-9]+", ipy):
            k = int(ipy)
            if not 1 <= k <= arrays[nm]:
                raise TranslationError(ln, f"index {k} out of bounds for "
                                       f"{nm}({arrays[nm]})")
            py_i = str(k - 1)
        else:
            py_i = f"({ipy}) - 1"
        return f"{nm}[{py_i}]", types[nm], iused
    if s in arrays:
        raise TranslationError(ln, "assignment to a whole array unsupported")
    if s not in types:
        raise TranslationError(ln, f"undeclared variable {s!r}")
    return s, types[s], set()


def _assign_code(lhs_raw, rhs, types, ln, arrays=None):
    """-> (code, used_names, target_key)."""
    lpy, ltyp, lused = _lhs(lhs_raw, types, arrays or {}, ln)
    py, typ, used = _parse_expr(rhs, types, ln, arrays)
    used = used | lused
    if ltyp == "f":
        return f"{lpy} = np.float32({py})", used, lpy
    if ltyp == "d":
        return f"{lpy} = np.float64({py})", used, lpy
    return f"{lpy} = int({py})", used, lpy


def _dtype_name(t):
    return "float32" if t == "f" else "float64"


def _emit(stmts, indent):
    """Structured codegen (no GOTO): nodes -> indented Python lines."""
    out = []
    pad = " " * indent
    for node in stmts:
        kind = node[0]
        if kind == "s":
            if node[2] is not None:
                out.append(pad + node[2])
        elif kind == "n":
            continue
        elif kind == "goto":
            raise TranslationError(0, "internal: goto in structured mode")
        elif kind == "do":
            out.append(f"{pad}for {node[2]} in range({node[3]}, "
                       f"({node[4]}) + 1):")
            body = _emit(node[5], indent + 4)
            out += body if body else [pad + "    pass"]
        elif kind == "if":
            cond, then, els = node[2], node[3], node[4]
            out.append(f"{pad}if {cond}:")
            body = _emit(then, indent + 4)
            out += body if body else [pad + "    pass"]
            if els is not None:
                out.append(f"{pad}else:")
                ebody = _emit(els, indent + 4)
                out += ebody if ebody else [pad + "    pass"]
    return out


def _flatten(body, post_labels):
    """Compile nodes to basic blocks for program-counter dispatch.

    Blocks: [{'code': [str], 'term': ('end',) | ('jmp', idx) | ('if', cond,
    then_idx, else_idx)}]. Labels start a block; a goto to a label defined
    after `end do` compiles to the same 'end' (leave the step).
    -> (blocks, exit_block_index)
    """
    blocks = []
    label_block = {}
    defined = set()

    def new_block():
        blocks.append({"code": [], "term": None})
        return len(blocks) - 1

    def flatten(stmts, cur):
        for node in stmts:
            label = node[1]
            prev = cur
            if label is not None:
                if label in defined:
                    raise TranslationError(0, f"duplicate label {label}")
                defined.add(label)
                if cur is None or blocks[cur]["code"] or \
                        blocks[cur]["term"] is not None:
                    cur = None
            if cur is None or blocks[cur]["term"] is not None:
                cur = new_block()
                if prev is not None and blocks[prev]["term"] is None:
                    blocks[prev]["term"] = ("jmp", cur)
            if label is not None:
                label_block[label] = cur
            kind = node[0]
            if kind == "s":
                if node[2] is not None:
                    blocks[cur]["code"].append(node[2])
            elif kind == "n":
                pass
            elif kind == "goto":
                # label target: patched to a block index after flattening
                blocks[cur]["term"] = ("goto", node[2])
                blocks[cur]["goto_ln"] = node[3] if len(node) > 3 else 0
                cur = None
            elif kind == "do":
                raise TranslationError(node[6] if len(node) > 6 else 0,
                                       "nested do-loop with goto unsupported")
            elif kind == "if":
                cond, then, els = node[2], node[3], node[4]
                then_b = new_block()
                join_b = new_block()
                els_b = new_block() if els is not None else None
                blocks[cur]["term"] = ("if", cond, then_b,
                                       els_b if els_b is not None else join_b)
                t_exit = flatten(then, then_b)
                if t_exit is not None:
                    blocks[t_exit]["term"] = ("jmp", join_b)
                if els is not None:
                    e_exit = flatten(els, els_b)
                    if e_exit is not None:
                        blocks[e_exit]["term"] = ("jmp", join_b)
                cur = join_b
        return cur

    exit_b = flatten(body, None)
    if exit_b is None:
        exit_b = new_block()
    blocks[exit_b]["term"] = ("end",)

    for b in blocks:
        t = b.get("term")
        if t is None:
            b["term"] = ("end",)            # defensive: fall off the end
            continue
        if t[0] == "goto":                  # resolve label -> block index
            tgt = t[1]
            if tgt in label_block:
                b["term"] = ("jmp", label_block[tgt])
            elif tgt in post_labels:
                b["term"] = ("end",)        # leaves the step (loop over)
            else:
                raise TranslationError(b.get("goto_ln", 0),
                                       f"goto to undefined label {tgt}")
    return blocks, exit_b


def _pc_emit(blocks):
    """Program-counter dispatch codegen for one step body."""
    src = [f"    _nb = 0", "    _pc = 0", "    while True:",
           "        _nb += 1",
           f"        if _nb > {_BLOCK_BUDGET}:",
           '            raise RuntimeError("legacy step: goto budget '
           'exceeded (cyclic control flow)")']
    for idx, b in enumerate(blocks):
        kw = "if" if idx == 0 else "elif"
        src.append(f"        {kw} _pc == {idx}:")
        src += ["            " + line for line in b["code"]]
        t = b["term"]
        if t[0] == "end":
            src.append("            break")
        elif t[0] == "jmp":
            src.append(f"            _pc = {t[1]}")
        else:
            src.append(f"            if {t[1]}:")
            src.append(f"                _pc = {t[2]}")
            src.append("            else:")
            src.append(f"                _pc = {t[3]}")
    src += ["        else:", "            break"]
    return src


def _expand_labelled_do(lines):
    """Rewrite classic labelled loops `do L var = a, b ... L continue`
    into `do var = a, b ... end do` so the structured translator accepts
    them. Rules (anything else raises TranslationError):
      - the terminator statement must be CONTINUE carrying label L
      - several loops may share one terminator (do 10 i / do 10 j / 10 continue)
      - no GOTO may target L (a jump to the terminator means "next
        iteration", which the strict subset does not model)
    """
    goto_targets = set()
    for _ln, _lab, st in lines:
        gm = re.match(r"^(?:go\s*to)\s+([0-9]+)\s*$", st.strip(), re.I)
        if gm:
            goto_targets.add(int(gm.group(1)))
    out, pending = [], []
    for ln, label, st in lines:
        hm = re.match(r"^do\s+([0-9]+)\s*,?\s*([A-Za-z].*)$", st.strip(), re.I)
        if hm:
            pending.append((int(hm.group(1)), ln))
            out.append((ln, label, "do " + hm.group(2)))
            continue
        if label is not None and pending and pending[-1][0] == label:
            if st.strip().lower() != "continue":
                raise TranslationError(
                    ln, f"labelled do {label} must end with continue")
            if label in goto_targets:
                raise TranslationError(
                    ln, f"goto to do-terminator label {label} unsupported")
            first = True
            while pending and pending[-1][0] == label:
                pending.pop()
                out.append((ln, None, "end do"))
            continue
        out.append((ln, label, st))
    if pending:
        raise TranslationError(pending[-1][1],
                               f"labelled do {pending[-1][0]} never closed")
    return out


def translate(text, dim=None):
    """Translate strict-subset F77 source -> executable Python source.

    `dim` = expected state dimension (entry["dim"]); the recorded/observed
    variables are then the first `dim` state floats in declaration order.
    """
    lines = _expand_labelled_do(logical_lines(text))
    types, decl_order = {}, []
    arrays = {}
    init_codes, init_names = [], []     # init_codes kept for order reference
    init_tree, body_tree = [], []
    avail, body_avail = set(), set()
    loop = None          # outermost (var, start_py, stop_py): the step loop
    loop_stack = []      # open do frames: {"var": name, "open": [nodes]}
    write_vars = None
    phase = "pre"
    ended = False
    stack = []           # open IF frames
    post_labels = set()
    defined_labels = set()
    has_goto = False

    def reg_label(label, ln):
        if label is None:
            return
        if label in defined_labels:
            raise TranslationError(ln, f"duplicate label {label}")
        defined_labels.add(label)
        if phase == "post":
            post_labels.add(label)

    def top_list():
        if stack:
            return stack[-1]["open"]
        if loop_stack:
            return loop_stack[-1]["open"]
        return init_tree if phase == "pre" else body_tree

    def cur_avail():
        if stack:
            return stack[-1]["open_avail"]
        return avail if phase == "pre" else (avail | body_avail)

    def add_avail(name):
        if phase == "body":
            body_avail.add(name)
        if stack:
            stack[-1]["open_avail"].add(name)
        elif phase == "pre":
            avail.add(name)

    def check_used(ln, used):
        missing = sorted(used - cur_avail())
        if missing:
            raise TranslationError(ln, f"read of not-yet-assigned "
                                   f"variable(s) {missing}")

    def push_if(node, ln):
        # one frame per if-CHAIN: else / else-if re-point `open`, a single
        # `end if` closes the whole chain (classic F77 semantics)
        base = set(cur_avail())
        stack.append({"node": node, "els": None, "open": node[3],
                      "avail_in": base, "open_avail": set(base),
                      "completed": [], "exhaustive": False})

    def decl(ln, st):
        m = re.match(r"^(integer|real|double\s+precision)\b\s*(.*)$",
                     st, re.I)
        kw = re.sub(r"\s+", " ", m.group(1).lower())
        rest = m.group(2)
        if kw == "integer":
            typ, names_part = "i", rest
            if re.match(r"^\s*[* (]", names_part):
                raise TranslationError(ln, "kinds/arrays on integer unsupported")
        elif kw == "double precision":
            typ, names_part = "d", rest
        else:
            mm = re.match(
                r"^\s*(?:\*\s*(?P<s>[0-9]+)"
                r"|\(\s*(?:kind\s*=\s*)?(?P<k>[0-9]+)\s*\))\s*(?P<rest>.*)$",
                rest)
            if mm:
                code = mm.group("s") or mm.group("k")
                if code == "8":
                    typ = "d"
                elif code == "4":
                    typ = "f"
                else:
                    raise TranslationError(ln, f"unsupported real kind {code}")
                names_part = mm.group("rest")
            else:
                typ, names_part = "f", rest
        for nm in names_part.split(","):
            nm = nm.strip()
            ma = re.fullmatch(
                r"([A-Za-z][A-Za-z0-9_]*)\s*\(\s*([0-9]+)\s*\)", nm)
            if ma:
                if typ == "i":
                    raise TranslationError(ln, "integer arrays unsupported")
                an, size = ma.group(1).lower(), int(ma.group(2))
                if an in types and (types[an] != typ or an not in arrays):
                    raise TranslationError(ln, f"conflicting declaration of "
                                           f"{an!r}")
                if an not in arrays:
                    types[an] = typ
                    arrays[an] = size
                    decl_order.extend(f"{an}[{j}]" for j in range(size))
                continue
            if not re.match(r"^[A-Za-z][A-Za-z0-9_]*$", nm):
                raise TranslationError(ln, f"unsupported declaration item "
                                       f"{nm!r} (only name or name(literal))")
            nm = nm.lower()
            if nm in arrays:
                raise TranslationError(ln, f"conflicting declaration of "
                                       f"{nm!r}")
            if nm in types and types[nm] != typ:
                raise TranslationError(ln, f"conflicting declaration of {nm!r}")
            if nm not in types:
                types[nm] = typ
                decl_order.append(nm)

    def parse_if_header(low, ln):
        """low starts at 'if (' -> (cond_py, cond_type_used, rest)."""
        p = low.index("(")
        depth = 0
        for i in range(p, len(low)):
            if low[i] == "(":
                depth += 1
            elif low[i] == ")":
                depth -= 1
                if depth == 0:
                    cond_txt, rest = low[p + 1:i], low[i + 1:]
                    break
        else:
            raise TranslationError(ln, "unbalanced parentheses in if condition")
        py, typ, used = _parse_expr(cond_txt, types, ln, arrays)
        if typ != "l":
            raise TranslationError(ln, "if condition must be logical")
        return py, used, rest.strip()

    for ln, label, raw_st in lines:
        st = raw_st.strip()
        low = st.lower()
        if ended:
            raise TranslationError(ln, "statement after program end")
        if label is not None and phase == "pre":
            raise TranslationError(ln, "labels before the do-loop unsupported")
        if re.match(r"^(program|module)\b", low) or re.match(r"^implicit\b", low):
            continue
        if phase == "post" and re.match(r"^end\s*(program)?\s*$", low):
            reg_label(label, ln)
            ended = True
            continue
        if re.match(r"^end\s*do\s*$", low) or re.match(r"^enddo\s*$", low):
            if phase != "body":
                raise TranslationError(ln, "end do without do")
            if stack:
                raise TranslationError(ln, "if not closed before end do")
            if label is not None:
                raise TranslationError(ln, "label on end do unsupported")
            if not loop_stack:
                raise TranslationError(ln, "end do without do")
            loop_stack.pop()
            if not loop_stack:
                phase = "post"
            continue
        if re.match(r"^end\s*if\s*$", low) or low == "endif":
            if not stack:
                raise TranslationError(ln, "end if without if")
            if label is not None:
                raise TranslationError(ln, "label on end if unsupported")
            fr = stack.pop()
            union = set(fr["open_avail"])
            for done in fr["completed"]:
                union |= done
            if fr["exhaustive"]:
                if stack:
                    stack[-1]["open_avail"] |= union
                elif phase == "pre":
                    avail.update(union)
                else:
                    body_avail.update(union)
            continue
        is_elseif = bool(re.match(r"^else\s*if\s*\(", low))
        is_else = low == "else"
        if is_elseif or is_else:
            if not stack:
                raise TranslationError(ln, "else without if")
            if label is not None:
                raise TranslationError(ln, "label on else unsupported")
            fr = stack[-1]
            if fr["exhaustive"]:
                raise TranslationError(ln, "duplicate else")
            fr["completed"].append(fr["open_avail"])
            fr["els"] = []
            fr["node"][4] = fr["els"]
            fr["open"] = fr["els"]
            fr["open_avail"] = set(fr["avail_in"])
            fr["exhaustive"] = is_else
            if is_else:
                continue
            # `else if (...) then`: open an inner if node INSIDE the same
            # chain frame, so one `end if` closes every branch
            py, used, rest = parse_if_header(low[4:].lstrip(), ln)
            check_used(ln, used)
            if rest != "then":
                raise TranslationError(ln, "else if must end with 'then'")
            inner = ["if", None, py, [], None]
            fr["els"].append(inner)
            fr["node"] = inner
            fr["open"] = inner[3]
            fr["open_avail"] = set(fr["avail_in"])
            continue
        if re.match(r"^if\s*\(", low):
            if phase == "post":
                raise TranslationError(ln, "if after end do unsupported")
            py, used, rest = parse_if_header(low, ln)
            check_used(ln, used)
            if rest == "then":
                node = ["if", label, py, [], None]
                reg_label(label, ln)
                top_list().append(node)
                push_if(node, ln)
            elif rest:
                node = ["if", label, py, [], None]
                reg_label(label, ln)
                top_list().append(node)
                push_if(node, ln)
                gm = re.match(r"^(?:go\s+to|goto)\s+([0-9]+)$", rest)
                if gm:
                    if phase == "pre":
                        raise TranslationError(ln, "goto before do-loop "
                                               "unsupported")
                    has_goto = True
                    top_list().append(["goto", None, int(gm.group(1)), ln])
                elif rest == "continue":
                    top_list().append(["n", None])
                else:
                    am2 = re.match(
                        r"^([A-Za-z][A-Za-z0-9_]*)\s*(?:\(([^()]*?)\))?"
                        r"\s*=\s*(.+)$", rest)
                    if not am2 or rest.lstrip().startswith("="):
                        raise TranslationError(ln, "unsupported single-line "
                                               f"if body: {rest!r}")
                    if am2.group(1).lower() in {f["var"] for f in loop_stack}:
                        raise TranslationError(ln, "assignment to do-loop "
                                               "variable unsupported "
                                               "(counter excluded)")
                    lhs_raw = am2.group(1) + \
                        (f"({am2.group(2)})" if am2.group(2) else "")
                    code, used2, key = _assign_code(lhs_raw, am2.group(3),
                                                    types, ln, arrays)
                    check_used(ln, used2)
                    add_avail(key)
                    top_list().append(["s", None, code])
                stack.pop()                 # no else: avail stays conservative
            else:
                raise TranslationError(ln, "if without a body")
            continue
        gm = re.match(r"^(?:go\s+to|goto)\s+([0-9]+)$", low)
        if gm:
            if phase == "pre":
                raise TranslationError(ln, "goto before do-loop unsupported")
            if phase == "post":
                raise TranslationError(ln, "goto after end do unsupported")
            has_goto = True
            reg_label(label, ln)
            top_list().append(["goto", label, int(gm.group(1)), ln])
            continue
        if low == "continue":
            if phase == "post":
                reg_label(label, ln)
            else:
                reg_label(label, ln)
                top_list().append(["n", label])
            continue
        if re.match(r"^(integer|real|double\s+precision)\b", low, re.I):
            if phase != "pre":
                raise TranslationError(ln, "declaration after do-loop")
            decl(ln, st)
            continue
        pm = re.match(r"^parameter\s*\((.*)\)$", st, re.I)
        if pm:
            if phase != "pre":
                raise TranslationError(ln, "parameter after do-loop")
            for part in pm.group(1).split(","):
                nm, eq, lit = part.partition("=")
                nm = nm.strip().lower()
                if not eq:
                    raise TranslationError(ln, f"bad parameter {part!r}")
                if re.search(r"[A-Za-z]", lit):
                    raise TranslationError(ln, "parameter expressions with "
                                            "names unsupported (literal only)")
                code, used, key = _assign_code(nm, lit.strip(), types, ln,
                                               arrays)
                check_used(ln, used)
                init_codes.append(code)
                top_list().append(["s", None, code])
                if key not in init_names:
                    init_names.append(key)
                add_avail(key)
            continue
        dm = re.match(r"^do\s+([A-Za-z][A-Za-z0-9_]*)\s*=\s*(.+?),\s*(.+)$",
                      st, re.I)
        if dm:
            if phase == "post":
                raise TranslationError(ln, "do after end do unsupported")
            if stack:
                raise TranslationError(ln, "if not closed before do-loop")
            if label is not None:
                raise TranslationError(ln, "label on do-loop header "
                                       "unsupported")
            var = dm.group(1).lower()
            if types.get(var) != "i":
                raise TranslationError(ln, f"do variable {var!r} must be "
                                        "declared integer")
            sp = _parse_expr(dm.group(2).strip(), types, ln, arrays)
            stop = _parse_expr(dm.group(3).strip(), types, ln, arrays)
            check_used(ln, sp[2] | stop[2])
            if sp[1] != "i" or stop[1] != "i":
                raise TranslationError(ln, "do bounds must be integer")
            if loop is None:
                loop = (var, sp[0], stop[0])
                phase = "body"
                loop_stack.append({"var": var, "open": body_tree})
            else:
                node = ["do", None, var, sp[0], stop[0], [], ln]
                top_list().append(node)
                loop_stack.append({"var": var, "open": node[5]})
            add_avail(var)
            continue
        wm = re.match(r"^write\s*\(\s*\*\s*,\s*\*\s*\)\s*(.*)$", st, re.I)
        if wm:
            if phase != "post":
                raise TranslationError(ln, "write only allowed after end do")
            reg_label(label, ln)
            write_vars = [v.strip().lower() for v in wm.group(1).split(",")]
            for v in write_vars:
                if v in types:
                    continue
                mw = re.fullmatch(r"([a-z][a-z0-9_]*)\s*\(\s*([0-9]+)\s*\)", v)
                if mw and mw.group(1) in arrays and \
                        1 <= int(mw.group(2)) <= arrays[mw.group(1)]:
                    continue
                raise TranslationError(ln, f"undeclared write target {v!r}")
            continue
        am = re.match(
            r"^([A-Za-z][A-Za-z0-9_]*)\s*(?:\(([^()]*?)\))?\s*=\s*(.+)$", st)
        if am and not st.startswith("="):
            rhs = am.group(3)
            if rhs.lstrip().startswith("="):
                raise TranslationError(ln, "relational == on the left of "
                                       "an assignment unsupported")
            if phase == "post":
                raise TranslationError(ln, "assignment after end do "
                                       "(only write/end allowed)")
            if am.group(1).lower() in {f["var"] for f in loop_stack}:
                raise TranslationError(ln, "assignment to do-loop variable "
                                       "unsupported (counter excluded)")
            lhs_raw = am.group(1) + (f"({am.group(2)})" if am.group(2) else "")
            code, used, key = _assign_code(lhs_raw, rhs, types, ln, arrays)
            check_used(ln, used)
            add_avail(key)
            # a write with a computed index (e.g. `y(j) = ...` inside a do)
            # targets a dynamic key like `y[(j) - 1]`; register every literal
            # element so state arrays stay visible to the recorded-var filter
            km = re.fullmatch(r"([a-z][a-z0-9_]*)\[([^\]]*)\]", key)
            if km and km.group(1) in arrays and not km.group(2).isdigit():
                for _j in range(arrays[km.group(1)]):
                    add_avail(f"{km.group(1)}[{_j}]")
            if phase == "pre" and "[" not in key and key not in init_names:
                init_names.append(key)
            if phase != "pre":
                reg_label(label, ln)
            top_list().append(["s", label, code])
            continue
        raise TranslationError(ln, f"unsupported statement: {st!r}")

    if loop is None:
        raise TranslationError(0, "no do-loop found (cannot build a step)")
    if phase != "post":
        raise TranslationError(0, "do-loop never closed with end do")
    if stack:
        raise TranslationError(0, "if never closed with end if")
    if not body_tree:
        raise TranslationError(0, "empty do-loop body")

    def _typ_of(key):
        return types[key.split("[")[0]] if "[" in key else types.get(key)

    stored = set(init_names)
    for a, sz in arrays.items():
        stored.update(f"{a}[{j}]" for j in range(sz))
    # record ONLY the first `dim` state floats in declaration order: keeps
    # the state vector, drops clock/parameter/scratch floats (t, dt, k...).
    float_vars = [k for k in decl_order
                  if k in stored and k in body_avail and _typ_of(k) in ("f", "d")]
    if dim is not None:
        float_vars = float_vars[:int(dim)]

    loads = [f"    {nm} = v[{nm!r}]" for nm in init_names]
    for a in arrays:
        keys = ", ".join(f"v['{a}[{j}]']" for j in range(arrays[a]))
        loads.append(f"    {a} = [{keys}]")
    stores = [f"    v[{nm!r}] = {nm}" for nm in init_names]
    for a in arrays:
        for j in range(arrays[a]):
            stores.append(f"    v['{a}[{j}]'] = {a}[{j}]")

    if has_goto:
        blocks, _ = _flatten(body_tree, post_labels)
        body_src = _pc_emit(blocks)
        mode = "goto_dispatch"
    else:
        body_src = _emit(body_tree, 4)
        mode = "structured"
    if not body_src:
        raise TranslationError(0, "empty do-loop body")

    src = ["import numpy as np", "",
           f"FLOAT_VARS = {float_vars!r}",
           f"STATE_NAMES = {init_names!r}",
           f"LOOP_VAR = {loop[0]!r}",
           f"MODE = {mode!r}", "",
           "def legacy_init():",
           "    v = {}"]
    for a in arrays:
        src.append(f"    {a} = [np.{_dtype_name(types[a])}(0.0)] "
                   f"* {arrays[a]}")
    init_src = _emit(init_tree, 4)
    src += init_src if init_src else ["    pass"]
    src += stores
    src += ["    return v", "",
            "def legacy_step(v, i=0):"]
    src += loads
    src += body_src
    src += stores
    src += ["    return v", "",
            "def legacy_steps(v=None):"]
    src += loads
    src += [f"    _s = {loop[1]}", f"    _e = {loop[2]}",
            "    return max(0, int(_e) - int(_s) + 1)"]
    if write_vars:
        src += ["", "def legacy_final(v):"]
        fin_loads, fin_vals, loaded = [], [], set()
        for v in write_vars:
            if v in arrays:
                if v not in loaded:
                    keys = ", ".join(f"v['{v}[{j}]']" for j in range(arrays[v]))
                    fin_loads.append(f"    {v} = [{keys}]")
                    loaded.add(v)
                fin_vals.append(v)
            elif "[" in v or "(" in v:
                mw = re.fullmatch(r"([a-z][a-z0-9_]*)\s*\(\s*([0-9]+)\s*\)", v)
                a, j = mw.group(1), int(mw.group(2)) - 1
                if a not in loaded:
                    keys = ", ".join(f"v['{a}[{k}]']" for k in range(arrays[a]))
                    fin_loads.append(f"    {a} = [{keys}]")
                    loaded.add(a)
                fin_vals.append(f"{a}[{j}]")
            else:
                if v not in loaded:
                    fin_loads.append(f"    {v} = v[{v!r}]")
                    loaded.add(v)
                fin_vals.append(v)
        src += fin_loads + [f"    return [{', '.join(fin_vals)}]"]
    src += [""]
    return "\n".join(src)


def load_legacy(text, dim=None):
    src = translate(text, dim)
    ns = {}
    exec(compile(src, "<legacy_f77>", "exec"), ns)
    ns["__source"] = src
    return ns


def lyap_legacy(step, v0, float_vars, n_tr, n_steps, h, d0=1e-8):
    """Benettin on the EXECUTED legacy step (two copies of the state dict)."""
    x = dict(v0)
    with np.errstate(all="ignore"):
        for _ in range(n_tr):
            try:
                x = step(x, 0)
            except Exception:
                return float("nan")
            if not all(math.isfinite(float(x[k])) for k in float_vars):
                return float("nan")
        y = dict(x)
        for k in float_vars:
            y[k] = x[k] + d0 / math.sqrt(len(float_vars))
        acc = 0.0
        for _ in range(n_steps):
            try:
                x = step(x, 0)
                y = step(y, 0)
            except Exception:
                return float("nan")
            d, nd2 = {}, 0.0
            for k in float_vars:
                dk = float(y[k]) - float(x[k])
                d[k] = dk
                nd2 += dk * dk
            nd = math.sqrt(nd2)
            if not math.isfinite(nd) or nd == 0.0:
                return float("nan")
            acc += math.log(nd / d0)
            y = dict(x)
            for k in float_vars:
                y[k] = x[k] + (d0 / nd) * d[k]
    return acc / (n_steps * h)


def fingerprint_ns(ns, entry, h=None, translator="strict_f77_subset"):
    """Run an already-loaded legacy module -> (candidate_fingerprint, info).

    Shared execution driver: any front-end that produces the legacy_* module
    shape (legacy_init/steps/step + FLOAT_VARS) can be fingerprinted here.
    """
    v0 = ns["legacy_init"]()
    n_steps = ns["legacy_steps"](v0)
    float_vars = ns["FLOAT_VARS"]
    if not float_vars:
        raise TranslationError(0, "no float state variables to record")
    if h is None:
        h = float(entry.get("dt_lyap", 1.0)) if entry["family"] == "ode" else 1.0
    if entry["family"] == "ode":
        n_window = int(round(entry.get("t_lyap", 1000.0) / h))
    else:
        n_window = int(entry.get("n_lyap", 50000))
    n_window = max(1, n_window)

    v = dict(v0)
    pts, finite = [], True
    with np.errstate(all="ignore"):
        for i in range(n_steps):
            try:
                v = ns["legacy_step"](v, i)
            except Exception:
                finite = False
                break
            row = [float(v[k]) for k in float_vars]
            if not all(math.isfinite(r) for r in row):
                finite = False
                break
            pts.append(row)

    window = pts[-N_POINTS_D2:] if len(pts) >= N_POINTS_D2 else pts
    arr = np.asarray(window, dtype=float) if window else np.empty((0, len(float_vars)))
    if finite and window:
        tail = arr[len(arr) // 2:]
        uniq = np.unique(np.round(tail, 12), axis=0).shape[0]
        uf = uniq / float(tail.shape[0])
    else:
        uf = 0.0
    d2 = fingerprint_d2(arr, finite and len(window) > 0)

    if finite and pts:
        lam = lyap_legacy(ns["legacy_step"], v0, float_vars,
                          n_tr=n_steps, n_steps=n_window, h=h)
    else:
        lam = float("nan")

    cand = {"lambda1": float(lam),
            "corr_dim": d2,
            "unique_frac": float(uf),
            "finite": bool(finite)}
    info = {
        "status": "executed_source",
        "translator": translator,
        "mode": ns.get("MODE", "structured"),
        "steps_run": n_steps,
        "recorded_vars": float_vars,
        "lambda1_transient": n_steps,
        "lambda1_window": n_window,
        "h": h,
        "translated_lines": len(ns["__source"].splitlines()),
        "translated_source": ns["__source"],
    }
    return cand, info


def fingerprint_source(text, entry, h=None):
    """Run the legacy F77 source and return (candidate_fingerprint, info)."""
    return fingerprint_ns(load_legacy(text, entry["dim"]), entry, h)
