"""
Snake Math — mangez le fruit dont la valeur est la réponse au problème affiché.
Bon fruit  → la snake grandit, nouveau problème.
Mauvais fruit → -1 vie, le fruit disparaît (le problème reste).
Mur / soi-même → -1 vie, réinitialisation de la position.

Paramètres modulables : COLS, ROWS (grille), HUD_ROWS (hauteur du bandeau).
La taille des cellules et des polices s'adapte automatiquement à la taille de la fenêtre.

Ajout d'un problème : créez une fonction () -> Problem et ajoutez-la à PROBLEM_GENERATORS.
"""

import tkinter as tk
import random
import time
from pathlib import Path
from dataclasses import dataclass
from typing import Any, List, Set, Tuple

from PIL import Image, ImageTk

# ── Paramètres de grille (seuls réglages à modifier) ─────────────────────────

COLS        = 16     # nombre de colonnes
ROWS        = 12     # nombre de lignes
HUD_ROWS    = 2.8    # hauteur du HUD exprimée en cellules (fraction autorisée)
SPEED_MS    = 180    # ms entre chaque déplacement logique (avance d'une cellule)
RENDER_MS   = 16     # ms entre chaque rendu (~60 fps)
LIVES_START = 3      # vies de départ
NUM_FRUITS      = 4     # fruits simultanés (1 correct + NUM_FRUITS-1 faux)
LIFE_FRUIT_PROB = 0.35  # proba d'apparition d'un fruit-vie quand vies < max

# Taille initiale de la fenêtre (sera redimensionnable librement)
DEFAULT_CELL = 36
ASSETS_DIR = Path(__file__).with_name("assets")
SPRITE_FRAMES = 6
SPRITE_DIRECTIONS = ("Right", "Up", "Left", "Down")
SPRITE_ROTATION = {"Right": 0, "Up": 90, "Left": 180, "Down": 270}
FRUIT_KINDS = ("apple", "orange", "strawberry", "pear")

# ── Couleurs ──────────────────────────────────────────────────────────────────

PALETTE = dict(
    bg        = "#1a1a2e",
    grid      = "#16213e",
    hud_bg    = "#0f3460",
    snake_hd  = "#e94560",
    snake_bd  = "#53607a",
    fruit     = "#4ecca3",
    fruit_txt = "#1a1a2e",
    txt       = "#eaeaea",
    lives     = "#e94560",
    overlay   = "#0f3460",
)

from problems import (
    Problem, MathExpr, MathRenderer,
    Difficulty,
    PROBLEM_GENERATORS_BY_LEVEL,
)



# ── Jeu ───────────────────────────────────────────────────────────────────────

DIRS     = {"Up": (0, -1), "Down": (0, 1), "Left": (-1, 0), "Right": (1, 0)}
OPPOSITE = {"Up": "Down", "Down": "Up", "Left": "Right", "Right": "Left"}


@dataclass
class Fruit:
    col:     int
    row:     int
    value:   Any
    correct: bool
    is_life: bool = False


