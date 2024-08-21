// SPDX-License-Identifier: GPL-2.0-or-later

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>
#include <string.h>

#define NUMBERS_ASCII_OFFSET 48

#define CUBE_FILE_PATH "/dev/cube"
#define FILE_OWNER "root"
#define FILE_GROUP "root"

#define FILE_PERMISSIONS (0644)

#define CUBE_SIDE_SIZE 3
#define CUBE_FACE_COUNT 6
#define CUBE_FACE_SIZE (CUBE_SIDE_SIZE * CUBE_SIDE_SIZE)
#define CUBE_SIZE (CUBE_FACE_COUNT * CUBE_FACE_SIZE)

// Some enums and char arrays to make the code more readable
enum cube_moves {
	up = 0,
	left,
	front,
	right,
	back,
	down,
	up_tag,
	left_tag,
	front_tag,
	right_tag,
	back_tag,
	down_tag,
};

static const char *const MOVE_STR[] = { "U",  "L",  "F",  "R",	"B",  "D",
					"U'", "L'", "F'", "R'", "B'", "D'" };

// Some ioctl commands that should be defined in the device,
// but we don't yet have it, so we define them here.
// They are defined here if they are not defined in the device

#ifndef CUBE_SETUP
#define CUBE_SETUP _IOW('c', 1, unsigned short)
#endif // CUBE_SETUP

#ifndef CUBE_IS_SOLVED
#define CUBE_IS_SOLVED _IOR('c', 2, unsigned short)
#endif // CUBE_IS_SOLVED

// Some definitions for the tests

#define TEST(name) void test_##name(void)
#define RUN_TEST(name) run_test(test_##name)

// Some global variables to keep track of the tests

int test_count;
int test_passed;
bool should_bail;

void ok(bool condition, const char *description)
{
	test_count++;
	if (condition) {
		printf("%s %d - %s\n", __func__, test_count, description);
		test_passed++;
	} else {
		printf("not %s %d - %s\n", __func__, test_count, description);
	}
}

void run_test(void (*test_func)(void))
{
	test_func();
}

int open_cube_file(void)
{
	int fd = -1;

	fd = open(CUBE_FILE_PATH, O_RDWR);
	return fd;
}

TEST(cube_open)
{
	int fd = open_cube_file();

	ok(fd != -1, "cube file open at path: " CUBE_FILE_PATH);
	if (fd != -1)
		close(fd);
	else
		should_bail = true;
}

TEST(cube_init_state)
{
	int fd = open_cube_file();

	if (fd == -1) {
		ok(fd != -1, "cube file open");
		return;
	}
	bool res = true;
	int err_i = 0;
	int err_j = 0;
	int err_expected = 0;
	int err_actual = 0;

	for (int i = 0; i < CUBE_FACE_COUNT; i++) {
		char side[CUBE_FACE_SIZE];

		read(fd, side, CUBE_FACE_SIZE);
		for (int j = 0; j < CUBE_FACE_SIZE; j++) {
			if (side[j] != i + NUMBERS_ASCII_OFFSET) {
				res = false;
				err_i = i;
				err_j = j;
				err_expected = i + NUMBERS_ASCII_OFFSET;
				err_actual = side[j];
				goto end;
			}
		}
	}
end:
	if (!res) {
		char error_message[500];

		sprintf(error_message,
			"cube file initialized incorrectly, expected %c at index (%d, %d), got %c",
			err_expected, err_i, err_j, err_actual);
		ok(res, error_message);
	} else {
		ok(res, "cube file initialized correctly");
	}
	close(fd);
}

