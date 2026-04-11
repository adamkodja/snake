"""
Problèmes mathématiques pour Snake Math.

Conventions d'écriture des générateurs
───────────────────────────────────────
• Problem.statement  : chaîne mathtext matplotlib, toujours entre $…$
                       ex. r"$2x + 3 = 7$"
• MathExpr.value     : valeur Python pour la comparaison (int, str, float…)
• MathExpr.latex     : chaîne mathtext matplotlib entre $…$
                       ex. r"$\frac{\sqrt{3}}{2}$"

Pour ajouter un problème
────────────────────────
1. Définir une fonction () -> Problem ci-dessous.
2. L'ajouter dans PROBLEM_GENERATORS_BY_LEVEL au niveau voulu.
NUM_WRONGS doit rester égal à NUM_FRUITS - 1 (défini dans snake_math.py).
"""

from __future__ import annotations

import random
from dataclasses import dataclass, field
from enum import Enum
from io import BytesIO
from typing import Any, Callable

from PIL import Image, ImageTk

# Nombre de mauvaises réponses par question (= NUM_FRUITS - 1 dans snake_math.py)
NUM_WRONGS: int = 3


# ── Structures de base ────────────────────────────────────────────────────────

@dataclass
class MathExpr:
    """
    Paire (valeur, rendu LaTeX) pour une réponse mathématique.
    L'égalité porte uniquement sur `value` pour la comparaison dans le jeu.
    """
    value: Any   # utilisé pour savoir si le bon fruit est mangé
    latex: str   # mathtext matplotlib, ex. r"$\frac{1}{2}$"

    def __eq__(self, other: object) -> bool:
        if isinstance(other, MathExpr):
            return self.value == other.value
        return NotImplemented

    def __hash__(self) -> int:
        return hash(self.value)


@dataclass
class Problem:
    """Un problème mathématique avec énoncé et réponses sous forme de MathExpr."""
    statement:     str            # mathtext matplotlib ex. r"$\sin\!\left(\frac{\pi}{6}\right) = ?$"
    solution:      MathExpr
    wrong_answers: set[MathExpr] = field(default_factory=set)


ProblemGenerator = Callable[[], Problem]


# ── Rendu mathématique ────────────────────────────────────────────────────────

class MathRenderer:
    """
    Rend des chaînes mathtext matplotlib en PhotoImage tkinter.
    Cache les rendus par (latex, taille_pt, couleur).
    Dégradation gracieuse si matplotlib est absent.
    """

    _available: bool | None = None

    def __init__(self) -> None:
        self._cache: dict[tuple[str, int, str], ImageTk.PhotoImage | None] = {}
        if MathRenderer._available is None:
            try:
                import matplotlib
                matplotlib.use("Agg")
                MathRenderer._available = True
            except ImportError:
                MathRenderer._available = False

    def render(self, latex: str, fontsize: float,
               fg: str = "#eaeaea") -> ImageTk.PhotoImage | None:
        key = (latex, int(fontsize), fg)
        if key in self._cache:
            return self._cache[key]
        photo = self._make(latex, int(fontsize), fg) if MathRenderer._available else None
        self._cache[key] = photo
        return photo

    def _make(self, latex: str, fontsize: int, fg: str) -> ImageTk.PhotoImage | None:
        try:
            from matplotlib.figure import Figure
            from matplotlib.backends.backend_agg import FigureCanvasAgg

            dpi = 100
            fig = Figure(figsize=(6, 1.5), dpi=dpi)
            canvas = FigureCanvasAgg(fig)
            fig.patch.set_alpha(0.0)
            ax = fig.add_axes([0, 0, 1, 1])
            ax.set_axis_off()
            ax.patch.set_alpha(0.0)
            txt = ax.text(0.5, 0.5, latex, fontsize=fontsize,
                          ha="center", va="center", color=fg,
                          transform=ax.transAxes)
            canvas.draw()

            # Bounding box en pixels display (origine bas-gauche matplotlib)
            renderer = canvas.get_renderer()
            bb  = txt.get_window_extent(renderer)
            pad = max(3, fontsize * 0.35)
            fig_h_px = int(fig.get_figheight() * dpi)

            # Sauvegarder la figure entière, puis crop avec PIL
            buf = BytesIO()
            fig.savefig(buf, format="png", dpi=dpi, transparent=True)
            fig.clf()
            buf.seek(0)
            full = Image.open(buf).convert("RGBA")

            # Convertir bb en coords PIL (origine haut-gauche)
            x0 = max(0, int(bb.x0 - pad))
            x1 = min(full.width,  int(bb.x1 + pad))
            y0 = max(0, int(fig_h_px - bb.y1 - pad))
            y1 = min(full.height, int(fig_h_px - bb.y0 + pad))
            if x1 <= x0 or y1 <= y0:
                return None
            img = full.crop((x0, y0, x1, y1))
            return ImageTk.PhotoImage(img)
        except Exception:
            return None


