#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "screen.h"
#include "states.h"
#include "game_of_life.h"

static char main_buffer[GRID_STRING_SIZE];

static int browse_states(void);
static void clean_board(int state);
static void write_state(int fd, const char *state);
static void state_to_board(int state, char *buffer);
static void render_board(const char *msg, int idx, int rest_cur);

int main(void)
{
	// Enable raw-mode in terminal settings.
	struct termios old_tio;

	enable_raw_mode(&old_tio);
	//										Title.
	printf("				Welcome to the Game of Life!
			\n\n");

	// Let User choose the initial state.
	int state = browse_states();

	render_board("Chosen State:", state, 0);

	// Disable raw-mode in terminal settings temporarily.
	disable_raw_mode(&old_tio);

	// Open character device.
	int fd = open(DEVPATH, O_RDWR);

	if (fd < 0) {
		perror("\n\nFailed to open device.\n");
		return -1;
	}

	// Write chosen state to character device.
	write_state(fd, states[state]);

	// Let user choose between Dynamic and Static iteration.
	printf("\n- Dynamic iteration? [Y/n]" EMPTY_LINE);
	char choice = getchar();

	// If user chooses q exit successfully.
	if (choice == 'q')
		return 0;

	if (choice == 'n') {
		// Static iteration.
		printf("- Static iteration it is. How many generations?\n");

		// Get number of iterations from user.
		int generations = 1;

		scanf(" %d", &generations);

		// Clean and setup board.
		clean_board(state);

		// Run the simulation.
		for (int i = 1; i <= generations; ++i) {
			ioctl(fd, GOL_TICK);
			read(fd, main_buffer, 0);
			render_board("Generation", i, 1);
			usleep(500000);
		}

	} else {
		// Dynamic iteration.
		printf("- Dynamic iteration it is. Press Enter for next generation or 'q'
				to quit.\n");

		// Clean and setup board.
		clean_board(state);

		// Enable raw-mode in terminal settings.
		enable_raw_mode(&old_tio);

		int generations = 1;

		do {
			read(fd, main_buffer, 0);
			render_board("Generation", generations++, 1);
			ioctl(fd, GOL_TICK);
		}   while ((choice = getchar()) != 'q');

		// Disable raw-mode in terminal settings.
		disable_raw_mode(&old_tio);
	}

	// Move the curser down and exit.
	move_cursor_down(HEIGHT + 7);
	printf("\n");
	return 0;
}

static void clean_board(int state)
{
	// Restore Cursor.
	restore_cursor();

	// Cleanup board.
	move_cursor_up(2);
	save_cursor();
	for (int i = 0; i < HEIGHT + 4; ++i)
		printf(EMPTY_LINE);
	restore_cursor();

	// Add state to the board.
	move_cursor_right(WIDTH - 10);
	printf("State [%d]\n", state);
	move_cursor_right(WIDTH - 13);
}

static int browse_states(void)
{
	char choice;
	int state = 0;

	printf("Choose the initial state: press Enter to choose\n");

	while (1) {
		state_to_board(state, main_buffer);
		render_board("State", state, 1);

		choice = getchar();
		if (choice == '\n')
			break;
		if (choice != 27)
			continue;

		choice = getchar();
		if (choice != '[')
			continue;

		choice = getchar();
		if (choice == 'A' || choice == 'C') {
			++state;
			state %= num_states;
		}
		if (choice == 'B' || choice == 'D') {
			--state;
			if (state < 0)
				state += num_states;
		}
	}
	return state;
}

static void state_to_board(int state, char *buffer)
{
	int idx = 0;

	for (int j = 0; j < (WIDTH - 1); ++j)
		buffer[idx++] = '-';
	buffer[idx++] = '\n';

	for (int i = 0; i < ROWS; ++i) {
		buffer[idx++] = '|';
		for (int j = 0; j < COLUMNS; ++j)
			buffer[idx++] = states[state][i * COLUMNS + j];
		buffer[idx++] = '|';
		buffer[idx++] = '\n';
	}

	for (int j = 0; j < (WIDTH - 1); ++j)
		buffer[idx++] = '-';
	buffer[idx++] = '\n';
	buffer[idx++] = '\0';
}

static void write_state(int fd, const char *state)
{
	for (int i = 0; state[i]; ++i) {
		if (state[i] == '*') {
			lseek(fd, i, SEEK_SET);
			write(fd, NULL, 0);
		}
	}
}

static void render_board(const char *msg, int idx, int rest_cur)
{
	hide_cursor();
	save_cursor();
	printf("%s [%02u]" EMPTY_LINE, msg, idx);

	for (int idx = 0; main_buffer[idx]; ++idx) {
		if (idx % WIDTH == 0)
			printf("\n");

		if (idx == 0) {
			// Left Up Corner
			printf("\xE2\x94\x8F");

		} else if (idx == 33) {
			// Right Up Corner
			printf("\xE2\x94\x93");

		} else if (idx == 595) {
			// Left Bottom Corner
			printf("\xE2\x94\x97");

		} else if (idx == 628) {
			// Right Bottom Corner
			printf("\xE2\x94\x9B");

		} else if (main_buffer[idx] == '|') {
			// Vertical Border
			printf("\xE2\x94\x83");

		} else if (main_buffer[idx] == '-') {
			// Horizontal Border
			printf("\xE2\x94\x81\xE2\x94\x81");

		} else if (main_buffer[idx] == '*') {
			// Live Cell
			printf("\xE2\x97\xBD");

		} else if (main_buffer[idx] == ' ') {
			// Dead Cell
			printf("\xE2\x97\xBE");

		} else {
			continue;
		}
	}

	render_screen();
	if (rest_cur)
		restore_cursor();
	show_cursor();
}
