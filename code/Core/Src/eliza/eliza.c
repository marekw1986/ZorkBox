/* eliza.c
 * Non-blocking state-machine port for STM32 bare-metal (no RTOS).
 * Input:  ps2_kbd_getkey(&ch)
 * Output: vga_putc(char) / vga_puts(char*)
 * Call eliza_handle() every main-loop iteration alongside vga_handle().
 */

#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include "main.h"   /* must come before ps2.h — defines __weak via HAL */
#include "vga.h"
#include "ps2.h"

/* ── tunables ────────────────────────────────────────────────────────────── */
#define MAXLINELEN   80
#define NUMKEYWORDS  37
#define NUMSWAPS     14

/* ── keyword / response tables (unchanged from original) ─────────────────── */
static const char *keywords[NUMKEYWORDS] = {
    "CAN YOU","CAN I","YOU ARE","YOURE","I DONT","I FEEL",
    "WHY DONT YOU","WHY CANT I","ARE YOU","I CANT","I AM","IM ",
    "YOU ","I WANT","WHAT","HOW","WHO","WHERE",
    "WHEN","WHY",
    "NAME","CAUSE","SORRY","DREAM","HELLO","HI ","MAYBE",
    " NO","YOUR","ALWAYS","THINK","ALIKE","YES","FRIEND",
    "COMPUTER","CAR","NOKEYFOUND"
};

static const char *SWAPS[NUMSWAPS][2] = {
    {"ARE","AM"},  {"WERE","WAS"},   {"YOU","I"},     {"YOUR","MY"},
    {"IVE","YOU'VE"},{"IM","YOU'RE"},{"YOU","ME"},    {"ME","YOU"},
    {"AM","ARE"},  {"WAS","WERE"},   {"I","YOU"},     {"MY","YOUR"},
    {"YOUVE","I'VE"},{"YOURE","I'M"}
};

static const int ResponsesPerKeyword[NUMKEYWORDS] = {
    3,2,4,4,4,3, 3,2,3,3,4,4, 3,5,9,9,9,9, 9,9,
    2,4,4,4,1,1,5, 5,2,4,3,7,3,6, 7,5,6
};

