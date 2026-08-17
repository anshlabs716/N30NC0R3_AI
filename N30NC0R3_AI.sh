#!/usr/bin/env bash

# NEON-CLI - Terminal AI Assistant
# Version: 1.0
# Author: AnshLabs716

# Colors
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
BOLD='\033[1m'
RESET='\033[0m'

# Knowledge Base
declare -A KNOWLEDGE
KNOWLEDGE["gravity"]="Gravity is the force that attracts two bodies toward each other."
KNOWLEDGE["photosynthesis"]="Photosynthesis is the process by which plants use sunlight to synthesize foods."
KNOWLEDGE["black hole"]="A black hole is a region where gravity is so strong nothing can escape."
KNOWLEDGE["quantum"]="Quantum mechanics describes physical properties at the atomic scale."
KNOWLEDGE["dna"]="DNA carries the genetic instructions for all known organisms."
KNOWLEDGE["internet"]="The Internet is a global network of interconnected computers."
KNOWLEDGE["python"]="Python is a high-level programming language known for its clear syntax."
KNOWLEDGE["jupiter"]="Jupiter is the largest planet in our solar system."
KNOWLEDGE["mars"]="Mars is the fourth planet from the Sun, known as the Red Planet."
KNOWLEDGE["moon"]="The Moon is Earth's only natural satellite."
KNOWLEDGE["sun"]="The Sun is the star at the center of our solar system."
KNOWLEDGE["earth"]="Earth is the third planet from the Sun and the only known planet to harbor life."
KNOWLEDGE["water"]="Water is the main constituent of Earth's streams, lakes, and oceans."

# Jokes
JOKES=(
    "Why do programmers prefer dark mode? Because light attracts bugs."
    "What do you call a fake noodle? An impasta."
    "How many programmers does it take to change a light bulb? None, that's a hardware problem."
    "Why did the scarecrow win an award? Because he was outstanding in his field."
)

# Quotes
QUOTES=(
    "The only way to do great work is to love what you do. - Steve Jobs"
    "Be yourself; everyone else is already taken. - Oscar Wilde"
    "In the middle of difficulty lies opportunity. - Albert Einstein"
)

# ============================================
# Show Banner
# ============================================
show_banner() {
    clear
    echo -e "${CYAN}${BOLD}"
    echo "  +-----------------------------------------+"
    echo "  |                                         |"
    echo "  |     NEON-CLI v1.0                       |"
    echo "  |     Terminal AI Assistant               |"
    echo "  |     By AnshLabs716                      |"
    echo "  |                                         |"
    echo "  |  Type 'help' for commands               |"
    echo "  |  Type 'exit' to quit                    |"
    echo "  |                                         |"
    echo "  +-----------------------------------------+"
    echo -e "${RESET}"
}

# ============================================
# Help
# ============================================
show_help() {
    echo -e "${GREEN}${BOLD}Commands:${RESET}"
    echo ""
    echo -e "${CYAN}  help${RESET}      - Show this help"
    echo -e "${CYAN}  exit${RESET}      - Exit the program"
    echo -e "${CYAN}  clear${RESET}     - Clear screen"
    echo -e "${CYAN}  time${RESET}      - Show current time"
    echo -e "${CYAN}  date${RESET}      - Show current date"
    echo -e "${CYAN}  whoami${RESET}    - Show who you are"
    echo -e "${CYAN}  version${RESET}   - Show version info"
    echo ""
    echo -e "${YELLOW}Just type anything to chat!${RESET}"
}

# ============================================
# Time/Date
# ============================================
show_time() {
    echo -e "${BLUE}Time: $(date '+%I:%M %p')${RESET}"
}

show_date() {
    echo -e "${BLUE}Date: $(date '+%A, %B %d, %Y')${RESET}"
}

show_version() {
    echo -e "${CYAN}NEON-CLI v1.0${RESET}"
    echo -e "${DIM}Built by AnshLabs716${RESET}"
}

