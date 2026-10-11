# NEON CORE AI v3.0

A C-based AI chat assistant with local LLM, web search, ASCII art, and TTS.

NEON CORE AI is a native desktop chat assistant written entirely in C11. It
runs a local large language model either directly through llama.cpp or through a
local Ollama server, falls back to an embedded offline knowledge base for many
queries, can search the web, renders coloured ASCII art, and speaks its replies
aloud.

## Quick start (how to run it)

You need a Linux computer (Ubuntu / Debian style) and internet for the first
build. Copy-paste these commands one block at a time.

**1. Install the helper programs it needs**

```bash
sudo apt update
sudo apt install -y build-essential make pkg-config cmake git \
    libgtk-3-dev libcurl4-openssl-dev libespeak-ng-dev
```

**2. Download the app**

```bash
git clone https://github.com/anshlabs716/N30NC0R3_AI.git
cd N30NC0R3_AI
```

**3. Build it**

```bash
make
```

The first time, this also downloads and builds the AI engine (llama.cpp), so it
can take a few minutes. After that it is fast.

**4. Start it**

```bash
./neon_core
```

A window opens. That is the app.

**5. Give it a brain (only the first time)**

The app needs a small AI model to think with. Easiest way: click the **Get LLM**
button at the top of the window. It installs Ollama and downloads the AI model
for you, showing progress. When it finishes, close the app and start it again
with `./neon_core`.

Prefer to do it by hand? Install Ollama from <https://ollama.com>, then run:

```bash
ollama pull qwen3:1.7b
```

