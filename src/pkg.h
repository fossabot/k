#ifndef ARRAY_H_
#define ARRAY_H_

#include <limits.h>

#define PKG_DB "/var/db/kiss/installed/"

extern char **REPOS;
extern int  REPO_LEN;
extern char *PKG;
extern char HOME[PATH_MAX + 1];
extern char XDG_CACHE_HOME[PATH_MAX + 1];
extern char CAC_DIR[PATH_MAX + 1];
extern char MAK_DIR[PATH_MAX + 1];
extern char PKG_DIR[PATH_MAX + 1];
extern char TAR_DIR[PATH_MAX + 1];
extern char SRC_DIR[PATH_MAX + 1];
extern char BIN_DIR[PATH_MAX + 1];
extern char *OLD_CWD;
extern char old_cwd_buf[PATH_MAX+1];

struct version {
    char *version;
    char *release;
};

struct source {
    char **src;
    char **dest;
};

typedef struct package {
    char *name;
    char **sums;

    char **path;
    int  path_len;

    struct source source;
    struct version version;
    int  src_len;

    struct package *next;
    struct package *prev;
} package;

void pkg_load(package **head, char *pkg_name);
struct version pkg_version(char *repo_dir);
void cache_init(void);
void cache_destroy(void);

#define SAVE_CWD { \
    OLD_CWD = getcwd(old_cwd_buf, sizeof(old_cwd_buf)); \
}

#define LOAD_CWD { \
    xchdir(OLD_CWD); \
    free(OLD_CWD); \
}

#endif