static const char *responses[NUMKEYWORDS][9] = {
    {"DON'T YOU BELIEVE THAT I CAN*",
     "PERHAPS YOU WOULD LIKE TO BE ABLE TO*",
     "YOU WANT ME TO BE ABLE TO*"},
    {"PERHAPS YOU DON'T WANT TO*",
     "DO YOU WANT TO BE ABLE TO*"},
    {"WHAT MAKES YOU THINK I AM*",
     "DOES IT PLEASE YOU TO BELIEVE I AM*",
     "PERHAPS YOU WOULD LIKE TO BE*",
     "DO YOU SOMETIMES WISH YOU WERE*"},
    {"WHAT MAKES YOU THINK I AM*",
     "DOES IT PLEASE YOU TO BELIEVE I AM*",
     "PERHAPS YOU WOULD LIKE TO BE*",
     "DO YOU SOMETIMES WISH YOU WERE*"},
    {"DON'T YOU REALLY*",
     "WHY DON'T YOU*",
     "DO YOU WISH TO BE ABLE TO*",
     "DOES THAT TROUBLE YOU?"},
    {"TELL ME MORE ABOUT SUCH FEELINGS.",
     "DO YOU OFTEN FEEL*",
     "DO YOU ENJOY FEELING*"},
    {"DO YOU REALLY BELIEVE I DON'T*",
     "PERHAPS IN GOOD TIME I WILL*",
     "DO YOU WANT ME TO*"},
    {"DO YOU THINK YOU SHOULD BE ABLE TO*",
     "WHY CAN'T YOU*"},
    {"WHY ARE YOU INTERESTED IN WHETHER OR NOT I AM*",
     "WOULD YOU PREFER IF I WERE NOT*",
     "PERHAPS IN YOUR FANTASIES I AM*"},
    {"HOW DO YOU KNOW YOU CAN'T*",
     "HAVE YOU TRIED?",
     "PERHAPS YOU CAN NOW*"},
    {"DID YOU COME TO ME BECAUSE YOU ARE*",
     "HOW LONG HAVE YOU BEEN*",
     "DO YOU BELIEVE IT IS NORMAL TO BE*",
     "DO YOU ENJOY BEING*"},
    {"DID YOU COME TO ME BECAUSE YOU ARE*",
     "HOW LONG HAVE YOU BEEN*",
     "DO YOU BELIEVE IT IS NORMAL TO BE*",
     "DO YOU ENJOY BEING*"},
    {"WE WERE DISCUSSING YOU-- NOT ME.",
     "OH, I*",
     "YOU'RE NOT REALLY TALKING ABOUT ME, ARE YOU?"},
    {"WHAT WOULD IT MEAN TO YOU IF YOU GOT*",
     "WHY DO YOU WANT*",
     "SUPPOSE YOU SOON GOT*",
     "WHAT IF YOU NEVER GOT*",
     "I SOMETIMES ALSO WANT*"},
    {"WHY DO YOU ASK?",
     "DOES THAT QUESTION INTEREST YOU?",
     "WHAT ANSWER WOULD PLEASE YOU THE MOST?",
     "WHAT DO YOU THINK?",
     "ARE SUCH QUESTIONS ON YOUR MIND OFTEN?",
     "WHAT IS IT THAT YOU REALLY WANT TO KNOW?",
     "HAVE YOU ASKED ANYONE ELSE?",
     "HAVE YOU ASKED SUCH QUESTIONS BEFORE?",
     "WHAT ELSE COMES TO MIND WHEN YOU ASK THAT?"},
    {"WHY DO YOU ASK?",
     "DOES THAT QUESTION INTEREST YOU?",
     "WHAT ANSWER WOULD PLEASE YOU THE MOST?",
     "WHAT DO YOU THINK?",
     "ARE SUCH QUESTIONS ON YOUR MIND OFTEN?",
     "WHAT IS IT THAT YOU REALLY WANT TO KNOW?",
     "HAVE YOU ASKED ANYONE ELSE?",
     "HAVE YOU ASKED SUCH QUESTIONS BEFORE?",
     "WHAT ELSE COMES TO MIND WHEN YOU ASK THAT?"},
    {"WHY DO YOU ASK?",
     "DOES THAT QUESTION INTEREST YOU?",
     "WHAT ANSWER WOULD PLEASE YOU THE MOST?",
     "WHAT DO YOU THINK?",
     "ARE SUCH QUESTIONS ON YOUR MIND OFTEN?",
     "WHAT IS IT THAT YOU REALLY WANT TO KNOW?",
     "HAVE YOU ASKED ANYONE ELSE?",
     "HAVE YOU ASKED SUCH QUESTIONS BEFORE?",
     "WHAT ELSE COMES TO MIND WHEN YOU ASK THAT?"},
    {"WHY DO YOU ASK?",
     "DOES THAT QUESTION INTEREST YOU?",
     "WHAT ANSWER WOULD PLEASE YOU THE MOST?",
     "WHAT DO YOU THINK?",
     "ARE SUCH QUESTIONS ON YOUR MIND OFTEN?",
     "WHAT IS IT THAT YOU REALLY WANT TO KNOW?",
     "HAVE YOU ASKED ANYONE ELSE?",
     "HAVE YOU ASKED SUCH QUESTIONS BEFORE?",
     "WHAT ELSE COMES TO MIND WHEN YOU ASK THAT?"},
    {"WHY DO YOU ASK?",
     "DOES THAT QUESTION INTEREST YOU?",
     "WHAT ANSWER WOULD PLEASE YOU THE MOST?",
     "WHAT DO YOU THINK?",
     "ARE SUCH QUESTIONS ON YOUR MIND OFTEN?",
     "WHAT IS IT THAT YOU REALLY WANT TO KNOW?",
     "HAVE YOU ASKED ANYONE ELSE?",
     "HAVE YOU ASKED SUCH QUESTIONS BEFORE?",
     "WHAT ELSE COMES TO MIND WHEN YOU ASK THAT?"},
    {"WHY DO YOU ASK?",
     "DOES THAT QUESTION INTEREST YOU?",
     "WHAT ANSWER WOULD PLEASE YOU THE MOST?",
     "WHAT DO YOU THINK?",
     "ARE SUCH QUESTIONS ON YOUR MIND OFTEN?",
     "WHAT IS IT THAT YOU REALLY WANT TO KNOW?",
     "HAVE YOU ASKED ANYONE ELSE?",
     "HAVE YOU ASKED SUCH QUESTIONS BEFORE?",
     "WHAT ELSE COMES TO MIND WHEN YOU ASK THAT?"},
    {"NAMES DON'T INTEREST ME.",
     "I DON'T CARE ABOUT NAMES-- PLEASE GO ON."},
    {"IS THAT THE REAL REASON?",
     "DON'T ANY OTHER REASONS COME TO MIND?",
     "DOES THAT REASON EXPLAIN ANY THING ELSE?",
     "WHAT OTHER REASONS MIGHT THERE BE?"},
    {"PLEASE DON'T APOLOGIZE.",
     "APOLOGIES ARE NOT NECESSARY.",
     "WHAT FEELINGS DO YOU HAVE WHEN YOU APOLOGIZE?",
     "DON'T BE SO DEFENSIVE!"},
    {"WHAT DOES THAT DREAM SUGGEST TO YOU?",
     "DO YOU DREAM OFTEN?",
     "WHAT PERSONS APPEAR IN YOUR DREAMS?",
     "ARE YOU DISTURBED BY YOUR DREAMS?"},
    {"HOW DO YOU DO--PLEASE STATE YOUR PROBLEM."},
    {"HOW DO YOU DO--PLEASE STATE YOUR PROBLEM."},
    {"YOU DON'T SEEM QUITE CERTAIN.",
     "WHY THE UNCERTAIN TONE?",
     "CAN'T YOU BE MORE POSITIVE?",
     "YOU AREN'T SURE?",
     "DON'T YOU KNOW?"},
    {"ARE YOU SAYING NO JUST TO BE NEGATIVE?",
     "YOU ARE BEING A BIT NEGATIVE.",
     "WHY NOT?",
     "ARE YOU SURE?",
     "WHY NO?"},
    {"WHY ARE YOU CONCERNED ABOUT MY*",
     "WHAT ABOUT YOUR OWN*"},
    {"CAN YOU THINK OF A SPECIFIC EXAMPLE?",
     "WHEN?",
     "WHAT ARE YOU THINKING OF?",
     "REALLY, ALWAYS?"},
    {"DO YOU REALLY THINK SO?",
     "BUT YOU ARE NOT SURE YOU*",
     "DO YOU DOUBT YOU*"},
    {"IN WHAT WAY?",
     "WHAT RESEMBLANCE DO YOU SEE?",
     "WHAT DOES THE SIMILARITY SUGGEST TO YOU?",
     "WHAT OTHER CONNECTIONS DO YOU SEE?",
     "COULD THERE REALLY BE SOME CONNECTION?",
     "HOW?"},
    {"YOU SEEM QUITE POSITIVE.",
     "ARE YOU SURE?",
     "I SEE.",
     "I UNDERSTAND."},
    {"WHY DO YOU BRING UP THE TOPIC OF FRIENDS?",
     "DO YOUR FRIENDS WORRY YOU?",
     "DO YOUR FRIENDS PICK ON YOU?",
     "ARE YOU SURE YOU HAVE ANY FRIENDS?",
     "DO YOU IMPOSE ON YOUR FRIENDS?",
     "PERHAPS YOUR LOVE FOR FRIENDS WORRIES YOU?"},
    {"DO COMPUTERS WORRY YOU?",
     "ARE YOU TALKING ABOUT ME IN PARTICULAR?",
     "ARE YOU FRIGHTENED BY MACHINES?",
     "WHY DO YOU MENTION COMPUTERS?",
     "WHAT DO YOU THINK MACHINES HAVE TO DO WITH YOUR PROBLEM?",
     "DON'T YOU THINK COMPUTERS CAN HELP PEOPLE?",
     "WHAT IS IT ABOUT MACHINES THAT WORRIES YOU?"},
    {"OH, DO YOU LIKE CARS?",
     "MY FAVORITE CAR IS A LAMBORGINI COUNTACH. WHAT IS YOUR FAVORITE CAR?",
     "MY FAVORITE CAR COMPANY IS FERRARI.  WHAT IS YOURS?",
     "DO YOU LIKE PORSCHES?",
     "DO YOU LIKE PORSCHE TURBO CARRERAS?"},
    {"SAY, DO YOU HAVE ANY PSYCHOLOGICAL PROBLEMS?",
     "WHAT DOES THAT SUGGEST TO YOU?",
     "I SEE.",
     "I'M NOT SURE I UNDERSTAND YOU FULLY.",
     "COME, COME ELUCIDATE YOUR THOUGHTS.",
     "CAN YOU ELABORATE ON THAT?",
     "THAT IS QUITE INTERESTING."}
};

