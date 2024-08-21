/*
 *	  Final 0 - Technion CS 236009 Topics in Linux Kernel Development
 *
 *  Authors:
 *		  Emily Dror <emily.d@campus.technion.ac.il>
 *		  Michael Blum <michael.blum@campus.technion.ac.il>
 */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static const int test_count	 = 25;
static const char *dev_path	 = "/dev/battleship";

/* ............................ Submarines Definitions ............................ */

#define BATTLESHIP_MAGIC			('b')
#define BATTLESHIP_RESET		_IOW(BATTLESHIP_MAGIC, 0x01, struct battleship_config)
#define BATTLESHIP_UNDO			 _IO(BATTLESHIP_MAGIC, 0x02)
#define BATTLESHIP_REDO			 _IO(BATTLESHIP_MAGIC, 0x03)

#define BATTLESHIP_MOVE_SIZE		(7)
#define BATTLESHIP_RESP_SIZE		(2)
#define BATTLESHIP_READ_SIZE		(BATTLESHIP_MOVE_SIZE + BATTLESHIP_RESP_SIZE)

struct battleship_config {
	int ship_count;
	int board_width;
};

const struct battleship_config default_config = {
	.ship_count = 5,
	.board_width = 20,
};

const struct battleship_config empty_config = {
	.ship_count = 0,
	.board_width = 20,
};

/* ............................ Utilities for TAP testing ............................ */

static int test_current = 1;

static inline void tap_print_header(void)
{
	printf("1..%d\n", test_count);
}

static inline int tap_ok(int cond, const char *msg)
{
	if (!cond) {
		printf("not ok %d - %s\n", test_current, msg);
		return 1;
	}
	return 0;
}

static inline int tap_test_passed(const char *msg)
{
	printf("ok %d - %s passed\n", test_current, msg);
	return 0;
}

static void run_test(int (*test_method)(void))
{
	if (test_method())
		exit(EXIT_FAILURE);

	++test_current;
}

/* ............................ Tests' Implementation ............................ */

static int test_open(void)
{
	int retval = open(dev_path, O_RDWR);

	if (tap_ok(retval != -1, "open valid file path"))
		return 1;

	retval = open("/dev/nonsense", O_RDWR);
	if (tap_ok(retval == -1, "open corrupt file path"))
		return 1;

	return tap_test_passed(__func__);
}

static int test_close(void)
{
	int retval = open(dev_path, O_RDWR);

	if (tap_ok(retval != -1, "open valid file path"))
		return 1;

	retval = close(retval);
	if (tap_ok(retval != -1, "close file descriptor"))
		return 1;

	return tap_test_passed(__func__);
}

static int test_ioctl_sanity(void)
{
	int fd = open(dev_path, O_RDWR);

	if (tap_ok(fd != -1, "open valid file path"))
		return 1;

	int retval = ioctl(fd, BATTLESHIP_RESET, default_config);

	if (tap_ok(retval == 0, "reset board"))
		return 1;

	return tap_test_passed(__func__);
}

static int test_ioctl_errors(void)
{
	int fd = open(dev_path, O_RDWR);

	if (tap_ok(fd != -1, "open valid file path"))
		return 1;

	int retval = ioctl(-1, BATTLESHIP_RESET, default_config);

	if (tap_ok(retval == -EBADF, "invalid file descriptors"))
		return 1;

	retval = ioctl(fd, BATTLESHIP_RESET, (struct battleship_config) {
		.ship_count = 5,
		.board_width = -20,
	});
	if (tap_ok(retval == -EINVAL, "board size is negative"))
		return 1;

	retval = ioctl(fd, BATTLESHIP_RESET, (struct battleship_config) {
		.ship_count = -5,
		.board_width = 20,
	});
	if (tap_ok(retval == -EINVAL, "number of ships is negative"))
		return 1;

	retval = ioctl(fd, BATTLESHIP_RESET, (struct battleship_config) {
		.ship_count = -5,
		.board_width = -20,
	});
	if (tap_ok(retval == -EINVAL, "number of ships and board size are negative"))
		return 1;

	return tap_test_passed(__func__);
}

