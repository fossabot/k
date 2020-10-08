#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <glob.h>

#include "util.h"
#include "vec.h"
#include "pkg.h"
#include "str.h"
#include "repo.h"

char **repo_init(void) {
    char **repos = NULL;
    char *p = NULL;
    char *path = xgetenv("KISS_PATH");

    for (char *tok = strtok_r(path, ":", &p);
         tok != NULL;
         tok = strtok_r(NULL, ":", &p)) {

        if (tok[0] != '/') {
            die("relative path found in KISS_PATH");
        }

        vec_add(repos, strdup(tok));
    }

    vec_add(repos, strdup(DB_DIR));

    free(p);
    free(path);

    return repos;
}

void repo_free(char **repos) {
    for (size_t i = 0; i < vec_size(repos); i++) {
        free(repos[i]);
    }
    vec_free(repos);
}

char *repo_find(const char *pattern, int all, char **repos) {
    glob_t buf;

    for (size_t i = 0; i < vec_size(repos); ++i) {
        str query = {0};

        str_cat(&query, repos[i]);
        str_cat(&query, "/");
        str_cat(&query, pattern);
        str_cat(&query, "/");

        glob(query.buf, i ? GLOB_APPEND : 0, NULL, &buf);

        str_free(&query);
    }

    char *match = NULL;

    if (buf.gl_pathc != 0 && buf.gl_pathv[0]) {
        match = strdup(buf.gl_pathv[0]);

        if (all) {
            for (size_t i = 0; i < buf.gl_pathc; i++) {
                puts(buf.gl_pathv[i]);
            }
        }
    }

    globfree(&buf);
    return match;
}
