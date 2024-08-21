#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

//TODO: YOU MUST CHANGE THE PATHS OF THE DEVICE FILES!!!
#define BLACK_DEVICE_PATH "./devices/black_device"
#define RED_DEVICE_PATH "./devices/red_device"
#define GREEN_DEVICE_PATH "./devices/green_device"
#define YELLOW_DEVICE_PATH "./devices/yellow_device"
#define BLUE_DEVICE_PATH "./devices/blue_device"
#define MAGENTA_DEVICE_PATH "./devices/magenta_device"
#define CYAN_DEVICE_PATH "./devices/cyan_device"
#define WHITE_DEVICE_PATH "./devices/white_device"
#define GREY_DEVICE_PATH "./devices/grey_device"
#define ORANGE_DEVICE_PATH "./devices/grey_device"

#define N_COLORS 10

typedef int (*test_func)(void);

int fd_arr[10] = {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1};
enum color {black, red, green, yellow, blue, megenta, cyan, white, grey, orange};

char *devices_path[N_COLORS] = {
	BLACK_DEVICE_PATH, RED_DEVICE_PATH, GREEN_DEVICE_PATH, YELLOW_DEVICE_PATH,
	BLUE_DEVICE_PATH, MAGENTA_DEVICE_PATH, CYAN_DEVICE_PATH, WHITE_DEVICE_PATH,
	GREY_DEVICE_PATH, ORANGE_DEVICE_PATH
};

//TODO: set this flag to 1 in order to draw the picture in the last test
//	if it drew a cool picture, you successully used the modules to create a cool picture!
#define DRAW_FINAL_TEST 0

// test - can open all of the device files
int test_0(void)
{
	for (int i = 0; i < N_COLORS; i++) {
		fd_arr[i] = open(devices_path[i], O_RDWR);
		if (fd_arr[i] < 0)
			return 0;
	}
	return 1;
}

// test - write a single byte to the black device file
int test_1(void)
{
	int buf[1] = {0};

	if (write(fd_arr[black], buf, 1) != 1)
		return 0;

	return 1;
}

// test - write with a NULL buffer with size 0
int test_2(void)
{
	int *my_null_buff = NULL;

	if (fd_arr[black] != -1 && write(fd_arr[black], my_null_buff, 0) == -1 && errno == EINVAL)
		return 1;

	return 0;
}

// test - write with a NULL buffer with > 0 size
int test_3(void)
{
	int *my_null_buff = NULL;

	if (write(fd_arr[black], my_null_buff, 2332) == -1 && errno == EINVAL)
		return 1;

	return 0;
}

// test - write to the entire file
int test_4(void)
{
	int buf[20 * 20] = {0};

	if (write(fd_arr[yellow], buf, 20 * 20) != 20 * 20)
		return 0;

	return 0;
}

// test - try to write to the EOF
int test_5(void)
{
	int buf[321] = {0};

	if (write(fd_arr[yellow], buf, 321) == 0)
		return 1;

	return 0;
}

// test - read one byte
int test_6(void)
{
	int buf[1] = {0};

	if (read(fd_arr[cyan], buf, 1) != 1)
		return 0;

	if (buf[0] != 3) // should be painted yellow
		return 0;

	return 1;
}

// test - read the first pixal after it was painted
int test_7(void)
{
	int buf[1] = {0};

	if (write(fd_arr[green], buf, 1) != 1)
		return 0;

	if (read(fd_arr[megenta], buf, 1) != 1 || buf[1] != 2)
		return 0;

	return 1;
}

// test - read the entire file
int test_8(void)
{
	int buf[20 * 20 - 1] = {0};

	if (read(fd_arr[megenta], buf, 20 * 20 - 1) != 20 * 20 - 1)
		return 0;

	return 1;
}

// test - read the EOF
int test_9(void)
{
	int buf[100] = {0};

	if (read(fd_arr[megenta], buf, 100) != 0)
		return 0;

	return 1;
}

// test - read with NULL buffer
int test_10(void)
{
	int *my_null_buffer = NULL;

	if (read(fd_arr[red], my_null_buffer, 23) != -1 || errno != EINVAL)
		return 0;

	return 1;
}

// test - ioctl, color the entrie canvas
int test_11(void)
{
	if (ioctl(fd_arr[megenta], 0) == -1)
		return 0;

	return 1;
}

// test - ioctl, even when fp at EOF
int test_12(void)
{
	if (ioctl(fd_arr[megenta], 0) == -1)
		return 0;

	return 1;
}

// test - ioctl, unsupported command
int test_13(void)
{
	if (ioctl(fd_arr[megenta], 3) == -1 && errno == ENOTTY)
		return 1;

	return 0;
}

// test - ioctl, unsupported arguments
int test_14(void)
{
	int args[3] = {1, 2, 3};

	if (ioctl(fd_arr[black], 0, args) == -1 && errno == EINVAL)
		return 1;

	return 0;
}