(You can also drop a `.gguf` model file into the `models/` folder instead — see
[Model setup](#model-setup).)

**Useful extras**

```bash
make clean-llama   # delete the downloaded AI engine and build it fresh
make clean         # delete compiled files (keeps the AI engine)
```

## How it works (the simple version)

Think of the app as a team of little workers inside one window. Each worker is a
separate `.c` file, and they only do one job each:

| Worker (file)      | What it does, in plain words |
|--------------------|------------------------------|
| `ui.c`             | The **face**. Draws the window, buttons and chat bubbles. |
| `brain.c`          | The **receptionist**. Reads your message and decides who should handle it. |
| `llm.c`            | The **big brain**. The AI model that actually writes sentences. |
| `web.c`            | The **librarian**. Looks things up on the internet. |
| `ascii_art.c`      | The **artist**. Draws pictures using keyboard characters. |
| `tts.c`            | The **voice**. Reads the answer out loud. |
| `storage.c`        | The **notebook**. Remembers your chats. |
| `settings.c`       | The **control panel**. Colours, fonts and preferences. |
| `cJSON.c`          | The **translator**. Reads and writes the save files in JSON. |

### What happens when you type a message

1. You type something and press Enter. The **face** (`ui.c`) passes it to the
   **receptionist** (`brain.c`).
2. The receptionist looks for clues. A maths question, a joke, a reminder or an
   ASCII picture it can handle **by itself, instantly**.
3. If the message is open-ended ("why is the sky blue?"), the receptionist asks
   the **big brain** (`llm.c`).
4. The big brain writes the reply **one word at a time** — like the autocomplete
   on a phone that keeps guessing the next word. That is all a "large language
   model" really does, just much better and much faster.
5. If the answer needs fresh facts, the **librarian** (`web.c`) searches the web.
6. The answer appears in the chat, maybe with colour or ASCII art, and the
   **voice** (`tts.c`) can read it aloud. The **notebook** (`storage.c`) saves
   the conversation for next time.

### The two "brains"

The big brain can run in two ways; the app picks whichever is ready, in order:

1. **llama.cpp** — the AI engine is built right inside the project and reads a
   `.gguf` model file from `models/`. Everything runs on your own computer.
2. **Ollama** — if there is no model file, the app talks to a local Ollama
   server (`qwen3:1.7b`) through the internet port on your own machine. The
   **Get LLM** button sets this one up.

Either way, **nothing is sent to a company's servers** — the thinking happens on
your computer. If neither brain is available, the app still works using its
built-in offline knowledge and local tools.

## Features

- **Local LLM inference** via llama.cpp, with an automatic fallback to a local
  Ollama server running Qwen3 1.7B.
- **One-click LLM setup**: the **Get LLM** button installs Ollama and pulls
  `qwen3:1.7b`, streaming the installer output in a dialog.
- **Intent router** built on POSIX regular expressions that answers common
  requests locally and forwards open-ended ones to the model.
- **GTK 3 interface** with a header bar, session sidebar, chat view and status
  bar.
- **Web search** through DuckDuckGo (HTML endpoint) using libcurl.
- **ASCII art engine** with a built-in 7-row font, pattern generators and
  7-colour markers for the GUI.
- **Text-to-speech** using libespeak-ng, with configurable rate and pitch.
- **Session storage** persisted as JSON (atomic writes) in `~/.neon_core_ai/`.
- **12 colour themes** loaded from `assets/themes.json`.
- **Reminders, to-dos and notes**, jokes, quotes, facts, poems, stories and
  "battle" generators.
- **Code-block highlighting** and coloured ASCII rendering in the chat view.

## Dependencies

| Library      | Purpose                      | Typical package            |
|--------------|------------------------------|----------------------------|
| llama.cpp    | Direct local LLM inference   | fetched by the Makefile    |
| Ollama       | Alternative local LLM server | installed by **Get LLM**   |
| GTK 3        | Graphical interface          | `libgtk-3-dev`             |
| libcurl      | Web search and Ollama HTTP   | `libcurl4-openssl-dev`     |
| libespeak-ng | Text-to-speech               | `libespeak-ng-dev`         |
| cJSON        | JSON parsing (embedded)      | bundled in `src/`          |
| libc / libm  | Runtime and maths            | base system                |

You also need a C11 compiler (`gcc`), `make`, `pkg-config` and `cmake` (used to
build llama.cpp).

## Build

```bash
make
```

The first build fetches llama.cpp into `./llama.cpp`, compiles it, and then
links the application. The result is the `neon_core` executable in the project
root. Run `make clean-llama` to remove the fetched llama.cpp and start over.

## Model setup

NEON CORE AI uses whichever of two local backends it finds, in this order:

1. **llama.cpp (direct).** Place a GGUF file in `models/`. The default expected
   filename is:

   ```
   models/qwen3-1.7b-q4_k_m.gguf
   ```

   The loader also accepts `models/Qwen3-1.7B-Q4_K_M.gguf` and
   `models/Qwen3-1.7B-Q4_K_M-00001-of-00001.gguf`.

2. **Ollama.** If no GGUF is present, the app looks for a local Ollama server
   at `http://127.0.0.1:11434` with the `qwen3:1.7b` model pulled. You can set
   this up entirely from the app: click the **Get LLM** button in the header
   bar. It runs the Ollama installer and `ollama pull qwen3:1.7b`, streaming
   the progress into a dialog. Restart the app afterwards.

If neither backend is available the assistant still runs, using its embedded
offline knowledge and local tools.

## Usage

```bash
./neon_core
```

Type a message and press Enter (or click **Send**). Useful commands:

| Command                                                        | Result                       |
|----------------------------------------------------------------|------------------------------|
| `help`                                                         | list commands                |
| `what is gravity`                                              | knowledge-base answer        |
| `calculate 12 * 8 + 4`                                         | arithmetic                   |
| `generate a password 20`                                       | random password              |
| `roll 2d20` / `flip a coin`                                    | dice / coin                  |
| `remind me in 5 minutes to stretch`                            | reminder                     |
| `ascii art of NEON` / `pattern diamond`                        | ASCII art                    |
| `joke`, `quote`, `fact`, `poem`, `story`, `battle`             | content generators           |
| `search for black holes`, `wikipedia ...`, `news`              | web-backed answers           |
| anything else                                                  | routed to the local LLM      |

## Project layout

```
.
├── Makefile
├── README.md
├── assets/
│   └── themes.json
├── models/
│   └── .gitkeep
└── src/
    ├── main.c          application entry point
    ├── ui.c / ui.h     GTK layout, chat rendering, session list, themes
    ├── brain.c / .h    intent router, knowledge base, content libraries
    ├── llm.c / .h      llama.cpp and Ollama backends, sampling and streaming
    ├── web.c / .h      DuckDuckGo search and HTML parsing
    ├── ascii_art.c/.h  font, ASCII art and pattern generation
    ├── tts.c / .h      libespeak-ng speech wrapper
    ├── storage.c / .h  JSON-backed chat history persistence
    ├── settings.c / .h settings dialog, themes and preferences
    └── cJSON.c / .h    embedded JSON library
```

## Themes

Themes are defined in `assets/themes.json` and selected from the settings
dialog. The palette includes Neon Orange, Pink, Green, Blue, Red, Purple and
Yellow, the Soft Night family, Retro Terminal and Matrix.

## Configuration

Settings and chat history are stored under `~/.neon_core_ai/`:

- `settings.json` - theme, font size, ASCII style, personality, voice and
  reminder preferences.
- `history.json` - chat sessions and their messages.

## License

Released under the Apache License 2.0.
