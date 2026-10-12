# NEON CORE AI v3.0

A C-based AI chat assistant with local LLM, web search, ASCII art, and TTS.

## Features

- Local LLM inference powered by llama.cpp
- GTK 3 graphical user interface
- Web search via DuckDuckGo
- 7-color ASCII art generator with pattern support
- Text-to-speech using libespeak-ng
- Chat session management and persistence
- Multiple color themes
- Code highlighting in chat
- Reminders and to-do support

## Installation

### Prerequisites

- gcc with C11 support
- cmake
- GTK 3 development libraries (`gtk+-3.0-dev` or equivalent)
- libcurl development libraries
- espeak-ng development libraries
- pthread support

### Build

```bash
make
```

This will build llama.cpp first, then compile the application.

## Model Setup

Download the Qwen3 1.7B Q4_K_M GGUF model and place it in the `models/` directory:

```bash
mkdir -p models
# Download model from huggingface.co or other source
# Place qwen3-1.7b-q4_k_m.gguf in models/
```

## Usage

```bash
./neon_core
```

## Controls

- Type in the input box and press Enter or click Send
- Left panel: manage chat sessions
- Settings button: configure themes, font, personality, etc.
- ASCII button: generate ASCII art

## Themes

Supports 12 different themes including Neon variants, Soft Night, Retro Terminal, and Matrix.

## Troubleshooting

If the model fails to load, ensure the GGUF file is in the `models/` directory with the correct filename.

