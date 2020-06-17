#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "repo.h"
#include "log.h"
#include "pkg.h"

char **repo_load(void) {
    char *path = strdup(getenv("KISS_PATH"));
    char *tok;
    char **res = NULL;
    int n = 0;

    if (!path || path[0] == '\0') {
        log_error("KISS_PATH must be set");
    }

    // 24 is "/var/db/kiss/installed" (22) + ":" + "\0".
    tok = strtok(path, ":"); 
    res = calloc(strlen(path) + 24, sizeof(char *));

    if (!res) {
        log_error("Failed to allocate memory");
        exit(1);
    }

    while (tok) {
        res[++n - 1] = tok;
        tok = strtok(NULL, ":");
    }
    res[n] = "/var/db/kiss/installed";
    REPO_LEN = n + 1;

    return res;
}