TEST(test_file_permissions)
{
	struct stat file_stat;
	int result = stat(CUBE_FILE_PATH, &file_stat);

	if (result != 0) {
		ok(result == 0, "stat call succeeded");
		return;
	}
	// Check if file permissions are 644
	mode_t actual_permissions = file_stat.st_mode & 0777;
	char error_message[500];

	if (actual_permissions != FILE_PERMISSIONS) {
		sprintf(error_message,
			"file permissions are as expected, got %o instead of %o",
			actual_permissions, FILE_PERMISSIONS);
		ok(false, error_message);
		return;
	}
	ok(actual_permissions == FILE_PERMISSIONS,
	   "file permissions are as expected");
}

TEST(file_owner)
{
	struct stat file_stat;
	struct passwd *pw;

	if (stat(CUBE_FILE_PATH, &file_stat) != 0) {
		ok(false, "Unable to stat file " CUBE_FILE_PATH);
		return;
	}

	pw = getpwnam(FILE_OWNER);
	if (!pw) {
		ok(false, "Unable to get user info " FILE_OWNER);
		return;
	}

	if (file_stat.st_uid != pw->pw_uid) {
		ok(false, "File owned by user " FILE_OWNER);
		return;
	}
}

TEST(file_group)
{
	struct stat file_stat;
	struct group *gr;

	if (stat(CUBE_FILE_PATH, &file_stat) != 0) {
		ok(false, "Unable to stat file " CUBE_FILE_PATH);
		return;
	}

	gr = getgrnam(FILE_GROUP);
	if (!gr) {
		ok(false, "Unable to get group info for " FILE_GROUP);
		return;
	}
	if (file_stat.st_gid != gr->gr_gid) {
		ok(false, "File owned by group " FILE_GROUP);
		return;
	}
}

TEST(read_more_then_file_size)
{
	int fd = open_cube_file();

	if (fd == -1) {
		ok(fd != -1, "cube file open");
		return;
	}
	int to_read = CUBE_SIZE * 2;
	char cube[to_read];
	int read_res = read(fd, cube, to_read);

	if (read_res != CUBE_SIZE) {
		char error_message[500];

		sprintf(error_message,
			"read more then file size, read %d bytes instead of %d",
			read_res, CUBE_SIZE);
		ok(read_res == CUBE_SIZE, error_message);
	} else {
		ok(read_res == CUBE_SIZE, "read more then file size");
	}
	close(fd);
}

TEST(read_with_null_buffer)
{
	//  If the pointer is invalid it will return -1 with errno set to `EFAULT`.
	int fd = open_cube_file();

	if (fd == -1) {
		ok(fd != -1, "cube file open");
		return;
	}
#pragma GCC diagnostic ignored "-Wnonnull"
	int read_res = read(fd, NULL, CUBE_SIZE);

	if (read_res == -1 && errno == EFAULT) {
		ok(read_res != -1 && errno != EFAULT, "read with null buffer");
	} else {
		char error_message[500];

		sprintf(error_message,
			"read with null buffer, returned %d with errno %d, expected return value of -1 with errno %d",
			read_res, errno, EFAULT);
		ok(read_res == -1 && errno == EFAULT, error_message);
	}
	close(fd);
}

TEST(consecutive_reads)
{
	int fd = open_cube_file();

	if (fd == -1) {
		ok(fd != -1, "cube file open");
		return;
	}

	off_t previous_pos, current_pos;

	previous_pos = lseek(fd, 0, SEEK_CUR); // Initial position

	for (int i = 0; i < CUBE_SIZE; i++) {
		char cube[1];
		int read_res = read(fd, cube, 1);

		if (read_res != 1) {
			char error_message[500];

			sprintf(error_message,
				"read more than file size, read %d bytes instead of %d",
				read_res, 1);
			ok(read_res == 1, error_message);
			goto end;
		}

		current_pos =
			lseek(fd, 0, SEEK_CUR); // Current position after read
		if (current_pos != previous_pos + 1) {
			ok(current_pos == previous_pos + 1,
			   "file position incremented by 1");
			goto end;
		}
		previous_pos = current_pos; // Update previous position
	}
	ok(true, "consecutive reads"); // No errors
end:
	close(fd);
}

