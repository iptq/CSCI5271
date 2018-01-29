/* Badly Coded Text Editor BCVI, a product of Badly Coded, Inc. */

/* This is an example insecure program for CSci 5271 only: don't copy
   code from here to any program that is supposed to work
   correctly! */

/*         version 1.0 was for exploits due  9/15/2017 */
/*         version 1.1 was for exploits due  9/22/2017. */
/*         version 1.2 was for exploits due  9/29/2017. */
/* This is version 1.3,    for exploits due 10/ 6/2017. */

long bcvi_version = 130; /* 1.3 */

#define _XOPEN_SOURCE_EXTENDED /* for wide characters in curses */

#include <ctype.h>
#include <errno.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curses.h>

#include <pwd.h>
#include <signal.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/time.h>
#include <sys/types.h>

#include <arpa/inet.h>

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

/* We implement our own version of assert() so that we can both
   restore the terminal settings and ensure the assertion message is
   visible. */
#define my_assert(expr)							\
    ((expr)								\
     ? (void)0								\
     : my_assert_fail(#expr, __FILE__, __LINE__, __PRETTY_FUNCTION__))


void my_assert_fail(const char *msg, const char *file, const int line,
		    const char *func) {
    if (!isendwin())
	endwin();
    fprintf(stderr, "bcvi: %s:%d: %s: Assertion `%s' failed.\n",
	    file, line, func, msg);
    abort();
}

#define MAX_CHARS 10000
#define MAX_LINES  2500

/* Gap buffer containing all the characters of the file being
   edited. The characters before the insertion point start at the
   beginning of this array, while the ones after the insertion point
   are at the end. */
char chars_buffer[MAX_CHARS];
char *chars_buf_end = chars_buffer + MAX_CHARS;
char *char_before = 0;
char *char_after = 0;

/* Compute the number of bytes in the buffer */
int num_chars(void) {
    int num = 0;
    if (char_before)
	num += char_before - chars_buffer + 1;
    if (char_after)
	num += chars_buf_end - char_after;
    return num;
}

/* Gap buffer of pointers to all of the newline characters in
   chars_buffer, for ease of moving line by line. */
char *lines_buffer[MAX_LINES];
char **lines_buf_end = lines_buffer + MAX_LINES;
char **nl_before = 0;
char **nl_after = 0;

/* Count the lines in the buffer. If the buffer contains N newlines,
   we say it has N+1 lines, numbered 0 through N. The final line,
   after the last newline, is often empty. */
int num_newlines(void) {
    int num = 0;
    if (nl_before)
	num += nl_before - lines_buffer + 1;
    if (nl_after)
	num += lines_buf_end - nl_after;
    return num;
}

int cur_line(void) {
    if (!nl_before)
	return 0;
    return nl_before - lines_buffer + 1;
}

int cur_column(void) {
    if (!nl_before) {
	if (!char_before)
	    return 0;
	return char_before - chars_buffer + 1;
    }
    return char_before - *nl_before;
}

/* Move the insertion point back one position, by moving the buffer
   gap forward. */
int prev_char(void) {
    if (!char_before)
	return 0; /* already at beginning */
    if (!char_after)
	char_after = chars_buf_end;
    /* pop from char_before, push to char_after */
    *(--char_after) = *(char_before--);
    if (char_before + 1 < char_after)
	*(char_before + 1) = 'x'; /* overwrite popped */
    if (*char_after == '\n') {
	/* moved from beginning of one line to end of previous */
	if (!nl_after)
	    nl_after = lines_buf_end;
	my_assert(nl_before);
	/* pop from nl_before */
	*(nl_before--) = 0;
	if (nl_before < lines_buffer)
	    nl_before = 0;
	/* push new newline location to nl_after */
	*(--nl_after) = char_after;
    }
    if (char_before < chars_buffer)
	char_before = 0; /* now at the beginning */
    return 1;
}

int next_char(void) {
    if (!char_after)
	return 0; /* already at end */
    if (!char_before)
	char_before = chars_buffer - 1;
    /* pop from char_after, push to char_before */
    *(++char_before) = *(char_after++);
    if (char_after - 1 > char_before)
	*(char_after - 1) = 'x'; /* overwrite popped */
    if (*char_before == '\n') {
	/* moved from end of one line to beginning of next */
	if (!nl_before)
	    nl_before = lines_buffer - 1;
	my_assert(nl_after);
	/* pop from nl_after */
	*(nl_after++) = 0;
	if (nl_after >= lines_buf_end)
	    nl_after = 0;
	/* push new newline location to nl_before */
	*(++nl_before) = char_before;
    }
    if (char_after >= chars_buf_end)
	char_after = 0; /* now at the end */
    return 1;
}

/* Delete a character from before the buffer gap */
int delete_before(void) {
    if (!char_before)
	return 0; /* nothing before */
    if (*char_before == '\n') {
	/* deleting a newline */
	my_assert(nl_before);
	my_assert(*nl_before == char_before);
	/* pop from nl_before */
	*(nl_before--) = 0;
	if (nl_before < lines_buffer)
	    nl_before = 0;
    }
    *(char_before--) = 'd'; /* overwrite deleted */
    if (char_before < chars_buffer)
	char_before = 0; /* now at the end */
    return 1;
}

/* Delete a character from after the buffer gap */
int delete_after(void) {
    if (!char_after)
	return 0; /* nothing after */
    if (*char_after == '\n') {
	/* deleting a newline */
	my_assert(nl_after);
	my_assert(*nl_after == char_after);
	/* pop from nl_after */
	*(nl_after++) = 0;
	if (nl_after >= lines_buf_end)
	    nl_after = 0;
    }
    *(char_after++) = 'd'; /* overwrite deleted */
    if (char_after >= chars_buf_end)
	char_after = 0; /* now at the end */
    return 1;
}

/* Insert a character before the buffer gap */
int insert_before(char ch) {
    if (!char_before)
	char_before = chars_buffer - 1;
    if ((char_after && (char_before + 1 >= char_after)) ||
	char_before >= chars_buf_end - 1)
	return 0; /* buffer full, can't add */
    if (ch == '\n') {
	if (!nl_before)
	    nl_before = lines_buffer - 1;
	if ((nl_after && nl_before + 1 >= nl_after) ||
	    nl_before + 1 >= lines_buf_end)
	    return 0; /* newline buffer full, can't add */
	*(++nl_before) = char_before + 1;
    }
    *(++char_before) = ch;
    return 1;
}

/* Insert a character after the buffer gap */
int insert_after(char ch) {
    if (!char_after)
	char_after = chars_buf_end;
    if ((char_before && char_after - 1 <= char_before) ||
	char_after <= chars_buffer)
	return 0; /* buffer full, can't add */
    if (ch == '\n') {
	if (!nl_after)
	    nl_after = lines_buf_end;
	if ((nl_before && nl_after - 1 <= nl_before) ||
	    nl_after <= lines_buffer)
	    return 0; /* newline buffer full, can't add */
	*(--nl_after) = char_after - 1;
    }
    *(--char_after) = ch;
    return 1;
}

/* apple \n banana \n cit       rus \n durian \n fig \n
        /         /                  \         \      |
    ---- ---------                    ------    -     |
   0    1                                   97   98  99

line 0: chars_buffer to lines_buffer[0]
line 1: lines_buffer[0] to lines_buffer[1]
line 2: lines_buffer[1] to char_before,
        also char_after to lines_buffer[MAX_LINES - 3]
line 3: lines_buffer[MAX_LINES - 3] to lines_buffer[MAX_LINES - 2]
line 4: lines_buffer[MAX_LINES - 2] to lines_buffer[MAX_LINES - 1]
line 5: lines_buffer[MAX_LINES - 1] to chars_buf_end (empty)

*/
/* Return pointers to the up to two contiguous sequences of characters
   that represent one logical line. Pointers and lengths are returned
   via the _p and _len arguments respectively. If the file contains N
   newlines, it has N+1 lines numbered 0 through N. Lines 0 through
   N-1 are the ones followed by newlines in the file, while line N is
   all the characters after the last newline (conventionally none).
   Check both e1_len and e2_len to determine which extents are
   non-empty; if both are non-empty, e1 comes before e2. */
int get_line_extents(int i, char **e1_p, int *e1_len,
		            char **e2_p, int *e2_len)
{
    int nls_before = 0, nls_after = 0, num_nls;
    if (nl_before)
	nls_before = nl_before - lines_buffer + 1;
    if (nl_after)
	nls_after = lines_buf_end - nl_after;
    num_nls = nls_before + nls_after;
    my_assert(i >= 0);
    my_assert(i <= num_nls);
    if (nl_before && i < nls_before) {
	/* entirely before the insertion point */
	char *next_nl;
	if (i == 0) {
	    /* starts at the very beginning of the buffer */
	    *e1_p = chars_buffer;
	} else {
	    /* starts after a newline */
	    *e1_p = lines_buffer[i - 1] + 1;
	}
	next_nl = lines_buffer[i];
	*e1_len = next_nl - *e1_p;
	*e2_len = 0;
	return 1;
    } else if (nl_after && i > nls_before) {
	/* entirely after the insertion point */
	char *next_nl;
	if (i == num_nls) {
	    /* ends at the very end of the buffer */
	    next_nl = chars_buf_end;
	} else {
	    next_nl = lines_buffer[MAX_LINES - num_nls + i];
	}
	*e1_p = lines_buffer[MAX_LINES - num_nls + i - 1] + 1;
	*e1_len = next_nl - *e1_p;
	*e2_len = 0;
	return 1;
    } else {
	/* the line containing the insertion point */
	char *start, *end;
	if (nl_before) {
	    start = *nl_before + 1;
	} else {
	    start = chars_buffer;
	}
	*e1_p = start;
	if (char_before) {
	    *e1_len = char_before - start + 1;
	} else {
	    *e1_len = 0;
	}
	if (nl_after) {
	    end = *nl_after;
	} else {
	    end = chars_buf_end;
	}
	if (char_after) {
	    *e2_p = char_after;
	} else {
	    *e2_p = chars_buf_end;
	}
	*e2_len = end - *e2_p;
	return 2;
    }
}

/* A more convenient wrapper around get_line_extents that copies the
   extents into a single contiguous buffer. */
int get_line(int line_num, char *buf, int buf_len, int include_term) {
    char *e1_p, *e2_p;
    int e1_len, e2_len;
    char *p;
    int buf_left = buf_len;
    int copy_limit;
    size_t copy_amt;
    get_line_extents(line_num, &e1_p, &e1_len, &e2_p, &e2_len);
    p = buf;
    copy_limit = MIN(MAX(e1_len, e2_len), buf_left);
    if (e1_len) {
	copy_amt = MIN(e1_len, copy_limit);
	memcpy(p, e1_p, copy_amt);
	p += copy_amt;
	buf_left -= copy_amt;
    }
    if (e2_len) {
	copy_amt = MIN(e2_len, copy_limit);
	memcpy(p, e2_p, copy_amt);
	p += copy_amt;
	buf_left -= copy_amt;
    }
    if (include_term && buf_left >= 2) {
	if (line_num != num_newlines())
	    *p++ = '\n';
	*p++ = 0;
    }
    return p - buf;
}

/* Replace the previous buffer contents, if any, with the contents of
   a file. */
void read_file(const char *fname) {
    FILE *fh;
    int res;
    char *p = chars_buffer;
    char **nlp = lines_buffer;
    fh = fopen(fname, "r");
    if (!fh) {
	fprintf(stderr, "Failed to open %s for reading: %s\n",
		fname, strerror(errno));
	exit(1);
    }
    while (p < chars_buf_end) {
	size_t size = MIN(4096, chars_buf_end - p);
	size_t num_read;
	char *next_p;
	num_read = fread(p, 1, size, fh);
	next_p = p + num_read;
	for (; p < next_p; p++) {
	    if (*p == '\n') {
		*(nlp++) = p;
		if (nlp >= lines_buffer + MAX_LINES) {
		    fprintf(stderr, "Too many lines!\n");
		    exit(1);
		}
	    }
	}
	if (num_read < size) {
	    if (feof(fh)) {
		break;
	    } else if (ferror(fh)) {
		fprintf(stderr, "Error when reading from %s\n", fname);
		exit(1);
	    }
	}
    }
    if (p > chars_buffer)
	char_before = p - 1;
    if (nlp > lines_buffer)
	nl_before = nlp - 1;
    char_after = 0;
    nl_after = 0;
    res = fclose(fh);
    if (res) {
	fprintf(stderr, "Failed to close %s: %s\n", fname, strerror(errno));
    }
    /* Move the insertion point back to the beginning */
    while (prev_char())
	;
}

/* Move the insertion point to the beginning of the current line:
   i.e., move it backwards until the previous character is a newline,
   or the beginning of the buffer is reached. */
void go_to_bol(void) {
    if (!char_before)
	return; /* already at beginning of file */
    if (*char_before == '\n')
	return; /* already right after a newline */
    while (char_before && *char_before != '\n')
	prev_char();
    return;
}

void go_to_column(int col) {
    int n = col;
    go_to_bol();
    while (n && char_after && *char_after != '\n') {
	next_char();
	n--;
    }
}

void go_to_eol(void) {
    if (!char_after)
	return; /* already at end of file */
    if (*char_after == '\n')
	return; /* already right before a newline */
    while (char_after && *char_after != '\n')
	next_char();
    return;
}

void go_to_last_char(void) {
    go_to_eol();
    if (char_before && *char_before != '\n')
	prev_char();
}

/* When we move up and down between lines, we'd like to stay in the
   same column, but we can't be at a position that doesn't exist in
   the current line. The goal column is the position we would like to
   be in, if the line is long enough. Some but not all motion commands
   update it. */
int goal_column = 0;

void save_column(void) {
    goal_column = cur_column();
}

void restore_column(void) {
    go_to_column(goal_column);
}

/* Move to the previous line, while keeping the same column if possible */
void prev_line(void) {
    go_to_bol();
    prev_char();
    restore_column();
}

void next_line(void) {
    go_to_eol();
    next_char();
    restore_column();
}

void go_to_last_line(void) {
    int more = 1;
    while (more)
	more = next_char();
    go_to_bol();
}

void replace_after(char ch) {
    if (char_after) {
	delete_after();
	insert_after(ch);
    }
}

char *error_msg = 0;
char *status_msg = 0;

/* Write the buffer contents to a file */
void fwrite_buffer(FILE *fh) {
    int i;
    int num_newlines0 = num_newlines();
    for (i = 0; i <= num_newlines0; i++) {
	char *e1_p, *e2_p;
	int e1_len, e2_len;
	get_line_extents(i, &e1_p, &e1_len, &e2_p, &e2_len);
	if (e1_len)
	    fwrite(e1_p, 1, e1_len, fh);
	if (e2_len)
	    fwrite(e2_p, 1, e2_len, fh);
	if (i < num_newlines0)
	    fputc('\n', fh);
    }
}

int sudo_mode = 0;

/* Make a command fully qualified and quote its arguments, for
   safety. For instance, turns "tr a-z A-Z" into "/usr/bin/tr 'a-z'
   'A-Z'". */
char *quoted_cmd(const char *prog, const char *args) {
    int buflen = 9 + strlen(prog) + 1 + 3*strlen(args) + 1;
    char *cmd_out = malloc(buflen);
    const char *p;
    char *q;
    int in_space;
    if (!cmd_out) {
	endwin();
	fprintf(stderr, "Memory allocation failure");
	exit(1);
    }
    strcpy(cmd_out, "/usr/bin/");
    strcat(cmd_out, prog);
    p = args;
    q = cmd_out + strlen(cmd_out);
    *q++ = ' ';
    in_space = 1;
    while (*p) {
	int next_is_space = isspace(*p) ? 1 : 0;
	if (next_is_space != in_space)
	    *q++ = '\'';
	*q++ = *p++;
	in_space = next_is_space;
    }
    if (!in_space)
	*q++ = '\'';
    *q++ = '\0';
    return cmd_out;
}

/* Filter the buffer contents through an external program */
void filter_buffer(char *cmd) {
    char cmd_buf[200];
    char temp_fname[] = "/tmp/bcvi_filt_XXXXXX";
    int temp_fd;
    int free_cmd = 0;
    FILE *pipe_fh;
    int saved_line, saved_col, i;
    saved_line = cur_line();
    saved_col = cur_column();
    if (strlen(cmd) + strlen(temp_fname) + 3 > sizeof(cmd_buf)) {
	error_msg = "Command too long in filter";
	return;
    }
    /* Sanitize the command to just something safe */
    if (!strcmp(cmd, "expand")) {
	cmd = "/usr/bin/expand";
    } else if (!sudo_mode && !memcmp(cmd, "tr ", 3)) {
	char *quoted;
	cmd[2] = '\0';
	quoted = quoted_cmd(cmd, cmd + 3);
	cmd = quoted;
	free_cmd = 1;
    } else {
	error_msg = "Unsupported command used to filter";
	return;
    }
    temp_fd = mkstemp(temp_fname);
    if (temp_fd == -1) {
	endwin();
	fprintf(stderr, "Failed to create temporary file: %s\n",
		strerror(errno));
	exit(1);
    }
    snprintf(cmd_buf, sizeof(cmd_buf), "%s >%s", cmd, temp_fname);
    if (free_cmd)
	free(cmd);
    pipe_fh = popen(cmd_buf, "w");
    fwrite_buffer(pipe_fh);
    pclose(pipe_fh);
    read_file(temp_fname);
    unlink(temp_fname);
    for (i = 0; i < saved_line; i++)
	next_line();
    go_to_column(saved_col);
}

const char *filename;
int readonly_mode = 0;
int modified = 0;
double modified_time = 0;

/* Return the time since the Unix epoch, in possibly-fractional
   seconds. */
double time_float(void) {
    char time_buf[100];
    struct timeval tv;
    gettimeofday(&tv, 0);
    snprintf(time_buf, sizeof(time_buf), "%ld.%ld", tv.tv_sec, tv.tv_usec);
    return strtod(time_buf, 0);
}

/* Record that the user modified the buffer, so we can warn them later
   if they try to quit without saving. */
void set_mod(void) {
    if (!modified) {
	modified_time = time_float();
    }
    modified = 1;
}

void clear_mod(void) {
    modified = 0;
}

char error_msg_buffer[100];

/* Save the file */
void write_file(void) {
    FILE *fh;
    if (readonly_mode) {
	error_msg = "Can't save: read-only file";
	return;
    }
    fh = fopen(filename, "w");
    if (!fh) {
	snprintf(error_msg_buffer, sizeof(error_msg_buffer),
		 "Failed to open `%.50s' for writing: %s",
		 filename, strerror(errno));
	error_msg = error_msg_buffer;
	return;
    }
    fwrite_buffer(fh);
    fclose(fh);
    clear_mod();
}

/* Check whether the ModR/M byte of an instruction is safe in that the
   effective destination is a caller-saved register %eax, %ecx, or
   %edx. */
int is_safe_dest_modrm(int b) {
    int dest_reg = b & 7;
    return b >= 0xc0 && (dest_reg <= 2);
}

/* Check whether the x86 instruction pointed to by "p" is safe to
   allow in a macro. Return 0 if unsafe, or the length of the
   instruction if safe. */
int insn_whitelist(const unsigned char *p) {
    int is64 = sizeof(long) > sizeof(int);
    int is32 = !is64;
    if (p[0] == 0x01 && is_safe_dest_modrm(p[1])) {
	/* Add two registers */
	return 2;
    } else if (p[0] == 0x09 && is_safe_dest_modrm(p[1])) {
	/* OR two registers */
	return 2;
    } else if (p[0] == 0x0d) {
	/* OR %eax with a constant */
	return 5;
    } else if (p[0] == 0x25) {
	/* AND %eax with a constant */
	return 5;
    } else if (p[0] == 0x31 && is_safe_dest_modrm(p[1])) {
	/* XOR of two registers */
	return 2;
    } else if (p[0] == 0x37 && is32) {
	/* AAA */
	return 1;
    } else if (p[0] == 0x3c) {
	/* Compare %al with byte constant */
	return 2;
    } else if (p[0] >= 0x40 && p[0] <= 0x42 && is32) {
	/* Inc/dec of register */
	return 1;
    } else if (p[0] == 0xb2) {
	/* Move 8-bit constant into register */
	return 2;
    } else if (p[0] == 0xc1 && is_safe_dest_modrm(p[1])) {
	/* Shift/rotate register by fixed amount */
	return 3;
    } else if (p[0] == 0xc3) {
	/* Return */
	return 1;
    } else if (p[0] == 0xf7 && p[1] >= 0xd0 && is_safe_dest_modrm(p[1])) {
	/* Unary arithmetic on register */
	return 2;
    } else if (p[0] == 0xfe && p[1] >= 0xc0 && p[1] <= 0xcf) {
	/* Inc/dec of byte register */
	return 2;
    } else if (p[0] == 0x0f) {
	if (p[1] >= 0x40 && p[1] <= 0x4f && p[2] >= 0xc0 && p[2] <= 0xd7) {
	    /* Conditional move between registers */
	    return 3;
	} else {
	    return 0;
	}
    } else {
	return 0;
    }
}

#define PAGE_SIZE 4096
unsigned char *macro_area = 0;

int run_macro(void) {
    int i, res;
    int illegal = -1;
    int saw_ret = 0;
    unsigned int x_host, x;
    unsigned char *code;
    if (!char_before || char_before - chars_buffer < 3) {
	error_msg = "Too few input characters for macro";
	return 0;
    }
    if (!char_after) {
	error_msg = "Missing code for macro";
	return 0;
    }
    for (i = 0; i < 100; ) {
	unsigned char b;
	int wl_len;
	if (char_after + i >= chars_buf_end)
	    break;
	b = char_after[i];
	if (b == 0xc3) {
	    saw_ret = 1;
	    i++;
	    break;
	}
	wl_len = insn_whitelist((unsigned char *)char_after + i);
	if (!wl_len) {
	    illegal = b;
	    break;
	}
	i += wl_len;
    }
    if (illegal != -1) {
	error_msg = "Illegal instruction in macro";
	return 0;
    } else if (!saw_ret) {
	error_msg = "Macro too long or missing return";
	return 0;
    }
    if (!macro_area) {
	macro_area = mmap(0, 3*PAGE_SIZE, PROT_READ|PROT_EXEC,
			  MAP_PRIVATE|MAP_ANONYMOUS,
			  -1, 0);
	if (macro_area == MAP_FAILED) {
	    error_msg = "Memory allocation for macro failed";
	    return 0;
	}
    }
    res = mprotect(macro_area, 3*PAGE_SIZE, PROT_READ|PROT_WRITE);
    if (res) {
	error_msg = "Failed to modify memory permissions";
	return 0;
    }
    /* 0xf4 is the opcode of an illegal instruction (hlt) */
    memset(macro_area, 0xf4, 3*PAGE_SIZE);
    code = macro_area + 4096;
    memcpy(code, char_after, i);
    res = mprotect(macro_area, 3*PAGE_SIZE, PROT_READ|PROT_EXEC);
    if (res) {
	error_msg = "Failed to modify memory permissions";
	return 0;
    }

    memcpy(&x_host, char_before - 3, 4);
    x = htonl(x_host);
    /* The calling convention of macros is that both the input and
       output are in %eax */
    asm("call *%1" : "+a" (x) : "r" (code));
    x_host = ntohl(x);
    memcpy(char_before - 3, &x_host, 4);
    status_msg = "Macro executed.";
    set_mod();
    return 1;
}

/* We assume a standard 80x24 terminal, where the last row is used for
   commands and status messages. */
#define WINDOW_HEIGHT 23
#define WINDOW_WIDTH 80

void page_down(void) {
    int i;
    for (i = 0; i < WINDOW_HEIGHT; i++)
	next_line();
}

void page_up(void) {
    int i;
    for (i = 0; i < WINDOW_HEIGHT; i++)
	prev_line();
}

int window_first_line = 0;

void signal_cleanup(int signal) {
    endwin();
    exit(1);
}

int mvadd_wchar(int r, int c, int wc) {
    cchar_t wchar;
    wchar.attr = 0;
    wchar.chars[0] = wc;
    wchar.chars[1] = 0;
    return mvadd_wch(r, c, &wchar);
}

/* Decide how to print a Windows-1252 character, using reverse video
   and Unicode to be relatively unambiguous. Generally, printable
   characters are passed through using the Unicode mapping of
   Windows-1252, while characters that don't have a distinguishable
   glyph are replaced with reversed ASCII characters.
*/
int printable_char(char c, int *ascii_out, int *unicode_out,
		   int *reverse_out) {
    int unicode = 0;
    int ascii = 0;
    int reverse = 0;
    unsigned char uc = c;
    if (c >= ' ' && c <= '~') {
	/* printable ASCII */
	ascii = c;
    } else if (uc >= 0xa1 && uc <= 0xff && uc != 0xad) {
	/* printable Latin-1 */
	unicode = uc;
    } else {
	switch (c) {
	case '\0':   ascii = '0'; reverse = 1; break;
	case '\x01': ascii = 'A'; reverse = 1; break;
	case '\x02': ascii = 'B'; reverse = 1; break;
	case '\x03': ascii = 'C'; reverse = 1; break;
	case '\x04': ascii = 'D'; reverse = 1; break;
	case '\x05': ascii = 'E'; reverse = 1; break;
	case '\x06': ascii = 'F'; reverse = 1; break;
	case '\x07': ascii = 'a'; reverse = 1; break;
	case '\x08': ascii = 'b'; reverse = 1; break;
	case '\x09': ascii = 't'; reverse = 1; break;
	case '\x0a': ascii = 'n'; reverse = 1; break;
	case '\x0b': ascii = 'v'; reverse = 1; break;
	case '\x0c': ascii = 'f'; reverse = 1; break;
	case '\x0d': ascii = 'r'; reverse = 1; break;
	case '\x0e': ascii = 'N'; reverse = 1; break;
	case '\x0f': ascii = 'O'; reverse = 1; break;
	case '\x10': ascii = 'P'; reverse = 1; break;
	case '\x11': ascii = 'Q'; reverse = 1; break;
	case '\x12': ascii = 'R'; reverse = 1; break;
	case '\x13': ascii = 'S'; reverse = 1; break;
	case '\x14': ascii = 'T'; reverse = 1; break;
	case '\x15': ascii = 'U'; reverse = 1; break;
	case '\x16': ascii = 'V'; reverse = 1; break;
	case '\x17': ascii = 'W'; reverse = 1; break;
	case '\x18': ascii = 'X'; reverse = 1; break;
	case '\x19': ascii = 'Y'; reverse = 1; break;
	case '\x1a': ascii = 'Z'; reverse = 1; break;
	case '\x1b': ascii = 'e'; reverse = 1; break;
	case '\x1c': ascii = '\\'; reverse = 1; break;
	case '\x1d': ascii = ']'; reverse = 1; break;
	case '\x1e': ascii = '^'; reverse = 1; break;
	case '\x1f': ascii = '_'; reverse = 1; break;
	case '\x7f': ascii = '?'; reverse = 1; break;
	case '\x80': unicode = 0x20ac; break;
	case '\x81': ascii = '1'; reverse = 1;  break;
	case '\x82': unicode = 0x201a; break;
	case '\x83': unicode = 0x0192; break;
	case '\x84': unicode = 0x201e; break;
	case '\x85': unicode = 0x2026; break;
	case '\x86': unicode = 0x2020; break;
	case '\x87': unicode = 0x2021; break;
	case '\x88': unicode = 0x02c6; break;
	case '\x89': unicode = 0x2030; break;
	case '\x8a': unicode = 0x0160; break;
	case '\x8b': unicode = 0x2039; break;
	case '\x8c': unicode = 0x0152; break;
	case '\x8d': ascii = '2'; reverse = 1; break;
	case '\x8e': unicode = 0x017d; break;
	case '\x8f': ascii = '3'; reverse = 1; break;
	case '\x90': ascii = '4'; reverse = 1; break;
	case '\x91': unicode = 0x2018; break;
	case '\x92': unicode = 0x2019; break;
	case '\x93': unicode = 0x201c; break;
	case '\x94': unicode = 0x201d; break;
	case '\x95': unicode = 0x2022; break;
	case '\x96': unicode = 0x2013; break;
	case '\x97': unicode = 0x2014; break;
	case '\x98': unicode = 0x20dc; break;
	case '\x99': unicode = 0x2122; break;
	case '\x9a': unicode = 0x0161; break;
	case '\x9b': unicode = 0x203a; break;
	case '\x9c': unicode = 0x0153; break;
	case '\x9d': ascii = '5'; reverse = 1; break;
	case '\x9e': unicode = 0x017e; break;
	case '\x9f': unicode = 0x0178; break;
	case '\xa0': ascii = ' '; reverse = 1; break;
	case '\xad': ascii = '-'; reverse = 1; break;
	}
    }
    my_assert(ascii != 0 || unicode != 0);
    *ascii_out = ascii;
    *unicode_out = unicode;
    *reverse_out = reverse;
    return (unicode != 0);
}

/* Redraw the screen based on the current buffer contents and window
   and cursor locations. Note that we always redraw the whole screen,
   since that's simplest. The curses library ensures that this doesn't
   lead to excessive screen updates or flicker. But curses had to work
   somewhat hard to determine that the screen has hardly changed, so
   this is not the most efficient approach. */
void redraw(void) {
    int r, c;
    char line_buf[200];
    if (LINES < WINDOW_HEIGHT + 1) {
	endwin();
	fprintf(stderr, "Terminal too small: must be at least %d lines\n",
		WINDOW_HEIGHT + 1);
	exit(1);
    }
    if (COLS < WINDOW_WIDTH) {
	endwin();
	fprintf(stderr, "Terminal too small: must be at least %d columns\n",
		WINDOW_WIDTH);
	exit(1);
    }
    for (r = 0; r < WINDOW_HEIGHT + 1; r++) {
	for (c = 0; c < WINDOW_WIDTH; c++) {
	    mvaddch(r, c, ' ');
	}
    }
    for (r = 0; r < WINDOW_HEIGHT; r++) {
	int len;
	int line_num = r + window_first_line;
	int print_len;
	if (line_num > num_newlines()) {
	    line_buf[0] = '~';
	    len = 1;
	} else {
	    len = get_line(line_num, line_buf, sizeof(line_buf), 0);
	}
	print_len = MIN(len, WINDOW_WIDTH - 1);
	for (c = 0; c < print_len; c++) {
	    int ascii, unicode, reverse;
	    if (printable_char(line_buf[c], &ascii, &unicode, &reverse)) {
		mvadd_wchar(r, c, unicode);
	    } else {
		mvaddch(r, c, ascii | (reverse ? A_REVERSE : 0));
	    }
	}
	if (print_len < len) {
	    /* Print a line-too-long symbol */
	    mvaddch(r, c, '$' | A_REVERSE);
	    c++;
	} else if (c < WINDOW_WIDTH && line_num < num_newlines()) {
	    /* Print the newline, which isn't otherwise counted as
	       part of the line. */
	    mvaddch(r, c, 'n' | A_REVERSE);
	    // mvadd_wchar(r, c, 0x23ce);
	    c++;
	}
	for (; c < WINDOW_WIDTH; c++) {
	    mvaddch(r, c, ' ');
	}
    }
    if (error_msg) {
	/* Error message: reverse video, goes away immediately */
	attron(A_REVERSE);
	mvaddstr(WINDOW_HEIGHT, 0, error_msg);
	attroff(A_REVERSE);
	error_msg = 0;
    } else if (status_msg) {
	/* Status messge: bold, stays up until modified */
	attron(A_BOLD);
	mvaddstr(WINDOW_HEIGHT, 0, status_msg);
	attroff(A_BOLD);
    }
    move(cur_line() - window_first_line, MIN(WINDOW_WIDTH - 1, cur_column()));
    refresh();
}

int parse_hex_nibble(char c) {
    if (c >= 'A' && c <= 'F') {
	return 10 + c - 'A';
    } else if (c >= 'a' && c <= 'f') {
	return 10 + c - 'A';
    } else if (c >= '0' && c <= '9') {
	return c - '0';
    } else {
	my_assert(0 && "Unexpected character in parse_hex_nibble");
	return 0;
    }
}

int parse_hex_byte(char *p) {
    return 16*parse_hex_nibble(p[0]) + parse_hex_nibble(p[1]);
}

void ex_mode(void) {
    char cmd_buf[100];
    mvaddch(WINDOW_HEIGHT, 0, ':');
    echo();
    mvgetnstr(WINDOW_HEIGHT, 1, cmd_buf, MIN(COLS, sizeof(cmd_buf) - 1));
    noecho();
    if (cmd_buf[0] == 'q' && !cmd_buf[1]) {
	/* exit if unchanged, otherwise warn */
	double age = 0;
	if (modified) {
	    age = time_float() - modified_time;
	}
	if (age > 0) {
	    snprintf(error_msg_buffer, sizeof(error_msg_buffer),
		     "Changes from last %.1f s not saved. "
		     "Use :q! to quit anyway.",
		     age);
	    error_msg = error_msg_buffer;
	} else if (age < 0) {
	    /* Can't happen */
	    modified = 0;
	    readonly_mode = 0;
	} else {
	    endwin();
	    exit(0);
	}
    } else if (cmd_buf[0] == 'q' && cmd_buf[1] == '!' && !cmd_buf[2]) {
	/* exit unconditionally */
	endwin();
	exit(0);
    } else if (cmd_buf[0] == 'w' && !cmd_buf[1]) {
	write_file();
    } else if (cmd_buf[0] == 'i' && cmd_buf[1] == 'h'
	       && isxdigit(cmd_buf[2]) && isxdigit(cmd_buf[3])
	       && !cmd_buf[4]) {
	/* insert a single character by its two-digit hex code */
	insert_before(parse_hex_byte(&cmd_buf[2])); set_mod();
    } else if (cmd_buf[0] == '%' && cmd_buf[1] == '!') {
	filter_buffer(cmd_buf + 2); set_mod();
    }
}

/* In insert mode, most keys just insert their character into the
   buffer. */
void insert_mode(void) {
    status_msg = "-- INSERT --";
    for (;;) {
	redraw();
	int ch = getch();
	switch (ch) {
	case 'h': case KEY_LEFT:
	    prev_char(); save_column(); break;
	case 8: case KEY_BACKSPACE:
	    delete_before(); save_column(); break;
	case 'j': case KEY_DOWN:
	    next_line(); break;
	case 'k': case KEY_UP:
	    prev_line(); break;
	case 'l': case KEY_RIGHT:
	    next_char(); save_column(); break;
	case ERR: /* E.g., EOF */
	case 0x1b: /* escape */
	    status_msg = 0;
	    return;
	case '\r':
	    insert_before('\n'); set_mod(); break;
	default:
	    insert_before(ch); set_mod(); break;
	}
    }
}

/* This outer loop implements the command-mode commands. */
void command_loop(void) {
    for (;;) {
	redraw();
	int ch = getch();
	switch (ch) {
	case 'h': case KEY_LEFT: case 8: case KEY_BACKSPACE:
	    prev_char(); save_column(); break;
	case 'j': case KEY_DOWN:
	    next_line(); break;
	case 'k': case KEY_UP:
	    prev_line(); break;
	case 'l': case KEY_RIGHT:
	    next_char(); save_column(); break;
	case KEY_NPAGE:
	    page_down(); break;
	case KEY_PPAGE:
	    page_up(); break;
	case 'G':
	    go_to_last_line(); break;
	case 'X': delete_before(); save_column(); set_mod(); break;
	case 'x': delete_after(); save_column(); set_mod(); break;
	case 'J': go_to_eol(); replace_after(' ');
	    save_column(); set_mod(); break;
	case '^': go_to_bol(); save_column(); break;
	case '$': go_to_last_char(); save_column(); break;
	case ':': ex_mode(); break;
	case 'i': insert_mode(); break;
	case 'I': go_to_bol(); insert_mode(); break;
	case 'a': next_char(); insert_mode(); break;
	case 'A': go_to_eol(); insert_mode(); break;
	case 'R': run_macro(); break;
	}
	if (cur_line() < window_first_line) {
	    /* scroll up so cursor is on first line */
	    window_first_line = cur_line();
	} else if (cur_line() >= window_first_line + WINDOW_HEIGHT) {
	    /* scroll down so cursor is on last line */
	    window_first_line = cur_line() - WINDOW_HEIGHT + 1;
	}
    }
}

char welcome_msg_buffer[1000];

/* Print a greeting message */
void welcome_msg(void) {
    char buf[1000];
    const char *username = getenv("USER");
    if (!username) {
	username = "fine user";
    }
    snprintf(buf, sizeof(buf), "Hello %s, welcome to BCVI", username);
    if (strlen(buf) < 900) {
	strcat(buf, " version %3.1f");
    }
    snprintf(welcome_msg_buffer, sizeof(welcome_msg_buffer), buf,
	     bcvi_version / 100.0);
    error_msg = welcome_msg_buffer;
}

/* If we were run as "sudobcvi", enable extra checks to keep from
   exceeding the user's authority. */
void check_sudo_mode(const char *argv0) {
    size_t len = strlen(argv0);
    if ((len >= 8 && !strcmp(argv0 + len - 8, "sudobcvi")) ||
	(len >= 10 && !strcmp(argv0 + len - 10, "sudobcvi64"))) {
	sudo_mode = 1;
	status_msg = "In sudobcvi mode";
    } else if (getuid() != geteuid()) {
	sudo_mode = 1;
	status_msg = "In sudobcvi (setuid) mode";
    }
}

/* In sudobcvi mode, check if it's OK to edit a file, as based either
   on the normal file permissions or the system configuration file. */
void check_sudo_open(const char *filename) {
    int res;
    char line_buf[100];
    int lineno = 1;
    struct passwd *pw_info;
    errno = 0;
    pw_info = getpwuid(getuid());
    if (!pw_info) {
	fprintf(stderr, "Could not find user name: %s\n", strerror(errno));
	exit(1);
    }
    /* If you're testing BCVI on a machine where you don't have root
       access, you might want to temporarily change this config file
       location to a place you can modify. */
    FILE *fh = fopen("/etc/sudobcvi.conf", "r");
    if (!fh) {
	fprintf(stderr, "Could not open sudobcvi.conf: %s\n", strerror(errno));
	exit(1);
    }
    while (fgets(line_buf, sizeof(line_buf), fh)) {
	char *conf_user, *conf_fname, *nl_p;
	char *sep_p = strchr(line_buf, ':');
	if (!sep_p) {
	    fprintf(stderr, "Missing : on line %d of sudobcvi.conf", lineno);
	    exit(1);
	}
	*sep_p = '\0';
	conf_user = line_buf;
	conf_fname = sep_p + 1;
	nl_p = strchr(conf_fname, '\n');
	if (!nl_p) {
	    fprintf(stderr, "Truncated/incomplete line %d of sudobcvi.conf",
		    lineno);
	    exit(1);
	}
	*nl_p = '\0';
	if (!strcmp(pw_info->pw_name, conf_user)
	    && !strcmp(filename, conf_fname)) {
	    /* Permission granted by configuration file */
	    fclose(fh);
	    return;
	}
	lineno++;
    }
    fclose(fh);
    /* No match in config file, check regular file permissions */
    res = access(filename, R_OK);
    if (res == -1) {
	fprintf(stderr, "You do not have permission to read %s\n", filename);
	exit(1);
    }
    res = access(filename, W_OK);
    if (res == -1) {
	readonly_mode = 1;
	error_msg = "[readonly]";
    }
}

int main(int argc, char **argv) {
    check_sudo_mode(argv[0]);
    setlocale(LC_ALL, "");
    if (argc != 2) {
	fprintf(stderr, "Usage: bcvi file.txt\n");
	exit(1);
    }
    filename = argv[1];
    if (sudo_mode)
	check_sudo_open(filename);
    read_file(filename);
    welcome_msg();
    clear_mod();

    if (signal(SIGINT, signal_cleanup) == SIG_ERR) {
	fprintf(stderr, "Failed to setup up SIGINT handler: %s\n",
		strerror(errno));
	exit(1);
    }

    /* Curses setup */
    initscr();
    keypad(stdscr, TRUE); /* interpret arrow keys */
    nonl(); /* skip LF -> CRLF conversion */
    noecho(); /* do not echo keystrokes */
    cbreak(); /* read one character at a time */

    command_loop();

    endwin();

    return 0;
}
