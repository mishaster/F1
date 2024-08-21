#include <stdio.h>
#include <fcntl.h>
#include <stdbool.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/ioctl.h>

#define SPONGEBOB 32
// Each row has 32 cells + 2 walls + new line
// Each col has 16 cells + 2 walls
#define GRID_STRING_SIZE (35 * 18)

#define EMPTY_GRID			"----------------------------------\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"----------------------------------\n" \

#define FULL_GRID			"----------------------------------\n" \
					"|********************************|\n" \
					"|********************************|\n" \
					"|********************************|\n" \
					"|********************************|\n" \
					"|********************************|\n" \
					"|********************************|\n" \
					"|********************************|\n" \
					"|********************************|\n" \
					"|********************************|\n" \
					"|********************************|\n" \
					"|********************************|\n" \
					"|********************************|\n" \
					"|********************************|\n" \
					"|********************************|\n" \
					"|********************************|\n" \
					"|********************************|\n" \
					"----------------------------------\n" \

#define GRID1				"----------------------------------\n" \
					"|*                               |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"----------------------------------\n" \

#define GRID2				"----------------------------------\n" \
					"|**                              |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"----------------------------------\n" \

#define GRID3				"----------------------------------\n" \
					"| *                              |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"----------------------------------\n" \

#define GRID1_LIVE_CELL_OTHER_CHAR \
					"----------------------------------\n" \
					"|$                               |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"----------------------------------\n" \

#define GRID_LIVE_CORNER_SQUARE \
					"----------------------------------\n" \
					"|**                              |\n" \
					"|**                              |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"----------------------------------\n" \

#define GRID_DEAD_STAYS_DEAD \
					"----------------------------------\n" \
					"|*                               |\n" \
					"|**                              |\n" \
					"| *                              |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"----------------------------------\n" \

#define GRID_OVERPOPULATION_DIES \
					"----------------------------------\n" \
					"|* *                             |\n" \
					"|* *                             |\n" \
					"| *                              |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"----------------------------------\n" \

#define GRID_STABLE_SMALL \
					"----------------------------------\n" \
					"| *                              |\n" \
					"|* *                             |\n" \
					"| *                              |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"----------------------------------\n" \

#define GRID_STABLE_LARGE \
					"----------------------------------\n" \
					"| **                             |\n" \
					"|*  *                            |\n" \
					"| ** *                           |\n" \
					"|    *                           |\n" \
					"|    **                          |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"----------------------------------\n" \

#define GRID_BLINKER_1 \
					"----------------------------------\n" \
					"|                                |\n" \
					"|***                             |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"----------------------------------\n" \

#define GRID_BLINKER_2 \
					"----------------------------------\n" \
					"| *                              |\n" \
					"| *                              |\n" \
					"| *                              |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"----------------------------------\n" \

#define GRID_0_0_NEIGHBORS_0 \
					"----------------------------------\n" \
					"|**                             *|\n" \
					"|**                             *|\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|**                             *|\n" \
					"----------------------------------\n" \

#define GRID_0_0_NEIGHBORS_1 \
					"----------------------------------\n" \
					"|                                |\n" \
					"| *                             *|\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"| *                             *|\n" \
					"----------------------------------\n" \

#define GRID_STABLE_SMALL_MODULO \
					"----------------------------------\n" \
					"|*                               |\n" \
					"| *                             *|\n" \
					"|*                               |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"|                                |\n" \
					"----------------------------------\n" \

#define FILE_PATH "/dev/game_of_life"
//#define FILE_PATH "/dev/null"
#define TESTS_NUM 28

void print_tests_num(int num)
{
	printf("1..%d\n", num);
}

void print_result(bool success, int test_num, char *message)
{
	if (!success)
		printf("not ");

	printf("ok %d - %s\n", test_num, message);
}

bool test_file_exists(void)
{
	int fd = open(FILE_PATH, O_RDWR);

	bool result = fd >= 0;

	if (result)
		close(fd);

	return result;
}

bool test_open_twice_fails(void)
{
	int fd = open(FILE_PATH, O_RDWR);
	bool result = fd >= 0;

	if (!result)
		return result;

	int fd1 = open(FILE_PATH, O_RDWR);

	result = result && (errno == EBUSY);

	if (fd1 >= 0)
		close(fd1);

	close(fd);
	return result;
}

bool test_open_returns_empty_grid(void)
{
	int fd = open(FILE_PATH, O_RDWR);

	if (fd < 0)
		return false;

	char buf[GRID_STRING_SIZE];

	read(fd, buf, 0);

	bool result = strcmp(buf, EMPTY_GRID) == 0;

	close(fd);
	return result;
}