# ── Helpers entiers ───────────────────────────────────────────────────────────

def _e(n: int) -> MathExpr:
    """Entier → MathExpr."""
    return MathExpr(n, f"${n}$")


def _int_wrongs(solution_val: int,
                candidate: Callable[[], int],
                n: int = NUM_WRONGS,
                positive: bool = True) -> set[MathExpr]:
    """Génère n MathExpr entières distinctes différentes de solution_val."""
    seen: set[int] = {solution_val}
    wrongs: set[MathExpr] = set()
    max_tries = 300
    while len(wrongs) < n and max_tries:
        v = candidate()
        max_tries -= 1
        if v in seen:
            continue
        if positive and v <= 0:
            continue
        seen.add(v)
        wrongs.add(_e(v))
    return wrongs


# ── Helpers trig ──────────────────────────────────────────────────────────────

# Toutes les valeurs exactes utilisables comme réponses / distracteurs
_TRIG_EXPRS: dict[str, MathExpr] = {
    "0":     MathExpr("0",    "$0$"),
    "1":     MathExpr("1",    "$1$"),
    "-1":    MathExpr("-1",   "$-1$"),
    "1/2":   MathExpr("1/2",  r"$\frac{1}{2}$"),
    "-1/2":  MathExpr("-1/2", r"$-\frac{1}{2}$"),
    "√2/2":  MathExpr("√2/2", r"$\frac{\sqrt{2}}{2}$"),
    "-√2/2": MathExpr("-√2/2",r"$-\frac{\sqrt{2}}{2}$"),
    "√3/2":  MathExpr("√3/2", r"$\frac{\sqrt{3}}{2}$"),
    "-√3/2": MathExpr("-√3/2",r"$-\frac{\sqrt{3}}{2}$"),
    "√3/3":  MathExpr("√3/3", r"$\frac{\sqrt{3}}{3}$"),
    "-√3/3": MathExpr("-√3/3",r"$-\frac{\sqrt{3}}{3}$"),
    "√3":    MathExpr("√3",   r"$\sqrt{3}$"),
    "-√3":   MathExpr("-√3",  r"$-\sqrt{3}$"),
}
_TRIG_POOL = list(_TRIG_EXPRS.keys())


def _trig_wrongs(solution_key: str, n: int = NUM_WRONGS) -> set[MathExpr]:
    pool = [k for k in _TRIG_POOL if k != solution_key]
    return {_TRIG_EXPRS[k] for k in random.sample(pool, min(n, len(pool)))}


# Table : (latex_fn, latex_angle, clé_solution)
_TRIG_TABLE: list[tuple[str, str, str]] = [
    (r"\sin", r"0",                    "0"),
    (r"\sin", r"\frac{\pi}{6}",        "1/2"),
    (r"\sin", r"\frac{\pi}{4}",        "√2/2"),
    (r"\sin", r"\frac{\pi}{3}",        "√3/2"),
    (r"\sin", r"\frac{\pi}{2}",        "1"),
    (r"\sin", r"\pi",                  "0"),
    (r"\sin", r"\frac{3\pi}{2}",       "-1"),
    (r"\cos", r"0",                    "1"),
    (r"\cos", r"\frac{\pi}{6}",        "√3/2"),
    (r"\cos", r"\frac{\pi}{4}",        "√2/2"),
    (r"\cos", r"\frac{\pi}{3}",        "1/2"),
    (r"\cos", r"\frac{\pi}{2}",        "0"),
    (r"\cos", r"\pi",                  "-1"),
    (r"\cos", r"\frac{3\pi}{2}",       "0"),
    (r"\tan", r"0",                    "0"),
    (r"\tan", r"\frac{\pi}{6}",        "√3/3"),
    (r"\tan", r"\frac{\pi}{4}",        "1"),
    (r"\tan", r"\frac{\pi}{3}",        "√3"),
]


