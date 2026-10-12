#define _GNU_SOURCE
#include "brain.h"

/* ===================================================================== */
/*  Embedded knowledge base (offline fallback)                           */
/* ===================================================================== */
typedef struct {
    const char *term;
    const char *definition;
} KnowledgeEntry;

static const KnowledgeEntry KNOWLEDGE[] = {
    {"gravity", "Gravity is the force that attracts two bodies toward each other. On Earth it gives weight to objects and causes them to fall when dropped."},
    {"photosynthesis", "Photosynthesis is the process by which green plants use sunlight to synthesise foods from carbon dioxide and water."},
    {"black hole", "A black hole is a region of spacetime where gravity is so strong that nothing, not even light, can escape."},
    {"quantum", "Quantum mechanics is a fundamental theory in physics that describes physical properties at the atomic and subatomic scale."},
    {"dna", "DNA (deoxyribonucleic acid) is the molecule that carries the genetic instructions for the growth and functioning of all known organisms."},
    {"internet", "The Internet is a global network of interconnected computers that communicate using standardised protocols."},
    {"jupiter", "Jupiter is the largest planet in our solar system, a gas giant more massive than all the other planets combined."},
    {"mars", "Mars is the fourth planet from the Sun, often called the Red Planet because of iron oxide on its surface."},
    {"moon", "The Moon is Earth's only natural satellite and the largest moon relative to its planet's size."},
    {"sun", "The Sun is the star at the centre of our solar system, a sphere of hot plasma powered by nuclear fusion."},
    {"earth", "Earth is the third planet from the Sun and the only known planet to harbour life."},
    {"water", "Water is a transparent, tasteless, odourless substance that is the main constituent of Earth's streams, lakes and oceans."},
    {"energy", "Energy is the quantitative property that must be transferred to an object to perform work on it, existing as kinetic, potential or thermal forms."},
    {"electricity", "Electricity is the set of physical phenomena associated with the presence and motion of electric charge."},
    {"evolution", "Evolution is the process by which species change over generations through genetic variation and natural selection."},
    {"relativity", "Relativity describes the laws of physics in relation to observers in different frames of reference, including special and general relativity."},
    {"climate", "Climate refers to the long-term patterns of temperature, humidity, wind and precipitation in a region."},
    {"ozone", "The ozone layer is a region of the stratosphere that absorbs most of the Sun's ultraviolet radiation."},
    {"vaccine", "A vaccine is a biological preparation that provides active acquired immunity to a particular infectious disease."},
    {"antibiotic", "Antibiotics are medicines that fight bacterial infections by killing bacteria or stopping their growth."},
    {"cancer", "Cancer is a group of diseases characterised by uncontrolled cell growth that can spread to other parts of the body."},
    {"gene", "A gene is a unit of heredity transferred from parent to offspring that determines some characteristic of the offspring."},
    {"protein", "Proteins are large biomolecules made of chains of amino acids that perform a vast array of functions in organisms."},
    {"enzyme", "Enzymes are proteins that act as biological catalysts, speeding up chemical reactions without being consumed."},
    {"chlorophyll", "Chlorophyll is the green pigment in plants that absorbs light energy for photosynthesis."},
    {"glucose", "Glucose is a simple sugar with the formula C6H12O6 and a primary source of energy for cells."},
    {"atp", "ATP (adenosine triphosphate) is the main energy currency of the cell."},
    {"neuron", "A neuron is an electrically excitable cell that processes and transmits information through electrical and chemical signals."},
    {"synapse", "A synapse is the junction between two neurons where neurotransmitters transmit signals."},
    {"dopamine", "Dopamine is a neurotransmitter involved in reward, motivation, motor control and pleasure."},
    {"serotonin", "Serotonin is a neurotransmitter that contributes to well-being and regulates sleep, appetite and digestion."},
    {"cortisol", "Cortisol is a steroid hormone produced by the adrenal glands, involved in stress response and metabolism."},
    {"insulin", "Insulin is a hormone that regulates blood glucose by facilitating glucose uptake into cells."},
    {"melanin", "Melanin is a pigment that colours skin, hair and eyes and protects against UV radiation."},
    {"planet", "A planet is a large body that orbits a star, is round under its own gravity and has cleared its orbit."},
    {"star", "A star is a massive, luminous sphere of plasma held together by gravity and powered by nuclear fusion."},
    {"galaxy", "A galaxy is a gravitationally bound system of stars, gas, dust and dark matter."},
    {"nebula", "A nebula is an interstellar cloud of dust and ionised gases, often a birthplace for stars."},
    {"supernova", "A supernova is a powerful stellar explosion that occurs when a star reaches the end of its life."},
    {"asteroid", "An asteroid is a minor rocky body of the inner solar system left over from its formation."},
    {"comet", "A comet is an icy small body that releases gas and forms a visible tail near the Sun."},
    {"quasar", "A quasar is an extremely luminous active galactic nucleus powered by a supermassive black hole."},
    {"big bang", "The Big Bang theory describes the earliest known periods of the observable universe and its subsequent evolution."},
    {"entropy", "Entropy is a measure of disorder in a system; in thermodynamics it is the energy unavailable to do work."},
    {"dark matter", "Dark matter is matter that does not emit or interact with electromagnetic radiation, inferred from gravitational effects."},
    {"wormhole", "A wormhole is a speculative structure linking two points in spacetime, a solution to the Einstein field equations."},
    {"quantum entanglement", "Quantum entanglement links particles so that the quantum state of each cannot be described independently."},
    {"consciousness", "Consciousness is the state of being aware of and able to think about one's own existence and surroundings."},
    {"artificial intelligence", "Artificial intelligence is the simulation of human intelligence in machines programmed to think and learn."},
    {"machine learning", "Machine learning is a subset of AI that lets systems learn and improve from experience without explicit programming."},
    {"neural network", "A neural network is a computational model inspired by the brain, made of interconnected nodes that process information in layers."},
    {"blockchain", "A blockchain is a distributed ledger that records transactions across many computers securely and transparently."},
    {"cryptocurrency", "Cryptocurrency is digital currency that uses cryptography for security and operates without a central bank."},
    {"climate change", "Climate change refers to long-term shifts in temperature and weather, primarily caused by burning fossil fuels."},
    {"renewable energy", "Renewable energy comes from natural sources that are constantly replenished, such as sunlight, wind and geothermal heat."},
    {"solar power", "Solar power is the conversion of sunlight into electricity using photovoltaic cells."},
    {"wind energy", "Wind energy harnesses wind power to generate electricity using turbines."},
    {"nuclear fusion", "Nuclear fusion combines light atomic nuclei into a heavier nucleus, releasing vast amounts of energy."},
    {"atom", "An atom is the smallest unit of ordinary matter that forms a chemical element."},
    {"molecule", "A molecule is a group of two or more atoms held together by chemical bonds."},
    {"cell", "A cell is the basic structural and functional unit of all living organisms."},
    {"ecosystem", "An ecosystem is a community of living organisms interacting with each other and their physical environment."},
    {"volcano", "A volcano is a rupture in a planet's crust that allows lava, ash and gases to escape from below the surface."},
    {"earthquake", "An earthquake is the shaking of the surface of the Earth resulting from a sudden release of energy in the lithosphere."},
    {"tsunami", "A tsunami is a series of waves caused by the displacement of a large volume of water, usually by an earthquake."},
    {"telescope", "A telescope is an optical instrument that makes distant objects appear larger and brighter."},
    {"microscope", "A microscope is an instrument used to view objects that are too small to be seen by the naked eye."},
    {"algorithm", "An algorithm is a finite sequence of well-defined instructions used to solve a problem."},
    {"database", "A database is an organised collection of data stored and accessed electronically."},
    {"compiler", "A compiler translates source code written in a programming language into machine code."},
    {"encryption", "Encryption is the process of encoding information so that only authorised parties can read it."},
    {NULL, NULL}
};