/* ── module state ────────────────────────────────────────────────────────── */
typedef enum {
    ELIZA_GREETING,   /* print banner once, then wait for input   */
    ELIZA_INPUT,      /* accumulate keystrokes into inputstr[]     */
    ELIZA_RESPOND,    /* compute + print response, back to INPUT   */
    ELIZA_DONE        /* "BYE" received                            */
} ElizaState;

static ElizaState  state;
static char        inputstr[MAXLINELEN];
static char        lastinput[MAXLINELEN];
static int         inputlen;
static int         whichReply[NUMKEYWORDS];

/* ── helpers (no stdio, no strtok — strtok uses a hidden static pointer
      which is fine here, but we keep everything explicit) ──────────────── */

/* In-place: strip non-alpha/non-space, uppercase */
static void sanitize_upper(char *s, int len)
{
    int w = 0;
    for (int r = 0; r < len && s[r]; r++) {
        char c = s[r];
        if (isalpha((unsigned char)c) || c == ' ') {
            s[w++] = (char)toupper((unsigned char)c);
        }
    }
    s[w] = '\0';
}

/* Echo a single character to VGA and back to the user as local echo */
static void echo_char(char c)
{
    vga_putc(c);
}

/* Build Eliza's reply into out[] given the matched keyword index k
   and the position in inputstr[] where the keyword was found.        */