// test - ioctl, unsupported argument and unsupported command
int test_15(void)
{
	int args[3] = {1, 2, 3};

	if (ioctl(fd_arr[black], 2, args) == -1 && (errno == EINVAL || errno == EFAULT))
		return 1;

	return 0;
}

// test - lseek, set all file pointers to EOF
int test_16(void)
{
	for (int i = 0; i < N_COLORS; i++)
		if (lseek(fd_arr[i], 0, SEEK_END) != 20 * 20 - 1)
			return 0;

	return 1;
}

// test - lseek, reset all file pointers to the start
int test_17(void)
{
	for (int i = 0; i < N_COLORS; i++)
		if (lseek(fd_arr[i], 0, SEEK_SET) != 0)
			return 0;

	return 1;
}

// test - move file position via lseek
int test_18(void)
{
	if (lseek(fd_arr[cyan], 15, SEEK_CUR) != 15)
		return 0;

	if (lseek(fd_arr[megenta], 15, SEEK_CUR) != 15)
		return 0;

	if (lseek(fd_arr[yellow], 15, SEEK_CUR) != 15)
		return 0;

	return 1;
}

// test - Read Write together to create a chess board image
int test_19(void)
{
	int buf[1] = {0};

	for (int i = 0; i < 20 * 20; i++) {
		if (i % 2 == 0) {
			if (read(fd_arr[megenta], buf, 1) != 1)
				return 0;
			if (write(fd_arr[black], buf, 1) != 1)
				return 0;

		} else {
			if (write(fd_arr[megenta], buf, 1) != 1)
				return 0;
			if (read(fd_arr[black], buf, 1) != 1)
				return 0;
		}
	}
	return 1;
}

int test_20(void)
{
	int buf[20 * 20] = {0};

	if (write(fd_arr[blue], buf, 20 * 20) != 20 * 20)
		return 0;

	int buf_[20] = {0};

	if (write(fd_arr[red], buf_, 20) != 20)
		return 0;

	if (lseek(fd_arr[red], 20, SEEK_CUR) != 40)
		return 0;

	return 1;
}

// test - draw a chess board using ioctl and write
int test_21(void)
{
	int buf[1] = {0};

	if (ioctl(fd_arr[megenta], 0) == -1)
		return 0;

	if (lseek(fd_arr[black], 0, SEEK_SET) != 0)
		return 0;

	for (int i = 0; i < 20 * 20; i += 2)
		if (write(fd_arr[black], buf, 1) != 1)
			return 0;

	return 1;
}

// test - ioctl read together to parse through the canvas
int test_22(void)
{
	int buf[1] = {0};

	if (lseek(fd_arr[black], 0, SEEK_SET) != 0)
		return 0;

	int expected_fp = 0;

	if (lseek(fd_arr[black], 0, SEEK_CUR) != expected_fp)
		return 0;

	for (int i = 0; i < 20; i++)
		if (read(fd_arr[black], buf, 1) != 1)
			return 0;

	expected_fp += 20;

	if (lseek(fd_arr[black], 0, SEEK_CUR) != expected_fp)
		return 0;

	expected_fp += 20;

	if (ioctl(fd_arr[black], 20, SEEK_CUR) != expected_fp)
		return 0;

	return 1;
}