bool test_one_write(void)
{
	int fd = open(FILE_PATH, O_RDWR);

	if (fd < 0)
		return false;

	char buf[GRID_STRING_SIZE];

	write(fd, NULL, 0);
	read(fd, buf, 0);
	bool result = strcmp(buf, GRID1) == 0;

	close(fd);
	return result;
}

bool test_write_with_count(void)
{
	int fd = open(FILE_PATH, O_RDWR);

	if (fd < 0)
		return false;

	char buf[GRID_STRING_SIZE];

	write(fd, "*******", 3);
	read(fd, buf, 0);
	bool result = strcmp(buf, GRID1) == 0;

	close(fd);
	return result;
}

bool test_two_writes(void)
{
	int fd = open(FILE_PATH, O_RDWR);

	if (fd < 0)
		return false;

	char buf[GRID_STRING_SIZE];

	write(fd, NULL, 0);
	write(fd, NULL, 0);
	read(fd, buf, 0);
	bool result = strcmp(buf, GRID2) == 0;

	close(fd);
	return result;
}

bool test_lseek_without_write(void)
{
	int fd = open(FILE_PATH, O_RDWR);

	if (fd < 0)
		return false;

	char buf[GRID_STRING_SIZE];

	lseek(fd, 0, SEEK_SET);
	read(fd, buf, 0);
	bool result = strcmp(buf, EMPTY_GRID) == 0;

	close(fd);
	return result;
}

bool test_lseek_bounds(void)
{
	int fd = open(FILE_PATH, O_RDWR);

	if (fd < 0)
		return false;

	char buf[GRID_STRING_SIZE];

	lseek(fd, 4096, SEEK_SET);
	read(fd, buf, 0);
	bool result = strcmp(buf, EMPTY_GRID) == 0;

	close(fd);
	return result;
}

bool test_lseek_and_one_write_without_offset(void)
{
	int fd = open(FILE_PATH, O_RDWR);

	if (fd < 0)
		return false;

	char buf[GRID_STRING_SIZE];

	lseek(fd, 0, SEEK_SET);
	write(fd, NULL, 0);
	read(fd, buf, 0);
	bool result = strcmp(buf, GRID1) == 0;

	close(fd);
	return result;
}

bool test_lseek_and_two_writes_without_offset(void)
{
	int fd = open(FILE_PATH, O_RDWR);

	if (fd < 0)
		return false;

	char buf[GRID_STRING_SIZE];

	lseek(fd, 0, SEEK_SET);
	write(fd, NULL, 0);
	write(fd, NULL, 0);
	read(fd, buf, 0);
	bool result = strcmp(buf, GRID2) == 0;

	close(fd);
	return result;
}

bool test_lseek_and_toggle_undo(void)
{
	int fd = open(FILE_PATH, O_RDWR);

	if (fd < 0)
		return false;

	char buf[GRID_STRING_SIZE];

	write(fd, NULL, 0);
	lseek(fd, -1, SEEK_CUR);
	write(fd, NULL, 0);
	read(fd, buf, 0);
	bool result = strcmp(buf, EMPTY_GRID) == 0;

	close(fd);
	return result;
}

bool test_lseek_and_one_write_with_offset(void)
{
	int fd = open(FILE_PATH, O_RDWR);

	if (fd < 0)
		return false;

	char buf[GRID_STRING_SIZE];

	lseek(fd, 1, SEEK_SET);
	write(fd, NULL, 0);
	read(fd, buf, 0);
	bool result = strcmp(buf, GRID3) == 0;

	close(fd);
	return result;
}

bool test_ioctl_with_invalid_op(void)
{
	int fd = open(FILE_PATH, O_RDWR);

	if (fd < 0)
		return false;

	ioctl(fd, 2);

	bool result = errno == EPERM;

	close(fd);
	return result;
}

bool test_changing_live_cell_character(void)
{
	int fd = open(FILE_PATH, O_RDWR);

	if (fd < 0)
		return false;

	char buf[GRID_STRING_SIZE];

	lseek(fd, 0, SEEK_SET);
	write(fd, NULL, 0);

	ioctl(fd, 0, '$');

	read(fd, buf, 0);

	bool result = strcmp(buf, GRID1_LIVE_CELL_OTHER_CHAR) == 0;

	close(fd);
	return result;
}

