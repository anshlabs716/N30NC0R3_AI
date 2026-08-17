# 🧠 N30NC0R3_AI
> **An AI assistant built from scratch in Python**  
> *By AnshLabs716 & shozanthebozan*

## 📌 About
N30NC0R3_AI is a terminal-based AI assistant that provides:
- ✅ Natural language understanding
- ✅ Knowledge base with 12+ topics
- ✅ Web search via DuckDuckGo
- ✅ Wikipedia integration
- ✅ Fun commands (jokes, quotes, dice, coin flip)
- ✅ Password generation
- ✅ Clean CLI interface with colors

## 🚀 Features
| Feature | Description |
|---------|-------------|
| **Knowledge Base** | Quick answers on science, tech, and more |
| **Web Search** | Fetches live results from DuckDuckGo |
| **Wikipedia** | Retrieves article summaries |
| **Jokes & Quotes** | Entertainment on demand |
| **Dice Roll** | Supports d4, d6, d20, etc. |
| **Coin Flip** | Heads or tails |
| **Password Gen** | Generate strong passwords |
| **Terminal UI** | Clean, colorful CLI |

## 📦 Dependencies
```bash
# Required
python 3.6+
tkinter  # For GUI version (optional)

# For web search
curl   # For terminal version
jq     # For JSON parsing (optional)

🔧 Installation
bash

# Option 1: Terminal Version (Recommended)
git clone https://github.com/anshlabs716/N30NC0R3_AI.git
cd N30NC0R3_AI
chmod +x neon-cli.sh
./neon-cli.sh

# Option 2: Python Version
pip install -r requirements.txt
python3 neon_core.py

🖥️ Usage
bash

./neon-cli.sh

Example Commands
bash

neon> hello
Hello! I'm NEON. What can I help with?

neon> what is gravity
Gravity: The force that attracts two bodies toward each other.

neon> tell me a joke
Why do programmers prefer dark mode? Because light attracts bugs.

neon> roll a d20
Rolled 1d20: 17 (Sum: 17)

neon> generate password
Generated password: aB3#kL9$mN2@

neon> exit
Goodbye!

📁 Project Structure
text

N30NC0R3_AI/
├── neon-cli.sh          # Bash CLI version
├── neon_core.py         # Python version
├── requirements.txt     # Python dependencies
├── README.md            # This file
├── CHANGELOG.md         # Version history
├── CONTRIBUTING.md      # Contribution guide
├── LICENSE              # Apache 2.0
└── docs/
    ├── MIGRATION_SUMMARY.md
    └── ALL_MARKDOWN.md

🛠️ Coming Soon

    □

    Android (.apk) support
    □

    Windows (.exe) support
    □

    Linux (.AppImage) support
    □

    GUI version (Tkinter)
    □

    Voice input/output
    □

    More knowledge topics
    □

    Custom plugins

🤝 Contributing

We welcome contributions!
bash

# 1. Fork the repo
# 2. Create a feature branch
git checkout -b feature/amazing
# 3. Commit your changes
git commit -m 'Add amazing feature'
# 4. Push and open a PR
git push origin feature/amazing

📬 Contact
Person	Email	Discord
shozanthebozan	kmoruihrdp@hotmail.com	-
AnshLabs716 (Ansh Bhatia)	bhatiaansh716@gmail.com	veryfastcar2_07525
⚠️ Disclaimer

N30NC0R3_AI is an open-source, hobbyist project provided "AS IS" under the Apache 2.0 License. This software automatically retrieves summaries and search snippets directly from live third-party web pages (such as Wikipedia) via public APIs. We do not filter, endorse, or assume any responsibility or liability for the accuracy, lawfulness, or content of the data fetched from the web. Use at your own risk.
📄 License
text

Apache License 2.0
Copyright 2026 AnshLabs716 & shozanthebozan
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

⭐ Support

If you like this project, please give it a ⭐ on GitHub!

Built with ❤️ by AnshLabs716 & shozanthebozan
🤝 Contributing to N30NC0R3_AI

We love contributions! Here's how you can help:
🐛 Report Bugs

    Check if the bug already exists in Issues

    Include your OS, Python version, and error logs

    Provide steps to reproduce

💡 Suggest Features

    Open an Issue with the enhancement label

    Describe the feature and why it's useful

🔧 Submit Code
bash

git clone https://github.com/anshlabs716/N30NC0R3_AI.git
cd N30NC0R3_AI
git checkout -b feature/amazing
git commit -m 'Add amazing feature'
git push origin feature/amazing

📝 Guidelines

    Keep code clean and readable

    Add comments for complex logic

    Update README.md if adding features

    Test on at least 2 different OSes

📝 Changelog
[1.0.0] - 2026-08-17

Added

    Initial release

    Knowledge base with 12+ topics

    Web search via DuckDuckGo

    Wikipedia integration

    Jokes, quotes, dice roll, coin flip

    Password generator

    Clean CLI interface

    Bash and Python versions

Coming Soon

    Android .apk support

    Windows .exe support

    Linux .AppImage support

    GUI version

📦 requirements.txt
txt

# N30NC0R3_AI Dependencies
# Python 3.6+

# For GUI version (optional)
tkinter

# For web search
requests
beautifulsoup4

# For JSON parsing
simplejson

📁 ALL_MARKDOWN.md - Complete Documentation
Project Overview

N30NC0R3_AI is a collaborative project between AnshLabs716 and shozanthebozan. It's an AI assistant built from scratch in Python, designed to run in the terminal with a clean, colorful interface.
Core Features

    Natural Language Understanding: Intent detection using regex patterns, support for multiple query types, context-aware responses

    Knowledge Base: 12+ built-in topics, science/technology/general knowledge, extensible dictionary

    Web Integration: DuckDuckGo search via API, Wikipedia article summaries, live data fetching

    Entertainment: Jokes database, motivational quotes, dice rolling (d4, d6, d20, etc.), coin flip

    Utilities: Password generation, date/time display, system information

Technical Stack
Component	Technology
Language	Python 3.6+, Bash
GUI	Tkinter (optional)
Web Requests	requests, urllib
JSON Parsing	json, simplejson
HTML Parsing	beautifulsoup4
Terminal	ANSI colors
Commands Reference
Command	Description
help	Show all commands
exit	Exit the program
clear	Clear screen
time	Show current time
date	Show current date
version	Show version info
whoami	Show current user
[any text]	Chat with AI
Intent Detection Patterns
Intent	Pattern
Greeting	hi, hello, hey, yo
How are you	how are you, how do you do
Who are you	who are you, your name
Time	time, what time
Date	date, what date
Joke	joke, funny, make me laugh
Quote	quote, inspire, motivation
Roll	roll, dice, d20
Flip	flip, coin, heads, tails
Password	password, pass, generate
Define	define, what is, what are
Deployment Targets
Platform	Format	Status
Linux	Bash script	✅ Available
Linux	Python script	✅ Available
Android	.apk	🚧 Coming soon
Windows	.exe	🚧 Coming soon
Linux	.AppImage	🚧 Coming soon
MIGRATION_SUMMARY.md
Migration from v0.1 to v1.0
Feature	v0.1	v1.0
Interface	Basic	Clean CLI with colors
Knowledge	Hardcoded	Extensible dictionary
Web Search	❌	✅ DuckDuckGo
Wikipedia	❌	✅ API integration
Jokes	❌	✅ 10+ jokes
Quotes	❌	✅ 5+ quotes
Dice Roll	❌	✅ d4, d6, d20
Coin Flip	❌	✅ Heads/Tails
Password	❌	✅ Generator
Colors	❌	✅ ANSI support
Migration Steps
bash

git clone https://github.com/anshlabs716/N30NC0R3_AI.git
cd N30NC0R3_AI
chmod +x neon-cli.sh
./neon-cli.sh

New Dependencies
bash

pip install requests beautifulsoup4 simplejson

Migration complete! Your AI is now better than ever. 🚀
text