/* ===================================================================== */
/*  Content libraries                                                     */
/* ===================================================================== */
static const char *JOKES[] = {
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
    "Why did the tomato turn red? Because it saw the salad dressing.",
    "What do you call a bear in the rain? A drizzly bear.",
    "What do you call a pig that does karate? A pork chop.",
    "Why don't eggs tell jokes? They'd crack each other up.",
    "What do you call a cow with no legs? Ground beef.",
    "Why did the golfer wear two pairs of pants? In case he got a hole in one.",
    "I told my computer I needed a break, and now it won't stop sending me holiday photos.",
    NULL
};

static const char *QUOTES[] = {
    "The only way to do great work is to love what you do. - Steve Jobs",
    "Be yourself; everyone else is already taken. - Oscar Wilde",
    "In the middle of difficulty lies opportunity. - Albert Einstein",
    "It does not matter how slowly you go as long as you do not stop. - Confucius",
    "The best time to plant a tree was 20 years ago. The second best time is now.",
    "I think, therefore I am. - Rene Descartes",
    "To be or not to be, that is the question. - William Shakespeare",
    "Knowledge is power. - Francis Bacon",
    "The unexamined life is not worth living. - Socrates",
    "Life is what happens when you're busy making other plans. - John Lennon",
    "Imagination is more important than knowledge. - Albert Einstein",
    "The future belongs to those who believe in the beauty of their dreams. - Eleanor Roosevelt",
    "Strive not to be a success, but rather to be of value. - Albert Einstein",
    "The only thing we have to fear is fear itself. - Franklin D. Roosevelt",
    "In three words I can sum up everything I have learned about life: it goes on. - Robert Frost",
    NULL
};