# ── Niveau facile (primaire) ──────────────────────────────────────────────────

def addition_problem() -> Problem:
    a, b = random.randint(1, 20), random.randint(1, 20)
    sol  = a + b
    return Problem(
        statement     = f"${a} + {b} = ?$",
        solution      = _e(sol),
        wrong_answers = _int_wrongs(sol, lambda: sol + random.randint(-6, 6)),
    )


def subtraction_problem() -> Problem:
    a   = random.randint(6, 30)
    b   = random.randint(1, a - 1)
    sol = a - b
    return Problem(
        statement     = f"${a} - {b} = ?$",
        solution      = _e(sol),
        wrong_answers = _int_wrongs(sol, lambda: sol + random.randint(-5, 5)),
    )


def multiplication_problem() -> Problem:
    a, b = random.randint(2, 9), random.randint(2, 9)
    sol  = a * b
    return Problem(
        statement     = f"${a} \\times {b} = ?$",
        solution      = _e(sol),
        wrong_answers = _int_wrongs(sol, lambda: sol + random.choice(
            [-b, b, -a, a, -2*a, 2*b, random.randint(-8, 8)])),
    )


def division_problem() -> Problem:
    d   = random.randint(2, 9)
    q   = random.randint(2, 9)
    return Problem(
        statement     = f"${d * q} \\div {d} = ?$",
        solution      = _e(q),
        wrong_answers = _int_wrongs(q, lambda: q + random.randint(-4, 4)),
    )


def square_problem() -> Problem:
    a   = random.randint(2, 12)
    sol = a * a
    return Problem(
        statement     = f"${a}^{{2}} = ?$",
        solution      = _e(sol),
        wrong_answers = _int_wrongs(sol, lambda: sol + random.choice(
            [-(2*a-1), 2*a+1, 2*a-1, -(2*a+1), random.randint(-15, 15)])),
    )


def modulo_problem() -> Problem:
    b   = random.randint(2, 7)
    sol = random.randint(1, b - 1)
    a   = b * random.randint(2, 8) + sol
    return Problem(
        statement     = f"${a} \\,\\mathrm{{mod}}\\, {b} = ?$",
        solution      = _e(sol),
        wrong_answers = _int_wrongs(sol, lambda: random.randint(0, b - 1),
                                    positive=False),
    )


def pgcd_problem() -> Problem:
    g    = random.randint(2, 9)
    a, b = g * random.randint(2, 8), g * random.randint(2, 8)
    return Problem(
        statement     = f"$\\gcd({a},\\, {b}) = ?$",
        solution      = _e(g),
        wrong_answers = _int_wrongs(g, lambda: g + random.choice([-2, -1, 1, 2, 3])),
    )


# ── Niveau moyen (lycée) ──────────────────────────────────────────────────────

def linear_equation_problem() -> Problem:
    """ax + b = c  →  x entier."""
    a   = random.choice([-4, -3, -2, 2, 3, 4])
    sol = random.randint(-9, 9)
    c   = random.randint(-15, 15)
    b   = c - a * sol
    sign_b = f"+ {b}" if b >= 0 else f"- {-b}"
    return Problem(
        statement     = f"${a}x {sign_b} = {c}$",
        solution      = _e(sol),
        wrong_answers = _int_wrongs(sol, lambda: sol + random.randint(-5, 5),
                                    positive=False),
    )


def linear_equation2_problem() -> Problem:
    """ax + b = cx + d  →  x entier."""
    sol = random.randint(-8, 8)
    a   = random.choice([-4, -3, -2, 2, 3, 4])
    c   = random.choice([v for v in [-3, -2, -1, 1, 2, 3] if v != a])
    b   = random.randint(-10, 10)
    d   = b + (a - c) * sol
    sign_b = f"+ {b}" if b >= 0 else f"- {-b}"
    sign_d = f"+ {d}" if d >= 0 else f"- {-d}"
    return Problem(
        statement     = f"${a}x {sign_b} = {c}x {sign_d}$",
        solution      = _e(sol),
        wrong_answers = _int_wrongs(sol, lambda: sol + random.randint(-5, 5),
                                    positive=False),
    )