static int test_read_sanity(void)
{
	int fd = open(dev_path, O_RDWR);

	if (tap_ok(fd != -1, "open valid file path"))
		return 1;

	char buffer[BATTLESHIP_READ_SIZE];
	ssize_t bytes_read = read(fd, NULL, BATTLESHIP_READ_SIZE);

	if (tap_ok(bytes_read < 0, "reading into NULL"))
		return 1;

	bytes_read = read(fd, buffer, -BATTLESHIP_READ_SIZE);
	if (tap_ok(bytes_read < 0, "reading negative bytes"))
		return 1;

	bytes_read = read(fd, buffer, BATTLESHIP_READ_SIZE);
	if (tap_ok(bytes_read != -1, "reading into normal buffer"))
		return 1;

	return tap_test_passed(__func__);
}

static int test_read_errors(void)
{
	int fd = open(dev_path, O_RDWR);

	if (tap_ok(fd != -1, "open valid file path"))
		return 1;

	char buffer[BATTLESHIP_READ_SIZE * 2];
	ssize_t bytes_read = read(-1, buffer, BATTLESHIP_READ_SIZE);

	if (tap_ok(bytes_read == -EBADF, "read from invalid file descriptors"))
		return 1;

	int write_fd = open("/tmp/tmp_file", O_WRONLY | O_CREAT);

	bytes_read = read(write_fd, buffer, BATTLESHIP_READ_SIZE);
	if (tap_ok(bytes_read == -EINVAL, "read from an object which isn`t suitable for reading"))
		return 1;

	bytes_read = read(fd, buffer, -BATTLESHIP_READ_SIZE);
	if (tap_ok(bytes_read == -EINVAL, "reading negative bytes"))
		return 1;

	return tap_test_passed(__func__);
}

static int test_write_sanity(void)
{
	int fd = open(dev_path, O_RDWR);

	if (tap_ok(fd != -1, "open valid file path"))
		return 1;

	char buffer[] = "(01,01)";
	ssize_t moves_written = write(fd, NULL, BATTLESHIP_MOVE_SIZE);

	if (tap_ok(moves_written < 0, "writing into NULL"))
		return 1;

	moves_written = write(fd, buffer, -BATTLESHIP_MOVE_SIZE);
	if (tap_ok(moves_written < 0, "writing negative bytes"))
		return 1;

	return tap_test_passed(__func__);
}

static int test_write_errors(void)
{
	int fd = open(dev_path, O_RDWR);

	if (tap_ok(fd != -1, "open valid file path"))
		return 1;

	char buffer[] = "(01,01)(01,02)(01,03)";
	ssize_t moves_written = write(-1, buffer, BATTLESHIP_MOVE_SIZE);

	if (tap_ok(moves_written == -EBADF, "write into invalid file descriptors"))
		return 1;

	moves_written = write(fd, (void *)open, BATTLESHIP_MOVE_SIZE);
	if (tap_ok(moves_written == -EFAULT, "write into outside our accessible address space"))
		return 1;

	int read_fd = open("./spec.txt", O_RDONLY);

	moves_written = write(read_fd, buffer, BATTLESHIP_MOVE_SIZE);
	if (tap_ok(moves_written == -EINVAL,
		   "write into an object which isn`t suitable for reading"))
		return 1;

	moves_written = write(fd, buffer, -BATTLESHIP_MOVE_SIZE);
	if (tap_ok(moves_written == -EINVAL, "writing negative bytes"))
		return 1;

	return tap_test_passed(__func__);
}

static int test_lseek_sanity(void)
{
	int fd = open(dev_path, O_RDWR);

	if (tap_ok(fd != -1, "open valid file path"))
		return 1;

	int retval = ioctl(fd, BATTLESHIP_RESET, default_config);

	if (tap_ok(retval == 0, "reset board"))
		return 1;

	off_t offset = lseek(fd, 0, SEEK_SET);

	if (tap_ok(offset == 0, "lseek to beginning of file"))
		return 1;

	offset = lseek(fd, 0, SEEK_END);
	if (tap_ok(offset == 0, "lseek to end of file"))
		return 1;

	return tap_test_passed(__func__);
}

static int test_lseek_errors(void)
{
	int fd = open(dev_path, O_RDWR);

	if (tap_ok(fd != -1, "open valid file path"))
		return 1;

	off_t offset = lseek(-1, 0, SEEK_SET);

	if (tap_ok(offset == -EBADF, "lseek into invalid file descriptors"))
		return 1;

	offset = lseek(fd, 1, SEEK_END);
	if (tap_ok(offset == -EINVAL, "lseek into an invalid location"))
		return 1;

	offset = lseek(fd, 0, 4);
	if (tap_ok(offset == -EINVAL, "lseek's whence is not valid"))
		return 1;

	return tap_test_passed(__func__);
}