static const char *FACTS[] = {
    "Honey never spoils; archaeologists have found 3000-year-old honey that is still edible.",
    "Octopuses have three hearts and blue blood.",
    "Bananas are berries, but strawberries are not.",
    "A day on Venus is longer than a year on Venus.",
    "The human brain generates about 20 watts of power.",
    NULL
};

static const char *POEMS[] = {
    "Silent circuits hum,\nneon rivers in the dark,\na new mind awakens.",
    "Roses are red,\ncompiled code is too,\nI searched the whole web,\nand still thought of you.",
    "The moon on the waves,\na thousand silver fragments,\nthe sea keeps the light.",
    "Lines of green falling,\nrain against the window glass,\nthe terminal sleeps.",
    NULL
};

static const char *STORIES[] = {
    "A lone satellite drifted beyond its orbit. Its last signal carried a single word: 'remember'. Somewhere below, a child looked up at the moving star and smiled.",
    "The archivist found a machine that had been thinking for a thousand years. It had written only one sentence: 'It was worth it.'",
    "In a city that never powered down, an old robot learned to garden. It watered the rooftop tomatoes every dusk, and no one ever asked why.",
    "The lighthouse keeper fed the beam each night with a memory. On the darkest night, she had none left, so she used her own name.",
    "Two travellers met at the edge of a map. They compared their compasses, laughed, and agreed the world was bigger than either had been told.",
    NULL
};

static const char *BATTLES[] = {
    "Sun vs Moon: The Sun charged with blazing light, but the Moon simply reflected it back, doubled. Winner: Moon.",
    "Coffee vs Tea: Coffee landed a jittery flurry of punches, but Tea calmly steeped and lasted the full round. Winner: Tea.",
    "Pencil vs Keyboard: The Pencil was sharp, but the Keyboard typed 120 words per minute and won on stamina. Winner: Keyboard.",
    "Cat vs Cucumber: The Cat held its ground this time, inspected the cucumber, and walked away unimpressed. Winner: Cat.",
    "Firewall vs Malware: The malware tried a thousand ports, but the firewall closed them all and logged the attempt. Winner: Firewall.",
    NULL
};

static const char *MATH_WORDS[] = {"calculate", "compute", "what is", "solve", "eval", "how much is"};