static void build_reply(int k, const char *match_pos, char *out, int outmax)
{
    const char *base   = responses[k][whichReply[k]];
    int         baselen = (int)strlen(base);
    int         w       = 0;

    if (base[baselen - 1] != '*') {
        /* No wildcard — copy as-is, guaranteed NUL-terminated */
        int copy = baselen < outmax ? baselen : outmax - 1;
        memcpy(out, base, copy);
        out[copy] = '\0';
        return;
    }

    /* Copy all but the trailing '*'.
       Use memcpy instead of strncpy to avoid -Wstringop-truncation;
       the bound (baselen-1) is intentional — we drop the '*'. */
    int copy = baselen - 1;
    if (copy >= outmax) copy = outmax - 1;
    memcpy(out, base, copy);
    w = copy;

    /* Append the user's tail, word by word, with pronoun swaps.
       We avoid strtok so there's no hidden state conflict with callers. */
    const char *p = match_pos + strlen(keywords[k]);
    while (*p == ' ') p++;          /* skip leading spaces after keyword */

    int first_word = 1;
    while (*p && w < outmax - 2) {
        /* collect one word */
        char word[MAXLINELEN];
        int  wlen = 0;
        while (*p && *p != ' ' && wlen < (int)sizeof(word) - 1)
            word[wlen++] = *p++;
        word[wlen] = '\0';
        while (*p == ' ') p++;      /* eat inter-word spaces */
        if (!wlen) break;

        /* pronoun swap */
        const char *emit = word;
        for (int s = 0; s < NUMSWAPS; s++) {
            if (strcmp(SWAPS[s][0], word) == 0) {
                emit = SWAPS[s][1];
                break;
            }
        }

        /* space before every word except the first */
        if (!first_word) out[w++] = ' ';
        first_word = 0;
        while (*emit && w < outmax - 2)
            out[w++] = *emit++;
    }

    if (w < outmax - 1) out[w++] = '?';
    out[w] = '\0';
}

