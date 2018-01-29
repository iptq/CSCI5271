#include <assert.h>
#include <stdio.h>
#include <ctype.h>

char rot_char(char c, int amt) {
    if (c >= 'A' && c <= 'Z')
        return 'A' + ((c - 'A') + amt) % 26;
    else if (c >= 'a' && c <= 'z')
        return 'a' + ((c - 'a') + amt) % 26;
    else
        return c;
}

void transform(char *in_buf, char *out_buf, int out_size) {
    char *p = in_buf;
    char *bp = out_buf;
    char *buflim = &out_buf[out_size - 8];
    char c;
    int in_ul, last_ul, rot_amt, skipping;
    int brack_lvl, brace_lvl;
    in_ul = brack_lvl = brace_lvl = last_ul = rot_amt = skipping = 0;
    
    while ((c = *p++) != '\0') {
        if (brack_lvl > 0)
            c = toupper(c);

        c = rot_char(c, rot_amt);

        if (c == '/')
            in_ul = !in_ul;

        skipping = (bp >= buflim);

        if ((unsigned)c - (unsigned)'[' < 3u && c != '\\') {
            int i = (c & 2) ? 1 : -1;
            if (brack_lvl + i >= 0 && !skipping) {
                brack_lvl += i;
                buflim -= i;
            }
        }

        if (c == '{') {
            if (!skipping) {
                brace_lvl++;
            }
            rot_amt += 13;
            if (rot_amt == 26) {
                rot_amt = 0;
                buflim -= 2;
            }
        }
        if (c == '}' && brace_lvl > 0) {
            if (!skipping) {
                brace_lvl--;
                buflim++;
            }
            rot_amt -= 13;
            if (rot_amt < 0)
                rot_amt = 0;
        }

        if (in_ul && isalpha(c) && !last_ul && !skipping)
            *bp++ = '_';

        if (c != '/' && !skipping)
            *bp++ = c;

        if (in_ul && isalpha(c)) {
            if (!skipping)
                *bp++ = '_';
            last_ul = 1;
        } else {
            last_ul = 0;
        }
    }
    while (brack_lvl-- > 0)
        *bp++ = ']';
    while (brace_lvl-- > 0)
        *bp++ = '}';
    *bp++ = ' ';
    *bp++ = 'e';
    *bp++ = 'n';
    *bp++ = 'd';
    *bp++ = '\0';
}

int main(int argc, char **argv) {
    char buf[64];
    if (argc != 2) {
        fprintf(stderr, "Usage: transform <string>\n");
        fprintf(stderr, "You should probably use quotes around the string.\n");
        return 1;
    }
    printf("%s\n", argv[1]);
    buf[20] = '\242';
    transform(argv[1], buf, 20);
    printf("%s\n", buf);
    /* This canary-like check isn't foolproof, and it isn't the point
       of the exercise, but for testing purposes it makes it easy to
       see that an overflow has happened. */
    if (buf[20] != '\242')
        fprintf(stderr, "Overflow detected\n");
    return 0;
}