# ============================================
# AI Response
# ============================================
ask_ai() {
    local query="$1"
    local q=$(echo "$query" | tr '[:upper:]' '[:lower:]')
    
    # Greeting
    if echo "$q" | grep -qE "^(hi|hello|hey|yo|sup)"; then
        echo -e "${GREEN}Hello! I'm NEON. What can I help with?${RESET}"
        return
    fi
    
    # How are you
    if echo "$q" | grep -qE "how are you|how do you do|how is it going"; then
        echo -e "${CYAN}I'm great, thanks for asking!${RESET}"
        return
    fi
    
    # Who are you
    if echo "$q" | grep -qE "who are you|your name|what is your name"; then
        echo -e "${CYAN}I'm NEON, your terminal AI assistant.${RESET}"
        return
    fi
    
    # Time
    if echo "$q" | grep -qE "time|what time|current time"; then
        show_time
        return
    fi
    
    # Date
    if echo "$q" | grep -qE "date|what date|today"; then
        show_date
        return
    fi
    
    # Joke
    if echo "$q" | grep -qE "joke|make me laugh|funny|humor"; then
        local idx=$((RANDOM % ${#JOKES[@]}))
        echo -e "${YELLOW}${JOKES[$idx]}${RESET}"
        return
    fi
    
    # Quote
    if echo "$q" | grep -qE "quote|inspire|motivation|wise"; then
        local idx=$((RANDOM % ${#QUOTES[@]}))
        echo -e "${MAGENTA}${QUOTES[$idx]}${RESET}"
        return
    fi
    
    # Roll dice
    if echo "$q" | grep -qE "roll|dice|d[0-9]+"; then
        if echo "$q" | grep -qE "[0-9]+d[0-9]+"; then
            local dice=$(echo "$q" | grep -oE "[0-9]+d[0-9]+")
            local num=$(echo "$dice" | cut -d'd' -f1)
            local sides=$(echo "$dice" | cut -d'd' -f2)
            [[ -z "$num" ]] && num=1
            [[ -z "$sides" ]] && sides=6
            local total=0
            local results=""
            for ((i=0; i<num; i++)); do
                local roll=$((RANDOM % sides + 1))
                total=$((total + roll))
                results="$results $roll"
            done
            echo -e "${CYAN}Rolled ${num}d${sides}:${results} (Sum: $total)${RESET}"
        else
            echo -e "${CYAN}You rolled a $((RANDOM % 6 + 1))${RESET}"
        fi
        return
    fi
    
    # Flip coin
    if echo "$q" | grep -qE "flip|coin|heads|tails"; then
        if [[ $((RANDOM % 2)) -eq 0 ]]; then
            echo -e "${GREEN}Heads${RESET}"
        else
            echo -e "${CYAN}Tails${RESET}"
        fi
        return
    fi
    
    # Password
    if echo "$q" | grep -qE "password|pass|generate"; then
        local length=12
        local chars="abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789!@#$%^&*()"
        local pwd=""
        for ((i=0; i<length; i++)); do
            pwd="${pwd}${chars:$((RANDOM % ${#chars})):1}"
        done
        echo -e "${YELLOW}Generated password: ${pwd}${RESET}"
        return
    fi
    
    # Define / What is
    if echo "$q" | grep -qE "^(define|what is|what are|whats|what'?s)"; then
        local term=$(echo "$q" | sed -E 's/^(define|what is|what are|whats|what'\''s)\s+//' | sed 's/?$//' | tr -d ' ')
        if [[ -n "${KNOWLEDGE[$term]}" ]]; then
            echo -e "${GREEN}${term^}: ${KNOWLEDGE[$term]}${RESET}"
        else
            echo -e "${YELLOW}I don't have info on '${term}'.${RESET}"
        fi
        return
    fi
    
    # Fact
    if echo "$q" | grep -qE "fact|did you know|tell me"; then
        local keys=($(echo "${!KNOWLEDGE[@]}"))
        local key="${keys[$((RANDOM % ${#keys[@]}))]}"
        echo -e "${BLUE}Did you know? ${KNOWLEDGE[$key]}${RESET}"
        return
    fi
    
    # Check if query matches any knowledge
    for key in "${!KNOWLEDGE[@]}"; do
        if echo "$q" | grep -q "$key"; then
            echo -e "${GREEN}${key^}: ${KNOWLEDGE[$key]}${RESET}"
            return
        fi
    done
    
    # Default
    echo -e "${YELLOW}I'm not sure about that. Try 'help' to see what I can do.${RESET}"
}

# ============================================
# MAIN LOOP
# ============================================
main() {
    show_banner
    
    while true; do
        echo ""
        echo -ne "${BOLD}${CYAN}neon> ${RESET}"
        read -r input
        
        [[ -z "$input" ]] && continue
        
        case "$input" in
            help|--help|-h)
                show_help
                ;;
            exit|quit|q)
                echo -e "${GREEN}Goodbye!${RESET}"
                break
                ;;
            clear)
                show_banner
                ;;
            time)
                show_time
                ;;
            date)
                show_date
                ;;
            version|--version|-v)
                show_version
                ;;
            whoami)
                echo -e "${GREEN}You are $(whoami)${RESET}"
                ;;
            *)
                ask_ai "$input"
                ;;
        esac
    done
}

# ============================================
# START
# ============================================
main
