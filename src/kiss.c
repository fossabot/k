#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "vec.h"
#include "pkg.h"

#define die(...) _m(":(", __FILE__, __LINE__, __VA_ARGS__),exit(1)
#define msg(...) _m("OK", __FILE__, __LINE__, __VA_ARGS__)

static void _m(const char* t, const char *f, const int l, const char *fmt, ...) {
    va_list args;

    va_start(args, fmt);

    printf("[%s] (%s:%d) ", t, f, l);
    vprintf(fmt, args);
    printf("\n");

    va_end(args);
}

static void *xmalloc(size_t n) {
    void *p = malloc(n);

    assert(p);

    return p;
}

static int xsnprintf(char *str, size_t size, const char *fmt, ...) {
    va_list va;
    int len;

    va_start(va, fmt);
    len = vsnprintf(str, size, fmt, va);
    va_end(va);

    assert(len > 1);
    assert(len <= (int) size);

    return len;
}

static char *xstrdup(const char *s) {
    char *p = strdup(s);

    assert(p);

    return p;
}

static char *xgetenv(const char *s) {
    char *p = getenv(s);

    return p[0] ? xstrdup(p) : NULL;
}

static void mkdir_p(const char *dir, const int mod) {
    char *tmp;
    char *p = 0;

    assert(dir);
    tmp = xstrdup(dir);

    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = 0;

            if ((mkdir(tmp, mod) == -1) && errno != EEXIST) {
               die("Failed to create directory %s", tmp);
            }

            *p = '/';
        }
    }

    free(tmp);
}

static void mkdir_at(const char *p, const char *d, const int m) {
   char *tmp;
   size_t len;

   len = strlen(p) + strlen(d) + 2;
   tmp = xmalloc(len);

   xsnprintf(tmp, len, "%s/%s", p, d);
   mkdir_p(tmp, m);

   free(tmp);
}

static FILE *fopenat(const char *d, const char *f, const int o, const char *m) {
    int dfd;
    int ffd;

    dfd = open(d, O_SEARCH);

    if (dfd == -1) {
        return NULL;
    }

    ffd = openat(dfd, f, o, 0644);
    close(dfd);

    if (ffd == -1) {
        return NULL;
    }

    /* fclose() by caller also closes the open()'d fd here */
    return fdopen(ffd, m);
}

static int existsat(const char *d, const char *f, const int o, const char *m) {
    FILE *file = fopenat(d, f, o, m);

    return file ? fclose(file) : -1;
}

static char *xdg_dir(void) {
    char *env;
    char *dir;
    char *tmp = "kiss";
    size_t len;

    env = xgetenv("XDG_CACHE_HOME");

    if (!env) {
       env = xgetenv("HOME");
       tmp = ".cache/kiss";
    }

    assert(env);

    /* 2 == '/' + '\0' */
    len = strlen(env) + strlen(tmp) + 2;
    dir = xmalloc(len);

    xsnprintf(dir, len, "%s/%s", env, tmp);
    free(env);

    return dir;
}

static char *cache_init(void) {
    char *xdg;
    char *cac;
    pid_t pid;
    size_t len;

    const char *caches[2] = { "sources", "bin" };
    const char *states[3] = { "build", "extract", "pkg" };

    pid = getpid();
    xdg = xdg_dir();
    len = snprintf(NULL, 0, "%s/%u", xdg, pid);
    cac = xmalloc(len + 1);

    xsnprintf(cac, len + 1, "%s/%u", xdg, pid);

    for (int i = 0; i < 2; i++) {
        mkdir_at(xdg, caches[i], 0755);
    }

    for (int i = 0; i < 3; i++) {
        mkdir_at(cac, states[i], 0755);
    }

    free(xdg);

    return cac;
}

static char **repo_init(void) {
    char *tmp;
    char *env;
    char **repos = NULL;
    int i = 0;

    if (!(env = xgetenv("KISS_PATH"))) {
        die("KISS_PATH must be set");
    }

    while ((tmp = strtok(i++ ? NULL : env, ":"))) {
        vec_append(repos, xstrdup(tmp));
    }
    free(env);
    vec_append(repos, "/var/db/kiss/installed");

    return repos;
}