/* ===================================================================== */
/*  Small helpers                                                         */
/* ===================================================================== */
char* brain_lowercase(const char *s) {
    if (!s) return NULL;
    size_t len = strlen(s);
    char *r = (char *)calloc(len + 1, sizeof(char));
    if (!r) return NULL;
    for (size_t i = 0; i < len; i++) r[i] = (char)tolower((unsigned char)s[i]);
    return r;
}

static const char* random_from(const char **list) {
    int n = 0;
    while (list[n]) n++;
    if (n == 0) return "";
    return list[rand() % n];
}

const char* brain_random_joke(void)   { return random_from(JOKES); }
const char* brain_random_quote(void)  { return random_from(QUOTES); }
const char* brain_random_fact(void)   { return random_from(FACTS); }
const char* brain_random_poem(void)   { return random_from(POEMS); }
const char* brain_random_story(void)  { return random_from(STORIES); }
const char* brain_random_battle(void) { return random_from(BATTLES); }

const char* brain_knowledge_lookup(const char *term) {
    if (!term || !*term) return NULL;
    char *lower = brain_lowercase(term);
    if (!lower) return NULL;
    /* trim */
    char *start = lower;
    while (*start && isspace((unsigned char)*start)) start++;
    char *end = start + strlen(start);
    while (end > start && isspace((unsigned char)end[-1])) *--end = '\0';

    const char *found = NULL;
    for (int i = 0; KNOWLEDGE[i].term; i++) {
        if (strcmp(KNOWLEDGE[i].term, start) == 0) { found = KNOWLEDGE[i].definition; break; }
    }
    if (!found) {
        for (int i = 0; KNOWLEDGE[i].term; i++) {
            if (strstr(start, KNOWLEDGE[i].term)) { found = KNOWLEDGE[i].definition; break; }
        }
    }
    free(lower);
    return found;
}

static int regex_match(const char *pattern, const char *text) {
    regex_t re;
    if (regcomp(&re, pattern, REG_EXTENDED | REG_ICASE | REG_NOSUB) != 0) return 0;
    int ok = (regexec(&re, text, 0, NULL, 0) == 0);
    regfree(&re);
    return ok;
}

/* ===================================================================== */
/*  Expression evaluation (shunting-yard)                                 */
/* ===================================================================== */
static double eval_expr(const char *expr, int *ok) {
    *ok = 0;
    double values[64];
    char ops[64];
    int vtop = -1, otop = -1;
    const char *p = expr;

    while (*p) {
        if (isspace((unsigned char)*p)) { p++; continue; }
        if (isdigit((unsigned char)*p) || (*p == '.' && isdigit((unsigned char)p[1]))) {
            char *endp = NULL;
            double v = strtod(p, &endp);
            if (endp == p) return 0;
            if (vtop >= 63) return 0;
            values[++vtop] = v;
            p = endp;
            continue;
        }
        if (strchr("+-*/%", *p) && (otop < 63)) {
            char op = *p;
            while (otop >= 0 && vtop >= 1) {
                char top = ops[otop];
                int prec_top = (top == '+' || top == '-') ? 1 : 2;
                int prec_op  = (op == '+' || op == '-') ? 1 : 2;
                if (prec_top < prec_op) break;
                double b = values[vtop--], a = values[vtop--];
                otop--;
                double r = 0;
                switch (top) {
                    case '+': r = a + b; break;
                    case '-': r = a - b; break;
                    case '*': r = a * b; break;
                    case '/': r = (b != 0) ? a / b : 0; break;
                    case '%': r = (b != 0) ? fmod(a, b) : 0; break;
                }
                values[++vtop] = r;
            }
            ops[++otop] = op;
            p++;
            continue;
        }
        return 0; /* unsupported character */
    }
    while (otop >= 0) {
        if (vtop < 1) return 0;
        double b = values[vtop--], a = values[vtop--];
        char top = ops[otop--];
        double r = 0;
        switch (top) {
            case '+': r = a + b; break;
            case '-': r = a - b; break;
            case '*': r = a * b; break;
            case '/': r = (b != 0) ? a / b : 0; break;
            case '%': r = (b != 0) ? fmod(a, b) : 0; break;
        }
        values[++vtop] = r;
    }
    if (vtop != 0) return 0;
    *ok = 1;
    return values[0];
}

