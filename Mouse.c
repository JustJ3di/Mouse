#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_PROG 500
#define MAX_STACK 20
#define MAX_DATA 260


typedef enum FrameType { FRAME_MACRO, FRAME_PARAM, FRAME_LOOP } FrameType;


typedef struct {
    FrameType type;
    int pos;
    int offset;
} Frame;

/* --------------------------------------------------------------------------
 * Variabili globali
 * -------------------------------------------------------------------------- */

char PROG[MAX_PROG + 2];
int DEF_MACRO[26];
int calcStack[MAX_STACK];
Frame frameStack[MAX_STACK];
int DATA[MAX_DATA];

int calcTop = 0;
int charPos = -1;
int frameLevel = 0;
int offset = 0;
char currentChar;

/* --------------------------------------------------------------------------
 * Utility
 * -------------------------------------------------------------------------- */

static inline int idxFromLetter(char c) { return c - 'A'; }
static inline int digitVal(char c) { return c - '0'; }


void nextChar() {
    if (charPos + 1 >= MAX_PROG)
        currentChar = '\0';
    else
        currentChar = PROG[++charPos];
}


void pushCalc(int value) {
    if (calcTop >= MAX_STACK) {
        fprintf(stderr, "Error: Calculation stack overflow.\n");
        exit(1);
    }
    calcStack[calcTop++] = value;
}

int popCalc() {
    if (calcTop <= 0) {
        fprintf(stderr, "Error: Calculation stack underflow.\n");
        exit(1);
    }
    return calcStack[--calcTop];
}


void pushFrame(FrameType type) {
    if (frameLevel >= MAX_STACK) {
        fprintf(stderr, "Error: Frame stack overflow.\n");
        exit(1);
    }
    frameStack[frameLevel].type = type;
    frameStack[frameLevel].pos = charPos;
    frameStack[frameLevel].offset = offset;
    frameLevel++;
}

void popFrame() {
    if (frameLevel <= 0) {
        fprintf(stderr, "Error: Frame stack underflow.\n");
        exit(1);
    }

    frameLevel--;
    charPos = frameStack[frameLevel].pos;
    offset = frameStack[frameLevel].offset;
}

void skipUntil(char open, char close) {
    int count = 1;

    while (count > 0) {
        nextChar();

        if (currentChar == '\0')
            break;

        if (currentChar == open) count++;
        else if (currentChar == close) count--;
    }
}

/* --------------------------------------------------------------------------
 * MAIN
 * -------------------------------------------------------------------------- */

int main(int argc, char const *argv[]) {

    if (argc < 2) {
        fprintf(stderr, "Usage: %s programfile\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "rb");
    if (!fp) {
        fprintf(stderr, "Cannot open file %s\n", argv[1]);
        return 1;
    }

    size_t size = fread(PROG, 1, MAX_PROG, fp);
    PROG[size] = '\0';
    fclose(fp);

    memset(DEF_MACRO, 0, sizeof(DEF_MACRO));
    memset(DATA, 0, sizeof(DATA));

    calcTop = 0;
    frameLevel = 0;
    charPos = -1;
    offset = 0;


    char prev = 0, curr = 0;
    for (int pos = 0; pos <= (int)size; pos++) {
        prev = curr;
        curr = PROG[pos];

        if (prev == '$' && curr >= 'A' && curr <= 'Z')
            DEF_MACRO[idxFromLetter(curr)] = pos + 1;

        if (prev == '$' && curr == '$')
            break;
    }


    charPos = -1;

    do {
        nextChar();

        switch (currentChar) {

        case ' ': case ']': case '$':
            break;

        case '0' ... '9': {
            int num = 0;
            while (currentChar >= '0' && currentChar <= '9') {
                num = num * 10 + digitVal(currentChar);
                nextChar();
            }
            pushCalc(num);

            charPos--;
            break;
        }

        case 'A' ... 'Z':
            pushCalc(idxFromLetter(currentChar) + offset);
            break;

        case '?': {
            int ch = getchar();
            pushCalc(ch);
            break;
        }

        case '!':
            putchar(popCalc());
            break;

        case '+': pushCalc(popCalc() + popCalc()); break;
        case '*': pushCalc(popCalc() * popCalc()); break;

        case '-': {
            int b = popCalc();
            pushCalc(popCalc() - b);
            break;
        }

        case '/': {
            int b = popCalc();
            pushCalc(popCalc() / b);
            break;
        }

        // read memory
        case '.':
            pushCalc(DATA[popCalc()]);
            break;

        // write memory
        case '=': {
            int val = popCalc();
            DATA[popCalc()] = val;
            break;
        }

        // string literal
        case '\"':
            while (1) {
                nextChar();
                if (currentChar == '\"' || currentChar == '\0')
                    break;

                if (currentChar == '!')
                    putchar('\n');
                else
                    putchar(currentChar);
            }
            break;

        case '[':
            if (popCalc() <= 0)
                skipUntil('[', ']');
            break;

        case '(':
            pushFrame(FRAME_LOOP);
            break;

        case '^':
            if (popCalc() <= 0)
                skipUntil('(', ')');
            break;

        case ')':
            if (frameLevel > 0)
                charPos = frameStack[frameLevel - 1].pos;
            break;

        case '#': { // macro call
            nextChar();
            if (currentChar >= 'A' && currentChar <= 'Z'
                && DEF_MACRO[idxFromLetter(currentChar)] > 0) {

                pushFrame(FRAME_MACRO);
                charPos = DEF_MACRO[idxFromLetter(currentChar)];
                offset += 26;

            } else {
                skipUntil('#', ';');
            }
            break;
        }

        case '@': // exit macro
            popFrame();
            skipUntil('#', ';');
            break;

        case '%': { // parameter reference
            nextChar();
            if (!(currentChar >= 'A' && currentChar <= 'Z')) break;

            int paramIndex = idxFromLetter(currentChar);
            pushFrame(FRAME_PARAM);

            int balance = 1;
            int t = frameLevel - 1;

            while (t >= 0 && balance != 0) {
                if (frameStack[t].type == FRAME_MACRO) balance--;
                else if (frameStack[t].type == FRAME_PARAM) balance++;
                t--;
            }

            if (t < 0) {
                popFrame();
                break;
            }

            charPos = frameStack[t].pos;
            offset = frameStack[t].offset;

            do {
                nextChar();

                if (currentChar == '#') {
                    skipUntil('#', ';');
                    nextChar();
                }
                if (currentChar == ',')
                    paramIndex--;

                if (currentChar == '\0') break;

            } while (paramIndex != 0 || currentChar != ';');

            if (currentChar == ';')
                popFrame();

            break;
        }

        case ',':
        case ';':
            popFrame();
            break;

        default:
            break;
        }

    } while (currentChar != '$' && currentChar != '\0');

    return 0;
}