/* ── public API ─────────────────────────────────────────────────────────── */

void eliza_init(void)
{
    state    = ELIZA_GREETING;
    inputlen = 0;
    inputstr[0]  = '\0';
    lastinput[0] = '\0';
    for (int i = 0; i < NUMKEYWORDS; i++) whichReply[i] = 0;
}

/*
 * Call this every main-loop iteration.
 * It consumes at most ONE character per call to stay non-blocking.
 */
void eliza_handle(void)
{
    uint8_t ch;

    switch (state) {

    /* ── GREETING: print banner once, switch to INPUT ── */
    case ELIZA_GREETING:
        vga_puts("\n\n");
        vga_puts("          *** ELIZA ***\n");
        vga_puts("   Original code by Weizenbaum, 1966\n");
        vga_puts("      Type 'bye' to quit.\n\n");
        vga_puts("HI!  I'M ELIZA.  WHAT'S YOUR PROBLEM?\n> ");
        state    = ELIZA_INPUT;
        inputlen = 0;
        break;

    /* ── INPUT: accumulate one character at a time ── */
    case ELIZA_INPUT:
        if (ps2_kbd_getkey(&ch) != 1) return;   /* nothing ready — yield */

        if (ch == '\r' || ch == '\n') {
            /* line complete */
            vga_puts("\n");
            inputstr[inputlen] = '\0';
            sanitize_upper(inputstr, inputlen);
            state = ELIZA_RESPOND;
            return;
        }

        if (ch == '\b' || ch == 127) {
            /* backspace */
            if (inputlen > 0) {
                inputlen--;
                vga_puts("\b \b");   /* erase on screen */
            }
            return;
        }

        if (inputlen < MAXLINELEN - 1) {
            inputstr[inputlen++] = ch;
            echo_char(ch);           /* local echo */
        }
        break;

    /* ── RESPOND: process the completed line, print reply ── */
    case ELIZA_RESPOND: {
        char reply[MAXLINELEN * 2];   /* enough for base + user tail */

        /* Goodbye? */
        if (strcmp(inputstr, "BYE") == 0) {
            vga_puts("GOODBYE!  THANKS FOR VISITING WITH ME...\n");
            state = ELIZA_DONE;
            return;
        }

        /* Repeated input? */
        if (strcmp(lastinput, inputstr) == 0) {
            vga_puts("PLEASE DON'T REPEAT YOURSELF!\n> ");
            state    = ELIZA_INPUT;
            inputlen = 0;
            return;
        }
        strncpy(lastinput, inputstr, MAXLINELEN - 1);
        lastinput[MAXLINELEN - 1] = '\0';

        /* Keyword scan */
        int         k        = NUMKEYWORDS - 1;  /* default */
        const char *match_at = inputstr;

        for (int i = 0; i < NUMKEYWORDS - 1; i++) {
            const char *loc = strstr(inputstr, keywords[i]);
            if (loc) {
                k        = i;
                match_at = loc;
                break;
            }
        }

        /* Build and emit reply */
        build_reply(k, match_at, reply, sizeof(reply));
        vga_puts(reply);
        vga_puts("\n> ");

        /* Advance round-robin counter */
        whichReply[k]++;
        if (whichReply[k] >= ResponsesPerKeyword[k])
            whichReply[k] = 0;

        /* Back to input */
        state    = ELIZA_INPUT;
        inputlen = 0;
        break;
    }

    case ELIZA_DONE:
        /* Nothing more to do — sit here silently */
        break;
    }
}