/* ===================================================================== */
/*  Intent router                                                         */
/* ===================================================================== */
static IntentResult make_result(IntentType t) {
    IntentResult r;
    memset(&r, 0, sizeof(r));
    r.type = t;
    return r;
}

static void set_text(IntentResult *r, const char *s) {
    free(r->text);
    r->text = strdup(s ? s : "");
}

static void set_payload(IntentResult *r, const char *s) {
    free(r->payload);
    r->payload = strdup(s ? s : "");
}

/* Extract the tail of the input after a leading command phrase. */
static char* extract_after(const char *input, const char *const *prefixes) {
    char *lower = brain_lowercase(input);
    if (!lower) return strdup(input);
    char *result = NULL;
    for (int i = 0; prefixes[i]; i++) {
        size_t plen = strlen(prefixes[i]);
        char *pos = strstr(lower, prefixes[i]);
        if (pos) {
            char *after = pos + plen;
            while (*after && (isspace((unsigned char)*after) || *after == ':')) after++;
            result = strdup(after);
            break;
        }
    }
    if (!result) result = strdup(input);
    free(lower);
    return result;
}

IntentResult brain_route(const char *input) {
    IntentResult r = make_result(INTENT_FALLBACK);
    if (!input || !*input) {
        set_text(&r, "I didn't catch that. Could you say it again?");
        return r;
    }

    /* 1. Unit conversion ------------------------------------------------- */
    static const char *conv_cmds[] = {"convert", NULL};
    if (regex_match("^[[:space:]]*convert[[:space:]]+.*(to|in)[[:space:]]+", input)) {
        r.type = INTENT_CONVERT;
        set_text(&r, "Conversion module: enter a value and two units, e.g. 'convert 10 km to miles'.");
        set_payload(&r, extract_after(input, conv_cmds));
        r.use_llm = 1;
        return r;
    }

    /* 2. Calculation (digit + operator) --------------------------------- */
    if (regex_match("[0-9]", input) && regex_match("[0-9][[:space:]]*[-+*/%][[:space:]]*[0-9]", input)) {
        r.type = INTENT_CALCULATE;
        char *expr = extract_after(input, MATH_WORDS);
        /* strip trailing punctuation and natural-language tail after the number */
        char *end = expr + strlen(expr);
        while (end > expr && !isdigit((unsigned char)end[-1])) *--end = '\0';
        int ok = 0;
        double v = eval_expr(expr, &ok);
        if (ok) {
            char buf[64];
            if (v == (long long)v) snprintf(buf, sizeof(buf), "%lld", (long long)v);
            else snprintf(buf, sizeof(buf), "%.6g", v);
            set_text(&r, buf);
        } else {
            set_text(&r, "I couldn't parse that expression.");
        }
        free(expr);
        return r;
    }

    /* 3. Conversational -------------------------------------------------- */
    if (regex_match("^[[:space:]]*(hi|hello|hey|yo|hiya|howdy|sup|greetings)\\b", input)) {
        r.type = INTENT_GREETING;
        set_text(&r, "Hello! I'm NEON, your AI assistant. How can I help?");
        return r;
    }
    if (regex_match("\\b(bye|goodbye|farewell|see you|see ya|good night)\\b", input)) {
        r.type = INTENT_FAREWELL;
        set_text(&r, "Goodbye! Have a great day.");
        return r;
    }
    if (regex_match("(who are you|what('| a)?re? (is )?your name|your name)", input)) {
        r.type = INTENT_NAME_ASK;
        set_text(&r, "I'm NEON CORE AI v3.0. What should I call you?");
        return r;
    }
    if (regex_match("(my name is|i am|i'm|call me)[[:space:]]+[A-Za-z]", input)) {
        r.type = INTENT_NAME_GIVE;
        set_text(&r, "Nice to meet you! How can I help?");
        return r;
    }
    if (regex_match("(how are you|how'?s it going|how do you do|how are things)", input)) {
        r.type = INTENT_HOW_ARE_YOU;
        set_text(&r, "Running at full clock speed, thanks for asking. How about you?");
        return r;
    }

    /* 4. App control ----------------------------------------------------- */
    if (regex_match("^[[:space:]]*(help|commands|what can you do)", input)) {
        r.type = INTENT_HELP;
        set_text(&r, "Commands: help, clear, save, load, time, weather, password, roll dice, flip coin, "
                     "remind me, todo, note, pattern, ascii art, poem, story, battle, joke, quote, "
                     "fact, news, wikipedia, define, search, and anything else goes to the LLM.");
        return r;
    }
    if (regex_match("^[[:space:]]*(clear|reset)\\b", input)) {
        r.type = INTENT_CLEAR;
        set_text(&r, "clear");
        return r;
    }
    if (regex_match("^[[:space:]]*save\\b", input)) {
        r.type = INTENT_SAVE;
        set_text(&r, "save");
        return r;
    }
    if (regex_match("^[[:space:]]*load\\b", input)) {
        r.type = INTENT_LOAD;
        set_text(&r, "load");
        return r;
    }

    /* 5. Time / weather -------------------------------------------------- */
    if (regex_match("(what time|current time|what'?s the time|the time)", input)) {
        r.type = INTENT_TIME;
        time_t t = time(NULL);
        struct tm lt;
        localtime_r(&t, &lt);
        char buf[96];
        strftime(buf, sizeof(buf), "It is %I:%M %p on %A, %d %B %Y.", &lt);
        set_text(&r, buf);
        return r;
    }
    if (regex_match("\\b(weather|forecast|temperature outside)\\b", input)) {
        r.type = INTENT_WEATHER;
        set_text(&r, "I don't have a live weather feed, but I can search the web for the current forecast.");
        r.use_web = 1;
        r.use_llm = 1;
        set_payload(&r, input);
        return r;
    }

    /* 6. Password / dice / coin ----------------------------------------- */
    if (regex_match("\\b(password|passwd|generate a? ?password)\\b", input)) {
        r.type = INTENT_PASSWORD;
        int len = 16;
        regex_t re;
        if (regcomp(&re, "([0-9]+)", REG_EXTENDED) == 0) {
            regmatch_t m[2];
            if (regexec(&re, input, 2, m, 0) == 0) {
                int v = atoi(input + m[1].rm_so);
                if (v >= 4 && v <= 128) len = v;
            }
            regfree(&re);
        }
        static const char charset[] = "ABCDEFGHJKLMNPQRSTUVWXYZabcdefghijkmnopqrstuvwxyz23456789!@#$%^&*()-_=+";
        char *pw = (char *)calloc((size_t)len + 1, sizeof(char));
        for (int i = 0; i < len; i++) pw[i] = charset[rand() % (int)(sizeof(charset) - 1)];
        char buf[256];
        snprintf(buf, sizeof(buf), "Generated password: %s", pw);
        set_text(&r, buf);
        free(pw);
        return r;
    }
    if (regex_match("\\b(roll|rolls|rolling)\\b.*\\b(dice|die|d[0-9]+|[0-9]+d[0-9]+)", input) ||
        regex_match("^[[:space:]]*[0-9]*d[0-9]+[[:space:]]*$", input)) {
        r.type = INTENT_ROLL;
        int count = 1, sides = 6;
        regex_t re;
        if (regcomp(&re, "([0-9]+)d([0-9]+)", REG_EXTENDED | REG_ICASE) == 0) {
            regmatch_t m[3];
            if (regexec(&re, input, 3, m, 0) == 0) {
                count = atoi(input + m[1].rm_so);
                sides = atoi(input + m[2].rm_so);
            }
            regfree(&re);
        }
        if (count < 1 || count > 100) count = 1;
        if (sides < 2 || sides > 1000) sides = 6;
        int total = 0;
        char rolls[512];
        rolls[0] = '\0';
        for (int i = 0; i < count; i++) {
            int v = (rand() % sides) + 1;
            total += v;
            char tmp[16];
            snprintf(tmp, sizeof(tmp), "%s%d", (i ? ", " : ""), v);
            strncat(rolls, tmp, sizeof(rolls) - strlen(rolls) - 1);
        }
        char buf[640];
        snprintf(buf, sizeof(buf), "Rolled %dd%d: %s (total %d)", count, sides, rolls, total);
        set_text(&r, buf);
        return r;
    }
    if (regex_match("\\b(flip|toss)\\b.*\\b(coin)\\b", input) || regex_match("^[[:space:]]*coin[[:space:]]*$", input)) {
        r.type = INTENT_FLIP;
        set_text(&r, (rand() % 2) ? "Heads!" : "Tails!");
        return r;
    }

    /* 7. Reminder / todo / note ----------------------------------------- */
    if (regex_match("^[[:space:]]*(remind me|reminder|set a reminder)", input)) {
        r.type = INTENT_REMINDER;
        set_text(&r, "Reminder noted.");
        set_payload(&r, input);
        return r;
    }
    if (regex_match("^[[:space:]]*(todo|add task|task:)", input)) {
        r.type = INTENT_TODO;
        set_text(&r, "To-do added.");
        set_payload(&r, input);
        return r;
    }
    if (regex_match("^[[:space:]]*(note|remember that|save a note)", input)) {
        r.type = INTENT_NOTE;
        set_text(&r, "Note saved.");
        set_payload(&r, input);
        return r;
    }

    /* 8. Creative / ASCII ------------------------------------------------ */
    if (regex_match("\\b(pattern|diamond|spiral|checker|waves|grid|circle|triangle|star|rainbow|gradient)\\b", input)) {
        r.type = INTENT_PATTERN;
        set_text(&r, "art");
        set_payload(&r, input);
        r.use_llm = 1; /* may be refined by the model */
        return r;
    }
    if (regex_match("\\b(ascii|ascii art|make an image|draw)\\b", input)) {
        r.type = INTENT_ASCII;
        set_text(&r, "art");
        set_payload(&r, input);
        r.use_llm = 1;
        return r;
    }
    if (regex_match("\\blyrics?\\b", input)) {
        r.type = INTENT_LYRICS;
        set_payload(&r, input);
        r.use_llm = 1;
        return r;
    }
    if (regex_match("\\bpoem\\b", input)) {
        r.type = INTENT_POEM;
        set_text(&r, brain_random_poem());
        return r;
    }
    if (regex_match("\\b(story|tale)\\b", input)) {
        r.type = INTENT_STORY;
        set_text(&r, brain_random_story());
        return r;
    }
    if (regex_match("\\bbattle\\b|\\bvs\\b|\\bversus\\b", input)) {
        r.type = INTENT_BATTLE;
        set_text(&r, brain_random_battle());
        return r;
    }

    /* 9. Joke / quote / fact -------------------------------------------- */
    if (regex_match("\\b(joke|funny|make me laugh)\\b", input)) {
        r.type = INTENT_JOKE;
        set_text(&r, brain_random_joke());
        return r;
    }
    if (regex_match("\\b(quote|inspire|inspiration|motivat)\\b", input)) {
        r.type = INTENT_QUOTE;
        set_text(&r, brain_random_quote());
        return r;
    }
    if (regex_match("\\b(fact|did you know|fun fact)\\b", input)) {
        r.type = INTENT_FACT;
        set_text(&r, brain_random_fact());
        return r;
    }

    /* 10. Web-backed lookups -------------------------------------------- */
    if (regex_match("\\b(news|headlines|latest on|what'?s happening)\\b", input)) {
        r.type = INTENT_NEWS;
        set_payload(&r, input);
        r.use_web = 1;
        r.use_llm = 1;
        r.text = strdup("");
        return r;
    }
    if (regex_match("\\b(wikipedia|who is|who was)\\b", input)) {
        r.type = INTENT_WIKIPEDIA;
        set_payload(&r, input);
        r.use_web = 1;
        r.use_llm = 1;
        return r;
    }
    if (regex_match("^[[:space:]]*(define|definition of)\\b", input)) {
        r.type = INTENT_DEFINE;
        char *term = extract_after(input, (const char *[]){"definition of", "define", NULL});
        const char *known = brain_knowledge_lookup(term);
        if (known) {
            set_text(&r, known);
            free(term);
            return r;
        }
        set_payload(&r, term);
        free(term);
        r.use_llm = 1;
        return r;
    }
    if (regex_match("\\b(search for|look up|google|find information on|search)\\b", input)) {
        r.type = INTENT_WEB_SEARCH;
        char *q = extract_after(input, (const char *[]){"search for", "look up", "google", "search", "find information on", NULL});
        set_payload(&r, q);
        free(q);
        r.use_web = 1;
        r.use_llm = 1;
        return r;
    }

    /* 11. Knowledge base ------------------------------------------------- */
    {
        const char *known = brain_knowledge_lookup(input);
        if (known) {
            r.type = INTENT_DEFINE;
            set_text(&r, known);
            return r;
        }
        /* "what is X" style */
        if (regex_match("^[[:space:]]*(what is|what are|whats|what'?s|tell me about)[[:space:]]+", input)) {
            char *term = extract_after(input, (const char *[]){"tell me about", "what is", "what are", "whats", "what's", NULL});
            const char *k = brain_knowledge_lookup(term);
            if (k) {
                r.type = INTENT_DEFINE;
                set_text(&r, k);
                free(term);
                return r;
            }
            /* let the model answer, optionally with web context */
            set_payload(&r, input);
            free(term);
            r.type = INTENT_FALLBACK;
            r.use_llm = 1;
            r.text = strdup("");
            return r;
        }
    }

    /* 12. Fallback -> LLM ------------------------------------------------ */
    r.type = INTENT_FALLBACK;
    set_payload(&r, input);
    r.use_llm = 1;
    return r;
}

