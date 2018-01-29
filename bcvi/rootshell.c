#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <fcntl.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <termios.h>

/* Effectively a simplified version of /bin/id */
void print_ids(void) {
    uid_t uid = getuid();
    uid_t euid = geteuid();
    gid_t gid = getgid();
    gid_t egid = getegid();
    struct passwd *pw;
    struct group *grp;
    pw = getpwuid(uid);
    printf("uid=%d(%s) ", uid, pw ? pw->pw_name : "?");
    pw = getpwuid(euid);
    printf("euid=%d(%s) ", euid, pw ? pw->pw_name : "?");
    grp = getgrgid(gid);
    printf("gid=%d(%s) ", gid, grp ? grp->gr_name : "?");
    grp = getgrgid(egid);
    printf("egid=%d(%s)", egid, grp ? grp->gr_name : "?");
    printf("\n");
}

void restore_tty_sysopen(void) {
    if (!isatty(0)) {
        int newfd;
        printf("Reopening stdin to /dev/tty: ");
        close(0); /* OK if it's not actually open */
        newfd = open("/dev/tty", O_RDONLY);
        if (newfd == 0) {
            printf("success.\n");
        } else if (newfd == -1) {
            printf("failed, %s. Continuing\n", strerror(errno));
        } else {
            printf("failed, unexpected FD %d. Continuing\n", newfd);
        }
    }
    if (!isatty(1)) {
        int newfd;
        printf("Reopening stdout to /dev/tty: ");
        close(1); /* OK if it's not actually open */
        newfd = open("/dev/tty", O_WRONLY);
        if (newfd == 1) {
            printf("success.\n");
        } else if (newfd == -1) {
            printf("failed, %s. Continuing\n", strerror(errno));
        } else {
            printf("failed, unexpected FD %d. Continuing\n", newfd);
        }
    }
}

void restore_tty_stdio(void) {
    if (!isatty(0)) {
        FILE *res;
        printf("Reopening stdin to /dev/tty: ");
        res = freopen("/dev/tty", "r", stdin);
        if (!res) {
            printf("failed, %s. Continuing\n", strerror(errno));
        } else {
            printf("success.\n");
        }
    }
    if (!isatty(1)) {
        FILE *res;
        fprintf(stderr, "Reopening stdout to /dev/tty: ");
        res = freopen("/dev/tty", "w", stdout);
        if (!res) {
            fprintf(stderr, "failed, %s. Continuing\n", strerror(errno));
        } else {
            fprintf(stderr, "success.\n");
        }
    }
}

void sane_tty(void) {
    struct termios t;
    tcgetattr(0, &t);
    t.c_iflag |= ICRNL;
    t.c_oflag |= ONLCR;
    tcsetattr(0, TCSADRAIN, &t);
}

void run_shell(void) {
    execl("/bin/bash", "bash", (const char *)0);
    printf("Exec failed: %s\n", strerror(errno));
}

int main(int argc, char **argv) {
    restore_tty_stdio();
    sane_tty();
    print_ids();
    if (getuid() == 0 || geteuid() == 0) {
        /* Make sure all the different UIDs are all root */
        if (geteuid() != 0)
            seteuid(0);
        setuid(0);
        printf("Congratulations, you're root. Here's your shell:\n");
        run_shell();
    } else {
        printf("Only root may run `rootshell', go away.\n");
    }
    return 0;
}