static int test_write_moves(void)
{
	int fd = open(dev_path, O_RDWR);

	if (tap_ok(fd != -1, "open valid file path"))
		return 1;

	int retval = ioctl(fd, BATTLESHIP_RESET, default_config);

	if (tap_ok(retval == 0, "reset board"))
		return 1;

	char buffer[] = "(01,01)(01,02)(01,03)";
	ssize_t moves_written = write(fd, buffer, BATTLESHIP_MOVE_SIZE);

	if (tap_ok(moves_written == 1, "write one move"))
		return 1;

	moves_written = write(fd, buffer + BATTLESHIP_MOVE_SIZE, 2 * BATTLESHIP_MOVE_SIZE);
	if (tap_ok(moves_written == 2, "write multiple moves"))
		return 1;

	return tap_test_passed(__func__);
}

static int test_read_move(void)
{
	int fd = open(dev_path, O_RDWR);

	if (tap_ok(fd != -1, "open valid file path"))
		return 1;

	int retval = ioctl(fd, BATTLESHIP_RESET, empty_config);

	if (tap_ok(retval == 0, "create empty board"))
		return 1;

	char buffer[BATTLESHIP_READ_SIZE * 4];
	ssize_t bytes_read = read(fd, buffer, BATTLESHIP_READ_SIZE);

	if (tap_ok(bytes_read == 0 && strlen(buffer) == 0, "read from empty board"))
		return 1;

	char moves[] = "(01,01)";
	ssize_t moves_written = write(fd, moves, BATTLESHIP_MOVE_SIZE);

	if (tap_ok(moves_written == 1, "write one move"))
		return 1;

	bytes_read = read(fd, buffer, BATTLESHIP_READ_SIZE);
	if (tap_ok(bytes_read == BATTLESHIP_READ_SIZE &&
		   strcmp(buffer, "(01,01):m") == 0, "read one move"))
		return 1;

	return tap_test_passed(__func__);
}

static int test_read_multiple_moves(void)
{
	int fd = open(dev_path, O_RDWR);

	if (tap_ok(fd != -1, "open valid file path"))
		return 1;

	int retval = ioctl(fd, BATTLESHIP_RESET, empty_config);

	if (tap_ok(retval == 0, "create empty board"))
		return 1;

	char moves[] = "(01,01)(01,02)(01,03)";
	ssize_t moves_written = write(fd, moves, 3 * BATTLESHIP_MOVE_SIZE);

	if (tap_ok(moves_written == 3, "write multiple moves"))
		return 1;

	char buffer[BATTLESHIP_READ_SIZE * 4];
	ssize_t bytes_read = read(fd, buffer, 3 * BATTLESHIP_READ_SIZE);

	if (tap_ok(bytes_read == 3 * BATTLESHIP_READ_SIZE &&
		   strcmp(buffer, "(01,01):m(01,02):m(01,03):m") == 0, "read multiple moves"))
		return 1;

	return tap_test_passed(__func__);
}

static int test_read_round_down(void)
{
	int fd = open(dev_path, O_RDWR);

	if (tap_ok(fd != -1, "open valid file path"))
		return 1;

	int retval = ioctl(fd, BATTLESHIP_RESET, empty_config);

	if (tap_ok(retval == 0, "create empty board"))
		return 1;

	char moves[] = "(01,01)(01,02)(01,03)";
	ssize_t moves_written = write(fd, moves, 3 * BATTLESHIP_MOVE_SIZE);

	if (tap_ok(moves_written == 3, "write multiple moves"))
		return 1;

	char buffer[BATTLESHIP_READ_SIZE * 4];
	ssize_t bytes_read = read(fd, buffer, BATTLESHIP_READ_SIZE / 2);

	if (tap_ok(bytes_read == 0 && strlen(buffer) == 0, "read no moves"))
		return 1;

	bytes_read = read(fd, buffer, BATTLESHIP_READ_SIZE + BATTLESHIP_READ_SIZE / 2);
	if (tap_ok(bytes_read == BATTLESHIP_READ_SIZE &&
		   strcmp(buffer, "(01,01):m") == 0, "read one move"))
		return 1;

	return tap_test_passed(__func__);
}

