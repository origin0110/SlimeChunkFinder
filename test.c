#include "slimechunkfinder.h"

int main(void) {
    int error = 0;

    error = slime_initialization();
    if (error)
        goto catch_error;

    puts(WELCOME_DOC);

    while (1) {
        putchar('\n');

        int x1, z1, x2, z2, n, thr;
        switch (getchar()) {
            case 'm':
                if (scanf("%d %d %d %d", &x1, &z1, &x2, &z2) != 4) {
                    puts(WRONG_COMMAND_DOC);
                    break;
                }
                slime_map(x1, z1, x2, z2);
                break;
            case 'f':
                if (scanf("%d %d %d %d %d %d", &x1, &z1, &x2, &z2, &n, &thr) != 6) {
                    puts(WRONG_COMMAND_DOC);
                    break;
                }
                slime_finder(x1, z1, x2, z2, n, thr);
                break;
            case 'h':
                puts(HELP_DOC);
                break;
            case 'q':
                return 0;
            default:
                break;
        }
    }

catch_error:  //直接退出吧，我摆烂
    return error;
}