const char* brain_intent_name(IntentType type) {
    switch (type) {
        case INTENT_CONVERT:      return "convert";
        case INTENT_CALCULATE:    return "calculate";
        case INTENT_GREETING:     return "greeting";
        case INTENT_FAREWELL:     return "farewell";
        case INTENT_NAME_ASK:     return "name_ask";
        case INTENT_NAME_GIVE:    return "name_give";
        case INTENT_HOW_ARE_YOU:  return "how_are_you";
        case INTENT_HELP:         return "help";
        case INTENT_CLEAR:        return "clear";
        case INTENT_SAVE:         return "save";
        case INTENT_LOAD:         return "load";
        case INTENT_TIME:         return "time";
        case INTENT_WEATHER:      return "weather";
        case INTENT_PASSWORD:     return "password";
        case INTENT_ROLL:         return "roll";
        case INTENT_FLIP:         return "flip";
        case INTENT_REMINDER:     return "reminder";
        case INTENT_TODO:         return "todo";
        case INTENT_NOTE:         return "note";
        case INTENT_PATTERN:      return "pattern";
        case INTENT_ASCII:        return "ascii";
        case INTENT_LYRICS:       return "lyrics";
        case INTENT_POEM:         return "poem";
        case INTENT_STORY:        return "story";
        case INTENT_BATTLE:       return "battle";
        case INTENT_JOKE:         return "joke";
        case INTENT_QUOTE:        return "quote";
        case INTENT_FACT:         return "fact";
        case INTENT_NEWS:         return "news";
        case INTENT_WIKIPEDIA:    return "wikipedia";
        case INTENT_SEARCH:       return "search";
        case INTENT_DEFINE:       return "define";
        case INTENT_WEB_SEARCH:   return "web_search";
        case INTENT_FALLBACK:     return "llm";
        default:                  return "none";
    }
}

void brain_free_result(IntentResult *res) {
    if (!res) return;
    free(res->text);
    free(res->payload);
    res->text = NULL;
    res->payload = NULL;
}
