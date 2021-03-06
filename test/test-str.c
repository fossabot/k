#include <string.h>

#include "str.h"
#include "test.h"

int main(int argc, char *argv[]) {
    (void) argc;
    (void) argv;

    int ret = test_begin(__FILE__);

    str *s = str_init(0); {
        test(s);
        test(s->len == 0);
        test(s->cap == 1);
    }

    ret = str_push_s(&s, "hello, world"); {
        test(ret == 0);
        test(s->len == 12);
        test(s->cap == 20);
        test(strcmp(s->buf, "hello, world") == 0);
    }

    str_set_len(s, 0); {
        test(s->len == 0);
        test(s->cap == 20);
        test(s->buf[0] == 0);
        test(strcmp(s->buf, "") == 0);
    }

    ret = str_push_l(&s, "123456", 6); {
        test(ret == 0);
        test(s->len == 6);
        test(s->cap == 20);
        test(strcmp(s->buf, "123456") == 0);
    }

    ret = str_push_c(&s, '!'); {
        test(ret == 0);
        test(s->len == 7);
        test(s->cap == 20);
        test(strcmp(s->buf, "123456!") == 0);
    }

    ret = str_rstrip(&s, '!'); {
        test(ret == 1);
        test(s->len == 6);
        test(s->cap == 20);
        test(strcmp(s->buf, "123456") == 0);
    }

    ret = str_undo_c(&s, '6'); {
        test(ret == 0);
        test(s->len == 5);
        test(s->cap == 20);
        test(strcmp(s->buf, "12345") == 0);
    }

    ret = str_undo_c(&s, '6'); {
        test(ret == -1);
        test(s->len == 5);
        test(s->cap == 20);
        test(strcmp(s->buf, "12345") == 0);
    }

    FILE *f = fopen(__FILE__, "r");

    ret = str_getline(&s, f, 30); {
        test(ret == 0);
        test(s->len == 24);
        test(s->cap == 66);
        test(strcmp(s->buf, "12345#include <string.h>") == 0);
    }

    fclose(f);

    str_free(&s); {
        test(!s);
    }

    return test_finish();
}

