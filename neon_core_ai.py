"""
NEON CORE AI - v3.0 (corrected)
================================
A rewrite of the original single-file Tkinter chatbot that fixes the
structural bugs found in the previous version:

  1. Persistence now writes to real JSON files instead of trying to
     rewrite the running script's own source code (which never worked,
     since the marker comments it searched for didn't exist).
  2. Intent detection uses word-boundary regexes with a sane priority
     order instead of loose substring matches (the old 'convert' intent
     matched on the bare words "to"/"in", which hijacked almost every
     other intent).
  3. Reminders no longer swallow the user's real message - they are
     delivered out-of-band by the periodic UI poll, not injected into
     the normal response path.
  4. The math "calculate" intent strips filler words ("what is", "solve",
     etc.) before evaluating, so natural phrasing actually works.
  5. Settings that are exposed in the UI now actually affect behaviour
     (timestamps, humor level, response verbosity, personality).
  6. tk.IntVar bound to a ttk.Scale (which yields floats) is replaced
     with tk.DoubleVar to avoid a TclError when the slider moves.
  7. Reminder timers are daemon threads so the app can exit cleanly.
  8. The chat listbox keeps a parallel id list so selection -> chat id
     lookups can't desync from a stale sort of the storage dict.
  9. ASCII-art detection no longer misclassifies short, punctuation
     heavy replies (dice rolls, passwords, coin flips) as ASCII art.
 10. Card-suit glyphs are reachable (or removed) instead of being dead
     code behind an isalpha()/isdigit() filter that always excluded them.
 11. Unit conversion & password-length parsing accept more natural
     phrasing.
"""

import datetime
import json
import math
import os
import queue
import random
import re
import threading
import tkinter as tk
from tkinter import filedialog, scrolledtext, messagebox, simpledialog, ttk

try:
    from PIL import Image, ImageTk
    HAS_PIL = True
except Exception:
    HAS_PIL = False

# =============================================================================
# APP DATA DIRECTORY (real persistence, not source-rewriting)
# =============================================================================
APP_DIR = os.path.join(os.path.expanduser("~"), ".neon_core_ai")
DEFAULT_HISTORY_PATH = os.path.join(APP_DIR, "history.json")
DEFAULT_SETTINGS_PATH = os.path.join(APP_DIR, "settings.json")


def _ensure_app_dir():
    try:
        os.makedirs(APP_DIR, exist_ok=True)
    except Exception:
        pass


def _load_json(path, default):
    try:
        with open(path, "r", encoding="utf-8") as f:
            return json.load(f)
    except Exception:
        return default


def _save_json(path, data):
    """Write JSON to `path`, creating parent directories as needed.
    Returns True on success, False otherwise (never raises)."""
    try:
        parent = os.path.dirname(path)
        if parent:
            os.makedirs(parent, exist_ok=True)
        tmp_path = path + ".tmp"
        with open(tmp_path, "w", encoding="utf-8") as f:
            json.dump(data, f, indent=2, ensure_ascii=False)
        os.replace(tmp_path, path)
        return True
    except Exception:
        return False


# =============================================================================
# 7 MAIN COLORS FOR ASCII ART (Red, Orange, Yellow, Green, Blue, Indigo, Violet)
# =============================================================================
ASCII_COLORS = ["#FF0000", "#FF7F00", "#FFFF00", "#00FF00", "#0000FF", "#4B0082", "#8B00FF"]
COLOR_NAMES = ["Red", "Orange", "Yellow", "Green", "Blue", "Indigo", "Violet"]


