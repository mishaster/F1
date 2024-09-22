#ifndef SCREEN_H
#define SCREEN_H

#include <stdio.h>
#include <termios.h>
#include <unistd.h>

#define EMPTY_LINE \
"																  \n"

static void move_cursor(int row, int col) { printf("\033[%d;%dH", row, col); }

static void move_cursor_up(int lines) { printf("\033[%dA", lines); }

static void move_cursor_down(int lines) { printf("\033[%dB", lines); }

static void move_cursor_left(int lines) { printf("\033[%dD", lines); }

static void move_cursor_right(int lines) { printf("\033[%dC", lines); }

static void save_cursor(void) { printf("\0337"); }

static void restore_cursor(void) { printf("\0338"); }

static void hide_cursor(void) { printf("\033[?25l"); }

static void show_cursor(void) { printf("\033[?25h"); }

static void clear_screen(void) { printf("\033[2J"); }

static void render_screen(void)
{
	fflush(stdout);
	usleep(50000);
}

static void enable_raw_mode(struct termios *old_tio)
{
	struct termios new_tio;

	tcgetattr(STDIN_FILENO, old_tio);

	new_tio = *old_tio;
	new_tio.c_lflag &= ~(ICANON | ECHO);

	tcsetattr(STDIN_FILENO, TCSAFLUSH, &new_tio);
}

static void disable_raw_mode(struct termios *old_tio)
{
	tcsetattr(STDIN_FILENO, TCSAFLUSH, old_tio);
}

#endif	// SCREEN_H