TEST(read_after_end_of_file)
{
	int fd = open_cube_file();

	if (fd == -1) {
		ok(fd != -1, "cube file open");
		return;
	}
	// Move the file pointer to the end of the file
	int lseek_res = lseek(fd, CUBE_SIZE, SEEK_SET);

	if (lseek_res == -1) {
		ok(lseek_res != -1,
		   "lseek to end of file when testing read after end of file");
		close(fd);
		return;
	}
	char cube[1];
	int read_res = read(fd, cube, 1);

	if (read_res == 0) {
		ok(read_res == 0, "read after end of file");
	} else {
		char error_message[500];

		sprintf(error_message,
			"read after end of file, read %d bytes instead of 0",
			read_res);
		ok(read_res == 0, error_message);
	}
	close(fd);
}

TEST(read_buffer_too_small)
{
	int fd = open_cube_file();

	if (fd == -1) {
		ok(fd != -1, "cube file open");
		return;
	}
	int size_to_read = CUBE_SIZE / 2;
	char cube[size_to_read];

#pragma GCC diagnostic ignored "-Wstringop-overflow"
	int read_res = read(fd, cube, CUBE_SIZE);

	if (read_res != size_to_read) {
		char error_message[500];

		sprintf(error_message,
			"read buffer too small, read %d bytes instead of %d",
			read_res, size_to_read);
		ok(read_res == size_to_read, error_message);
	} else {
		ok(read_res == size_to_read, "read buffer too small");
	}
	close(fd);
}

TEST(write_invalid_value)
{
	int fd = open_cube_file();

	if (fd == -1) {
		ok(fd != -1, "cube file open");
		return;
	}
	char invalid_value = 'a';
	int write_res = write(fd, &invalid_value, 1);

	if (write_res == -1 && errno == EPERM) {
		ok(write_res != -1 && errno != EPERM, "write invalid value");
	} else {
		char error_message[500];

		sprintf(error_message,
			"write invalid value, returned %d with errno %d, expected return value of -1 with errno %d",
			write_res, errno, EPERM);
		ok(write_res == -1 && errno == EPERM, error_message);
	}
	close(fd);
}

TEST(write_invalid_format)
{
	char invalid_format[] = "FF";
	int fd = open_cube_file();

	if (fd == -1) {
		ok(fd != -1, "cube file open");
		return;
	}
	int write_res = write(fd, invalid_format, 2);

	if (write_res == -1 && errno == EPERM) {
		ok(write_res != -1 && errno != EPERM, "write invalid format");
	} else {
		char error_message[500];

		sprintf(error_message,
			"write invalid format, returned %d with errno %d, expected return value of -1 with errno %d",
			write_res, errno, EPERM);
		ok(write_res == -1 && errno == EPERM, error_message);
	}
	close(fd);
}

TEST(write_invalid_format_change_state)
{
	char invalid_format[] = "FF";
	int fd = open_cube_file();

	if (fd == -1) {
		ok(fd != -1, "cube file open");
		return;
	}
	char cube_init[CUBE_SIZE];
	int res = read(fd, cube_init, CUBE_SIZE);

	if (res != CUBE_SIZE) {
		ok(res == CUBE_SIZE, "read cube file wrong size");
		close(fd);
		return;
	}
	write(fd, invalid_format, 2);

	// Move the file pointer to the beginning of the file
	lseek(fd, 0, SEEK_SET);

	char cube_after[CUBE_SIZE];

	res = read(fd, cube_after, CUBE_SIZE);

	if (res != CUBE_SIZE) {
		ok(res == CUBE_SIZE, "read cube file wrong size");
		close(fd);
		return;
	}
	long cube_size = CUBE_SIZE;

	ok(memcmp(cube_init, cube_after, cube_size) == 0,
	   "write invalid format does not change state");
	close(fd);
}

