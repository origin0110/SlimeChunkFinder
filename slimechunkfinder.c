#include "slimechunk.h"

int main(void) {
    int x1, x2, z1, z2, n, thr, tmp;
    slime_init();
    fprintf(stderr, WELCOM_DOC);

    while (1) {
        switch (getchar()) {
            case 't':
                tmp = scanf("%d %d", &x1, &z1);
                if (tmp != 2)
                    fprintf(stderr, WRONG_COMMAND_DOC);
                else
                    puts(is_slime_chunk(x1, z1) ? "true" : "false");
                break;
            case 'm':
                tmp = scanf("%d %d %d %d", &x1, &x2, &z1, &z2);
                if (tmp != 4)
                    fprintf(stderr, WRONG_COMMAND_DOC);
                else
                    slime_map(x1, x2, z1, z2);
                break;
            case 'f':
                tmp = scanf("%d %d %d %d %d %d", &x1, &x2, &z1, &z2, &n, &thr);
                if (tmp != 6)
                    fprintf(stderr, WRONG_COMMAND_DOC);
                else
                    slime_finder_parallel(x1, x2, z1, z2, n, thr);
                break;
            case 'h':
                fprintf(stderr, HELP_DOC);
                break;
            case 'q':
                return 0;
        }
    }
}