static int test_write_invalid(void)
{
	int fd = open(dev_path, O_RDWR);

	if (tap_ok(fd != -1, "open valid file path"))
		return 1;

	int retval = ioctl(fd, BATTLESHIP_RESET, default_config);

	if (tap_ok(retval == 0, "create empty board"))
		return 1;

	char moves[] = "invalid";
	ssize_t moves_written = write(fd, moves, BATTLESHIP_MOVE_SIZE);

	if (tap_ok(moves_written < 0, "write invalid move"))
		return 1;

	return tap_test_passed(__func__);
}

static int test_write_out_of_bounds(void)
{
	int fd = open(dev_path, O_RDWR);

	if (tap_ok(fd != -1, "open valid file path"))
		return 1;

	const int board_width = 10;
	int retval = ioctl(fd, BATTLESHIP_RESET, default_config);

	if (tap_ok(retval == 0, "create empty board"))
		return 1;

	char moves[] = "(11,01)(01,11)(11,11)";
	ssize_t moves_written = write(fd, moves, BATTLESHIP_MOVE_SIZE);

	if (tap_ok(moves_written < 0, "write out of bounds"))
		return 1;

	moves_written = write(fd, moves + BATTLESHIP_MOVE_SIZE, BATTLESHIP_MOVE_SIZE);
	if (tap_ok(moves_written < 0, "write out of bounds"))
		return 1;

	moves_written = write(fd, moves + 2 * BATTLESHIP_MOVE_SIZE, BATTLESHIP_MOVE_SIZE);
	if (tap_ok(moves_written < 0, "write out of bounds"))
		return 1;

	return tap_test_passed(__func__);
}

static int test_write_whitespaces(void)
{
	int fd = open(dev_path, O_RDWR);

	if (tap_ok(fd != -1, "open valid file path"))
		return 1;

	int retval = ioctl(fd, BATTLESHIP_RESET, default_config);

	if (tap_ok(retval == 0, "create empty board"))
		return 1;

	char moves[] = " (01,01) ";
	ssize_t moves_written = write(fd, moves, BATTLESHIP_MOVE_SIZE);

	if (tap_ok(moves_written == 1, "write one move"))
		return 1;

	return tap_test_passed(__func__);
}

static int test_lseek_beginning(void)
{
	int fd = open(dev_path, O_RDWR);

	if (tap_ok(fd != -1, "open valid file path"))
		return 1;

	int retval = ioctl(fd, BATTLESHIP_RESET, empty_config);

	if (tap_ok(retval == 0, "reset board"))
		return 1;

	char moves[] = "(01,01)(01,02)(01,03)";
	ssize_t moves_written = write(fd, moves, 3 * BATTLESHIP_MOVE_SIZE);

	if (tap_ok(moves_written == 3, "write multiple moves"))
		return 1;

	off_t offset = lseek(fd, 0, SEEK_SET);

	if (tap_ok(offset == 0, "lseek to beginning of file"))
		return 1;

	char buffer[BATTLESHIP_READ_SIZE * 2];
	ssize_t bytes_read = read(fd, buffer, BATTLESHIP_READ_SIZE);

	if (tap_ok(bytes_read == BATTLESHIP_READ_SIZE &&
		   strcmp(buffer, "(01,01):m") == 0, "read first move"))
		return 1;

	return tap_test_passed(__func__);
}

static int test_lseek_end(void)
{
	int fd = open(dev_path, O_RDWR);

	if (tap_ok(fd != -1, "open valid file path"))
		return 1;

	int retval = ioctl(fd, BATTLESHIP_RESET, empty_config);

	if (tap_ok(retval == 0, "reset board"))
		return 1;

	char moves[] = "(01,01)(01,02)(01,03)";
	ssize_t moves_written = write(fd, moves, 3 * BATTLESHIP_MOVE_SIZE);

	if (tap_ok(moves_written == 3, "write multiple moves"))
		return 1;

	off_t offset = lseek(fd, -BATTLESHIP_READ_SIZE, SEEK_END);

	if (tap_ok(offset == (2 * BATTLESHIP_READ_SIZE), "lseek to end of file"))
		return 1;

	char buffer[BATTLESHIP_READ_SIZE * 2];
	ssize_t bytes_read = read(fd, buffer, BATTLESHIP_READ_SIZE);

	if (tap_ok(bytes_read == BATTLESHIP_READ_SIZE &&
		   strcmp(buffer, "(01,03):m") == 0, "read last move"))
		return 1;

	return tap_test_passed(__func__);
}

