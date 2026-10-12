# NEON CORE AI v3.0

A C-based AI chat assistant with local LLM, web search, ASCII art, and TTS.

NEON CORE AI is a native desktop chat assistant written entirely in C11. It
runs a local large language model through llama.cpp, falls back to an embedded
offline knowledge base for many queries, can search the web, renders coloured
ASCII art, and speaks its replies aloud.

## Features

- **Local LLM inference** via llama.cpp (Qwen3 1.7B Q4_K_M by default).
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

| Library      | Purpose                     | Typical package            |
|--------------|-----------------------------|----------------------------|
| llama.cpp    | Local LLM inference         | fetched by the Makefile    |
| GTK 3        | Graphical interface         | `libgtk-3-dev`             |
| libcurl      | Web search                  | `libcurl4-openssl-dev`     |
| libespeak-ng | Text-to-speech              | `libespeak-ng-dev`         |
| cJSON        | JSON parsing (embedded)     | bundled in `src/`          |
| libc / libm  | Runtime and maths           | base system                |

You also need a C11 compiler (`gcc`), `make`, `pkg-config` and `cmake` (used to
build llama.cpp).

## Build

```bash
make
```

The first build fetches llama.cpp, compiles it, and then links the application.
The result is the `neon_core` executable in the project root.

## Model setup

Place a GGUF model in `models/`. The default expected filename is:

```
models/qwen3-1.7b-q4_k_m.gguf
```

The loader also accepts `models/Qwen3-1.7B-Q4_K_M.gguf` and
`models/Qwen3-1.7B-Q4_K_M-00001-of-00001.gguf`. If no model is present the
assistant still runs, using its embedded offline knowledge and local tools.

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
    ├── llm.c / .h      llama.cpp model loading, sampling and streaming
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

- `settings.json` – theme, font size, ASCII style, personality, voice and
  reminder preferences.
- `history.json` – chat sessions and their messages.

## License

Released under the Apache License 2.0. See `LICENSE`.
