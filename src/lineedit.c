#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

#include "lineedit.h"

static struct termios old_termios;

static void terminal_restore(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &old_termios);
}

static int terminal_raw(void) {
    struct termios raw;

    if (tcgetattr(STDIN_FILENO, &old_termios) < 0)
        return -1;

    raw = old_termios;

    raw.c_lflag &= ~(ICANON | ECHO | ISIG);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) < 0)
        return -1;

    if (atexit(terminal_restore) != 0)
        return -1;

    return 0;
}

static void redraw_line(
    const char *buffer,
    char *prompt,
    size_t length,
    size_t cursor
) {
    fputs("\r\033[K", stdout);
    fputs(prompt, stdout);
    fwrite(buffer, 1, length, stdout);

    if (length > cursor)
        printf("\033[%zuD", length - cursor);

    fflush(stdout);
}

int line_read(char *buffer, char *prompt, size_t size) {
    size_t length = 0;
    size_t cursor = 0;
    char c;

    if (size == 0)
        return -1;

    if (terminal_raw() < 0)
        return -1;

    buffer[0] = '\0';

    for (;;) {
        if (read(STDIN_FILENO, &c, 1) != 1)
            continue;

        if (c == '\n') {
            buffer[length] = '\0';
            putchar('\n');
            terminal_restore();
            return (int)length;
        }

        if (c == 3) {
            buffer[0] = '\0';
            putchar('^');
            putchar('C');
            putchar('\n');
            terminal_restore();
            return 0;
        }

        if (c == 4) {
            terminal_restore();
            return -1;
        }

        if (c == 127) {
            if (cursor > 0) {
                memmove(
                    &buffer[cursor - 1],
                    &buffer[cursor],
                    length - cursor + 1
                );

                cursor--;
                length--;

                redraw_line(buffer, prompt, length, cursor);
            }

            continue;
        }

        if (c == 27) {
            char sequence[2];

            if (read(STDIN_FILENO, &sequence[0], 1) != 1)
                continue;

            if (sequence[0] != '[')
                continue;

            if (read(STDIN_FILENO, &sequence[1], 1) != 1)
                continue;

            if (sequence[1] == 'D' && cursor > 0) {
                cursor--;
                redraw_line(buffer, prompt, length, cursor);
            } else if (sequence[1] == 'C' && cursor < length) {
                cursor++;
                redraw_line(buffer, prompt, length, cursor);
            }

            continue;
        }

        if ((unsigned char)c < 32)
            continue;

        if (length >= size - 1)
            continue;

        memmove(
            &buffer[cursor + 1],
            &buffer[cursor],
            length - cursor + 1
        );

        buffer[cursor] = c;

        length++;
        cursor++;

        redraw_line(buffer, prompt, length, cursor);
    }
}
