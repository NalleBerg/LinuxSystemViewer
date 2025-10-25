#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

/*
 * Simple, small elevation helper intended to be installed as setuid-root.
 * Usage: lsv-elevate /absolute/path/to/LSV [args...]
 *
 * Security notes:
 * - This program intentionally restricts allowed target basenames to a short
 *   whitelist (LSV, lsv, lsv-x86_64.AppImage). Adjust with care.
 * - The safest installation is to place this binary in /usr/local/bin,
 *   chown root:root /usr/local/bin/lsv-elevate && chmod 4755 /usr/local/bin/lsv-elevate
 * - Running GUI apps as root is discouraged; the user asked for this for
 *   diagnostic/read-only needs. Use at your own risk.
 */

int main(int argc, char **argv)
{
    if (argc < 2) {
        fprintf(stderr, "Usage: %s /absolute/path/to/LSV [args...]\n", argv[0]);
        return 2;
    }

    const char *target = argv[1];
    if (target[0] != '/') {
        fprintf(stderr, "error: target path must be absolute\n");
        return 3;
    }

    const char *basename = strrchr(target, '/');
    basename = basename ? basename + 1 : target;

    /* Whitelist allowed executable basenames to reduce accidental misuse */
    if (strcmp(basename, "LSV") != 0 && strcmp(basename, "lsv") != 0 &&
        strcmp(basename, "lsv-x86_64.AppImage") != 0 && strcmp(basename, "lsv-elevated") != 0) {
        fprintf(stderr, "error: target '%s' not allowed (basename '%s')\n", target, basename);
        return 4;
    }

    struct stat st;
    if (stat(target, &st) < 0) {
        fprintf(stderr, "stat(%s) failed: %s\n", target, strerror(errno));
        return 5;
    }

    if (!(st.st_mode & S_IXUSR)) {
        fprintf(stderr, "error: target '%s' is not executable\n", target);
        return 6;
    }

    /* Build new argv: argv[1] becomes argv0 for the target, rest are preserved */
    int newargc = argc - 1;
    char **newargv = malloc(sizeof(char *) * (newargc + 1));
    if (!newargv) {
        perror("malloc");
        return 7;
    }
    for (int i = 0; i < newargc; ++i)
        newargv[i] = argv[i + 1];
    newargv[newargc] = NULL;

    /* Exec the requested target. When this binary is installed with setuid-root,
     * the new process will run as root. The environment of the calling user is
     * preserved which helps keep DISPLAY/XAUTHORITY and other vars intact.
     */
    execv(target, newargv);
    fprintf(stderr, "execv(%s) failed: %s\n", target, strerror(errno));
    return 10;
}