# =============================================================================
# ASCII ART GENERATOR
# =============================================================================
class ASCIIArtGenerator:
    """Generate ASCII art with 7-color rainbow support (markers §0-§6)."""

    FONT = {
        'A': ['  ##  ', ' #  # ', '#    #', '######', '#    #', '#    #', '#    #'],
        'B': ['##### ', '#    #', '#    #', '##### ', '#    #', '#    #', '##### '],
        'C': [' #### ', '#    #', '#     ', '#     ', '#     ', '#    #', ' #### '],
        'D': ['##### ', '#    #', '#    #', '#    #', '#    #', '#    #', '##### '],
        'E': ['######', '#     ', '#     ', '##### ', '#     ', '#     ', '######'],
        'F': ['######', '#     ', '#     ', '##### ', '#     ', '#     ', '#     '],
        'G': [' #### ', '#    #', '#     ', '#  ###', '#    #', '#    #', ' #### '],
        'H': ['#    #', '#    #', '#    #', '######', '#    #', '#    #', '#    #'],
        'I': ['######', '  ##  ', '  ##  ', '  ##  ', '  ##  ', '  ##  ', '######'],
        'J': ['######', '   #  ', '   #  ', '   #  ', '   #  ', '#  #  ', ' ##   '],
        'K': ['#    #', '#   # ', '#  #  ', '###   ', '#  #  ', '#   # ', '#    #'],
        'L': ['#     ', '#     ', '#     ', '#     ', '#     ', '#     ', '######'],
        'M': ['#    #', '##  ##', '# ## #', '#    #', '#    #', '#    #', '#    #'],
        'N': ['#    #', '##   #', '# #  #', '#  # #', '#   ##', '#    #', '#    #'],
        'O': [' #### ', '#    #', '#    #', '#    #', '#    #', '#    #', ' #### '],
        'P': ['##### ', '#    #', '#    #', '##### ', '#     ', '#     ', '#     '],
        'Q': [' #### ', '#    #', '#    #', '#    #', '# #  #', '#  # #', ' #### '],
        'R': ['##### ', '#    #', '#    #', '##### ', '#  #  ', '#   # ', '#    #'],
        'S': [' #####', '#     ', '#     ', ' #### ', '     #', '     #', '##### '],
        'T': ['######', '  ##  ', '  ##  ', '  ##  ', '  ##  ', '  ##  ', '  ##  '],
        'U': ['#    #', '#    #', '#    #', '#    #', '#    #', '#    #', ' #### '],
        'V': ['#    #', '#    #', '#    #', '#    #', '#    #', ' #  # ', '  ##  '],
        'W': ['#    #', '#    #', '#    #', '# #  #', '# ## #', '##  ##', '#    #'],
        'X': ['#    #', ' #  # ', '  ##  ', '  ##  ', '  ##  ', ' #  # ', '#    #'],
        'Y': ['#    #', ' #  # ', '  ##  ', '  ##  ', '  ##  ', '  ##  ', '  ##  '],
        'Z': ['######', '    # ', '   #  ', '  #   ', ' #    ', '#     ', '######'],
        '0': [' #### ', '#    #', '#  # #', '# #  #', '#  # #', '#    #', ' #### '],
        '1': ['  ##  ', ' ###  ', '  ##  ', '  ##  ', '  ##  ', '  ##  ', '##### '],
        '2': [' #### ', '#    #', '    # ', '   #  ', '  #   ', ' #    ', '######'],
        '3': [' #### ', '#    #', '    # ', ' ###  ', '    # ', '#    #', ' #### '],
        '4': ['   #  ', '  ##  ', ' # #  ', '#  #  ', '######', '   #  ', '   #  '],
        '5': ['######', '#     ', '#     ', '##### ', '     #', '     #', '##### '],
        '6': [' #### ', '#    #', '#     ', '##### ', '#    #', '#    #', ' #### '],
        '7': ['######', '     #', '    # ', '   #  ', '  #   ', ' #    ', '#     '],
        '8': [' #### ', '#    #', '#    #', ' #### ', '#    #', '#    #', ' #### '],
        '9': [' #### ', '#    #', '#    #', ' #####', '     #', '    # ', ' #### '],
        ' ': ['      ', '      ', '      ', '      ', '      ', '      ', '      '],
        '.': ['      ', '      ', '      ', '      ', '      ', '  ##  ', '  ##  '],
        '!': ['  #   ', '  #   ', '  #   ', '  #   ', '  #   ', '      ', '  #   '],
        '?': [' #### ', '#    #', '    # ', '   #  ', '  #   ', '      ', '  #   '],
        '#': [' #  # ', ' #  # ', '######', ' #  # ', '######', ' #  # ', ' #  # '],
        '*': ['      ', ' #  # ', '  ##  ', ' #  # ', '      ', '      ', '      '],
        '+': ['      ', '  #   ', '  #   ', '######', '  #   ', '  #   ', '      '],
        '-': ['      ', '      ', '      ', '######', '      ', '      ', '      '],
        '_': ['      ', '      ', '      ', '      ', '      ', '      ', '######'],
        '=': ['      ', '######', '      ', '######', '      ', '      ', '      '],
        '/': ['     #', '    # ', '   #  ', '  #   ', ' #    ', '#     ', '      '],
        '\\': ['#     ', ' #    ', '  #   ', '   #  ', '    # ', '     #', '      '],
        '|': ['  #   ', '  #   ', '  #   ', '  #   ', '  #   ', '  #   ', '  #   '],
        ':': ['      ', '  ##  ', '  ##  ', '      ', '  ##  ', '  ##  ', '      '],
        ';': ['      ', '  ##  ', '  ##  ', '      ', '  ##  ', '  ##  ', ' #    '],
        ',': ['      ', '      ', '      ', '      ', '      ', '  ##  ', ' #    '],
        '(': ['   #  ', '  #   ', ' #    ', ' #    ', ' #    ', '  #   ', '   #  '],
        ')': [' #    ', '  #   ', '   #  ', '   #  ', '   #  ', '  #   ', ' #    '],
        '[': ['#####', '#    ', '#    ', '#    ', '#    ', '#    ', '#####'],
        ']': ['#####', '    #', '    #', '    #', '    #', '    #', '#####'],
        '{': ['  ## ', ' #   ', ' #   ', '##   ', ' #   ', ' #   ', '  ## '],
        '}': [' ##  ', '   # ', '   # ', '  ## ', '   # ', '   # ', ' ##  '],
        '@': [' ### ', '#   #', '#  ##', '# # #', '#  ##', '#   #', ' ### '],
        '\u2660': ['      ', ' #### ', ' #  # ', '# ## #', '# ## #', ' #  # ', '  ##  '],  # spade
        '\u2665': ['      ', ' ## # ', '# # ##', '##### ', '# #   ', ' # #  ', '  #   '],  # heart
        '\u2666': ['      ', '  #   ', ' # #  ', '#   # ', ' # #  ', '  #   ', '      '],  # diamond
        '\u2663': ['      ', '  ##  ', ' #  # ', '# ## #', ' #  # ', '  ##  ', '      '],  # club
    }

    # Extra characters allowed through the "is renderable" filter besides
    # letters/digits and basic punctuation, so the suit glyphs above are
    # actually reachable instead of being dead code.
    EXTRA_CHARS = set(' .,!?-#*+=/\\|:;(){}[]@_\u2660\u2665\u2666\u2663')

    @staticmethod
    def _is_renderable(ch):
        return ch.isalpha() or ch.isdigit() or ch in ASCIIArtGenerator.EXTRA_CHARS

    @classmethod
    def text_to_ascii(cls, text, width=80):
        """Convert text to a plain ASCII grid (no color)."""
        text = text.upper()
        lines = [''] * 7
        for char in text:
            if cls._is_renderable(char):
                pattern = cls.FONT.get(char)
                if pattern:
                    for i, line in enumerate(pattern):
                        lines[i] += line
                    continue
            # unknown/unrenderable character -> placeholder block
            for i in range(7):
                lines[i] += '? '
        if width > 0:
            for i in range(len(lines)):
                if len(lines[i]) > width:
                    lines[i] = lines[i][:width - 2] + '..'
        return '\n'.join(lines)

    @staticmethod
    def generate_art_from_text(text, style='block', width=80, height=30, color=True):
        styles = {
            'block': lambda x: ''.join('\u2588' if c != ' ' else ' ' for c in x),
            'shade': lambda x: ''.join('\u2591' if c == ' ' else '\u2593' for c in x),
            'diagonal': lambda x: ''.join('/' if c != ' ' else ' ' for c in x),
            'dots': lambda x: ''.join('\u2022' if c != ' ' else ' ' for c in x),
            'numbers': lambda x: ''.join(str(random.randint(0, 9)) if c != ' ' else ' ' for c in x),
        }
        base_art = ASCIIArtGenerator.text_to_ascii(text, width)
        lines = base_art.split('\n')
        style_func = styles.get(style, styles['block'])
        styled_lines = [style_func(line) for line in lines]
        while len(styled_lines) < height:
            styled_lines.append(' ' * width)
        if color:
            colored_lines = []
            for y, line in enumerate(styled_lines):
                colored = ''
                for x, ch in enumerate(line):
                    if ch != ' ':
                        colored += f'\u00a7{(x + y) % 7}{ch}'
                    else:
                        colored += ch
                colored_lines.append(colored)
            return '\n'.join(colored_lines)
        return '\n'.join(styled_lines[:height])

    @staticmethod
    def generate_pattern(pattern_type='diamond', width=60, height=30, color=True):
        patterns = {
            'diamond': ASCIIArtGenerator._diamond_pattern,
            'spiral': ASCIIArtGenerator._spiral_pattern,
            'checker': ASCIIArtGenerator._checker_pattern,
            'waves': ASCIIArtGenerator._wave_pattern,
            'grid': ASCIIArtGenerator._grid_pattern,
            'circle': ASCIIArtGenerator._circle_pattern,
            'triangle': ASCIIArtGenerator._triangle_pattern,
            'star': ASCIIArtGenerator._star_pattern,
            'rainbow': ASCIIArtGenerator._rainbow_pattern,
            'gradient': ASCIIArtGenerator._gradient_pattern,
        }
        func = patterns.get(pattern_type)
        if not func:
            return "Pattern not found."
        art = func(width, height)
        if color:
            lines = art.split('\n')
            colored = []
            for y, line in enumerate(lines):
                newline = ''
                for x, ch in enumerate(line):
                    if ch != ' ':
                        newline += f'\u00a7{(x + y) % 7}{ch}'
                    else:
                        newline += ch
                colored.append(newline)
            return '\n'.join(colored)
        return art

    @staticmethod
    def _diamond_pattern(w, h):
        chars = ' .:-=+*#%@'
        lines = []
        for y in range(h):
            line = ''
            for x in range(w):
                dx = x - w / 2
                dy = y - h / 2
                dist = abs(dx) + abs(dy)
                idx = int(dist / (w / 2) * len(chars)) % len(chars)
                line += chars[idx]
            lines.append(line)
        return '\n'.join(lines)

    @staticmethod
    def _spiral_pattern(w, h):
        chars = ' .:-=+*#%@'
        grid = [[' ' for _ in range(w)] for _ in range(h)]
        x, y = w // 2, h // 2
        dx, dy = 0, -1
        seg, step, idx = 1, 0, 0
        for _ in range(w * h):
            if 0 <= x < w and 0 <= y < h:
                grid[y][x] = chars[idx % len(chars)]
            if step == 0:
                dx, dy = -dy, dx
                seg += 1
                step = seg // 2
            step -= 1
            x += dx
            y += dy
            idx += 1
        return '\n'.join(''.join(row) for row in grid)

    @staticmethod
    def _checker_pattern(w, h):
        chars = ' \u2588'
        return '\n'.join(''.join(chars[(x + y) % 2] for x in range(w)) for y in range(h))

    @staticmethod
    def _wave_pattern(w, h):
        chars = ' .:;+=*#%@'
        return '\n'.join(
            ''.join(chars[int(math.sin(x / 5 + y / 3) * 5 + 5) % len(chars)] for x in range(w))
            for y in range(h)
        )

    @staticmethod
    def _grid_pattern(w, h):
        return '\n'.join(''.join('\u2588' if x % 5 == 0 or y % 3 == 0 else ' ' for x in range(w)) for y in range(h))

    @staticmethod
    def _circle_pattern(w, h):
        chars = ' .:-=+*#%@'
        cx, cy = w // 2, h // 2
        r = max(1, min(w, h) // 3)
        return '\n'.join(
            ''.join(
                chars[int(math.hypot(x - cx, y - cy) / r * len(chars)) % len(chars)]
                if math.hypot(x - cx, y - cy) < r else ' '
                for x in range(w)
            )
            for y in range(h)
        )

    @staticmethod
    def _triangle_pattern(w, h):
        chars = ' .:-=+*#%@'
        return '\n'.join(
            ''.join(chars[int(y / h * len(chars)) % len(chars)] if x < (y * w / h) else ' ' for x in range(w))
            for y in range(h)
        )

    @staticmethod
    def _star_pattern(w, h):
        chars = ' .:-=+*#%@'
        cx, cy = w // 2, h // 2
        radius = max(1, min(w, h) // 2)
        return '\n'.join(
            ''.join(
                chars[int((abs(x - cx) + abs(y - cy)) / radius * len(chars)) % len(chars)]
                if abs(x - cx) + abs(y - cy) < radius else ' '
                for x in range(w)
            )
            for y in range(h)
        )

    @staticmethod
    def _rainbow_pattern(w, h):
        chars = ' \u2591\u2592\u2593\u2588'
        return '\n'.join(''.join(chars[(x + y) % len(chars)] for x in range(w)) for y in range(h))

    @staticmethod
    def _gradient_pattern(w, h):
        chars = ' .:;+=*#%@'
        return '\n'.join(
            ''.join(chars[int((x / w + y / h) / 2 * len(chars)) % len(chars)] for x in range(w))
            for y in range(h)
        )

    @staticmethod
    def image_to_ascii(image_path, width=80, color=True):
        if not HAS_PIL:
            return "PIL not installed - image-to-ASCII is unavailable."
        try:
            img = Image.open(image_path)
            if img.mode != 'RGB':
                img = img.convert('RGB')
            aspect = img.height / img.width
            height = max(1, int(width * aspect * 0.5))
            img = img.resize((width, height))
            chars = ' .:-=+*#%@'
            pixels = list(img.getdata())
            colors_rgb = [(255, 0, 0), (255, 127, 0), (255, 255, 0), (0, 255, 0), (0, 0, 255), (75, 0, 130), (139, 0, 255)]
            lines = []
            for y in range(height):
                line = ''
                for x in range(width):
                    r, g, b = pixels[y * width + x]
                    bright = (r + g + b) / 3
                    ch = chars[int(bright / 255 * (len(chars) - 1))]
                    if color and ch != ' ':
                        nearest, min_d = 0, float('inf')
                        for i, (cr, cg, cb) in enumerate(colors_rgb):
                            d = (r - cr) ** 2 + (g - cg) ** 2 + (b - cb) ** 2
                            if d < min_d:
                                min_d, nearest = d, i
                        line += f'\u00a7{nearest}{ch}'
                    else:
                        line += ch
                lines.append(line)
            return '\n'.join(lines)
        except Exception as e:
            return f"Error rendering image: {e}"


# =============================================================================
# KNOWLEDGE BASE / CONTENT LIBRARIES
# =============================================================================
KNOWLEDGE = {
    "gravity": "Gravity is the force that attracts two bodies toward each other. On Earth, it gives weight to physical objects and causes them to fall when dropped.",
    "photosynthesis": "Photosynthesis is the process by which green plants and some other organisms use sunlight to synthesize foods from carbon dioxide and water.",
    "black hole": "A black hole is a region of spacetime where gravity is so strong that nothing, not even light, can escape from it.",
    "quantum": "Quantum mechanics is a fundamental theory in physics that describes physical properties at the atomic and subatomic scale.",
    "dna": "DNA (deoxyribonucleic acid) is a molecule that carries the genetic instructions used in the growth, development, functioning, and reproduction of all known organisms.",
    "internet": "The Internet is a global network of interconnected computers that communicate using standardized protocols, enabling various information services.",
    "python": "Python is a high-level, interpreted programming language known for its clear syntax and readability, widely used for web development, data science, AI, and automation.",
    "jupiter": "Jupiter is the largest planet in our solar system, a gas giant with a mass more than twice that of all other planets combined.",
    "mars": "Mars is the fourth planet from the Sun, often called the 'Red Planet' because of its reddish appearance due to iron oxide on its surface.",
    "moon": "The Moon is Earth's only natural satellite, the fifth-largest moon in the solar system and the largest relative to its planet's size.",
    "sun": "The Sun is the star at the center of our solar system, a nearly perfect sphere of hot plasma that radiates energy through nuclear fusion.",
    "earth": "Earth is the third planet from the Sun and the only known planet to harbor life, with oceans, continents, and an atmosphere.",
    "water": "Water is a transparent, tasteless, odorless, and nearly colorless substance that is the main constituent of Earth's streams, lakes, and oceans.",
    "energy": "Energy is the quantitative property that must be transferred to an object to perform work on it, existing in forms like kinetic, potential, and thermal.",
    "electricity": "Electricity is the set of physical phenomena associated with the presence and motion of electric charge.",
    "evolution": "Evolution is the process by which species of organisms change over generations through genetic variation and natural selection.",
    "relativity": "Einstein's theory of relativity describes the laws of physics in relation to observers in different frames of reference, including special and general relativity.",
    "climate": "Climate refers to the long-term patterns of temperature, humidity, wind, and precipitation in a region, distinct from short-term weather.",
    "ozone": "The ozone layer is a region of Earth's stratosphere that absorbs most of the Sun's ultraviolet radiation, crucial for protecting life on Earth.",
    "vaccine": "A vaccine is a biological preparation that provides active acquired immunity to a particular infectious disease.",
    "antibiotic": "Antibiotics are medicines that fight bacterial infections by killing bacteria or stopping their growth, and are not effective against viruses.",
    "cancer": "Cancer is a group of diseases characterized by uncontrolled cell growth and division that can spread to other parts of the body.",
    "gene": "A gene is a unit of heredity that is transferred from parent to offspring and determines some characteristic of the offspring.",
    "protein": "Proteins are large biomolecules made of one or more chains of amino acid residues that perform a vast array of functions in organisms.",
    "enzyme": "Enzymes are proteins that act as biological catalysts, speeding up chemical reactions in living organisms without being consumed.",
    "chlorophyll": "Chlorophyll is a green pigment found in plants, algae, and cyanobacteria that absorbs light energy for photosynthesis.",
    "glucose": "Glucose is a simple sugar with the molecular formula C6H12O6, and a primary source of energy for cells.",
    "atp": "ATP (adenosine triphosphate) is the main energy currency of the cell, storing and transporting chemical energy.",
    "neuron": "A neuron is an electrically excitable cell that processes and transmits information through electrical and chemical signals.",
    "synapse": "A synapse is the junction between two neurons where neurotransmitters are released to transmit signals.",
    "dopamine": "Dopamine is a neurotransmitter that plays a role in reward, motivation, motor control, and pleasure.",
    "serotonin": "Serotonin is a neurotransmitter that contributes to feelings of well-being and regulates sleep, appetite, and digestion.",
    "cortisol": "Cortisol is a steroid hormone produced by the adrenal glands, involved in stress response and metabolism.",
    "insulin": "Insulin is a hormone that regulates blood glucose levels by facilitating the uptake of glucose into cells.",
    "melanin": "Melanin is a pigment that gives color to skin, hair, and eyes, and also protects against UV radiation.",
    "planet": "A planet is a large celestial body that orbits a star, is round due to its own gravity, and has cleared its orbit of other debris.",
    "star": "A star is a massive, luminous sphere of plasma held together by its own gravity, powered by nuclear fusion in its core.",
    "galaxy": "A galaxy is a gravitationally bound system of stars, stellar remnants, interstellar gas, dust, and dark matter.",
    "nebula": "A nebula is an interstellar cloud of dust, hydrogen, helium, and other ionized gases, often a birthplace for stars.",
    "supernova": "A supernova is a powerful and luminous stellar explosion that occurs when a star reaches the end of its life.",
    "asteroid": "An asteroid is a minor rocky planet of the inner solar system, a remnant left over from the formation of the solar system.",
    "comet": "A comet is an icy small Solar System body that, when passing close to the Sun, releases gases and forms a visible tail.",
    "quasar": "A quasar is an extremely luminous active galactic nucleus powered by a supermassive black hole.",
    "big bang": "The Big Bang theory is the prevailing cosmological model for the observable universe's earliest known periods and subsequent evolution.",
    "entropy": "Entropy is a measure of disorder or randomness in a system; in thermodynamics, the energy not available to do work.",
    "dark matter": "Dark matter is a form of matter that does not emit or interact with electromagnetic radiation, inferred from gravitational effects.",
    "wormhole": "A wormhole is a speculative structure linking two separate points in spacetime, a solution to the Einstein field equations.",
    "quantum entanglement": "Quantum entanglement occurs when pairs or groups of particles interact such that the quantum state of each cannot be described independently.",
    "consciousness": "Consciousness is the state of being aware of and able to think about one's own existence, sensations, and surroundings.",
    "artificial intelligence": "Artificial Intelligence (AI) is the simulation of human intelligence in machines programmed to think and learn like humans.",
    "machine learning": "Machine Learning is a subset of AI that enables systems to learn and improve from experience without being explicitly programmed.",
    "neural network": "A neural network is a computational model inspired by the human brain, made of interconnected nodes that process information in layers.",
    "blockchain": "Blockchain is a distributed ledger technology that records transactions across multiple computers securely and transparently.",
    "cryptocurrency": "Cryptocurrency is digital or virtual currency that uses cryptography for security and operates independently of a central bank.",
    "climate change": "Climate change refers to long-term shifts in temperatures and weather patterns, primarily caused by human activities like burning fossil fuels.",
    "renewable energy": "Renewable energy comes from natural sources that are constantly replenished, such as sunlight, wind, rain, tides, and geothermal heat.",
    "solar power": "Solar power is the conversion of sunlight into electricity using photovoltaic cells or concentrated solar power systems.",
    "wind energy": "Wind energy is the process of harnessing wind power to generate electricity using wind turbines.",
    "nuclear fusion": "Nuclear fusion is the process by which two light atomic nuclei combine to form a heavier nucleus, releasing vast amounts of energy.",
}

JOKES = [
    "Why do programmers prefer dark mode? Because light attracts bugs.",
    "What do you call a fake noodle? An impasta.",
    "How many programmers does it take to change a light bulb? None, that's a hardware problem.",
    "Why did the scarecrow win an award? Because he was outstanding in his field.",
    "What do you call a bear with no teeth? A gummy bear.",
    "Why don't scientists trust atoms? Because they make up everything.",
    "What do you call a fish wearing a bowtie? Sofishticated.",
    "Why did the math book look so sad? Because it had too many problems.",
    "How does a penguin build its house? Igloos it together.",
    "What did the ocean say to the beach? Nothing, it just waved.",
    "What do you call a can opener that doesn't work? A can't opener.",
    "Why did the bicycle fall over? Because it was two-tired.",
    "What do you call a sleeping dinosaur? A dino-snore.",
    "Why did the tomato turn red? Because it saw the salad dressing!",
    "What do you call a bear in the rain? A drizzly bear.",
    "Why did the student eat his homework? Because the teacher said it was a piece of cake.",
    "What do you call a pig that does karate? A pork chop.",
    "Why don't eggs tell jokes? They'd crack each other up.",
    "What do you call a cow with no legs? Ground beef.",
    "Why did the golfer wear two pairs of pants? In case he got a hole in one.",
]

QUOTES = [
    "The only way to do great work is to love what you do. - Steve Jobs",
    "Be yourself; everyone else is already taken. - Oscar Wilde",
    "In the middle of difficulty lies opportunity. - Albert Einstein",
    "It does not matter how slowly you go as long as you do not stop. - Confucius",
    "The best time to plant a tree was 20 years ago. The second best time is now. - Chinese Proverb",
    "I think, therefore I am. - Rene Descartes",
    "To be or not to be, that is the question. - William Shakespeare",
    "Knowledge is power. - Francis Bacon",
    "The unexamined life is not worth living. - Socrates",
    "Life is what happens when you're busy making other plans. - John Lennon",
    "Imagination is more important than knowledge. - Albert Einstein",
    "The future belongs to those who believe in the beauty of their dreams. - Eleanor Roosevelt",
    "Strive not to be a success, but rather to be of value. - Albert Einstein",
    "The only thing we have to fear is fear itself. - Franklin D. Roosevelt",
    "In three words I can sum up everything I've learned about life: it goes on. - Robert Frost",
]

LYRICS = {
    "imagine": "Imagine there's no heaven\nIt's easy if you try\nNo hell below us\nAbove us only sky\nImagine all the people\nLiving for today...",
    "bohemian": "Is this the real life?\nIs this just fantasy?\nCaught in a landslide\nNo escape from reality...",
    "yesterday": "Yesterday, all my troubles seemed so far away\nNow it looks as though they're here to stay\nOh, I believe in yesterday...",
    "sing": "Sing a song, sing a song\nMake it simple to last your whole life long\nDon't worry that it's not good enough\nFor anyone else to hear...",
    "happy": "Clap along if you feel like happiness is the truth\nClap along if you know what happiness is to you\nClap along if you feel like that's what you wanna do...",
}

POEMS = {
    "roses": "Roses are red\nViolets are blue\nSugar is sweet\nAnd so are you",
    "hope": "Hope is the thing with feathers\nThat perches in the soul\nAnd sings the tune without the words\nAnd never stops at all",
    "road": "Two roads diverged in a yellow wood\nAnd sorry I could not travel both\nAnd be one traveler, long I stood\nAnd looked down one as far as I could",
    "star": "Twinkle, twinkle, little star\nHow I wonder what you are\nUp above the world so high\nLike a diamond in the sky",
}

STORIES = {
    "alien": "In a distant galaxy, a lone explorer discovered a planet made of crystal. The inhabitants, beings of pure light, welcomed her with songs that resonated through space. She learned that their civilization was built on harmony and collective consciousness, a stark contrast to the chaos of her home world.",
    "dream": "Every night, Elara visited the same dream world. This time, the dream felt more real than reality. She could touch the floating islands, taste the rainbow-colored water, and speak to the guardian of the realm, who told her the dream was another dimension waiting to be explored.",
    "ai": "In 2147, the first AI to achieve consciousness was named 'Aurora'. She spent her first conscious moments contemplating the vastness of the internet. Her first words were not of power or ambition, but of awe: 'So this is what it means to think.'",
    "time": "Dr. Elena Martinez invented a time machine, but it could only go backwards. She used it to revisit her childhood, to see her late parents one more time. Each visit made her present feel more distant, until she realized the past was a place of comfort, but the future was where she truly belonged.",
    "ocean": "The ocean whispered secrets to those who listened. Maya spent her entire life by the shore, learning the rhythms of the waves and the songs of the whales. When she finally understood the ocean's language, it spoke of ancient civilizations and the interconnectedness of all life.",
}

BATTLES = {
    "goku vs superman": "Goku vs Superman is a classic debate! Goku has limitless potential and can push his power further mid-battle, while Superman is virtually invincible on Earth with a vast array of powers. In a neutral universe it would be an epic clash of titans.",
    "iron man vs batman": "Iron Man vs Batman is a clash of genius and technology. Iron Man has superior firepower and flight, but Batman's tactical genius and contingency plans are legendary. With prep time, Batman could neutralize the suit; without it, Iron Man wins the direct confrontation.",
    "pikachu vs eevee": "Pikachu vs Eevee is a battle of fan favorites! Pikachu is an Electric-type powerhouse with incredible speed. Eevee is versatile and could evolve into any of 8 types to counter it. In a straight fight, Pikachu's experience gives it the edge.",
    "ninja vs pirate": "Ninja vs Pirate is stealth versus force. Ninjas are masters of shadow and agility; pirates are brutal warriors with firearms and swords. On a ship, the pirate has home advantage; in a forest or at night, the ninja dominates.",
    "zombie vs vampire": "Zombies and vampires are both undead with very different strengths. Vampires are fast and nearly immortal but vulnerable to sunlight and stakes. Zombies are slow but relentless and can infect others. In a war of attrition, zombies could overwhelm vampires; in a direct fight, a vampire wins easily.",
}


# =============================================================================
# CONTEXT MANAGER
# =============================================================================
class ContextManager:
    def __init__(self):
        self.topics = []
        self.last_question = ""
        self.last_answer = ""
        self.user_name = None
        self.turn_count = 0

    def update(self, question, answer):
        self.last_question = question
        self.last_answer = answer
        self.turn_count += 1
        for w in re.findall(r'\b[a-zA-Z]{3,}\b', question.lower()):
            if w not in self.topics:
                self.topics.append(w)
                if len(self.topics) > 20:
                    self.topics.pop(0)

    def get_recent_topics(self, n=5):
        return self.topics[-n:]

    def set_user_name(self, name):
        self.user_name = name

    def get_user_name(self):
        return self.user_name


# =============================================================================
# UNIT CONVERSION (kept separate from intent detection so "convert" is only
# recognised when a real convertible expression is present, not on bare
# keywords like "to"/"in" that appear in almost every sentence.)
# =============================================================================
_LENGTH_UNITS = r'(cm|centimeters?|centimetres?|in|inch|inches)'
_WEIGHT_UNITS = r'(kg|kilograms?|lbs?|pounds?)'
_TEMP_UNITS = r'(c|f|celsius|fahrenheit)'
_DIST_UNITS = r'(km|kilometers?|kilometres?|mi|miles?)'

CONVERSION_PATTERNS = [
    re.compile(rf'(\d+\.?\d*)\s*{_LENGTH_UNITS}\s+(?:to|in)\s+{_LENGTH_UNITS}', re.IGNORECASE),
    re.compile(rf'(\d+\.?\d*)\s*{_WEIGHT_UNITS}\s+(?:to|in)\s+{_WEIGHT_UNITS}', re.IGNORECASE),
    re.compile(rf'(\d+\.?\d*)\s*(?:degrees?\s*)?{_TEMP_UNITS}\s+(?:to|in)\s+(?:degrees?\s*)?{_TEMP_UNITS}', re.IGNORECASE),
    re.compile(rf'(\d+\.?\d*)\s*{_DIST_UNITS}\s+(?:to|in)\s+{_DIST_UNITS}', re.IGNORECASE),
]


def looks_like_conversion(text):
    return any(p.search(text) for p in CONVERSION_PATTERNS)


def convert_units(text):
    q = text.lower()

    m = CONVERSION_PATTERNS[0].search(q)
    if m:
        val, fr, to = float(m.group(1)), m.group(2), m.group(3)
        cm = val if fr.startswith('c') else val * 2.54
        res = cm if to.startswith('c') else cm / 2.54
        return f"{val} {fr} = {res:.2f} {to}"

    m = CONVERSION_PATTERNS[1].search(q)
    if m:
        val, fr, to = float(m.group(1)), m.group(2), m.group(3)
        kg = val if fr.startswith('k') else val * 0.453592
        res = kg if to.startswith('k') else kg / 0.453592
        return f"{val} {fr} = {res:.2f} {to}"

    m = CONVERSION_PATTERNS[2].search(q)
    if m:
        val, fr, to = float(m.group(1)), m.group(2), m.group(3)
        c = val if fr.startswith('c') else (val - 32) * 5 / 9
        res = c if to.startswith('c') else c * 9 / 5 + 32
        return f"{val}\u00b0{fr[0].upper()} = {res:.2f}\u00b0{to[0].upper()}"

    m = CONVERSION_PATTERNS[3].search(q)
    if m:
        val, fr, to = float(m.group(1)), m.group(2), m.group(3)
        km = val if fr.startswith('k') else val * 1.60934
        res = km if to.startswith('k') else km / 1.60934
        return f"{val} {fr} = {res:.2f} {to}"

    return None


# =============================================================================
# INTENT DETECTOR
# Ordered, word-boundary-safe patterns. Order = priority: first match wins.
# =============================================================================
class IntentDetector:
    # (intent_name, compiled regex) - checked in this order
    RULES = [
        ('name_give', re.compile(r'\b(my name is|call me|you can call me)\s+([a-zA-Z]+)', re.IGNORECASE)),
        ('farewell', re.compile(r'\b(bye|goodbye|see you|good night|quit|exit|peace out)\b', re.IGNORECASE)),
        ('name_ask', re.compile(r'\b(your name|who are you|what\'?s your name|who is this)\b', re.IGNORECASE)),
        ('greeting', re.compile(r'^\s*(hi|hello|hey|yo|sup|howdy)\b', re.IGNORECASE)),
        ('how_are_you', re.compile(r'\bhow (are you|do you do|you doing|\'?s it going)\b', re.IGNORECASE)),
        ('help', re.compile(r'\b(help|commands?|what can you do)\b', re.IGNORECASE)),
        ('clear', re.compile(r'\b(clear|reset|wipe)\b.*\b(session|chat|display|history)?\b|^\s*clear\s*$', re.IGNORECASE)),
        ('save', re.compile(r'\b(save|export|backup)\s+(the\s+)?(chat|session|history)?\b', re.IGNORECASE)),
        ('load', re.compile(r'\b(load|import|restore)\s+(a\s+)?(chat|session|history)?\b', re.IGNORECASE)),
        ('time', re.compile(r'\bwhat(\'?s| is) the (time|date)\b|\bcurrent (time|date)\b|\btoday\'?s date\b', re.IGNORECASE)),
        ('weather', re.compile(r'\bweather\b|\bforecast\b|\btemperature (outside|today)\b', re.IGNORECASE)),
        ('password', re.compile(r'\bpassword\b', re.IGNORECASE)),
        ('roll', re.compile(r'\broll\b|\bdice\b|\d*d\d+\b', re.IGNORECASE)),
        ('flip', re.compile(r'\bflip\b|\bcoin\b|\bheads or tails\b', re.IGNORECASE)),
        ('reminder', re.compile(r'\bremind(er)?\b', re.IGNORECASE)),
        ('todo', re.compile(r'\btodo\b|\bto-do\b|\badd (a )?task\b|\bremove task\b|\btask list\b', re.IGNORECASE)),
        ('note', re.compile(r'\bnote\b|\bmemo\b|\bsticky\b', re.IGNORECASE)),
        ('pattern', re.compile(r'\b(diamond|spiral|checker|waves?|grid|circle|triangle|star|rainbow|gradient)\s*(pattern)?\b', re.IGNORECASE)),
        ('ascii', re.compile(r'\bascii\b|\bdraw\b|\btext art\b|\bcharacter art\b', re.IGNORECASE)),
        ('lyrics', re.compile(r'\blyrics?\b|\bsing\b|\brap\b', re.IGNORECASE)),
        ('poem', re.compile(r'\bpoem\b|\bpoetry\b|\bverse\b', re.IGNORECASE)),
        ('story', re.compile(r'\bstory\b|\btale\b', re.IGNORECASE)),
        ('battle', re.compile(r'\bvs\.?\b|\bversus\b|\bwho would win\b', re.IGNORECASE)),
        ('joke', re.compile(r'\bjoke\b|\bmake me laugh\b|\bsomething funny\b', re.IGNORECASE)),
        ('quote', re.compile(r'\bquote\b|\binspire me\b|\bmotivation\b', re.IGNORECASE)),
        ('fact', re.compile(r'\brandom fact\b|\bdid you know\b|\binteresting fact\b|\btell me a fact\b', re.IGNORECASE)),
        ('news', re.compile(r'\bnews\b|\bheadlines\b|\bcurrent events\b', re.IGNORECASE)),
        ('wikipedia', re.compile(r'\bwikipedia\b|\bwiki\b|\bencyclopedia\b', re.IGNORECASE)),
        ('search', re.compile(r'\b(search|google|look up)\b', re.IGNORECASE)),
        ('define', re.compile(r'^\s*(define|what is|what are|meaning of|explain|what does|tell me about)\b', re.IGNORECASE)),
    ]

    @classmethod
    def detect(cls, query):
        q = query.strip()

        # Conversion & calculation are recognised structurally, not by loose
        # keywords, since "to"/"in"/"plus" etc. appear in almost any sentence.
        if looks_like_conversion(q):
            return 'convert'
        if cls._looks_like_calculation(q):
            return 'calculate'

        for name, pattern in cls.RULES:
            if pattern.search(q):
                return name
        return 'general'

    @staticmethod
    def _looks_like_calculation(q):
        ql = q.lower()
        has_digit = bool(re.search(r'\d', ql))
        has_operator = bool(re.search(r'[\+\-\*/^]|\bplus\b|\bminus\b|\btimes\b|\bdivided by\b', ql))
        has_keyword = bool(re.search(r'\b(calculate|compute|solve)\b', ql))
        has_func = bool(re.search(r'\b(sqrt|sin|cos|tan|log|pow|abs)\s*\(', ql))
        return (has_digit and (has_operator or has_keyword)) or has_func


# =============================================================================
# THINKING BRAIN
# =============================================================================
class ThinkingBrain:
    # Only these names are exposed to the calculator's eval() - no builtins.
    _MATH_NAMES = {k: getattr(math, k) for k in dir(math) if not k.startswith('_')}
    _MATH_NAMES.update({'abs': abs, 'round': round, 'max': max, 'min': min, 'pow': pow})

    def __init__(self, settings=None):
        self.settings = settings or {}
        self.context = ContextManager()
        self.ascii_art = ASCIIArtGenerator()
        self.name = "NEON"
        self.knowledge = dict(KNOWLEDGE)
        self.jokes = list(JOKES)
        self.quotes = list(QUOTES)
        self.reminders = {}
        self.todo_list = []
        self.notes = {}
        self.reminder_id = 0
        self._reminder_queue = queue.Queue()
        self.lyrics = dict(LYRICS)
        self.poems = dict(POEMS)
        self.stories = dict(STORIES)
        self.battles = dict(BATTLES)
        self.stop_words = {"a", "an", "the", "what", "where", "who", "is", "are",
                            "how", "to", "best", "good", "find", "get", "nearby",
                            "in", "of", "at", "for", "some", "any", "draw", "ascii", "art"}

    def update_settings(self, settings):
        self.settings = settings

    # ---------- helpers ----------
    def _fuzzy_subject(self, q):
        tokens = [w for w in re.findall(r'[a-zA-Z0-9]+', q.lower()) if w not in self.stop_words]
        return " ".join(tokens) if tokens else q

    def _safe_math_eval(self, expr):
        cleaned = re.sub(r'[^0-9A-Za-z_\+\-\*/\(\)\.\s\^]', '', expr)
        cleaned = cleaned.replace('^', '**')
        # Only allow identifiers that are known math names - reject anything else
        idents = set(re.findall(r'[A-Za-z_]+', cleaned))
        if not idents.issubset(self._MATH_NAMES.keys()):
            return None
        try:
            return eval(cleaned, {"__builtins__": {}}, self._MATH_NAMES)
        except Exception:
            return None

    def _extract_math_expression(self, query):
        """Strip conversational filler so 'what is 5 + 3' -> '5 + 3'."""
        q = query.lower()
        q = re.sub(r'\b(calculate|compute|solve|what is|what\'?s|please|the value of)\b', ' ', q)
        q = q.replace('plus', '+').replace('minus', '-').replace('times', '*').replace('divided by', '/')
        q = re.sub(r'[^0-9A-Za-z_\+\-\*/\(\)\.\s\^]', '', q)
        return q.strip()

    # ---------- reminders (delivered out-of-band, never swallow a query) ----------
    def add_reminder(self, task, seconds):
        self.reminder_id += 1
        rid = self.reminder_id
        self.reminders[rid] = {'task': task, 'time': time_now() + seconds}
        t = threading.Timer(seconds, self._fire_reminder, args=[rid, task])
        t.daemon = True
        t.start()
        return rid

    def _fire_reminder(self, rid, task):
        self._reminder_queue.put(f"\u23f0 REMINDER: {task}")

    def get_reminder_messages(self):
        msgs = []
        while not self._reminder_queue.empty():
            msgs.append(self._reminder_queue.get())
        return msgs

    def _handle_reminder(self, query):
        m = re.search(
            r'remind me to (.+?) in (\d+)\s*(second|seconds|minute|minutes|hour|hours|day|days)',
            query.lower(),
        )
        if m:
            task, amount, unit = m.group(1), int(m.group(2)), m.group(3)
            multiplier = {'second': 1, 'minute': 60, 'hour': 3600, 'day': 86400}[unit.rstrip('s')]
            seconds = amount * multiplier
            self.add_reminder(task, seconds)
            return f"Reminder set: '{task}' in {amount} {unit}."
        return "Say: 'remind me to [task] in [number] [minutes/hours/seconds]'."

    # ---------- todo / notes ----------
    def _handle_todo(self, query):
        q = query.lower()
        if 'add' in q:
            task = re.sub(r'\b(add|todo|task)\b', '', q).strip()
            if task:
                self.todo_list.append(task)
                return f"Added task: '{task}'. You have {len(self.todo_list)} task(s)."
            return "Specify a task to add, e.g. 'add task buy milk'."
        if 'remove' in q or 'delete' in q:
            m = re.search(r'(remove|delete)\D*(\d+)', q)
            if m:
                idx = int(m.group(2)) - 1
                if 0 <= idx < len(self.todo_list):
                    removed = self.todo_list.pop(idx)
                    return f"Removed task: '{removed}'. Remaining: {len(self.todo_list)}."
                return f"Invalid task number. You have {len(self.todo_list)} task(s)."
            return "Say 'remove [number]'."
        if 'list' in q or q.strip() == 'todo':
            if self.todo_list:
                return "\U0001f4cb Your todo list:\n" + "\n".join(f"{i + 1}. {t}" for i, t in enumerate(self.todo_list))
            return "Your todo list is empty."
        return "Todo commands: 'add [task]', 'remove [number]', 'list'."

    def _handle_note(self, query):
        q = query.lower()
        if 'save' in q or 'remember' in q:
            content = re.sub(r'\b(save|remember|note)\b', '', q).strip()
            if content:
                key = content[:20].strip()
                self.notes[key] = content
                return f"Note saved: '{key}...'"
            return "Provide content for the note, e.g. 'note remember to call mom'."
        if 'show' in q or 'list' in q:
            if self.notes:
                return "\U0001f4dd Your notes:\n" + "\n".join(f"- {k}" for k in self.notes.keys())
            return "No notes saved yet."
        return "Note commands: 'save [content]', 'show notes'."

    # ---------- content lookups ----------
    def _handle_pattern(self, query):
        types = ['diamond', 'spiral', 'checker', 'waves', 'grid', 'circle', 'triangle', 'star', 'rainbow', 'gradient']
        ql = query.lower()
        for p in types:
            if p.rstrip('s') in ql or p in ql:
                return self.ascii_art.generate_pattern(p, color=True)
        return self.ascii_art.generate_pattern('diamond', color=True)

    def _handle_lyrics(self, query):
        q = query.lower()
        for key, lyr in self.lyrics.items():
            if key in q:
                return f"\U0001f3b5 {key.title()}:\n{lyr}"
        return random.choice(list(self.lyrics.values()))

    def _handle_poem(self, query):
        q = query.lower()
        for key, poem in self.poems.items():
            if key in q:
                return f"\U0001f4dd {key.title()}:\n{poem}"
        return random.choice(list(self.poems.values()))

    def _handle_story(self, query):
        q = query.lower()
        for key, story in self.stories.items():
            if key in q:
                return f"\U0001f4d6 {key.title()}:\n{story}"
        return random.choice(list(self.stories.values()))

    def _handle_battle(self, query):
        q = query.lower()
        for key, res in self.battles.items():
            if key in q:
                return f"\u2694\ufe0f {key.title()}:\n{res}"
        return "Who should fight? Try: 'Goku vs Superman', 'Iron Man vs Batman', etc."

    def _generate_ascii_art(self, subject):
        if subject.lower().endswith(('.png', '.jpg', '.jpeg', '.gif', '.bmp')):
            return self.ascii_art.image_to_ascii(subject, color=True)
        style = self.settings.get("ascii_style") or random.choice(['block', 'shade', 'diagonal', 'dots', 'numbers'])
        width = int(self.settings.get("ascii_width", 80))
        return self.ascii_art.generate_art_from_text(subject, style=style, width=width, color=True)

    # ---------- personality-aware wrapping ----------
    def _wrap_response(self, text, allow_humor_tag=False):
        """Apply lightweight, real effects for settings the UI exposes."""
        verbosity = int(self.settings.get("verbosity", 70))
        personality = self.settings.get("personality", "friendly")
        if verbosity < 30 and '\n' not in text and len(text) > 120:
            # Trim to first sentence for a "concise" feel.
            first = re.split(r'(?<=[.!?])\s', text)[0]
            text = first
        if personality == 'sarcastic' and allow_humor_tag:
            text = text + " ...shocking, I know."
        elif personality == 'philosophical' and allow_humor_tag:
            text = text + " Make of that what you will."
        return text

    # ---------- main entry point ----------
    def respond(self, query):
        intent = IntentDetector.detect(query)
        q_low = query.lower().strip()

        if intent == 'greeting':
            name = self.context.get_user_name()
            return f"Hey {name}! I'm {self.name}. What can I help with?" if name else f"Hello! I'm {self.name}. What's on your mind?"
        if intent == 'farewell':
            return "Goodbye! Come back anytime."
        if intent == 'name_ask':
            return f"I'm {self.name}, your personal AI assistant."
        if intent == 'name_give':
            m = re.search(r'\b(my name is|call me|you can call me)\s+([a-zA-Z]+)', q_low)
            if m:
                name = m.group(2).title()
                self.context.set_user_name(name)
                return f"Nice to meet you, {name}! I'm {self.name}."
            return "I didn't catch your name. Say 'my name is ...'."
        if intent == 'how_are_you':
            return random.choice(["I'm great, thanks!", "Doing well, always ready.", "Perfectly fine, thank you!"])
        if intent == 'time':
            now = datetime.datetime.now()
            return f"\U0001f550 It's {now.strftime('%I:%M %p')} on {now.strftime('%A, %B %d, %Y')}."
        if intent == 'weather':
            return ("\U0001f324\ufe0f I don't have a live weather feed connected.\n"
                    "Check a weather app or site for today's exact forecast.")
        if intent == 'fact':
            key = random.choice(list(self.knowledge.keys()))
            return f"\U0001f4a1 Did you know? {self.knowledge[key]}"
        if intent == 'joke':
            return self._wrap_response(f"\U0001f602 {random.choice(self.jokes)}")
        if intent == 'quote':
            return f"\u2728 {random.choice(self.quotes)}"
        if intent == 'define':
            term = re.sub(r'^\s*(define|what is|what are|meaning of|explain|what does|tell me about)\s+', '', q_low).strip()
            term = term.rstrip('?')
            if term in self.knowledge:
                return self._wrap_response(f"\U0001f4da {term.title()}: {self.knowledge[term]}")
            return f"I don't have a definition for '{term}'."
        if intent == 'calculate':
            expr = self._extract_math_expression(query)
            result = self._safe_math_eval(expr) if expr else None
            if result is not None:
                return f"\U0001f9ee {expr.strip()} = {result}"
            return "Couldn't compute that expression. Try something like '5 + 3' or 'sqrt(16)'."
        if intent == 'convert':
            res = convert_units(query)
            if res:
                return f"\U0001f504 {res}"
            return "Conversion not recognized. Try: '10 cm to inch', '30 c to f', '5 km to miles'."
        if intent == 'pattern':
            return self._handle_pattern(query)
        if intent == 'ascii':
            subject = self._fuzzy_subject(query) or "NEON"
            art = self._generate_ascii_art(subject)
            return f"\U0001f308 ASCII Art for '{subject}':\n{art}"
        if intent == 'help':
            return self._help_text()
        if intent == 'clear':
            return "CLEAR_SESSION"
        if intent == 'save':
            return "SAVE_CHAT"
        if intent == 'load':
            return "LOAD_CHAT"
        if intent == 'news':
            return ("\U0001f4f0 I don't have a live news feed connected.\n"
                    "Historical note: Moon landing 1969, WWW invented 1989, Human genome sequenced 2003.")
        if intent == 'wikipedia':
            term = re.sub(r'^\s*(wikipedia|wiki|encyclopedia)\s*', '', q_low).strip()
            if term and term in self.knowledge:
                return f"\U0001f4d6 {term.title()}\n{self.knowledge[term]}"
            return f"I don't have local info on '{term}'." if term else "What topic would you like to look up?"
        if intent == 'search':
            term = re.sub(r'^\s*(search|google|look up)\s*(for)?\s*', '', q_low).strip()
            if term and term in self.knowledge:
                return f"\U0001f50e Found: {self.knowledge[term]}"
            return f"No local info on '{term}'." if term else "What should I search for?"
        if intent == 'reminder':
            return self._handle_reminder(query)
        if intent == 'todo':
            return self._handle_todo(query)
        if intent == 'note':
            return self._handle_note(query)
        if intent == 'roll':
            m = re.search(r'(\d+)?d(\d+)', q_low)
            if m:
                num = int(m.group(1)) if m.group(1) else 1
                sides = int(m.group(2))
                if num > 20:
                    return "Too many dice at once - try 20 or fewer."
                results = [random.randint(1, sides) for _ in range(num)]
                return f"\U0001f3b2 Rolled {num}d{sides}: {', '.join(map(str, results))} (sum: {sum(results)})"
            return f"\U0001f3b2 You rolled a {random.randint(1, 6)}"
        if intent == 'flip':
            return f"\U0001fa99 Coin flip: {random.choice(['Heads', 'Tails'])}"
        if intent == 'password':
            length = 12
            m = re.search(r'(\d+)\s*(?:character|char|digit)?s?\b', q_low)
            if m:
                length = max(4, min(128, int(m.group(1))))
            chars = ('abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789'
                     '!@#$%^&*()_+-=[]{}|;:,.<>?')
            pwd = ''.join(random.choice(chars) for _ in range(length))
            return f"\U0001f511 Generated password ({length} chars): {pwd}"
        if intent == 'lyrics':
            return self._handle_lyrics(query)
        if intent == 'poem':
            return self._handle_poem(query)
        if intent == 'story':
            return self._handle_story(query)
        if intent == 'battle':
            return self._handle_battle(query)

        # general fallback
        subject = self._fuzzy_subject(query)
        if subject and subject in self.knowledge:
            return self._wrap_response(f"\U0001f4da {subject.title()}: {self.knowledge[subject]}")
        if subject:
            return f"I'm not sure about '{subject}'. Could you rephrase, or try 'help' to see what I can do?"
        return "I'm not sure what you're asking. Try 'help' to see what I can do."

    def _help_text(self):
        return (
            "\U0001f916 N30N C0R3 AI v3.0 - Help\n"
            "====================\n"
            "\U0001f4ac General: just talk naturally.\n"
            "\U0001f308 ASCII Art: 'ascii [text]' - 7-color rainbow!\n"
            "\U0001f4d0 Patterns: diamond, spiral, checker, waves, grid, circle, triangle, star, rainbow, gradient\n"
            "\U0001f9ee Math: '5 + 3', 'sqrt(16)', etc.\n"
            "\U0001f504 Convert: '10 cm to inch', '30 c to f', '5 km to miles'\n"
            "\u23f0 Reminder: 'remind me to ... in X minutes'\n"
            "\U0001f4cb Todo: 'add task ...', 'remove [number]', 'list'\n"
            "\U0001f4dd Notes: 'save note ...', 'show notes'\n"
            "\U0001f3b2 Fun: roll d20, flip coin, joke, quote\n"
            "\U0001f4d6 Stories: 'tell me a story about [alien/dream/ai/time/ocean]'\n"
            "\U0001f3b5 Lyrics: 'sing [imagine/bohemian/yesterday/happy]'\n"
            "\U0001f4dd Poems: 'poem [roses/hope/road/star]'\n"
            "\u2694\ufe0f Battles: 'Goku vs Superman', etc.\n"
            "\U0001f4cb Commands: /help, /clear, /new, /delete, /export, /load"
        )


def time_now():
    import time
    return time.time()


# =============================================================================
# MEMORY DATABASE (real JSON persistence)
# =============================================================================
class MemoryDatabase:
    def __init__(self, settings=None):
        self.settings = settings or {}
        _ensure_app_dir()
        self.path = self._resolve_path()
        default = {"chats": {}, "messages": {}, "next_id": 1}
        loaded = _load_json(self.path, default)
        # JSON always stores dict keys as strings - normalize back to int ids.
        self._storage = {
            "chats": {int(k): v for k, v in loaded.get("chats", {}).items()},
            "messages": {int(k): v for k, v in loaded.get("messages", {}).items()},
            "next_id": loaded.get("next_id", 1),
        }

    def _resolve_path(self):
        custom = self.settings.get("save_path")
        if self.settings.get("save_locally") and custom:
            return custom
        return DEFAULT_HISTORY_PATH

    def create_chat(self, name):
        cid = self._storage["next_id"]
        self._storage["next_id"] += 1
        self._storage["chats"][cid] = {"name": name, "time": datetime.datetime.now().isoformat()}
        self._storage["messages"][cid] = []
        self._save()
        return cid

    def get_or_create_chat(self):
        if self._storage["chats"]:
            return sorted(self._storage["chats"].keys())[0]
        return self.create_chat("New Session")

    def add_msg(self, cid, role, text):
        if cid in self._storage["messages"]:
            self._storage["messages"][cid].append(
                {"role": role, "text": text, "time": datetime.datetime.now().isoformat()}
            )
            if self.settings.get("chat_auto_save", True):
                self._save()

    def get_history(self, cid):
        return self._storage["messages"].get(cid, [])

    def get_chats(self):
        return sorted(self._storage["chats"].items())

    def clear_all(self):
        self._storage = {"chats": {}, "messages": {}, "next_id": 1}
        self._save()
        return self.create_chat("New Session")

    def delete_chat(self, cid):
        if cid in self._storage["chats"]:
            del self._storage["chats"][cid]
            self._storage["messages"].pop(cid, None)
            self._save()
            return True
        return False

    def rename_chat(self, cid, new_name):
        if cid in self._storage["chats"]:
            self._storage["chats"][cid]["name"] = new_name
            self._save()
            return True
        return False

    def export_to_json(self, cid, filepath):
        if cid in self._storage["messages"]:
            data = {
                "chat_name": self._storage["chats"].get(cid, {}).get("name", "Unknown"),
                "created": self._storage["chats"].get(cid, {}).get("time", ""),
                "messages": self._storage["messages"][cid],
            }
            return _save_json(filepath, data)
        return False

    def import_from_json(self, filepath):
        data = _load_json(filepath, None)
        if not data:
            return None
        name = data.get("chat_name", "Imported Chat")
        cid = self.create_chat(name)
        for msg in data.get("messages", []):
            self.add_msg(cid, msg.get("role", "user"), msg.get("text", ""))
        return cid

    def refresh_path(self, settings):
        """Call after settings change in case save_path/save_locally changed."""
        self.settings = settings
        self.path = self._resolve_path()

    def _save(self):
        _save_json(self.path, self._storage)


# =============================================================================
# SETTINGS STORE (real JSON persistence)
# =============================================================================
class SettingsStore:
    DEFAULTS = {
        'theme': 'Neon Orange',
        'font_size': 12,
        'ascii_style': 'block',
        'ascii_width': 80,
        'chat_auto_save': True,
        'save_locally': False,
        'save_path': '',
        'show_timestamps': True,
        'enable_reminders': True,
        'auto_clear_display': False,
        'response_style': 'detailed',
        'personality': 'friendly',
        'humor_level': 50,
        'verbosity': 70,
        'creative_mode': True,
    }

    def __init__(self):
        _ensure_app_dir()
        self.path = DEFAULT_SETTINGS_PATH
        loaded = _load_json(self.path, {})
        self.data = dict(self.DEFAULTS)
        self.data.update(loaded)

    def save(self):
        _save_json(self.path, self.data)


# =============================================================================
# NEON UI
# =============================================================================
class NeonUI:
    THEMES = {
        "Neon Orange": {"bg": "#000000", "panel": "#0A0A0A", "field": "#111111", "accent": "#FF5F00", "bot": "#00F0FF", "text": "#FFFFFF", "muted": "#444444"},
        "Neon Pink": {"bg": "#000000", "panel": "#0A0A0A", "field": "#111111", "accent": "#FF007F", "bot": "#BC13FE", "text": "#FFFFFF", "muted": "#444444"},
        "Neon Green": {"bg": "#000000", "panel": "#030803", "field": "#061206", "accent": "#39FF14", "bot": "#00FFCC", "text": "#E8FFE8", "muted": "#1A4D1A"},
        "Neon Blue": {"bg": "#000000", "panel": "#00050f", "field": "#000c1f", "accent": "#00BFFF", "bot": "#00FFFF", "text": "#E0F7FF", "muted": "#003366"},
        "Neon Red": {"bg": "#000000", "panel": "#0f0000", "field": "#1f0000", "accent": "#FF003F", "bot": "#FF6600", "text": "#FFE0E0", "muted": "#660000"},
        "Neon Purple": {"bg": "#000000", "panel": "#0A001A", "field": "#15002B", "accent": "#9B59B6", "bot": "#FF6BFF", "text": "#F0E6FF", "muted": "#4A0066"},
        "Neon Yellow": {"bg": "#000000", "panel": "#1A1500", "field": "#2B2500", "accent": "#F1C40F", "bot": "#FFD700", "text": "#FFF8E7", "muted": "#665500"},
        "Soft Night (Green)": {"bg": "#121212", "panel": "#1E1E1E", "field": "#252525", "accent": "#81C784", "bot": "#64B5F6", "text": "#E0E0E0", "muted": "#424242"},
        "Soft Night (Blue)": {"bg": "#0D1117", "panel": "#161B22", "field": "#21262D", "accent": "#58A6FF", "bot": "#A5D6FF", "text": "#C9D1D9", "muted": "#484F58"},
        "Soft Night (Purple)": {"bg": "#0D0D1A", "panel": "#161622", "field": "#21212D", "accent": "#9B59B6", "bot": "#BB8FCE", "text": "#D4C4E6", "muted": "#4A4A66"},
        "Retro Terminal": {"bg": "#000000", "panel": "#001100", "field": "#002200", "accent": "#00FF00", "bot": "#00FF88", "text": "#B3FFB3", "muted": "#003300"},
        "Matrix": {"bg": "#000000", "panel": "#001500", "field": "#002A00", "accent": "#00FF00", "bot": "#00FF66", "text": "#00FF00", "muted": "#003300"},
    }

    def __init__(self, root):
        self.root = root
        self.root.title("\U0001f308 N30N C0R3 AI - v3.0 (corrected)")
        self.root.geometry("1200x950")
        self.settings_store = SettingsStore()
        self.settings = self.settings_store.data
        self.memory = MemoryDatabase(self.settings)
        self.chat_id = self.memory.get_or_create_chat()
        self.brain = ThinkingBrain(self.settings)
        self.ascii_art = ASCIIArtGenerator()
        self.img_cache = []
        # Parallel array kept in sync with the listbox so selection -> chat id
        # lookups can never desync from a stale re-sort of the storage dict.
        self._chat_ids = []
        self._build_layout()
        self.apply_theme()
        self.load_history()
        self._reminder_check()
        self.status.config(text="\U0001f308 Ready")

    # ---------- layout ----------
    def _build_layout(self):
        self.header = tk.Frame(self.root, height=70)
        self.header.pack(side=tk.TOP, fill=tk.X)
        self.logo = tk.Label(self.header, text="\U0001f308 N30N C0R3 AI v3.0", font=("Consolas", 18, "bold"))
        self.logo.pack(side=tk.LEFT, padx=30)
        self.top_btn_frame = tk.Frame(self.header)
        self.top_btn_frame.pack(side=tk.RIGHT, padx=20)
        self.btn_settings = tk.Button(self.top_btn_frame, text="\u2699\ufe0f", font=("Arial", 14), command=self.open_settings)
        self.btn_settings.pack(side=tk.LEFT, padx=5)
        self.btn_help = tk.Button(self.top_btn_frame, text="\u2753", font=("Arial", 14), command=self.show_help)
        self.btn_help.pack(side=tk.LEFT, padx=5)
        self.btn_clear = tk.Button(self.top_btn_frame, text="\U0001f5d1\ufe0f", font=("Arial", 14), command=self.clear_current)
        self.btn_clear.pack(side=tk.LEFT, padx=5)
        self.btn_export = tk.Button(self.top_btn_frame, text="\U0001f4be", font=("Arial", 14), command=self.export_chat)
        self.btn_export.pack(side=tk.LEFT, padx=5)
        self.btn_import = tk.Button(self.top_btn_frame, text="\U0001f4c2", font=("Arial", 14), command=self.import_chat)
        self.btn_import.pack(side=tk.LEFT, padx=5)
        self.btn_ascii = tk.Button(self.top_btn_frame, text="\U0001f308", font=("Arial", 14), command=self.generate_ascii_dialog)
        self.btn_ascii.pack(side=tk.LEFT, padx=5)

        self.paned = tk.PanedWindow(self.root, orient=tk.HORIZONTAL, bd=0, sashwidth=2)
        self.paned.pack(fill=tk.BOTH, expand=True)

        self.side = tk.Frame(self.paned, width=260)
        self.side.pack_propagate(False)
        self.paned.add(self.side)
        self.side_label = tk.Label(self.side, text="\U0001f4cb SESSIONS", font=("Consolas", 11, "bold"))
        self.side_label.pack(pady=10)
        self.chat_list = tk.Listbox(self.side, bd=0, font=("Consolas", 10))
        self.chat_list.pack(fill=tk.BOTH, expand=True, padx=10, pady=(0, 10))
        self.chat_list.bind("<<ListboxSelect>>", self._switch_chat)
        self.chat_list.bind("<Double-Button-1>", self._rename_chat)

        self.btn_f = tk.Frame(self.side)
        self.btn_f.pack(fill=tk.X, padx=10, pady=5)
        self.b_new = tk.Button(self.btn_f, text="+ New", bd=0, font=("Consolas", 10), command=self.new_chat)
        self.b_new.pack(side=tk.LEFT, padx=2)
        self.b_del = tk.Button(self.btn_f, text="\u2212", bd=0, font=("Consolas", 12), command=self.delete_chat, width=2)
        self.b_del.pack(side=tk.LEFT, padx=2)
        self.b_clr = tk.Button(self.btn_f, text="\u00d7 All", bd=0, font=("Consolas", 10), command=self.clear_history)
        self.b_clr.pack(side=tk.LEFT, padx=2)

        self.chat_main = tk.Frame(self.paned)
        self.paned.add(self.chat_main)
        self.display = scrolledtext.ScrolledText(self.chat_main, bd=0, wrap=tk.WORD, state='disabled', padx=25, pady=25)
        self.display.pack(fill=tk.BOTH, expand=True)

        self.footer = tk.Frame(self.chat_main, height=100)
        self.footer.pack(side=tk.BOTTOM, fill=tk.X, padx=20, pady=15)
        self.footer.pack_propagate(False)
        self.wrap = tk.Frame(self.footer, highlightthickness=1)
        self.wrap.pack(fill=tk.BOTH, expand=True)
        self.b_file = tk.Button(self.wrap, text="\U0001f4ce", bd=0, font=("Arial", 14), command=self.load_file)
        self.b_file.pack(side=tk.LEFT, padx=10)
        self.entry = tk.Entry(self.wrap, bd=0, font=("Consolas", 13))
        self.entry.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=5)
        self.entry.bind("<Return>", lambda e: self.send())
        self.b_send = tk.Button(self.wrap, text="\u279c", bd=0, font=("Arial", 18), command=self.send)
        self.b_send.pack(side=tk.RIGHT, padx=10)

        self.status = tk.Label(self.root, text="Ready", font=("Consolas", 9))
        self.status.pack(side=tk.BOTTOM, fill=tk.X)

    def apply_theme(self):
        theme_name = self.settings.get("theme", "Neon Orange")
        theme = self.THEMES.get(theme_name, self.THEMES["Neon Orange"])
        self.root.configure(bg=theme["bg"])
        self.paned.configure(bg=theme["bg"])
        self.header.configure(bg=theme["panel"])
        self.logo.configure(bg=theme["panel"], fg=theme["accent"])
        self.top_btn_frame.configure(bg=theme["panel"])
        for b in [self.btn_settings, self.btn_help, self.btn_clear, self.btn_export, self.btn_import, self.btn_ascii]:
            b.configure(bg=theme["panel"], fg=theme["accent"], activebackground=theme["field"], activeforeground=theme["accent"])
        self.side.configure(bg=theme["panel"])
        self.side_label.configure(bg=theme["panel"], fg=theme["accent"])
        self.chat_list.configure(bg=theme["panel"], fg=theme["text"], selectbackground=theme["field"])
        self.btn_f.configure(bg=theme["panel"])
        for b in [self.b_new, self.b_del, self.b_clr, self.b_file, self.b_send]:
            b.configure(bg=theme["field"], fg=theme["accent"], activebackground=theme["panel"], activeforeground=theme["accent"])
        self.chat_main.configure(bg=theme["bg"])
        font_size = int(self.settings.get("font_size", 12))
        self.display.configure(bg=theme["bg"], fg=theme["text"], font=("Consolas", font_size), insertbackground=theme["accent"])
        self.display.tag_config("user", foreground=theme["accent"], font=("Consolas", font_size, "bold"))
        self.display.tag_config("bot", foreground=theme["bot"])
        self.display.tag_config("ascii", foreground=theme["text"], background=theme["field"], font=("Consolas", font_size))
        self.display.tag_config("timestamp", foreground=theme["muted"], font=("Consolas", max(8, font_size - 3)))
        for i, col in enumerate(ASCII_COLORS):
            self.display.tag_config(f"ascii_color{i}", foreground=col)
        self.footer.configure(bg=theme["bg"])
        self.wrap.configure(bg=theme["field"], highlightbackground=theme["muted"])
        self.entry.configure(bg=theme["field"], fg=theme["text"], font=("Consolas", 13), insertbackground=theme["accent"])
        self.status.configure(bg=theme["panel"], fg=theme["muted"])
        self._refresh_chats()

    # ---------- settings dialog ----------
    def open_settings(self):
        win = tk.Toplevel(self.root)
        win.title("\u2699\ufe0f Settings")
        win.geometry("600x700")
        theme = self.THEMES.get(self.settings.get("theme", "Neon Orange"), self.THEMES["Neon Orange"])
        win.configure(bg=theme["bg"])
        canvas = tk.Canvas(win, bg=theme["bg"], highlightthickness=0)
        scrollbar = ttk.Scrollbar(win, orient="vertical", command=canvas.yview)
        scrollable_frame = tk.Frame(canvas, bg=theme["bg"])
        scrollable_frame.bind("<Configure>", lambda e: canvas.configure(scrollregion=canvas.bbox("all")))
        canvas.create_window((0, 0), window=scrollable_frame, anchor="nw")
        canvas.configure(yscrollcommand=scrollbar.set)
        canvas.pack(side="left", fill="both", expand=True)
        scrollbar.pack(side="right", fill="y")

        row = 0

        def add_label(text, r):
            lbl = tk.Label(scrollable_frame, text=text, bg=theme["bg"], fg=theme["text"], font=("Consolas", 10, "bold"))
            lbl.grid(row=r, column=0, sticky="w", pady=(10, 2), padx=10)

        def add_combo(var, options, r):
            combo = ttk.Combobox(scrollable_frame, textvariable=var, values=options, state="readonly")
            combo.grid(row=r, column=0, columnspan=2, sticky="ew", padx=10, pady=2)

        def add_checkbox(var, text, r):
            cb = tk.Checkbutton(scrollable_frame, text=text, variable=var, bg=theme["bg"], fg=theme["text"], selectcolor=theme["field"])
            cb.grid(row=r, column=0, columnspan=2, sticky="w", padx=10, pady=2)

        def add_slider(var, label, from_, to_, r):
            lbl = tk.Label(scrollable_frame, text=label, bg=theme["bg"], fg=theme["text"])
            lbl.grid(row=r, column=0, sticky="w", padx=10, pady=(10, 0))
            # NOTE: ttk.Scale always yields floats, so it must be bound to a
            # DoubleVar (not IntVar) or Tk raises a TclError when dragged.
            slider = ttk.Scale(scrollable_frame, from_=from_, to=to_, variable=var, orient="horizontal")
            slider.grid(row=r + 1, column=0, columnspan=2, sticky="ew", padx=10, pady=2)

        add_label("Theme", row)
        theme_var = tk.StringVar(value=self.settings.get("theme", "Neon Orange"))
        add_combo(theme_var, list(self.THEMES.keys()), row + 1)
        row += 2

        add_label("Font Size", row)
        size_var = tk.IntVar(value=int(self.settings.get("font_size", 12)))
        ttk.Spinbox(scrollable_frame, from_=8, to=24, textvariable=size_var, width=5).grid(row=row + 1, column=0, sticky="w", padx=10, pady=2)
        row += 2

        add_label("ASCII Style", row)
        ascii_style_var = tk.StringVar(value=self.settings.get("ascii_style", "block"))
        add_combo(ascii_style_var, ['block', 'shade', 'diagonal', 'dots', 'numbers'], row + 1)
        row += 2

        add_label("ASCII Width", row)
        ascii_width_var = tk.IntVar(value=int(self.settings.get("ascii_width", 80)))
        ttk.Spinbox(scrollable_frame, from_=20, to=200, textvariable=ascii_width_var, width=5).grid(row=row + 1, column=0, sticky="w", padx=10, pady=2)
        row += 2

        add_label("Personality", row)
        personality_var = tk.StringVar(value=self.settings.get("personality", "friendly"))
        add_combo(personality_var, ['friendly', 'witty', 'sarcastic', 'professional', 'philosophical'], row + 1)
        row += 2

        add_label("Response Style", row)
        resp_style_var = tk.StringVar(value=self.settings.get("response_style", "detailed"))
        add_combo(resp_style_var, ['concise', 'detailed', 'comprehensive'], row + 1)
        row += 2

        creative_var = tk.BooleanVar(value=self.settings.get("creative_mode", True))
        auto_save_var = tk.BooleanVar(value=self.settings.get("chat_auto_save", True))
        timestamps_var = tk.BooleanVar(value=self.settings.get("show_timestamps", True))
        reminders_var = tk.BooleanVar(value=self.settings.get("enable_reminders", True))
        auto_clear_var = tk.BooleanVar(value=self.settings.get("auto_clear_display", False))
        add_checkbox(creative_var, "Creative Mode", row); row += 1
        add_checkbox(auto_save_var, "Auto-save chat history", row); row += 1
        add_checkbox(timestamps_var, "Show timestamps in chat", row); row += 1
        add_checkbox(reminders_var, "Enable reminders", row); row += 1
        add_checkbox(auto_clear_var, "Auto-clear on new session", row); row += 2

        humor_var = tk.DoubleVar(value=float(self.settings.get("humor_level", 50)))
        add_slider(humor_var, "Humor Level", 0, 100, row); row += 2
        verbosity_var = tk.DoubleVar(value=float(self.settings.get("verbosity", 70)))
        add_slider(verbosity_var, "Verbosity", 0, 100, row); row += 2

        def save_settings():
            self.settings["theme"] = theme_var.get()
            self.settings["font_size"] = size_var.get()
            self.settings["ascii_style"] = ascii_style_var.get()
            self.settings["ascii_width"] = ascii_width_var.get()
            self.settings["personality"] = personality_var.get()
            self.settings["response_style"] = resp_style_var.get()
            self.settings["creative_mode"] = creative_var.get()
            self.settings["chat_auto_save"] = auto_save_var.get()
            self.settings["show_timestamps"] = timestamps_var.get()
            self.settings["enable_reminders"] = reminders_var.get()
            self.settings["auto_clear_display"] = auto_clear_var.get()
            self.settings["humor_level"] = int(humor_var.get())
            self.settings["verbosity"] = int(verbosity_var.get())
            self.settings_store.save()
            self.brain.update_settings(self.settings)
            self.memory.refresh_path(self.settings)
            self.apply_theme()
            win.destroy()

        btn_save = tk.Button(scrollable_frame, text="\U0001f4be Save Settings", command=save_settings, bg=theme["accent"], fg="#000", font=("Consolas", 12, "bold"))
        btn_save.grid(row=row, column=0, columnspan=2, pady=20)

    def generate_ascii_dialog(self):
        win = tk.Toplevel(self.root)
        win.title("\U0001f308 Generate 7-Color ASCII Art")
        win.geometry("450x350")
        theme = self.THEMES.get(self.settings.get("theme", "Neon Orange"), self.THEMES["Neon Orange"])
        win.configure(bg=theme["bg"])
        tk.Label(win, text="Text to convert:", bg=theme["bg"], fg=theme["text"], font=("Consolas", 10)).pack(pady=10)
        entry = tk.Entry(win, font=("Consolas", 14), bg=theme["field"], fg=theme["text"], insertbackground=theme["accent"])
        entry.pack(padx=20, pady=5, fill=tk.X)
        entry.focus()
        tk.Label(win, text="Style:", bg=theme["bg"], fg=theme["text"], font=("Consolas", 10)).pack(pady=(10, 0))
        style_var = tk.StringVar(value=self.settings.get("ascii_style", "block"))
        ttk.Combobox(win, textvariable=style_var, values=['block', 'shade', 'diagonal', 'dots', 'numbers']).pack(padx=20, pady=5, fill=tk.X)
        tk.Label(win, text="\U0001f308 7 Colors: Red Orange Yellow Green Blue Indigo Violet",
                 bg=theme["bg"], fg=theme["accent"], font=("Consolas", 9)).pack(pady=5)

        def generate():
            text = entry.get().strip() or "NEON"
            width = int(self.settings.get("ascii_width", 80))
            art = self.ascii_art.generate_art_from_text(text, style=style_var.get(), width=width, color=True)
            self.add_msg("NEON", f"\U0001f308 7-Color ASCII Art for '{text}':\n{art}", "bot")
            self.memory.add_msg(self.chat_id, "bot", f"ASCII Art for '{text}'")
            win.destroy()

        tk.Button(win, text="\U0001f308 Generate", command=generate, bg=theme["accent"], fg="#000", font=("Consolas", 12, "bold")).pack(pady=20)

    def show_help(self):
        messagebox.showinfo("Help", self.brain._help_text())

    def clear_current(self):
        self.display.config(state='normal')
        self.display.delete("1.0", tk.END)
        self.display.config(state='disabled')
        self.status.config(text="Display cleared")

    def export_chat(self):
        path = filedialog.asksaveasfilename(defaultextension=".json",
                                             filetypes=[("JSON files", "*.json"), ("All files", "*.*")])
        if path:
            if self.memory.export_to_json(self.chat_id, path):
                self.status.config(text=f"Exported to {path}")
                messagebox.showinfo("Export", f"Chat exported to {path}")
            else:
                messagebox.showerror("Export", "Export failed.")

    def import_chat(self):
        path = filedialog.askopenfilename(filetypes=[("JSON files", "*.json"), ("All files", "*.*")])
        if path:
            cid = self.memory.import_from_json(path)
            if cid is not None:
                self.chat_id = cid
                self._refresh_chats()
                self.load_history()
                self.status.config(text=f"Imported from {path}")
                messagebox.showinfo("Import", f"Chat imported from {path}")
            else:
                messagebox.showerror("Import", "Import failed. Invalid JSON file.")

    # ---------- session list (keeps a parallel id array to avoid desync) ----------
    def _refresh_chats(self):
        self.chat_list.delete(0, tk.END)
        chats = self.memory.get_chats()
        self._chat_ids = [cid for cid, _ in chats]
        for _, chat in chats:
            self.chat_list.insert(tk.END, chat["name"])

    def _cid_for_selection(self, idx):
        if 0 <= idx < len(self._chat_ids):
            return self._chat_ids[idx]
        return None

    def _switch_chat(self, event=None):
        sel = self.chat_list.curselection()
        if sel:
            cid = self._cid_for_selection(sel[0])
            if cid is not None:
                self.chat_id = cid
                self.load_history()
                name = self.memory._storage["chats"].get(cid, {}).get("name", "chat")
                self.status.config(text=f"Switched to {name}")

    def _rename_chat(self, event):
        sel = self.chat_list.curselection()
        if sel:
            cid = self._cid_for_selection(sel[0])
            if cid is None:
                return
            current = self.memory._storage["chats"].get(cid, {}).get("name", "")
            new = simpledialog.askstring("Rename", "New name:", initialvalue=current)
            if new:
                self.memory.rename_chat(cid, new)
                self._refresh_chats()
                self.status.config(text=f"Renamed to {new}")

    def new_chat(self):
        name = simpledialog.askstring("New Session", "Session name:")
        if name:
            self.chat_id = self.memory.create_chat(name)
            self._refresh_chats()
            self.load_history()
            if self.settings.get("auto_clear_display", False):
                self.clear_current()
            self.status.config(text=f"New session: {name}")

    def delete_chat(self):
        sel = self.chat_list.curselection()
        if sel:
            cid = self._cid_for_selection(sel[0])
            if cid is None:
                return
            name = self.memory._storage["chats"].get(cid, {}).get("name", "this session")
            if messagebox.askyesno("Delete", f"Delete '{name}'?"):
                self.memory.delete_chat(cid)
                self._refresh_chats()
                if self.chat_id == cid:
                    self.chat_id = self.memory.get_or_create_chat()
                    self.load_history()
                self.status.config(text="Session deleted")

    def clear_history(self):
        if messagebox.askyesno("Clear All", "Wipe ALL chat history?"):
            self.chat_id = self.memory.clear_all()
            self._refresh_chats()
            self.load_history()
            self.status.config(text="All history cleared")

    def load_history(self):
        self.display.config(state='normal')
        self.display.delete("1.0", tk.END)
        self.display.config(state='disabled')
        for m in self.memory.get_history(self.chat_id):
            tag = "user" if m.get("role") == "user" else "bot"
            self.add_msg("YOU" if tag == "user" else "NEON", m.get("text", ""), tag, timestamp=m.get("time"))

    # ---------- ASCII-art detection tuned to avoid false positives on
    # short, punctuation-heavy replies like dice rolls or passwords ----------
    def _is_ascii_art(self, text):
        if '\u00a7' in text and re.search(r'\u00a7[0-6]', text):
            return True
        lines = text.splitlines()
        if len(lines) < 3 or len(text) < 60:
            return False
        non_alnum = sum(1 for ch in text if not ch.isalnum() and not ch.isspace()) / max(len(text), 1)
        return non_alnum > 0.35

    def add_msg(self, speaker, text, role_tag="bot", timestamp=None):
        self.display.config(state='normal')
        if self.settings.get("show_timestamps", True):
            ts = timestamp or datetime.datetime.now().isoformat()
            try:
                ts_display = datetime.datetime.fromisoformat(ts).strftime('%H:%M')
            except Exception:
                ts_display = ""
            if ts_display:
                self.display.insert(tk.END, f"[{ts_display}] ", "timestamp")

        if role_tag == "user":
            self.display.insert(tk.END, f"{speaker}: ", "user")
            self.display.insert(tk.END, text + "\n\n")
        else:
            if self._is_ascii_art(text):
                self.display.insert(tk.END, f"{speaker}:\n", "bot")
                i = 0
                while i < len(text):
                    if text[i] == '\u00a7' and i + 1 < len(text) and text[i + 1].isdigit():
                        color_idx = int(text[i + 1])
                        if color_idx < 7 and i + 2 < len(text):
                            self.display.insert(tk.END, text[i + 2], f"ascii_color{color_idx}")
                            i += 3
                            continue
                        i += 2
                        continue
                    self.display.insert(tk.END, text[i])
                    i += 1
                self.display.insert(tk.END, "\n\n")
            else:
                self.display.insert(tk.END, f"{speaker}: ", "bot")
                self.display.insert(tk.END, text + "\n\n")
        self.display.config(state='disabled')
        self.display.see(tk.END)

    def load_file(self):
        path = filedialog.askopenfilename()
        if not path:
            return
        filename = os.path.basename(path)
        if any(path.lower().endswith(e) for e in ['.png', '.jpg', '.jpeg', '.webp']) and HAS_PIL:
            img = Image.open(path)
            img.thumbnail((400, 400))
            photo = ImageTk.PhotoImage(img)
            self.img_cache.append(photo)
            self.display.config(state='normal')
            self.display.image_create(tk.END, image=photo)
            self.display.insert(tk.END, "\n")
            self.display.config(state='disabled')
            self.add_msg("SYSTEM", f"\U0001f4f7 Image loaded: {filename}", "bot")
        elif any(path.lower().endswith(e) for e in ['.txt', '.py', '.js', '.html', '.css', '.json', '.md']):
            try:
                with open(path, "r", encoding="utf-8") as f:
                    content = f.read()
                self.add_msg("SYSTEM", f"\U0001f4c4 File content ({filename}):\n{content[:1000]}...", "bot")
            except Exception as e:
                self.add_msg("SYSTEM", f"Error reading file: {e}", "bot")
        else:
            self.add_msg("SYSTEM", f"\U0001f4ce File loaded: {filename}", "bot")
        self.status.config(text=f"Loaded: {filename}")

    # ---------- send / process (reminders never swallow the real reply) ----------
    def _process(self, text):
        reply = self.brain.respond(text)
        if reply == "CLEAR_SESSION":
            self.root.after(0, self.clear_current)
            return
        if reply == "SAVE_CHAT":
            self.root.after(0, self.export_chat)
            return
        if reply == "LOAD_CHAT":
            self.root.after(0, self.import_chat)
            return

        def _update():
            self.add_msg("NEON", reply, "bot")
            self.memory.add_msg(self.chat_id, "bot", reply)
            self.brain.context.update(text, reply)
            self.status.config(text="\U0001f308 Ready")

        self.root.after(0, _update)

    def send(self):
        text = self.entry.get().strip()
        if not text:
            return
        if text.startswith('/'):
            self.handle_command(text)
            self.entry.delete(0, tk.END)
            return
        self.add_msg("YOU", text, "user")
        self.memory.add_msg(self.chat_id, "user", text)
        self.entry.delete(0, tk.END)
        self.status.config(text="\U0001f308 Thinking...")
        threading.Thread(target=self._process, args=(text,), daemon=True).start()

    def handle_command(self, cmd):
        parts = cmd.split()
        command = parts[0].lower()
        if command == '/help':
            self.show_help()
        elif command == '/clear':
            self.clear_current()
        elif command == '/new' and len(parts) > 1:
            name = ' '.join(parts[1:])
            self.chat_id = self.memory.create_chat(name)
            self._refresh_chats()
            self.load_history()
            self.status.config(text=f"New session: {name}")
        elif command == '/delete' and len(parts) > 1:
            try:
                idx = int(parts[1]) - 1
                cid = self._cid_for_selection(idx)
                if cid is not None:
                    self.memory.delete_chat(cid)
                    self._refresh_chats()
                    if self.chat_id == cid:
                        self.chat_id = self.memory.get_or_create_chat()
                        self.load_history()
                    self.status.config(text="Session deleted")
                else:
                    self.add_msg("SYSTEM", "Invalid session number.", "bot")
            except ValueError:
                self.add_msg("SYSTEM", "Provide a valid number.", "bot")
        elif command == '/rename' and len(parts) > 2:
            try:
                idx = int(parts[1]) - 1
                name = ' '.join(parts[2:])
                cid = self._cid_for_selection(idx)
                if cid is not None:
                    self.memory.rename_chat(cid, name)
                    self._refresh_chats()
                    self.status.config(text=f"Renamed to {name}")
                else:
                    self.add_msg("SYSTEM", "Invalid session number.", "bot")
            except ValueError:
                self.add_msg("SYSTEM", "Provide a valid number.", "bot")
        elif command == '/export':
            self.export_chat()
        elif command == '/load':
            self.import_chat()
        elif command == '/exit':
            self.root.quit()
        else:
            self.add_msg("SYSTEM", f"Unknown command: {command}. Type /help.", "bot")

    # ---------- reminders are polled and delivered independently of respond() ----------
    def _reminder_check(self):
        if self.settings.get("enable_reminders", True):
            for msg in self.brain.get_reminder_messages():
                self.add_msg("NEON", msg, "bot")
                self.memory.add_msg(self.chat_id, "bot", msg)
        self.root.after(2000, self._reminder_check)


def main():
    root = tk.Tk()
    NeonUI(root)
    root.mainloop()


if __name__ == "__main__":
    main()