void test_rotation(enum cube_moves move, char *expected_final_state)
{
	int fd = open_cube_file();

	if (fd == -1) {
		ok(fd != -1, "cube file open");
		return;
	}
	const char *move_str = MOVE_STR[move];
	int move_size;

	if (move < 6)
		move_size = 1;
	else
		move_size = 2;

	int res = write(fd, move_str, move_size);

	if (res != 1) {
		char error_message[500];

		sprintf(error_message,
			"write move did not run 1 move when sent %s, run %d moves",
			move_str, res);
		ok(false, error_message);
		close(fd);
		return;
	}
	char cube_after[CUBE_SIZE];

	res = read(fd, cube_after, CUBE_SIZE);

	if (res != CUBE_SIZE) {
		ok(res == CUBE_SIZE, "read cube file wrong size");
		close(fd);
		return;
	}
	long cube_size = CUBE_SIZE;
	char error_message[500];

	sprintf(error_message,
		"Test for rotation of type %s, got %s and expected %s",
		move_str, cube_after, expected_final_state);
	ok(memcmp(expected_final_state, cube_after, cube_size) == 0,
	   error_message);
	close(fd);
}

TEST(write_test_rotate_front)
{
	test_rotation(front,
		      "000000111115115115222222222033033033444444444333555555");
}

TEST(write_test_rotate_right)
{
	test_rotation(right,
		      "002002002111111111225225225333333333044044044554554554");
}

TEST(write_test_rotate_up)
{
	test_rotation(up,
		      "000000000222111111333222222444333333111444444555555555");
}

TEST(write_test_rotate_left)
{
	test_rotation(left,
		      "400400400111111111022022022333333333445445445255255255");
}

TEST(write_test_rotate_back)
{
	test_rotation(back,
		      "333000000011011011222222222335335335444444444555555111");
}

TEST(write_test_rotate_down)
{
	test_rotation(down,
		      "000000000111111444222222111333333222444444333555555555");
}

TEST(write_test_rotate_front_tag)
{
	test_rotation(front_tag,
		      "000000333110110110222222222533533533444444444111555555");
}

TEST(write_test_rotate_right_tag)
{
	test_rotation(right_tag,
		      "004004004111111111220220220333333333544544544552552552");
}

TEST(write_test_rotate_up_tag)
{
	test_rotation(up_tag,
		      "000000000444111111111222222222333333333444444555555555");
}

TEST(write_test_rotate_left_tag)
{
	test_rotation(left_tag,
		      "200200200111111111522522522333333333440440440455455455");
}

TEST(write_test_rotate_back_tag)
{
	test_rotation(back_tag,
		      "111000000511511511222222222330330330444444444555555333");
}

TEST(write_test_rotate_down_tag)
{
	test_rotation(down_tag,
		      "000000000111111222222222333333333444444444111555555555");
}

TEST(write_null_buffer)
{
	int fd = open_cube_file();

	if (fd == -1) {
		ok(fd != -1, "cube file open");
		return;
	}
	int res = write(fd, NULL, 8);
	int err_got = errno;

	if (res == -1 && err_got == EINVAL) {
		ok(res == -1 && err_got == EINVAL, "write null buffer");
	} else {
		char error_message[500];

		sprintf(error_message,
			"write null buffer, returned %d with errno %d, expected return value of -1 with errno %d",
			res, err_got, EINVAL);
		ok(false, error_message);
	}
	close(fd);
}

TEST(lseek_seek_set)
{
	int fd = open_cube_file();

	if (fd == -1) {
		ok(fd != -1, "cube file open");
		return;
	}
	int seek_amount = 3;
	int res = lseek(fd, seek_amount, SEEK_SET);

	if (res == seek_amount) {
		ok(true, "lseek seek set");
	} else {
		char error_message[500];

		sprintf(error_message,
			"lseek seek set, returned %d, expected return value of %d after running lseek with SEEK_SET and %d",
			res, seek_amount, seek_amount);
		ok(false, error_message);
	}
	close(fd);
}