static int test_lseek_current(void)
{
	int fd = open(dev_path, O_RDWR);

	if (tap_ok(fd != -1, "open valid file path"))
		return 1;

	int retval = ioctl(fd, BATTLESHIP_RESET, empty_config);

	if (tap_ok(retval == 0, "reset board"))
		return 1;

	char moves[] = "(01,01)(01,02)(01,03)";
	ssize_t moves_written = write(fd, moves, 3 * BATTLESHIP_MOVE_SIZE);

	if (tap_ok(moves_written == 3, "write multiple moves"))
		return 1;

	off_t offset = lseek(fd, BATTLESHIP_READ_SIZE, SEEK_CUR);

	if (tap_ok(offset == 0, "lseek to 2nd move"))
		return 1;

	char buffer[BATTLESHIP_READ_SIZE * 2];
	ssize_t bytes_read = read(fd, buffer, BATTLESHIP_READ_SIZE);

	if (tap_ok(bytes_read == BATTLESHIP_READ_SIZE &&
		   strcmp(buffer, "(01,02):m") == 0, "read second move"))
		return 1;

	return tap_test_passed(__func__);
}

static int test_lseek_out_of_bounds(void)
{
	int fd = open(dev_path, O_RDWR);

	if (tap_ok(fd != -1, "open valid file path"))
		return 1;

	int retval = ioctl(fd, BATTLESHIP_RESET, default_config);

	if (tap_ok(retval == 0, "reset board"))
		return 1;

	char moves[] = "(01,01)(01,02)(01,03)";
	ssize_t moves_written = write(fd, moves, 3 * BATTLESHIP_MOVE_SIZE);

	if (tap_ok(moves_written == 3, "write multiple moves"))
		return 1;

	off_t offset = lseek(fd, 4 * BATTLESHIP_READ_SIZE, SEEK_SET);

	if (tap_ok(offset < 0, "lseek out of bounds"))
		return 1;

	return tap_test_passed(__func__);
}

static int test_ioctl_undo_sanity(void)
{
	int fd = open(dev_path, O_RDWR);

	if (tap_ok(fd != -1, "open valid file path"))
		return 1;

	int retval = ioctl(fd, BATTLESHIP_RESET, default_config);

	if (tap_ok(retval == 0, "reset board"))
		return 1;

	retval = ioctl(fd, BATTLESHIP_UNDO);
	if (tap_ok(retval < 0, "undo on empty board with no moves"))
		return 1;

	return tap_test_passed(__func__);
}

static int test_ioctl_redo_sanity(void)
{
	int fd = open(dev_path, O_RDWR);

	if (tap_ok(fd != -1, "open valid file path"))
		return 1;

	int retval = ioctl(fd, BATTLESHIP_RESET, default_config);

	if (tap_ok(retval == 0, "reset board"))
		return 1;

	retval = ioctl(fd, BATTLESHIP_REDO);
	if (tap_ok(retval < 0, "redo on empty board with no undoes"))
		return 1;

	return tap_test_passed(__func__);
}

static int test_ioctl_undo(void)
{
	int fd = open(dev_path, O_RDWR);

	if (tap_ok(fd != -1, "open valid file path"))
		return 1;

	int retval = ioctl(fd, BATTLESHIP_RESET, empty_config);

	if (tap_ok(retval == 0, "reset board"))
		return 1;

	char moves[] = "(01,01)(01,02)(01,03)";
	ssize_t moves_written = write(fd, moves, 3 * BATTLESHIP_MOVE_SIZE);

	if (tap_ok(moves_written == 3, "write multiple moves"))
		return 1;

	off_t offset = lseek(fd, -BATTLESHIP_READ_SIZE, SEEK_END);

	if (tap_ok(offset == (2 * BATTLESHIP_READ_SIZE), "lseek to end of file"))
		return 1;

	char buffer[BATTLESHIP_READ_SIZE * 2];
	ssize_t bytes_read = read(fd, buffer, BATTLESHIP_READ_SIZE);

	if (tap_ok(bytes_read == BATTLESHIP_READ_SIZE &&
		   strcmp(buffer, "(01,03):m") == 0, "read last move"))
		return 1;

	retval = ioctl(fd, BATTLESHIP_UNDO);
	if (tap_ok(retval == 0, "undo"))
		return 1;

	offset = lseek(fd, -BATTLESHIP_READ_SIZE, SEEK_END);
	if (tap_ok(offset == BATTLESHIP_READ_SIZE, "lseek to end of file"))
		return 1;

	bytes_read = read(fd, buffer, BATTLESHIP_READ_SIZE);
	if (tap_ok(bytes_read == BATTLESHIP_READ_SIZE &&
		   strcmp(buffer, "(01,02):m") == 0, "read last move"))
		return 1;

	return tap_test_passed(__func__);
}

