#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* strlcpy: secure version of strcpy(), copied from OpenBSD */
/*
 * Copy src to string dst of size siz.  At most siz-1 characters
 * will be copied.  Always NUL terminates (unless siz == 0).
 * Returns strlen(src); if retval >= siz, truncation occurred.
 */
size_t
strlcpy(char *dst, const char *src, size_t siz)
{
        char *d = dst;
        const char *s = src;
        size_t n = siz;

        /* Copy as many bytes as will fit */
        if (n != 0 && --n != 0) {
                do {
                        if ((*d++ = *s++) == 0)
                                break;
                } while (--n != 0);
        }

        /* Not enough room in dst, add NUL and traverse rest of src */
        if (n == 0) {
                if (siz != 0)
                        *d = '\0';              /* NUL-terminate dst */
                while (*s++)
                        ;
        }

        return(s - src - 1);    /* count does not include NUL */
}

void print_arg(char *str) {
    char buf[20];
    int len;
    int buf_sz = (sizeof(buf) - sizeof(NULL)) * sizeof(char *);
    len = strlcpy(buf, str, buf_sz);
    if (len > buf_sz) {
	fprintf(stderr, "Trucation occured when printing %s\n", str);
    }
    fwrite(buf, sizeof(char), len, stdout);
}

int main(int argc, char **argv) {
    int i;
    for (i = 1; i < argc; i++) {
	print_arg(argv[i]);
	if (i + 1 != argc) {
	    putchar(' ');
	}
    }
    putchar('\n');
    return 0;
}