static char *pkg_find(const char *name, char **repos, const int all) {
    size_t len;
    char *tmp;

    for (size_t j = 0; j < vec_size(repos); ++j) {
        if (!(existsat(repos[j], name, O_DIRECTORY, "r"))) {

            if (!all) {
                len = strlen(repos[j]) + strlen(name) + 2;
                tmp = xmalloc(len);
                xsnprintf(tmp, len, "%s/%s", repos[j], name);

                return tmp;
            }

            printf("%s/%s\n", repos[j], name);
        }
    }

    if (!all) {
        die("Package '%s' not in any repository", name);
    }

    return NULL;
}

static void pkg_version(package *pkg, char *repo) {
    FILE *file;
    char *ver_f;
    size_t ver_l;

    /* 10 == '/' + '/version' + '\0' */
    ver_l = strlen(repo) + strlen(pkg->name) + 10;
    ver_f = malloc(ver_l);

    xsnprintf(ver_f, ver_l, "%s/%s/version", repo, pkg->name);

    if (!(file = fopen(ver_f, "r"))) {
        die("[%s], Version file not found", pkg->name);
    }
    free(ver_f);

    if ((getline(&pkg->ver, &(size_t){0}, file)) == -1) {
        die("[%s] Failed to read version file", pkg->name);
    }
    fclose(file);

    pkg->ver[strcspn(pkg->ver, "\n")] = 0;
}

static void pkg_sources(package *pkg) {
    FILE *file;
    char *line = 0;
    char *src;
    char *des;

    if (!(file = fopenat(pkg->path, "sources", O_RDONLY, "r"))) {
        die("[%s], sources file does not exist", pkg->name);
    }

    while (getline(&line, &(size_t){0}, file) != -1) {
        if (line[0] != '#' && line[0] != '\n') {
            src = strtok(line, " 	\r\n");
            assert(src);
            des = strtok(NULL, " 	\r\n");

            if (strstr(src, "://")) {
                if (!(existsat(src, des, 0, "r"))) {
                    msg("[%s] Found cached source %s", pkg->name, src);
                }

            } else if (strncmp(src, "git+", 4) == 0) {
                die("[%s] Git sources not yet supported", pkg->name);

                /* look into libgit2 or other libraries if they exist */
                /* possibly compromise and exec git directly... */

            } else {
                if (!(existsat(pkg->path, src, 0, "r"))) {
                    msg("[%s] Found  local source %s", pkg->name, src);
                }

            }

            vec_append(pkg->des, des ? xstrdup(des) : "");
            vec_append(pkg->src, xstrdup(src));
        }
    }

    free(line);
    fclose(file);
}

static package pkg_new(char *name) {
    assert(name);

    return (package) {
        .name = name,
    };
}

static void pkg_list(package *pkg) {
    char *db = "/var/db/kiss/installed";
    struct dirent **list;
    int err;

    if (vec_size(pkg) == 0) {
        if ((err = scandir(db, &list, NULL, alphasort)) == -1) {
            die("Installed database not accessible");
        }

        for (int i = 0; i < err; i++) {
            if (list[i]->d_name[0] != '.' && list[i]->d_name[2]) {
                vec_append(pkg, pkg_new(list[i]->d_name));
            }
        }

        free(list);
    }

    vec_iter(pkg) {
        pkg_version(&pkg[i], db);

        printf("%s %s\n", pkg[i].name, pkg[i].ver);
    }
}

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
    package *pkgs = NULL;
    char **repos  = NULL;
    char *cac_dir = NULL;

    if (argc == 1) {
        usage();
    }

    repos   = repo_init();

    for (int i = 2; i < argc; i++) {
        vec_append(pkgs, pkg_new(argv[i]));
    }

    switch (argv[1][0]) {
        case 'b':
        case 'c':
        case 'd':
            cac_dir = cache_init();

            vec_iter(pkgs) {
                pkgs[i].path = pkg_find(pkgs[i].name, repos, 0);
            }

            vec_iter(pkgs) {
                pkg_sources(&pkgs[i]);
            }
            break;

        case 'l':
            pkg_list(pkgs);
            break;

        case 's':
            vec_iter(pkgs) {
                pkg_find(pkgs[i].name, repos, 1);
            }
            break;

        case 'v':
            printf("0.0.1\n");
            break;

        default:
            usage();
    }

    switch (argv[1][0]) {
        case 'b':
        case 'c':
        case 'd':
            free(cac_dir);
            break;
    }

    vec_free(repos);
    vec_free(pkgs);
}