TEST(lseek_seek_set_neg)
{
	int fd = open_cube_file();

	if (fd == -1) {
		ok(fd != -1, "cube file open");
		return;
	}
	int seek_amount = -5;
	int res = lseek(fd, seek_amount, SEEK_SET);
	int err_got = errno;

	if (res == -1 && err_got == EINVAL) {
		ok(true, "lseek seek set neg");
	} else {
		char error_message[500];

		sprintf(error_message,
			"lseek seek set neg, returned %d and errno %d, expected return value of -1 and errno of %d after running lseek with SEEK_SET and %d",
			res, err_got, EINVAL, seek_amount);
		ok(false, error_message);
	}
	close(fd);
}

TEST(lseek_seek_cur)
{
	int fd = open_cube_file();

	if (fd == -1) {
		ok(fd != -1, "cube file open");
		return;
	}
	int seek_amount = 3;
	int res = lseek(fd, seek_amount, SEEK_CUR);

	if (res != seek_amount) {
		char error_message[500];

		sprintf(error_message,
			"lseek seek set 1st call, returned %d, expected return value of %d after running lseek with SEEK_CUR and %d",
			res, seek_amount, seek_amount);
		ok(false, error_message);
		goto end;
	}
	res = lseek(fd, seek_amount, SEEK_CUR);
	int expected_pos = seek_amount * 2;

	if (res == expected_pos) {
		ok(true, "lseek seek cur");
	} else {
		char error_message[500];

		sprintf(error_message,
			"lseek seek set 2nd call, returned %d, expected return value of %d after running lseek with SEEK_CUR and %d on the 2nd call with this amount",
			res, expected_pos, seek_amount);
		ok(false, error_message);
	}
end:
	close(fd);
}

TEST(lseek_past_end)
{
	int fd = open_cube_file();

	if (fd == -1) {
		ok(fd != -1, "cube file open");
		return;
	}
	int seek_amount = 999;
	int res = lseek(fd, seek_amount, SEEK_SET);

	if (res == seek_amount) {
		ok(true, "lseek past end");
	} else {
		char error_message[500];

		sprintf(error_message,
			"lseek seek set, returned %d, expected return value of %d after running lseek with SEEK_SET and %d",
			res, seek_amount, seek_amount);
		ok(false, error_message);
	}
	close(fd);
}

TEST(ioctl_cube_setup) // 3 Tests
{
	int fd = open_cube_file();

	if (fd == -1) {
		// We do it three times to match the number of tests
		for (int i = 0; i < 3; i++)
			ok(false, "cube file open");
		return;
	}

	unsigned short setup_moves;
	int result;

	setup_moves = 0;
	result = ioctl(fd, CUBE_SETUP, &setup_moves);
	ok(result == 0, "setup cube with 0 moves");

	setup_moves = 10;
	result = ioctl(fd, CUBE_SETUP, &setup_moves);
	ok(result == 0, "setup cube with 10 moves");

	setup_moves = 255;
	result = ioctl(fd, CUBE_SETUP, &setup_moves);
	ok(result == 0, "setup cube with 255 moves");
	close(fd);
}

TEST(ioctl_cube_is_solved) // 2 Tests
{
	int fd = open_cube_file();

	if (fd == -1) {
		// We do it twice to match the number of tests
		for (int i = 0; i < 2; i++)
			ok(false, "cube file open");
		return;
	}

	unsigned short is_solved;
	int result;

	ioctl(fd, CUBE_SETUP, 0);
	result = ioctl(fd, CUBE_IS_SOLVED, &is_solved);
	ok(result == 0 && is_solved == 1,
	   "check if cube is solved after 0 moves (expected: 1)");

	ioctl(fd, CUBE_SETUP, 1);
	result = ioctl(fd, CUBE_IS_SOLVED, &is_solved);
	ok(result == 0 && is_solved == 0,
	   "check if cube is solved after 1 moves (expected: 0, since cube can't be solved with 1 move after clean setup)");

	close(fd);
}