// test - draw a cool pic
int test_23(void)
{
	if (ioctl(grey, 0) == -1)
		return 0;

	// yes, this is ridiculous, but it will test if the module indeed draw the intended drawing
	int cool_drawing[20][20] = {
		{7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7},
		{7, 7, 7, 7, 7, 7, 7, 0, 0, 0, 0, 7, 7, 7, 7, 7, 7, 7, 7, 7},
		{7, 7, 7, 7, 7, 7, 0, 0, 0, 0, 0, 0, 7, 7, 7, 7, 7, 7, 7, 7},
		{7, 7, 7, 7, 7, 7, 0, 0, 0, 0, 0, 0, 7, 7, 7, 7, 7, 7, 7, 7},
		{7, 7, 7, 7, 7, 7, 0, 9, 9, 9, 0, 0, 7, 7, 7, 7, 7, 7, 7, 7},
		{7, 7, 7, 7, 7, 7, 9, 3, 3, 3, 9, 0, 7, 7, 7, 7, 7, 7, 7, 7},
		{7, 7, 7, 7, 7, 7, 3, 3, 3, 3, 3, 0, 7, 7, 7, 7, 7, 7, 7, 7},
		{7, 7, 7, 7, 7, 7, 0, 7, 7, 7, 7, 0, 0, 0, 7, 7, 7, 7, 7, 7},
		{7, 7, 7, 7, 7, 0, 0, 7, 7, 7, 7, 0, 0, 0, 7, 7, 7, 7, 7, 7},
		{7, 7, 7, 7, 0, 0, 7, 7, 7, 7, 7, 7, 0, 0, 0, 7, 7, 7, 7, 7},
		{7, 7, 7, 0, 0, 0, 7, 7, 7, 7, 7, 7, 0, 0, 0, 7, 7, 7, 7, 7},
		{7, 7, 7, 0, 0, 0, 7, 7, 7, 7, 7, 7, 0, 0, 0, 7, 7, 7, 7, 7},
		{7, 7, 7, 0, 0, 0, 7, 7, 7, 7, 7, 7, 0, 0, 0, 7, 7, 7, 7, 7},
		{7, 7, 3, 3, 0, 0, 0, 7, 7, 7, 7, 7, 0, 0, 3, 3, 3, 7, 7, 7},
		{7, 3, 3, 3, 3, 3, 0, 0, 7, 7, 0, 0, 0, 3, 3, 3, 3, 3, 7, 7},
		{7, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3, 3, 7, 7},
		{7, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0, 0, 3, 3, 3, 3, 3, 3, 7, 7},
		{7, 7, 3, 3, 3, 3, 3, 7, 0, 0, 0, 0, 3, 3, 3, 3, 3, 7, 7, 7},
		{7, 7, 7, 3, 3, 3, 7, 7, 7, 7, 7, 7, 7, 3, 3, 7, 7, 7, 7, 7},
		{7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7}
	};

	for (int i = 0; i < 20; i++) {
		for (int j = 0; j < 20; j++) {
			if (cool_drawing[i][j] == 8)
				continue;

			int offset = i * 20 + j;

			char color = cool_drawing[i][j];
			int fd;

			if (color == 0)
				fd = black;
			else if (color == 3)
				fd = yellow;
			else if (color == 9)
				fd = orange;
			else
				fd = megenta;

			if (lseek(fd, offset, SEEK_SET) != offset)
				return 0;

			int buf[1] = {0};

			if (write(fd, buf, 1) != 1)
				return 0;
		}
	}

	// test if the module actually has our drawing
	int result[20][20] = {0};

	if (lseek(black, 0, SEEK_SET) != 0)
		return 0;

	if (read(black, result, 20 * 20) != 20 * 20)
		return 0;

	for (int i = 0; i < 20; i++) {
		for (int j = 0; j < 20; j++) {
			if (result[i][j] != cool_drawing[i][j])
				return 0;
		}
	}

	if (DRAW_FINAL_TEST == 1) {
		for (int i = 0; i < 20; i++, puts(""))
			for (int j = 0; j < 20; j++)
				printf("\033[4%dm	\033[0m", result[i][j]);
	}

	return 1;
}

// test - close all of the device files
int test_24(void)
{
	int res = 1;

	for (int i = 0; i < N_COLORS; i++) {
		if (close(fd_arr[i]) == -1)
			res = 0;
	}

	return res;
}

int run_test_i(int test_num, test_func foo)
{
	int ret = foo();

	return ret;
}

#define RUN_TEST_i(i, s) do { \
	if (run_test_i(i, test_ ## i)) { \
		puts(" ok - " s); \
	} else { \
		puts(" not ok - " s); \
	} \
} while (0)

int main(void)
{
	puts("1..25");

	// 1 basic test for opening the file

	RUN_TEST_i(0, "Device files opened O_RDWR");

	// 5 basic tests for writing
	RUN_TEST_i(1, "Write to a single pixal");
	RUN_TEST_i(2, "Write with a NULL buffer and size 0");
	RUN_TEST_i(3, "Write with a NULL buffer and size > 0");
	RUN_TEST_i(4, "Write to the entrie device file");
	RUN_TEST_i(5, "Write to the EOF");

	// 5 basic tests for reading
	RUN_TEST_i(6, "Read the first pixel");
	RUN_TEST_i(7, "Read the first pixel after it was colored again");
	RUN_TEST_i(8, "Read the entire file");
	RUN_TEST_i(9, "Read the EOF");
	RUN_TEST_i(10, "Read with NULL buffer");

	// 5 basic tests for ioctl
	RUN_TEST_i(11, "ioctl - color the entire canvas white");
	RUN_TEST_i(12, "ioctl - color the entire canvas white, at EOF");
	RUN_TEST_i(13, "ioctl - unsupported command");
	RUN_TEST_i(14, "ioctl - unsupported arguments");
	RUN_TEST_i(15, "ioctl - unsupported commands and	arguments");

	// 3 tests for lseek
	RUN_TEST_i(16, "lseek - All device files point to EOF");
	RUN_TEST_i(17, "lseek - All device files point to 0");
	RUN_TEST_i(18, "lseek - Move file position via lseek");

	// tests for different system calls working together
	RUN_TEST_i(19, "Read Write together to create a chess board image");
	RUN_TEST_i(20, "lseek Write together to create blue square with a red lines image");
	RUN_TEST_i(21, "ioctl Write together to create a chess board image");
	RUN_TEST_i(22, "lseek Read together to parse through the canvas");
	RUN_TEST_i(23, "Use eveything - pss, change DRAW_FINAL_TEST to 1");
	RUN_TEST_i(24, "Close all of the device files");

	return 0;
}