bool test_changing_live_cell_character_to_bad_value(void)
{
	int fd = open(FILE_PATH, O_RDWR);

	if (fd < 0)
		return false;

	ioctl(fd, 0, 0x20);

	bool result = errno == EINVAL;

	close(fd);
	return result;
}

bool test_cell_without_neighbors_dies(void)
{
	int fd = open(FILE_PATH, O_RDWR);

	if (fd < 0)
		return false;

	char buf[GRID_STRING_SIZE];

	lseek(fd, 0, SEEK_SET);
	write(fd, NULL, 0);

	ioctl(fd, 1);

	read(fd, buf, 0);
	bool result = strcmp(buf, EMPTY_GRID) == 0;

	close(fd);
	return result;
}

bool test_cell_with_3_neighbors_becomes_live(void)
{
	int fd = open(FILE_PATH, O_RDWR);

	if (fd < 0)
		return false;

	char buf[GRID_STRING_SIZE];

	lseek(fd, 1, SEEK_SET);
	write(fd, NULL, 0);

	lseek(fd, 32, SEEK_SET);
	write(fd, NULL, 0);

	lseek(fd, 33, SEEK_SET);
	write(fd, NULL, 0);

	ioctl(fd, 1);

	read(fd, buf, 0);
	bool result = strcmp(buf, GRID_LIVE_CORNER_SQUARE) == 0;

	close(fd);
	return result;
}

bool test_cell_with_4_neighbors_stays_dead(void)
{
	int fd = open(FILE_PATH, O_RDWR);

	if (fd < 0)
		return false;

	char buf[GRID_STRING_SIZE];

	lseek(fd, 0, SEEK_SET);
	write(fd, NULL, 0);

	lseek(fd, 32, SEEK_SET);
	write(fd, NULL, 0);

	lseek(fd, 33, SEEK_SET);
	write(fd, NULL, 0);

	lseek(fd, 34, SEEK_SET);
	write(fd, NULL, 0);

	ioctl(fd, 1);

	read(fd, buf, 0);
	bool result = strcmp(buf, GRID_DEAD_STAYS_DEAD) == 0;

	close(fd);
	return result;
}

bool test_cell_with_4_neighbors_dies(void)
{
	int fd = open(FILE_PATH, O_RDWR);

	if (fd < 0)
		return false;

	char buf[GRID_STRING_SIZE];

	lseek(fd, 0, SEEK_SET);
	write(fd, NULL, 0);

	lseek(fd, 1, SEEK_SET);
	write(fd, NULL, 0);

	lseek(fd, 32, SEEK_SET);
	write(fd, NULL, 0);

	lseek(fd, 33, SEEK_SET);
	write(fd, NULL, 0);

	lseek(fd, 34, SEEK_SET);
	write(fd, NULL, 0);

	ioctl(fd, 1);

	read(fd, buf, 0);
	bool result = strcmp(buf, GRID_OVERPOPULATION_DIES) == 0;

	close(fd);
	return result;
}

bool test_cell_with_2_neighbors_remains_alive(void)
{
	int fd = open(FILE_PATH, O_RDWR);

	if (fd < 0)
		return false;

	char buf[GRID_STRING_SIZE];

	lseek(fd, 1, SEEK_SET);
	write(fd, NULL, 0);

	lseek(fd, 32, SEEK_SET);
	write(fd, NULL, 0);

	lseek(fd, 34, SEEK_SET);
	write(fd, NULL, 0);

	lseek(fd, 65, SEEK_SET);
	write(fd, NULL, 0);

	ioctl(fd, 1);

	read(fd, buf, 0);
	bool result = strcmp(buf, GRID_STABLE_SMALL) == 0;

	close(fd);
	return result;
}

bool test_large_stable_structure(void)
{
	int fd = open(FILE_PATH, O_RDWR);

	if (fd < 0)
		return false;

	char buf[GRID_STRING_SIZE];

	lseek(fd, 1, SEEK_SET);
	write(fd, NULL, 0);

	lseek(fd, 2, SEEK_SET);
	write(fd, NULL, 0);

	lseek(fd, 32, SEEK_SET);
	write(fd, NULL, 0);

	lseek(fd, 35, SEEK_SET);
	write(fd, NULL, 0);

	lseek(fd, 65, SEEK_SET);
	write(fd, NULL, 0);

	lseek(fd, 66, SEEK_SET);
	write(fd, NULL, 0);

	lseek(fd, 68, SEEK_SET);
	write(fd, NULL, 0);

	lseek(fd, 100, SEEK_SET);
	write(fd, NULL, 0);

	lseek(fd, 132, SEEK_SET);
	write(fd, NULL, 0);

	lseek(fd, 133, SEEK_SET);
	write(fd, NULL, 0);

	ioctl(fd, 1);

	read(fd, buf, 0);
	bool result = strcmp(buf, GRID_STABLE_LARGE) == 0;

	ioctl(fd, 1);

	read(fd, buf, 0);
	result = result && (strcmp(buf, GRID_STABLE_LARGE) == 0);

	ioctl(fd, 1);

	read(fd, buf, 0);
	result = result && (strcmp(buf, GRID_STABLE_LARGE) == 0);

	close(fd);
	return result;
}