TEST(ioctl_invalid_command)
{
	int fd = open_cube_file();

	if (fd == -1) {
		ok(false, "cube file open");
		return;
	}

	int result = ioctl(fd, _IOC_NONE, 0);
	int err = errno;

	ok(result == -1 && err == EINVAL,
	   "invalid IOCTL command (expected: EINVAL)");

	close(fd);
}

TEST(ioctl_cube_setup_invalid_input)
{
	int fd = open_cube_file();

	if (fd == -1) {
		ok(false, "cube file open");
		return;
	}
	int result = ioctl(fd, CUBE_SETUP, NULL);
	int err = errno;

	ok(result == -1 && err == EFAULT,
	   "setup cube with NULL pointer (expected: EFAULT)");

	close(fd);
}

TEST(ioctl_cube_is_solved_invalid_input)
{
	int fd = open_cube_file();

	if (fd == -1) {
		ok(false, "cube file open");
		return;
	}

	int result = ioctl(fd, CUBE_IS_SOLVED, NULL);
	int err = errno;

	ok(result == -1 && err == EFAULT,
	   "check if cube is solved with NULL pointer (expected: EFAULT)");

	close(fd);
}

int main(void)
{
	test_count = 0;
	test_passed = 0;
	should_bail = false;

	// General Tests
	RUN_TEST(cube_open);
	if (should_bail)
		goto done_testing;

	RUN_TEST(cube_init_state);
	RUN_TEST(test_file_permissions);
	RUN_TEST(file_owner);
	RUN_TEST(file_group);

	// Read Tests
	RUN_TEST(read_more_then_file_size);
	RUN_TEST(read_with_null_buffer);
	RUN_TEST(consecutive_reads);
	RUN_TEST(read_after_end_of_file);
	RUN_TEST(read_buffer_too_small);

	// Write Tests
	RUN_TEST(write_invalid_value);
	RUN_TEST(write_invalid_format);
	RUN_TEST(write_invalid_format_change_state);
	RUN_TEST(write_null_buffer);

	// Write Tests - Rotations
	/// Clockwise rotations
	RUN_TEST(write_test_rotate_front);
	RUN_TEST(write_test_rotate_right);
	RUN_TEST(write_test_rotate_up);
	RUN_TEST(write_test_rotate_left);
	RUN_TEST(write_test_rotate_back);
	RUN_TEST(write_test_rotate_down);
	/// Counter-clockwise rotations
	RUN_TEST(write_test_rotate_front_tag);
	RUN_TEST(write_test_rotate_right_tag);
	RUN_TEST(write_test_rotate_up_tag);
	RUN_TEST(write_test_rotate_left_tag);
	RUN_TEST(write_test_rotate_back_tag);
	RUN_TEST(write_test_rotate_down_tag);

	// lseek tests
	RUN_TEST(lseek_seek_set);
	RUN_TEST(lseek_seek_set_neg);
	RUN_TEST(lseek_seek_cur);
	RUN_TEST(lseek_past_end);

	// ioctl tests
	RUN_TEST(ioctl_cube_setup); // 3 Tests
	RUN_TEST(ioctl_cube_is_solved); // 2 Tests
	RUN_TEST(ioctl_invalid_command);
	RUN_TEST(ioctl_cube_setup_invalid_input);
	RUN_TEST(ioctl_cube_is_solved_invalid_input);

done_testing:
	// It is ok for tha plan to be at the end https://testanything.org/tap-version-14-specification.html
	// "The Plan must appear exactly once in the file, (...), or after all Test Point lines."
	printf("1..%d\n", test_count);
	// Bail out if we couldn't open the cube file, following the TAP protocol
	if (should_bail)
		printf("Bail out! Unable to open cube file\n");
	return test_passed == test_count ? 0 : 1;
}