static int test_ioctl_redo(void)
{
	int fd = open(dev_path, O_RDWR);

	if (tap_ok(fd != -1, "open valid file path"))
		return 1;

	int retval = ioctl(fd, BATTLESHIP_RESET, empty_config);

	if (tap_ok(retval == 0, "reset board"))
		return 1;

	char moves[] = "(01,01)(01,02)(01,03)";
	ssize_t moves_written = write(fd, moves, 3 * BATTLESHIP_MOVE_SIZE);

	if (tap_ok(moves_written == 3, "write multiple moves"))
		return 1;

	off_t offset = lseek(fd, -BATTLESHIP_READ_SIZE, SEEK_END);

	if (tap_ok(offset == (2 * BATTLESHIP_READ_SIZE), "lseek to end of file"))
		return 1;

	char buffer[BATTLESHIP_READ_SIZE * 2];
	ssize_t bytes_read = read(fd, buffer, BATTLESHIP_READ_SIZE);

	if (tap_ok(bytes_read == BATTLESHIP_READ_SIZE &&
		   strcmp(buffer, "(01,03):m") == 0, "read last move"))
		return 1;

	retval = ioctl(fd, BATTLESHIP_UNDO);
	if (tap_ok(retval == 0, "undo"))
		return 1;

	offset = lseek(fd, -BATTLESHIP_READ_SIZE, SEEK_END);
	if (tap_ok(offset == BATTLESHIP_READ_SIZE, "lseek to end of file"))
		return 1;

	bytes_read = read(fd, buffer, BATTLESHIP_READ_SIZE);
	if (tap_ok(bytes_read == BATTLESHIP_READ_SIZE &&
		   strcmp(buffer, "(01,02):m") == 0, "read last move"))
		return 1;

	retval = ioctl(fd, BATTLESHIP_REDO);
	if (tap_ok(retval == 0, "redo"))
		return 1;

	offset = lseek(fd, -BATTLESHIP_READ_SIZE, SEEK_END);
	if (tap_ok(offset == BATTLESHIP_READ_SIZE, "lseek to end of file"))
		return 1;

	bytes_read = read(fd, buffer, BATTLESHIP_READ_SIZE);
	if (tap_ok(bytes_read == BATTLESHIP_READ_SIZE &&
		   strcmp(buffer, "(01,03):m") == 0, "read last move"))
		return 1;

	return tap_test_passed(__func__);
}

int main(void)
{
	tap_print_header();

	run_test(test_open);
	run_test(test_close);

	run_test(test_ioctl_sanity);
	run_test(test_ioctl_errors);

	run_test(test_read_sanity);
	run_test(test_read_errors);

	run_test(test_write_sanity);
	run_test(test_write_errors);

	run_test(test_lseek_sanity);
	run_test(test_lseek_errors);

	run_test(test_write_moves);
	run_test(test_read_move);

	run_test(test_read_multiple_moves);
	run_test(test_read_round_down);

	run_test(test_write_invalid);
	run_test(test_write_out_of_bounds);
	run_test(test_write_whitespaces);

	run_test(test_lseek_beginning);
	run_test(test_lseek_end);
	run_test(test_lseek_current);
	run_test(test_lseek_out_of_bounds);

	run_test(test_ioctl_undo_sanity);
	run_test(test_ioctl_redo_sanity);
	run_test(test_ioctl_undo);
	run_test(test_ioctl_redo);
	return 0;
}