def quadratic_roots_problem() -> Problem:
    """x² + px + q = 0  →  plus petite racine entière."""
    r1, r2 = sorted(random.sample(range(-7, 8), 2))
    p, q   = -(r1 + r2), r1 * r2
    sign_p = f"+ {p}" if p >= 0 else f"- {-p}"
    sign_q = f"+ {q}" if q >= 0 else f"- {-q}"
    return Problem(
        statement     = f"$x^{{2}} {sign_p}x {sign_q} = 0 \\ (\\min)$",
        solution      = _e(r1),
        wrong_answers = _int_wrongs(r1, lambda: r1 + random.randint(-4, 4),
                                    positive=False),
    )


def trig_exact_problem() -> Problem:
    fn_latex, angle_latex, sol_key = random.choice(_TRIG_TABLE)
    return Problem(
        statement     = (f"${fn_latex}\\!\\left({angle_latex}\\right) = ?$"),
        solution      = _TRIG_EXPRS[sol_key],
        wrong_answers = _trig_wrongs(sol_key),
    )


def log_integer_problem() -> Problem:
    """log_b(b^n) = n."""
    b   = random.choice([2, 3, 5, 10])
    n   = random.randint(1, 5)
    val = b ** n
    return Problem(
        statement     = f"$\\log_{{{b}}}({val}) = ?$",
        solution      = _e(n),
        wrong_answers = _int_wrongs(n, lambda: n + random.randint(-3, 3)),
    )


def exp_equation_problem() -> Problem:
    """b^x = val  →  x entier."""
    b   = random.choice([2, 3, 5])
    sol = random.randint(1, 5)
    val = b ** sol
    return Problem(
        statement     = f"${b}^x = {val} \\Rightarrow x = ?$",
        solution      = _e(sol),
        wrong_answers = _int_wrongs(sol, lambda: sol + random.randint(-3, 3)),
    )


def arithmetic_sequence_problem() -> Problem:
    """u_n = u_0 + n·r."""
    u0  = random.randint(-10, 10)
    r   = random.randint(-5, 5)
    n   = random.randint(3, 8)
    sol = u0 + n * r
    return Problem(
        statement     = f"$u_{{0}}={u0},\\ r={r} \\Rightarrow u_{{{n}}}=?$",
        solution      = _e(sol),
        wrong_answers = _int_wrongs(sol, lambda: sol + random.randint(-6, 6),
                                    positive=False),
    )


def geometric_sequence_problem() -> Problem:
    """u_n = u_0 · q^n."""
    u0  = random.choice([-3, -2, -1, 1, 2, 3])
    q   = random.choice([-2, 2, 3])
    n   = random.randint(2, 4)
    sol = u0 * (q ** n)
    return Problem(
        statement     = f"$u_{{0}}={u0},\\ q={q} \\Rightarrow u_{{{n}}}=?$",
        solution      = _e(sol),
        wrong_answers = _int_wrongs(sol,
                                    lambda: sol + random.choice(
                                        [-u0*2, -u0, u0, u0*2,
                                         random.randint(-10, 10)]),
                                    positive=False),
    )


# ── Niveaux de difficulté ─────────────────────────────────────────────────────

class Difficulty(Enum):
    EASY   = "Facile"
    MEDIUM = "Moyen"
    HARD   = "Difficile"


# Ajoutez vos générateurs dans la liste correspondante.
PROBLEM_GENERATORS_BY_LEVEL: dict[Difficulty, list[ProblemGenerator]] = {
    Difficulty.EASY2: [
        addition_problem,
        subtraction_problem,
        multiplication_problem,
        division_problem,
        square_problem,
        modulo_problem,
        pgcd_problem,
    ],
    Difficulty.MEDIUM: [
        linear_equation_problem,
        linear_equation2_problem,
        quadratic_roots_problem,
        trig_exact_problem,
        log_integer_problem,
        exp_equation_problem,
        arithmetic_sequence_problem,
        geometric_sequence_problem,
    ],
    Difficulty.HARD: [],   # à compléter
}
