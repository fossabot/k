#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>

#include "build.h"
#include "checksum.h"
#include "extract.h"
#include "source.h"
#include "repo.h"
#include "list.h"
#include "pkg.h"

char *OLD_CWD = NULL;
char **REPOS = NULL;
char *PKG = NULL;
int  REPO_LEN = 0;
char HOME[PATH_MAX + 1];
char XDG_CACHE_HOME[PATH_MAX + 1];
char CAC_DIR[PATH_MAX + 1];
char MAK_DIR[PATH_MAX + 22];
char PKG_DIR[PATH_MAX + 22];
char TAR_DIR[PATH_MAX + 22];
char SRC_DIR[PATH_MAX + 1];
char BIN_DIR[PATH_MAX + 1];
char *OLD_CWD;
char old_cwd_buf[PATH_MAX + 1];

static void usage(void) {
    printf("kiss [b|c|d|l|s|v] [pkg]...\n");
    printf("build:        Build a package\n");
    printf("checksum:     Generate checksums\n");
    printf("download:     Pre-download all sources\n");
    printf("list:         List installed packages\n");
    printf("search:       Search for a package\n");
    printf("version:      Package manager version\n");

    exit(0);
}

int main (int argc, char *argv[]) {
    if (argc == 1)
        usage();

    cache_init();
    atexit(cache_destroy);
    package *head = NULL;
    REPOS = repo_load();

    for (int i = 2; i < argc; i++) {
        pkg_load(&head, argv[i]);
        PKG = head->name;
    }

    switch (argv[1][0]) {
    case 'b':
        do1(pkg_sources);
        do1(pkg_verify);
        do2(pkg_extract, pkg_build);
        break;

    case 'c':
        do1(pkg_sources);
        do1(pkg_checksums);
        break;

    case 'd':
        do1(pkg_sources);
        break;

    case 'l':
        if (argc == 2) {
           pkg_list_all(); 

        } else {
            do1(pkg_list);
        }
        break;

    case 's':
        for (package *tmp = head; tmp; tmp = tmp->next) {
            PKG = tmp->name;

            for (char *c = *tmp->path; c; c=*++tmp->path) {
                printf("%s\n", *tmp->path);
            }
        }
        break;

    case 'v':
        printf("0.0.1\n");
        break;

    default:
        usage();
    }

    free(head);
    return 0;
}