bool test_oscillating_structure(void)
{
	int fd = open(FILE_PATH, O_RDWR);

	if (fd < 0)
		return false;

	char buf[GRID_STRING_SIZE];

	lseek(fd, 32, SEEK_SET);
	write(fd, NULL, 0);

	lseek(fd, 33, SEEK_SET);
	write(fd, NULL, 0);

	lseek(fd, 34, SEEK_SET);
	write(fd, NULL, 0);

	read(fd, buf, 0);
	bool result = strcmp(buf, GRID_BLINKER_1) == 0;

	ioctl(fd, 1);

	read(fd, buf, 0);
	result = result && (strcmp(buf, GRID_BLINKER_2) == 0);

	ioctl(fd, 1);

	read(fd, buf, 0);
	result = result && (strcmp(buf, GRID_BLINKER_1) == 0);

	close(fd);
	return result;
}

bool test_modulo_0_0_neighbors(void)
{
	int fd = open(FILE_PATH, O_RDWR);

	if (fd < 0)
		return false;

	char buf[GRID_STRING_SIZE];

	lseek(fd, 0, SEEK_SET);
	write(fd, NULL, 0);

	lseek(fd, 1, SEEK_SET);
	write(fd, NULL, 0);

	lseek(fd, 31, SEEK_SET);
	write(fd, NULL, 0);

	lseek(fd, 32, SEEK_SET);
	write(fd, NULL, 0);

	lseek(fd, 33, SEEK_SET);
	write(fd, NULL, 0);

	lseek(fd, 63, SEEK_SET);
	write(fd, NULL, 0);

	lseek(fd, 480, SEEK_SET);
	write(fd, NULL, 0);

	lseek(fd, 481, SEEK_SET);
	write(fd, NULL, 0);

	lseek(fd, 511, SEEK_SET);
	write(fd, NULL, 0);

	read(fd, buf, 0);
	bool result = strcmp(buf, GRID_0_0_NEIGHBORS_0) == 0;

	ioctl(fd, 1);

	read(fd, buf, 0);
	result = result && (strcmp(buf, GRID_BLINKER_2) == 0);

	ioctl(fd, 1);

	read(fd, buf, 0);
	result = result && (strcmp(buf, GRID_0_0_NEIGHBORS_1) == 0);

	close(fd);
	return result;
}

bool test_cell_with_2_neighbors_remains_alive_with_modulo(void)
{
	int fd = open(FILE_PATH, O_RDWR);

	if (fd < 0)
		return false;

	char buf[GRID_STRING_SIZE];

	lseek(fd, 0, SEEK_SET);
	write(fd, NULL, 0);

	lseek(fd, 33, SEEK_SET);
	write(fd, NULL, 0);

	lseek(fd, 63, SEEK_SET);
	write(fd, NULL, 0);

	lseek(fd, 64, SEEK_SET);
	write(fd, NULL, 0);

	ioctl(fd, 1);

	read(fd, buf, 0);
	bool result = strcmp(buf, GRID_STABLE_SMALL_MODULO) == 0;

	close(fd);
	return result;
}

bool test_lseek_out_of_range(void)
{
	int fd = open(FILE_PATH, O_RDWR);

	if (fd < 0)
		return false;

	char buf[GRID_STRING_SIZE];

	lseek(fd, 4096, SEEK_SET);
	write(fd, NULL, 0);

	bool result = (strcmp(buf, GRID1) == 0) && (errno == EPERM);

	close(fd);
	return result;
}

bool test_lseek_invalid_whence(void)
{
	int fd = open(FILE_PATH, O_RDWR);

	if (fd < 0)
		return false;

	char buf[GRID_STRING_SIZE];

	lseek(fd, 0, SPONGEBOB);
	write(fd, NULL, 0);

	bool result = (strcmp(buf, GRID1) == 0) && (errno == EINVAL);

	close(fd);
	return result;
}

