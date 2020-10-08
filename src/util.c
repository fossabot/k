#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "str.h"
#include "util.h"

size_t xgetcwd(char *p[]) {
    long len = pathconf(".", _PC_PATH_MAX);

    if (len < 0) {
        perror("pathconf");
        exit(1);
    }

    char *buf = malloc((size_t) len);

    if (!buf) {
        perror("malloc");
        exit(1);
    }

    *p = getcwd(buf, (size_t) len);
    free(buf);

    if (!*p) {
        perror("getcwd");
        exit(1);
    }

    return (size_t) len;
}

int PATH_prepend(const char *path, const char *var) {
    if (!path || !var) {
        return 1;
    }

    str kiss_path = {0};

    str_cat(&kiss_path, path);
    str_cat(&kiss_path, ":");
    str_cat(&kiss_path, getenv(var));

    int err = setenv(var, kiss_path.buf, 1);
    str_free(&kiss_path);

    if (err == -1) {
        perror("setenv");
        return 1;
    }

    return 0;
}

int is_dir(const char *d) {
    struct stat s;

    if (lstat(d, &s) < 0) {
        return 1;
    }

    if (S_ISDIR(s.st_mode)) {
        return 0;
    }

    return 1;
}

char *path_basename(char *p, size_t len) {
    for (int i = 1; p[len - i] == '/'; i++) {
        p[len - i] = 0;
    }

    char *b = strrchr(p, '/');

    if (!b) {
        *p = '/';
        return p;
    }

    *b++ = 0;

    return b; // p now points to dirname
}

FILE *fopenat(const char *d, const char *f, const char *m) {
    if (!d || !f | !m) {
        return NULL;
    }

    str new = {0};
    str_cat(&new, d);
    str_cat(&new, "/");
    str_cat(&new, f);

    FILE *f2 = fopen(new.buf, m);

    str_free(&new);

    if (!f2) {
        return NULL;
    }

    return f2;
}
