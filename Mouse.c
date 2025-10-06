#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef enum tagtype { MACRO, PARAM, LOOP } TAGTYPE;
typedef struct frametipe {
    TAGTYPE TAG;
    int POS, OFF;
} FRAMETYPE;

char PROG[500 + 2]; 
int DEFINITION[26], CALSTACK[20];
FRAMETYPE STACK[20];
int DATA[260];
int CAL, CHPOS, LEVEL, OFFSET, PARNUM, PARBAL, TEMP;
char CH;

int NUM(char CH) {
    return CH - 'A';
}

int VAL(char CH) {
    return CH - '0';
}

void GETCHAR() {
   
    if (CHPOS + 1 >= 500) {
        CH = '\0';
        CHPOS++;
    } else {
        CH = PROG[++CHPOS];
    }
}

void PUSHCAL(int DATANUM) {
    if (CAL < 20) CALSTACK[CAL++] = DATANUM;
    else {
        fprintf(stderr, "CALSTACK overflow\n");
        exit(1);
    }
}

int POPCAL() {
    if (CAL <= 0) {
        fprintf(stderr, "CALSTACK underflow\n");
        exit(1);
    }
    return CALSTACK[--CAL];
}

void PUSH(TAGTYPE TAGVAL) {
    if (LEVEL < 20) {
        STACK[LEVEL].TAG = TAGVAL;
        STACK[LEVEL].POS = CHPOS;
        STACK[LEVEL].OFF = OFFSET;
        LEVEL++;
    } else {
        fprintf(stderr, "STACK overflow\n");
        exit(1);
    }
}

void POP() {
    if (LEVEL <= 0) {
        fprintf(stderr, "STACK underflow\n");
        exit(1);
    }
    LEVEL--;
    CHPOS = STACK[LEVEL].POS;
    OFFSET = STACK[LEVEL].OFF;
}

void SKIP(char LCH, char RCH) {
    int CNT = 1;
    do {
        GETCHAR();
        if (CH == LCH)
            CNT++;
        else if (CH == RCH)
            CNT--;
        if (CH == '\0') break;
    } while (CNT != 0);
}

int main(int argc, char const *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s progfile\n", argv[0]);
        return 1;
    }

    FILE *fp = fopen(argv[1], "rb");
    size_t size = 0;
    if (fp != NULL) {
        if (fseek(fp, 0, SEEK_END) == 0) {
            long t = ftell(fp);
            if (t > 0 && t <= 500) {
                size = (size_t)t;
                fseek(fp, 0, SEEK_SET);
                size_t read = fread(PROG, 1, size, fp);
                PROG[read] = '\0';
            } else {
                fseek(fp, 0, SEEK_SET);
                size = fread(PROG, 1, 500, fp);
                PROG[size] = '\0';
            }
        }
        fclose(fp);
    } else {
        fprintf(stderr, "Cannot open file %s\n", argv[1]);
        return 1;
    }

    for (int i = 0; i < 26; i++) DEFINITION[i] = 0;
    CAL = 0; CHPOS = -1; LEVEL = 0; OFFSET = 0;
    for (int i = 0; i < 260; i++) DATA[i] = 0;

  
    int pos = -1;
    char last = 0;
    char this = 0;
    do {
        last = this;
        pos++;
        if ((size_t)pos <= size)
            this = PROG[pos];
        else
            this = '\0';

        if (last == '$' && this >= 'A' && this <= 'Z') {
            
            DEFINITION[NUM(this)] = pos + 1;
        }

    } while (!((this == '$') && (last == '$')) && pos < (int)size && this != '\0');

    CHPOS = -1; LEVEL = 0; OFFSET = 0; CAL = 0;
    do {
        GETCHAR();
        switch (CH) {
        case ' ': case ']': case '$':
            break;
        case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7': case '8': case '9':
            TEMP = 0;
            while (CH >= '0' && CH <= '9') {
                TEMP = 10 * TEMP + VAL(CH);
                GETCHAR();
            }
            PUSHCAL(TEMP);
           
            CHPOS--;
            if (CHPOS < -1) CHPOS = -1;
            break;
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G': case 'H': case 'I': case 'J':
        case 'K': case 'L': case 'M': case 'N': case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T':
        case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
            PUSHCAL(NUM(CH) + OFFSET);
            break;
        case '?':
            TEMP = getchar();
            PUSHCAL(TEMP);
            break;
        case '!':
            putchar(POPCAL());
            break;
        case '+':
            PUSHCAL(POPCAL() + POPCAL());
            break;
        case '-':
            TEMP = POPCAL();
            PUSHCAL(POPCAL() - TEMP);
            break;
        case '*':
            PUSHCAL(POPCAL() * POPCAL());
            break;
        case '/': {
            TEMP = POPCAL();
            PUSHCAL(POPCAL() / TEMP);
        } break;
        case '.':
            PUSHCAL(DATA[POPCAL()]);
            break;
        case '=': {
            TEMP = POPCAL();
            DATA[POPCAL()] = TEMP;
        } break;
        case '\"':
            do {
                GETCHAR();
                if (CH == '!')
                    putchar('\n');
                else if (CH != '\"' && CH != '\0')
                    putchar(CH);
            } while (CH != '\"' && CH != '\0');
            break;
        case '[':
            if (POPCAL() <= 0) {
                SKIP('[', ']');
            }
            break;
        case '(':
            PUSH(LOOP);
            break;
        case '^':
            if (POPCAL() <= 0) {
                SKIP('(', ')');
            }
            break;
        case ')':
            if (LEVEL > 0) {
                CHPOS = STACK[LEVEL - 1].POS;
            }
            break;
        case '#': {
            GETCHAR();
            if (CH >= 'A' && CH <= 'Z' && DEFINITION[NUM(CH)] > 0) {
                PUSH(MACRO);
                CHPOS = DEFINITION[NUM(CH)];
                OFFSET = OFFSET + 26;
            } else {
                SKIP('#', ';');
            }
        } break;
        case '@':
            POP();
            SKIP('#', ';');
            break;
        case '%': {
            GETCHAR();
            if (!(CH >= 'A' && CH <= 'Z')) break;
            PARNUM = NUM(CH);
            PUSH(PARAM);
            PARBAL = 1;
            TEMP = LEVEL;
            do {
                TEMP--;
                if (TEMP < 0) break;
                switch (STACK[TEMP].TAG) {
                case MACRO:
                    PARBAL--;
                    break;
                case PARAM:
                    PARBAL++;
                    break;
                case LOOP:
                    break;
                }
            } while (PARBAL != 0 && TEMP > 0);
            if (TEMP < 0) {
                /* non trovato */
                POP();
                break;
            }
            CHPOS = STACK[TEMP].POS;
            OFFSET = STACK[TEMP].OFF;
            do {
                GETCHAR();
                if (CH == '#') {
                    SKIP('#', ';');
                    GETCHAR();
                }
                if (CH == ',') {
                    PARNUM--;
                }
                if (CH == '\0') break;
            } while (PARNUM != 0 || CH != ';');
            if (CH == ';') {
                POP();
            }
        } break;
        case ',':
        case ';':
            POP();
            break;
        default:
            break;
        }
    } while (CH != '$' && CH != '\0');

    return 0;
}