bool test_tick_on_empty_grid(void)
{
	int fd = open(FILE_PATH, O_RDWR);

	if (fd < 0)
		return false;

	char buf[GRID_STRING_SIZE];
	bool result = true;

	for (int i = 0; i < 10; i++)
		result = result && (strcmp(buf, EMPTY_GRID) == 0);

	close(fd);
	return result;
}

bool test_sequential_write_and_read(void)
{
	int fd = open(FILE_PATH, O_RDWR);

	if (fd < 0)
		return false;

	char buf[GRID_STRING_SIZE];
	bool result = true;

	for (int i = 0; i < 1024; i++) {
		write(fd, NULL, 0);
		read(fd, buf, 0);
		result = result && (strcmp(buf, GRID1)) == 0;
	}

	close(fd);
	return result;
}

bool test_sequential_write(void)
{
	int fd = open(FILE_PATH, O_RDWR);

	if (fd < 0)
		return false;

	char buf[GRID_STRING_SIZE];
	bool result = true;

	for (int i = 0; i < 512; i++) {
		write(fd, NULL, 0);
		result = result && (strcmp(buf, GRID1)) == 0;
	}

	write(fd, NULL, 0);
	result = result && (errno == EPERM);

	read(fd, buf, 0);
	result = result && (strcmp(buf, FULL_GRID)) == 0;

	close(fd);
	return result;
}

int main(void)
{
	print_tests_num(TESTS_NUM);

	int test_num = 1;

	print_result(test_file_exists(), test_num++, "File exists");
	print_result(test_open_twice_fails(), test_num++, "Can't open file twice");
	print_result(test_open_returns_empty_grid(), test_num++, "Open returns empty grid");
	print_result(test_one_write(), test_num++, "Toggle cell (0,0)");
	print_result(test_write_with_count(), test_num++,
		     "Toggle cell (0,0) with  irrelevant arguments");
	print_result(test_two_writes(), test_num++, "Toggle cells (0,0) and (0,1)");
	print_result(test_lseek_without_write(), test_num++, "lseek on empty grid");
	print_result(test_lseek_and_one_write_without_offset(), test_num++,
		     "Toggle cell (0,0) after lseek");
	print_result(test_lseek_and_two_writes_without_offset(), test_num++,
		     "Toggle cells (0,0) and (0,1) after lseek");
	print_result(test_lseek_and_toggle_undo(), test_num++,
		     "Toggle cell (0,0) twice using lseek");
	print_result(test_lseek_and_one_write_with_offset(), test_num++,
		     "Toggle (0,1) after lseek");
	print_result(test_ioctl_with_invalid_op(), test_num++,
		     "Ioctl with invalid op");
	print_result(test_changing_live_cell_character(), test_num++,
		     "Change live cells character");
	print_result(test_changing_live_cell_character_to_bad_value(), test_num++,
		     "Change live cells character to a bad value");
	print_result(test_cell_without_neighbors_dies(), test_num++,
		     "Cell without neighbor dies");
	print_result(test_cell_with_3_neighbors_becomes_live(), test_num++,
		     "Dead cell with 3 neighbors becomes alive");
	print_result(test_cell_with_4_neighbors_stays_dead(), test_num++,
		     "Dead cell with 4 neighbors stays dead");
	print_result(test_cell_with_4_neighbors_dies(), test_num++,
		     "Live cell with 4 neighbors dies");
	print_result(test_cell_with_2_neighbors_remains_alive(), test_num++,
		     "Live cell with 2 neighbors remains alive");
	print_result(test_large_stable_structure(), test_num++,
		     "Large stable structure");
	print_result(test_oscillating_structure(), test_num++,
		     "Oscillating structure");
	print_result(test_modulo_0_0_neighbors(), test_num++,
		     "Toggle (0,0) and all its neighbors, then tick once");
	print_result(test_cell_with_2_neighbors_remains_alive_with_modulo(), test_num++,
		     "Live cell with 2 neighbors remains alive, with modulo");
	print_result(test_lseek_out_of_range(), test_num++,
		     "When offset is out of range lseek does not change the current position and returns an error");
	print_result(test_lseek_invalid_whence(), test_num++,
		     "Invalid whence");
	print_result(test_tick_on_empty_grid(), test_num++,
		     "Empty grid remains empty when ticked");
	print_result(test_sequential_write_and_read(), test_num++,
		     "Read set offset to cell (0,0)");
	print_result(test_sequential_write(), test_num++,
		     "Write is only permitted to valid cells");

	return 0;
}
