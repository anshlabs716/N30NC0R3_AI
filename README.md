# 🧠 N30NC0R3_AI

> **An AI assistant built from scratch in Python**
> *By AnshLabs716 & shozanthebozan*

[![Python](https://img.shields.io/badge/Python-3.6%2B-yellow?style=for-the-badge&logo=python&logoColor=white)](https://www.python.org/)
[![License](https://img.shields.io/badge/License-Apache%202.0-green?style=for-the-badge)](https://www.apache.org/licenses/LICENSE-2.0)
[![Platform](https://img.shields.io/badge/Platform-Linux-orange?style=for-the-badge&logo=linux&logoColor=white)](https://github.com/anshlabs716/N30NC0R3_AI)
[![Bash](https://img.shields.io/badge/Shell-Bash-4EAA25?style=for-the-badge&logo=gnubash&logoColor=white)](https://www.gnu.org/software/bash/)
---

## 📌 About

**N30NC0R3_AI** is a terminal-based AI assistant built from scratch using **Python and Bash**.

It combines a built-in knowledge base, natural-language command detection, web search, Wikipedia integration, utilities, and entertainment features into one lightweight CLI assistant.

### ✨ Highlights

* 🧠 Natural language understanding
* 📚 12+ built-in knowledge topics
* 🌐 DuckDuckGo web search
* 📖 Wikipedia integration
* 😂 Jokes and quotes
* 🎲 Dice rolling
* 🪙 Coin flipping
* 🔐 Password generation
* 🖥️ Clean colorful terminal interface
* 🐍 Python implementation
* 🐚 Bash implementation

---

## 🚀 Features

| Feature                   | Description                                                       |
| ------------------------- | ----------------------------------------------------------------- |
| 🧠 **Knowledge Base**     | Quick answers covering science, technology, and general knowledge |
| 🌐 **Web Search**         | Fetch live results from DuckDuckGo                                |
| 📚 **Wikipedia**          | Retrieve article summaries                                        |
| 😂 **Jokes & Quotes**     | Entertainment on demand                                           |
| 🎲 **Dice Roll**          | Supports d4, d6, d8, d10, d12, d20, etc.                          |
| 🪙 **Coin Flip**          | Flip heads or tails                                               |
| 🔐 **Password Generator** | Generate strong random passwords                                  |
| 🎨 **Terminal UI**        | Clean and colorful ANSI-based interface                           |
| 🕐 **Date & Time**        | Display the current date and time                                 |
| 💻 **System Info**        | Display information about the current user/system                 |

---

## 🧠 Natural Language Understanding

NEON uses pattern-based intent detection to recognize common requests.

Supported intents include:

* 👋 Greetings
* 🙂 Conversation
* 🤖 Identity questions
* 🕐 Time requests
* 📅 Date requests
* 😂 Jokes
* 💡 Quotes
* 🎲 Dice rolls
* 🪙 Coin flips
* 🔐 Password generation
* 📖 Definitions
* 🌐 Web searches

The intent system is designed to be expanded with additional patterns and commands.

---

## 📚 Knowledge Base

N30NC0R3_AI includes a built-in knowledge base containing **12+ topics**.

Topics cover areas such as:

* 🔬 Science
* 💻 Technology
* 🌎 General knowledge
* 📖 Definitions
* 🧠 Common questions

The knowledge base uses an extensible dictionary structure, making it easy to add additional topics.

---

## 🌐 Web Integration

NEON can retrieve live information from the web.

### DuckDuckGo

Search the web directly from the terminal using DuckDuckGo.

### Wikipedia

Retrieve summaries from Wikipedia articles without leaving the terminal.

> **Note:** Web results are retrieved from third-party services and may change or become unavailable.

---

## 🎮 Entertainment

NEON includes several fun commands.

### 😂 Jokes

```text
neon> tell me a joke

Why do programmers prefer dark mode?
Because light attracts bugs.
```

### 💡 Quotes

```text
neon> quote

[Motivational quote]
```

### 🎲 Dice

Supports different dice sizes such as:

```text
d4
d6
d8
d10
d12
d20
```

Example:

```text
neon> roll a d20

Rolled 1d20: 17
Sum: 17
```

### 🪙 Coin Flip

```text
neon> flip a coin

Heads!
```

---

## 🔐 Password Generator

NEON can generate random passwords.

Example:

```text
neon> generate password

Generated password: aB3#kL9$mN2@
```

> Generated passwords should be stored securely and not shared publicly.

---

## 🖥️ Terminal Interface

N30NC0R3_AI provides a simple, colorful CLI interface using ANSI terminal colors.

Example:

```text
╔══════════════════════════════════════╗
║          N30NC0R3_AI / NEON          ║
╚══════════════════════════════════════╝

neon> hello
Hello! I'm NEON. What can I help with?

neon>
```

---

# 📦 Requirements

## Python

Python **3.6 or newer** is required.

Check your version:

```bash
python3 --version
```

## Python Dependencies

```text
requests
beautifulsoup4
simplejson
```

Install them with:

```bash
pip install -r requirements.txt
```

## Optional Dependencies

| Dependency | Purpose                             |
| ---------- | ----------------------------------- |
| `tkinter`  | GUI version                         |
| `curl`     | Web requests for terminal workflows |
| `jq`       | JSON parsing                        |

> `tkinter` is normally provided through your operating system's Python packages rather than installed through `pip`.

---

# 🔧 Installation

## 🐚 Option 1 — Terminal Version

**Recommended**

```bash
git clone https://github.com/anshlabs716/N30NC0R3_AI.git
cd N30NC0R3_AI

chmod +x neon-cli.sh
./neon-cli.sh
```

## 🐍 Option 2 — Python Version

```bash
git clone https://github.com/anshlabs716/N30NC0R3_AI.git
cd N30NC0R3_AI

pip install -r requirements.txt
python3 neon_core.py
```

---

# 🖥️ Usage

Start NEON:

```bash
./neon-cli.sh
```

Or run the Python version:

```bash
python3 neon_core.py
```

---

## 💬 Example Session

```text
neon> hello
Hello! I'm NEON. What can I help with?

neon> what is gravity
Gravity: The force that attracts two bodies toward each other.

neon> tell me a joke
Why do programmers prefer dark mode?
Because light attracts bugs.

neon> roll a d20
Rolled 1d20: 17 (Sum: 17)

neon> generate password
Generated password: aB3#kL9$mN2@

neon> exit
Goodbye!
```

---

# 💻 Commands

| Command       | Description              |
| ------------- | ------------------------ |
| `help`        | Show available commands  |
| `exit`        | Exit NEON                |
| `clear`       | Clear the terminal       |
| `time`        | Show the current time    |
| `date`        | Show the current date    |
| `version`     | Show version information |
| `whoami`      | Show the current user    |
| `joke`        | Tell a joke              |
| `quote`       | Show a quote             |
| `roll d20`    | Roll a d20               |
| `flip`        | Flip a coin              |
| `password`    | Generate a password      |
| `what is ...` | Ask a knowledge question |
| `[any text]`  | Chat with NEON           |

---

# 🧩 Intent Detection

| Intent             | Example Patterns                 |
| ------------------ | -------------------------------- |
| 👋 **Greeting**    | `hi`, `hello`, `hey`, `yo`       |
| 🙂 **How Are You** | `how are you`, `how do you do`   |
| 🤖 **Who Are You** | `who are you`, `your name`       |
| 🕐 **Time**        | `time`, `what time`              |
| 📅 **Date**        | `date`, `what date`              |
| 😂 **Joke**        | `joke`, `funny`, `make me laugh` |
| 💡 **Quote**       | `quote`, `inspire`, `motivation` |
| 🎲 **Roll**        | `roll`, `dice`, `d20`            |
| 🪙 **Flip**        | `flip`, `coin`, `heads`, `tails` |
| 🔐 **Password**    | `password`, `pass`, `generate`   |
| 📖 **Define**      | `define`, `what is`, `what are`  |

---

# 📁 Project Structure

```text
N30NC0R3_AI/
│
├── neon-cli.sh
├── neon_core.py
├── requirements.txt
├── README.md
├── CHANGELOG.md
├── CONTRIBUTING.md
├── LICENSE
│
└── docs/
    ├── MIGRATION_SUMMARY.md
    └── ALL_MARKDOWN.md
```

| File               | Purpose                    |
| ------------------ | -------------------------- |
| `neon-cli.sh`      | Bash CLI implementation    |
| `neon_core.py`     | Python implementation      |
| `requirements.txt` | Python dependencies        |
| `README.md`        | Main project documentation |
| `CHANGELOG.md`     | Version history            |
| `CONTRIBUTING.md`  | Contribution guide         |
| `LICENSE`          | Apache 2.0 license         |
| `docs/`            | Additional documentation   |

---

# 🛠️ Technical Stack

| Component        | Technology                      |
| ---------------- | ------------------------------- |
| 🐍 Main Language | Python 3.6+                     |
| 🐚 CLI Version   | Bash                            |
| 🖥️ GUI          | Tkinter                         |
| 🌐 HTTP Requests | `requests`, `urllib`            |
| 📄 JSON          | `json`, `simplejson`            |
| 🌐 HTML Parsing  | `beautifulsoup4`                |
| 🎨 Terminal      | ANSI colors                     |
| 🔎 Web Search    | DuckDuckGo                      |
| 📚 Knowledge     | Built-in dictionary + Wikipedia |

---

# 🎯 Deployment Targets

| Platform    | Format        | Status         |
| ----------- | ------------- | -------------- |
| 🐧 Linux    | Bash script   | ✅ Available    |
| 🐧 Linux    | Python script | ✅ Available    |
| 📱 Android  | `.apk`        | 🚧 Coming Soon |
| 🪟 Windows  | `.exe`        | 🚧 Coming Soon |
| 🐧 Linux    | `.AppImage`   | 🚧 Coming Soon |
| 🖥️ Desktop | Tkinter GUI   | 🚧 Coming Soon |

---

# 🔮 Roadmap

* [x] Initial terminal version
* [x] Python implementation
* [x] Bash implementation
* [x] Knowledge base
* [x] DuckDuckGo search
* [x] Wikipedia integration
* [x] Jokes
* [x] Quotes
* [x] Dice rolling
* [x] Coin flipping
* [x] Password generator
* [x] Colorful CLI

### Coming Soon

* [ ] 📱 Android `.apk`
* [ ] 🪟 Windows `.exe`
* [ ] 🐧 Linux `.AppImage`
* [ ] 🖥️ Tkinter GUI
* [ ] 🎤 Voice input/output
* [ ] 🧠 More knowledge topics
* [ ] 🔌 Custom plugin system
* [ ] ⚙️ More commands
* [ ] 💾 Persistent conversation history

---

# 🤝 Contributing

Contributions are welcome!

## 1. Fork the Repository

Fork the project on GitHub.

## 2. Clone Your Fork

```bash
git clone https://github.com/anshlabs716/N30NC0R3_AI.git
cd N30NC0R3_AI
```

## 3. Create a Feature Branch

```bash
git checkout -b feature/amazing
```

## 4. Make Your Changes

Keep your code clean, readable, and documented.

## 5. Commit

```bash
git add .
git commit -m "Add amazing feature"
```

## 6. Push

```bash
git push origin feature/amazing
```

Then open a pull request.

---

# 📝 Contribution Guidelines

When contributing:

* Keep code clean and readable
* Add comments for complex logic
* Update documentation when adding features
* Test changes on at least two operating systems when possible
* Keep commits focused
* Use clear commit messages
* Avoid unnecessary dependencies

---

# 🐛 Bug Reports

Before submitting a bug report:

1. Check whether the issue already exists.
2. Include your operating system.
3. Include your Python version.
4. Include the relevant error message.
5. Include steps to reproduce the problem.

### Example

```text
OS: Arch Linux
Python: 3.13
Version: N30NC0R3_AI 1.0.0

Steps:
1. Start NEON
2. Run `roll d20`
3. Error occurs
```

---

# 💡 Feature Requests

Have an idea for NEON?

Open an issue and describe:

* What you want added
* Why it would be useful
* How you think it should work
* Any examples or references

---

# 📝 Changelog

## [1.0.0] — 2026-08-17

### Added

* Initial release
* Knowledge base with 12+ topics
* Natural-language intent detection
* DuckDuckGo web search
* Wikipedia integration
* Jokes
* Quotes
* Dice rolling
* Coin flipping
* Password generator
* Colorful CLI
* Bash version
* Python version

### Planned

* Android `.apk`
* Windows `.exe`
* Linux `.AppImage`
* GUI version
* Voice input/output
* More knowledge topics
* Plugin system

---

# 📄 Migration Summary

## v0.1 → v1.0

| Feature            | v0.1      | v1.0                  |
| ------------------ | --------- | --------------------- |
| Interface          | Basic     | Clean CLI with colors |
| Knowledge          | Hardcoded | Extensible dictionary |
| Web Search         | ❌         | ✅                     |
| Wikipedia          | ❌         | ✅                     |
| Jokes              | ❌         | ✅                     |
| Quotes             | ❌         | ✅                     |
| Dice               | ❌         | ✅                     |
| Coin Flip          | ❌         | ✅                     |
| Password Generator | ❌         | ✅                     |
| Colors             | ❌         | ✅                     |

### Migration

```bash
git clone https://github.com/anshlabs716/N30NC0R3_AI.git
cd N30NC0R3_AI

chmod +x neon-cli.sh
./neon-cli.sh
```

### New Dependencies

```bash
pip install requests beautifulsoup4 simplejson
```

---

# ⚠️ Disclaimer

N30NC0R3_AI is an open-source hobbyist project provided **"AS IS"** under the Apache 2.0 License.

The software may automatically retrieve information from third-party services and websites, including Wikipedia and DuckDuckGo.

We do not control, filter, endorse, or guarantee the accuracy, availability, legality, or reliability of third-party content.

The developers are not responsible for information retrieved through external services or for how the software is used.

---

# 📬 Contact

| Developer          | Email                     | Discord              |
| ------------------ | ------------------------- | -------------------- |
| **shozanthebozan** | `kmoruihrdp@hotmail.com`  | —                    |
| **AnshLabs716**    | `bhatiaansh716@gmail.com` | `veryfastcar2_07525` |

---

# 📜 License

Copyright © 2026 **AnshLabs716 & shozanthebozan**

N30NC0R3_AI is licensed under the **Apache License 2.0**.

See the [`LICENSE`](LICENSE) file for the complete license text.

You may obtain a copy of the license from:

https://www.apache.org/licenses/LICENSE-2.0

The software is distributed on an **"AS IS"** basis, without warranties or conditions of any kind.

---

# ⭐ Support

If you like **N30NC0R3_AI**, consider giving the repository a ⭐ on GitHub!

**Built with ❤️ by AnshLabs716 & shozanthebozan**