class SnakeMathGame:
    snake: List[Tuple[int, int]]
    direction: str
    next_dir: str
    lives: int
    score: int
    game_over: bool
    paused: bool
    fruits: List[Fruit]
    problem: Problem | None
    anim_frame: int
    _fruit_sources: dict[str, list[Image.Image]]
    _fruit_cache: dict[str, list[ImageTk.PhotoImage]]
    _fruit_cache_size: int
    _fruit_refs: List[ImageTk.PhotoImage]
    _math_renderer_refs: List[ImageTk.PhotoImage]
    _fruit_label_refs: List[ImageTk.PhotoImage]

    def __init__(self, master: tk.Tk) -> None:
        self.root = master
        self.root.title("Snake Maths")

        # Taille initiale calculée depuis DEFAULT_CELL
        init_w = int(COLS * DEFAULT_CELL)
        init_h = int((ROWS + HUD_ROWS) * DEFAULT_CELL)
        self.root.geometry(f"{init_w}x{init_h}")
        self.root.minsize(COLS * 8, int((ROWS + HUD_ROWS) * 8))

        self.canvas = tk.Canvas(self.root, bg=PALETTE["bg"], highlightthickness=0)
        self.canvas.pack(fill=tk.BOTH, expand=True)

        # Dimensions dynamiques (recalculées à chaque resize)
        self._cell  = float(DEFAULT_CELL)
        self._hud_h = DEFAULT_CELL * HUD_ROWS
        self.snake: List[Tuple[int, int]] = []
        self.direction = "Right"
        self.next_dir = "Right"
        self.lives = LIVES_START
        self.score = 0
        self.game_over = False
        self.paused = False
        self.fruits: List[Fruit] = []
        self.problem: Problem | None = None
        self.anim_frame = 0
        self._sprite_sources = self._load_sprite_sources()
        self._sprite_cache_size = 0
        self._sprite_cache: dict[str, Any] = {"head": {}, "body": [], "tail": {}}
        self._sprite_refs: List[ImageTk.PhotoImage] = []
        self._fruit_sources = self._load_fruit_sources()
        self._fruit_cache_size = 0
        self._fruit_cache: dict[str, list[ImageTk.PhotoImage]] = {}
        self._fruit_refs: List[ImageTk.PhotoImage] = []

        self._math_renderer = MathRenderer()
        self.difficulty: Difficulty = Difficulty.EASY
        self.menu_open: bool = False
        self._menu_buttons: List[Tuple[float, float, float, float, str]] = []
        self._game_started: bool = False

        self.canvas.bind("<Configure>", self._on_resize)
        self.canvas.bind("<Button-1>", self._on_click)
        self.root.bind("<KeyPress>", self._on_key)

        self._refresh_sprite_cache()

        self._init_game()
        self.menu_open = True
        self.paused    = True
        self._render_loop()
        self._logic_tick()

    # ── Redimensionnement ─────────────────────────────────────────────────────

    def _on_resize(self, event: tk.Event) -> None:
        """Recalcule la taille de cellule à partir des nouvelles dimensions du canvas."""
        w, h = event.width, event.height
        # La cellule est carrée ; on choisit la plus petite dimension contraignante.
        self._cell  = min(w / COLS, h / (ROWS + HUD_ROWS))
        self._hud_h = self._cell * HUD_ROWS
        self._refresh_sprite_cache()

    @property
    def cell(self) -> float:
        return self._cell

    @property
    def hud_h(self) -> float:
        return self._hud_h

    # ── Tailles de polices (proportionnelles à la cellule) ────────────────────

    def _font(self, ratio: float, bold: bool = False) -> tuple:
        size   = max(7, int(self._cell * ratio))
        weight = "bold" if bold else "normal"
        return ("Helvetica", size, weight)

    def _load_sprite_sources(self) -> dict[str, list[Image.Image]]:
        sources: dict[str, list[Image.Image]] = {}
        for part in ("head", "body", "tail"):
            frames: list[Image.Image] = []
            for index in range(SPRITE_FRAMES):
                path = ASSETS_DIR / f"snake_neo_arcade_{part}_{index}.png"
                if not path.exists():
                    continue
                with Image.open(path) as image:
                    frames.append(image.convert("RGBA"))
            sources[part] = frames
        return sources

    def _load_fruit_sources(self) -> dict[str, list[Image.Image]]:
        sources: dict[str, list[Image.Image]] = {}
        fruit_dir = ASSETS_DIR / "fruits"
        for kind in FRUIT_KINDS:
            frames: list[Image.Image] = []
            for index in range(SPRITE_FRAMES):
                path = fruit_dir / f"{kind}_{index}.png"
                if not path.exists():
                    continue
                with Image.open(path) as image:
                    frames.append(image.convert("RGBA"))
            sources[kind] = frames
        return sources

    def _refresh_sprite_cache(self) -> None:
        size = max(20, int(self._cell * 0.92))
        if size == self._sprite_cache_size:
            return

        self._sprite_cache_size = size
        self._sprite_refs = []
        self._sprite_cache = {"head": {}, "body": [], "tail": {}}

        for frame in self._sprite_sources.get("body", []):
            photo = ImageTk.PhotoImage(frame.resize((size, size), Image.Resampling.LANCZOS))
            self._sprite_cache["body"].append(photo)
            self._sprite_refs.append(photo)

        for part in ("head", "tail"):
            rotated_frames: dict[str, list[ImageTk.PhotoImage]] = {}
            for direction in SPRITE_DIRECTIONS:
                angle = SPRITE_ROTATION[direction]
                frames: list[ImageTk.PhotoImage] = []
                for source in self._sprite_sources.get(part, []):
                    resized = source.resize((size, size), Image.Resampling.LANCZOS)
                    if angle:
                        resized = resized.rotate(angle, expand=True)
                    photo = ImageTk.PhotoImage(resized)
                    frames.append(photo)
                    self._sprite_refs.append(photo)
                rotated_frames[direction] = frames
            self._sprite_cache[part] = rotated_frames

        self._refresh_fruit_cache()

    def _refresh_fruit_cache(self) -> None:
        size = max(22, int(self._cell * 0.95))
        if size == self._fruit_cache_size:
            return

        self._fruit_cache_size = size
        self._fruit_refs = []
        self._fruit_cache = {}

        for kind, frames in self._fruit_sources.items():
            photos: list[ImageTk.PhotoImage] = []
            for frame in frames:
                resized = frame.resize((size, size), Image.Resampling.LANCZOS)
                photo = ImageTk.PhotoImage(resized)
                photos.append(photo)
                self._fruit_refs.append(photo)
            self._fruit_cache[kind] = photos

    # ── Initialisation ────────────────────────────────────────────────────────

    def _init_game(self) -> None:
        cx, cy = COLS // 2, ROWS // 2
        self.snake: List[Tuple[int, int]] = [(cx, cy), (cx - 1, cy), (cx - 2, cy)]
        self.prev_snake: List[Tuple[int, int]] = list(self.snake)
        self._last_step_time: float = time.monotonic()
        self.direction  = "Right"
        self.next_dir   = "Right"
        self.lives      = LIVES_START
        self.score      = 0
        self.game_over  = False
        self.paused     = False
        self.fruits: List[Fruit] = []
        self.problem: Problem | None = None
        self._new_problem()

    def _new_problem(self) -> None:
        generators = PROBLEM_GENERATORS_BY_LEVEL.get(self.difficulty)
        self.problem = random.choice(generators)()
        self._place_fruits()

    def _place_fruits(self) -> None:
        occupied: Set[Tuple[int, int]] = set(self.snake)
        answers = [self.problem.solution] + list(self.problem.wrong_answers)
        while len(answers) < NUM_FRUITS:
            sol_val = self.problem.solution.value
            extra   = (sol_val + random.randint(1, 10)
                       if isinstance(sol_val, int) else sol_val)
            answers.append(MathExpr(extra, f"${extra}$"))
        answers = answers[:NUM_FRUITS]
        random.shuffle(answers)

        self.fruits = []
        for val in answers:
            pos = self._free_cell(occupied)
            occupied.add(pos)
            self.fruits.append(Fruit(pos[0], pos[1], val, val == self.problem.solution))

        # Fruit-vie : apparaît aléatoirement quand on n'est pas à pleine vie
        if self.lives < LIVES_START and random.random() < LIFE_FRUIT_PROB:
            pos = self._free_cell(occupied)
            self.fruits.append(Fruit(pos[0], pos[1],
                                     MathExpr("life", r"$\heartsuit$"),
                                     False, is_life=True))

    def _free_cell(self, occupied: Set[Tuple[int, int]]) -> Tuple[int, int]:
        while True:
            c = random.randint(0, COLS - 1)
            r = random.randint(0, ROWS - 1)
            if (c, r) not in occupied:
                return (c, r)

    # ── Événements ────────────────────────────────────────────────────────────

    def _on_key(self, event: tk.Event) -> None:
        k = event.keysym
        if k == "Escape":
            self.menu_open = not self.menu_open
            self.paused    = self.menu_open
        elif self.menu_open:
            return                         # les autres touches sont bloquées par le menu
        elif k in DIRS and k != OPPOSITE.get(self.direction):
            self.next_dir = k
        elif k in ("p", "P"):
            self.paused = not self.paused
        elif k in ("r", "R") and self.game_over:
            self._init_game()

    def _on_click(self, event: tk.Event) -> None:
        if not self.menu_open:
            return
        x, y = event.x, event.y
        for x0, y0, x1, y1, action in self._menu_buttons:
            if x0 <= x <= x1 and y0 <= y <= y1:
                self._menu_action(action)
                break

    def _menu_action(self, action: str) -> None:
        if action == "resume":
            # Commencer (1ère fois) ou Reprendre
            self._game_started = True
            self.menu_open = False
            self.paused    = False
        elif action == "quit":
            self.root.destroy()
        elif action in (d.name for d in Difficulty):
            new_diff = Difficulty[action]
            self.difficulty = new_diff
            self._init_game()
            self._game_started = True
            self.menu_open = False
            self.paused    = False

    def _render_loop(self) -> None:
        """Rendu à ~60 fps, indépendant de la vitesse logique."""
        self._draw()
        self.root.after(RENDER_MS, self._render_loop)

    def _logic_tick(self) -> None:
        """Avance la logique du jeu d'une cellule, toutes les SPEED_MS ms."""
        if not self.game_over and not self.paused:
            self.prev_snake = list(self.snake)
            self.anim_frame = (self.anim_frame + 1) % SPRITE_FRAMES
            self._step()
            self._last_step_time = time.monotonic()
        self.root.after(SPEED_MS, self._logic_tick)

    def _step(self) -> None:
        self.direction = self.next_dir
        dx, dy = DIRS[self.direction]
        hx, hy = self.snake[0]
        nx, ny = hx + dx, hy + dy

        if not (0 <= nx < COLS and 0 <= ny < ROWS):
            self._lose_life(); return

        if (nx, ny) in set(self.snake):
            self._lose_life(); return

        eaten: Fruit | None = next(
            (f for f in self.fruits if f.col == nx and f.row == ny), None
        )

        self.snake.insert(0, (nx, ny))

        if eaten is None:
            self.snake.pop()
        elif eaten.is_life:
            self.snake.pop()              # pas de croissance
            self.lives = min(LIVES_START, self.lives + 1)
            self.fruits.remove(eaten)
        elif eaten.correct:
            self.score += 1
            self._new_problem()           # grandit (pas de pop)
        else:
            self.snake.pop()              # pas de croissance
            self.lives -= 1
            self.fruits.remove(eaten)
            if self.lives <= 0:
                self.game_over = True

    def _lose_life(self) -> None:
        self.lives -= 1
        if self.lives <= 0:
            self.game_over = True
            return
        cx, cy = COLS // 2, ROWS // 2
        self.snake     = [(cx, cy), (cx - 1, cy), (cx - 2, cy)]
        self.direction = "Right"
        self.next_dir  = "Right"

    # ── Rendu ─────────────────────────────────────────────────────────────────

    def _draw(self) -> None:
        cv   = self.canvas
        P    = PALETTE
        cell = self._cell
        oy   = self._hud_h     # décalage vertical de la grille
        cw   = cell * COLS
        cv.delete("all")

        # ── HUD ──────────────────────────────────────────────────────────────
        cv.create_rectangle(0, 0, cw, oy, fill=P["hud_bg"], outline="")

        if self.problem:
            fs    = max(10, int(cell * 0.52))
            photo = self._math_renderer.render(
                self.problem.statement, fs, P["txt"])
            if photo:
                self._math_renderer_refs = [photo]
                cv.create_image(cw / 2, oy * 0.38, image=photo)
            else:
                cv.create_text(cw / 2, oy * 0.38,
                               text=self.problem.statement,
                               fill=P["txt"], font=self._font(0.65, bold=True))

        hearts = "♥ " * self.lives + "♡ " * (LIVES_START - self.lives)
        cv.create_text(
            cell * 0.3, oy * 0.75,
            text=hearts, fill=P["lives"],
            font=self._font(0.45), anchor="w",
        )
        cv.create_text(
            cw - cell * 0.3, oy * 0.75,
            text=f"Score : {self.score}", fill=P["txt"],
            font=self._font(0.42), anchor="e",
        )

        # ── Grille ───────────────────────────────────────────────────────────
        for r in range(ROWS):
            for c in range(COLS):
                cv.create_rectangle(
                    c * cell,        r * cell + oy,
                    c * cell + cell, r * cell + oy + cell,
                    fill=P["grid"], outline=P["bg"],
                )

        # ── Fruits ───────────────────────────────────────────────────────────
        pad = max(2.0, cell * 0.06)
        self._fruit_label_refs = []   # garder les PhotoImage en vie

        for fruit in self.fruits:
            bx0 = fruit.col * cell + pad
            by0 = fruit.row * cell + oy + pad
            bx1 = fruit.col * cell + cell - pad
            by1 = fruit.row * cell + oy + cell - pad

            if fruit.is_life:
                # Case fruit-vie (rouge/rose)
                cv.create_rectangle(bx0, by0, bx1, by1,
                                     fill="#3a0a18", outline=P["lives"],
                                     width=max(1, int(cell * 0.06)))
                cv.create_text((bx0 + bx1) / 2, (by0 + by1) / 2,
                               text="♥", fill=P["lives"],
                               font=("Helvetica", max(10, int(cell * 0.5)), "bold"))
            else:
                # Case réponse normale
                cv.create_rectangle(bx0, by0, bx1, by1,
                                     fill="#0f1e3c", outline=P["fruit"],
                                     width=max(1, int(cell * 0.05)))
                fs    = max(7, int(cell * 0.35))
                photo = self._math_renderer.render(fruit.value.latex, fs, "#eaeaea")
                if photo:
                    self._fruit_label_refs.append(photo)
                    cv.create_image((bx0 + bx1) / 2, (by0 + by1) / 2, image=photo)
                else:
                    cv.create_text((bx0 + bx1) / 2, (by0 + by1) / 2,
                                   text=str(fruit.value.value), fill="#eaeaea",
                                   font=("Helvetica", max(7, int(cell * 0.38)), "bold"))

        # ── Serpent ───────────────────────────────────────────────────────────
        elapsed = time.monotonic() - self._last_step_time
        t = min(1.0, elapsed / (SPEED_MS / 1000.0))
        self._draw_snake(cv, oy, cell, t)

        # ── Overlays ──────────────────────────────────────────────────────────
        if self.menu_open:
            self._draw_menu()
        elif self.paused:
            self._overlay("PAUSE", "P  pour continuer")
        if self.game_over:
            self._overlay("GAME OVER", f"Score : {self.score}   —   R pour rejouer")

    def _interp_pos(self, index: int, t: float) -> Tuple[float, float]:
        """Retourne la position interpolée (col, row) d'un segment à l'instant t ∈ [0, 1]."""
        cx, cy = self.snake[index]
        if index < len(self.prev_snake):
            px, py = self.prev_snake[index]
        else:
            px, py = cx, cy          # nouveau segment (croissance) : pas d'interpolation
        return px + (cx - px) * t, py + (cy - py) * t

    def _draw_snake(self, cv: tk.Canvas, oy: float, cell: float, t: float) -> None:
        head_frames = self._sprite_cache.get("head", {}).get(self.direction, [])
        body_frames = self._sprite_cache.get("body", [])
        tail_frames = self._sprite_cache.get("tail", {}).get(self._tail_direction(), [])

        if not head_frames or not body_frames or not tail_frames:
            margin = max(1.0, cell * 0.06)
            for i in range(len(self.snake)):
                ic, ir = self._interp_pos(i, t)
                color = PALETTE["snake_hd"] if i == 0 else PALETTE["snake_bd"]
                cv.create_rectangle(
                    ic * cell + margin,        ir * cell + oy + margin,
                    ic * cell + cell - margin, ir * cell + oy + cell - margin,
                    fill=color, outline="",
                )
            return

        for index in range(len(self.snake)):
            ic, ir = self._interp_pos(index, t)
            px = ic * cell + cell / 2
            py = ir * cell + oy + cell / 2
            if index == 0:
                frame = head_frames[self.anim_frame % len(head_frames)]
            elif index == len(self.snake) - 1:
                frame = tail_frames[self.anim_frame % len(tail_frames)]
            else:
                frame = body_frames[(self.anim_frame + index) % len(body_frames)]
            cv.create_image(px, py, image=frame)

    def _tail_direction(self) -> str:
        if len(self.snake) < 2:
            return self.direction
        tail_x, tail_y = self.snake[-1]
        prev_x, prev_y = self.snake[-2]
        dx = tail_x - prev_x
        dy = tail_y - prev_y
        return "Right" if dx > 0 else "Left" if dx < 0 else ("Down" if dy > 0 else "Up")

    def _draw_menu(self) -> None:
        cv   = self.canvas
        P    = PALETTE
        cell = self._cell
        W    = cell * COLS
        H    = cell * ROWS + self._hud_h

        # Tailles basées sur cell
        btn_h   = max(22.0, cell * 0.85)
        btn_w   = max(140.0, cell * 6.5)
        gap     = max(5.0,   cell * 0.22)
        gap_sm  = gap * 0.45
        title_h = max(16.0, cell * 0.9)
        sep_h   = max(12.0, cell * 0.55)
        pad_v   = max(10.0, cell * 0.55)
        pad_h   = max(12.0, cell * 0.6)

        # Hauteur totale calculée depuis le contenu (pas de débordement possible)
        total_h = (pad_v + title_h + gap
                   + btn_h + gap
                   + sep_h + gap_sm
                   + btn_h + gap_sm + btn_h + gap_sm + btn_h
                   + gap
                   + btn_h + pad_v)

        bh    = total_h
        bw    = btn_w + 2 * pad_h
        bx0   = (W - bw) / 2
        by0   = (H - bh) / 2
        bx1   = bx0 + bw
        btn_x0 = bx0 + pad_h
        btn_x1 = bx1 - pad_h

        # Fond semi-transparent + boite
        cv.create_rectangle(0, 0, W, H, fill="#0a0a1a", stipple="gray50", outline="")
        cv.create_rectangle(bx0, by0, bx1, by0 + bh,
                            fill=P["overlay"], outline=P["snake_hd"],
                            width=max(1, int(cell * 0.08)))

        self._menu_buttons = []
        cursor = by0 + pad_v

        # ── Titre ──────────────────────────────────────────────────────────
        cv.create_text(W / 2, cursor + title_h / 2, text="MENU",
                       fill=P["snake_hd"],
                       font=("Helvetica", max(10, int(cell * 0.65)), "bold"))
        cursor += title_h + gap

        # ── Helper : dessine un bouton et avance le curseur ─────────────────
        def draw_btn(label, action, active=False, disabled=False, g_after=None):
            nonlocal cursor
            if g_after is None:
                g_after = gap
            y0 = cursor
            y1 = y0 + btn_h
            if disabled:
                fill, outline, tc = "#252538", "#44445a", "#55556a"
            elif active:
                fill, outline, tc = P["snake_hd"], P["txt"], P["bg"]
            else:
                fill, outline, tc = P["hud_bg"], P["snake_hd"], P["txt"]
            cv.create_rectangle(btn_x0, y0, btn_x1, y1, fill=fill,
                                outline=outline, width=max(1, int(cell * 0.05)))
            cv.create_text((btn_x0 + btn_x1) / 2, (y0 + y1) / 2,
                           text=label, fill=tc,
                           font=("Helvetica", max(8, int(cell * 0.36)), "bold"))
            if not disabled:
                self._menu_buttons.append((btn_x0, y0, btn_x1, y1, action))
            cursor = y1 + g_after

        # ── Commencer / Reprendre ───────────────────────────────────────────
        lbl = "▶ Reprendre" if self._game_started else "▶ Commencer"
        draw_btn(lbl, "resume")

        # ── Label Difficulté ────────────────────────────────────────────────
        cv.create_text(W / 2, cursor + sep_h / 2, text="Difficulté",
                       fill=P["txt"],
                       font=("Helvetica", max(7, int(cell * 0.3)), "normal"))
        cursor += sep_h + gap_sm

        # ── 3 boutons de difficulté ─────────────────────────────────────────
        diff_info = [
            (Difficulty.EASY,   "Facile"),
            (Difficulty.MEDIUM, "Moyen"),
            (Difficulty.HARD,   "Difficile (bientôt)"),
        ]
        for idx, (diff, lbl) in enumerate(diff_info):
            empty    = not PROBLEM_GENERATORS_BY_LEVEL.get(diff)
            is_last  = (idx == len(diff_info) - 1)
            draw_btn(lbl, diff.name,
                     active=  (diff == self.difficulty and not empty),
                     disabled= empty,
                     g_after=  gap if is_last else gap_sm)

        # ── Quitter ─────────────────────────────────────────────────────────
        draw_btn("✕ Quitter", "quit")

    def _overlay(self, title: str, subtitle: str) -> None:
        cv   = self.canvas
        P    = PALETTE
        cell = self._cell
        W    = cell * COLS
        H    = cell * ROWS + self._hud_h
        cv.create_rectangle(
            W / 5, H / 3, 4 * W / 5, 2 * H / 3,
            fill=P["overlay"], outline=P["snake_hd"], width=max(1, int(cell * 0.08)),
        )
        cv.create_text(W / 2, H / 2 - cell * 0.7,
                       text=title, fill=P["snake_hd"],
                       font=self._font(0.75, bold=True))
        cv.create_text(W / 2, H / 2 + cell * 0.5,
                       text=subtitle, fill=P["txt"],
                       font=self._font(0.38))


# ── Lancement ─────────────────────────────────────────────────────────────────

if __name__ == "__main__":
    app_root = tk.Tk()
    SnakeMathGame(app_root)
    app_root.mainloop()
