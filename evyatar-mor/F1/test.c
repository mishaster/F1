#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>

#define DEVICE_PATH "/dev/wordle"
#define TEST_OK 0
#define TEST_FAIL 1



// A macro for running a test and printing its result
/* #define RUN_TEST(test, test_name) do { \
	int result = (test)(); \
	printf("%s %s\n", test_name, result == TEST_OK ? "PASSED" : "FAILED"); \
} while (0) */

// IOCTL commands
#define WORDLE_IOC_RESET			_IO('W', 0)
#define WORDLE_IOC_GET_LEN		  _IOR('W', 1, int)
#define WORDLE_IOC_SET_LEN		  _IOW('W', 2, int)
#define WORDLE_IOC_GET_GUESSES	  _IOR('W', 3, int)
#define WORDLE_IOC_SET_GUESSES	  _IOW('W', 4, int)
#define WORDLE_IOC_SUBMIT_GUESS	 _IO('W', 5)
#define WORDLE_IOC_CLEAR_FEEDBACK   _IO('W', 6)
#define WORDLE_IOC_GET_STATS		_IOR('W', 7, int)


static int curr_test = 1;
static int ntests = 25;

static inline void tap_print_header(void)
{
	printf("1..%d\n", ntests);
}

static inline int tap_ok(int cond, const char *msg)
{
	if (cond) {
		printf("not ok %d - %s\n", curr_test, msg);
		return 1;
	}
	return 0;
}

static inline int tap_test_passed(const char *msg)
{
	printf("ok %d - %s passed\n", curr_test, msg);
	return 0;
}

static void run_test(int (*test_method)(void))
{
	if (test_method()) {
		exit(EXIT_FAILURE);
	}
	++curr_test;
}

static int open_device(void)
{
	int fd = open(DEVICE_PATH, O_RDWR);
	return fd;
}

static int test_open(void)
{
	int fd = open_device();
	if (tap_ok(fd < 0, "Failed to open device\n")) {
		return TEST_FAIL;
	}
	close(fd);
	return TEST_OK;
}

static int test_open_twice(void)
{
	int fd1 = open(DEVICE_PATH, O_RDWR);
	if (tap_ok(fd1 < 0, "Failed to open device\n")) {

		return TEST_FAIL;
	}

	int fd2 = open(DEVICE_PATH, O_RDWR);
	if (tap_ok(fd2 >= 0, "Test failed: Device opened twice successfully.\n")) {

		close(fd1);
		close(fd2);
		return TEST_FAIL;
	}

	// Check the errno value if available
	if (tap_ok(errno != EBUSY, "Test failed: Unexpected errno %d\n")) {

		close(fd1);
		return TEST_FAIL;
	}
	close(fd1);
	return TEST_OK;
}

static int test_write_invalid_chars(void)
{
	int fd = open_device();
	if (tap_ok(fd < 0, "Failed to open device\n")) {
		return TEST_FAIL;
	}
	const char *data = "1!@#$"; // Invalid characters, length 5
	if (tap_ok(write(fd, data, strlen(data)) != -EINVAL, "Write did not return -EINVAL for invalid characters")) {

		close(fd);
		return TEST_FAIL;
	}
	close(fd);
	return TEST_OK;
}

static int test_write_overflow(void)
{
	int fd = open_device();
	if (tap_ok(fd < 0, "Failed to open device\n")) {
		return TEST_FAIL;
	}
	const char *data = "overflowsize"; // word length 5
	if (tap_ok(write(fd, data, strlen(data)) != -EMSGSIZE, "Write did not return -EMSGSIZE for overflow")) {

		close(fd);
		return TEST_FAIL;
	}
	close(fd);
	return TEST_OK;
}

static int test_write_valid_input(void)
{
	int fd = open_device();
	if (tap_ok(fd < 0, "Failed to open device\n")) {
		return TEST_FAIL;
	}
	const char *data = "guess";
	if (tap_ok(write(fd, data, strlen(data)) < 0, "Failed to write valid input")) {

		close(fd);
		return TEST_FAIL;
	}
	close(fd);
	return TEST_OK;
}

static int test_write_partial_data(void)
{
	int fd = open_device();
	if (tap_ok(fd < 0, "Failed to open device\n")) {
		return TEST_FAIL;
	}
	const char *data = "part";
	if (tap_ok(write(fd, data, 4) < 0, "Failed to write partial data")) {

		close(fd);
		return TEST_FAIL;
	}
	// Check state after partial write
	char buf[256];
	if (tap_ok(read(fd, buf, sizeof(buf)) < 0 || strncmp(buf, data, strlen(data)) != 0, "Failed to read after partial write")) {

		close(fd);
		return TEST_FAIL;
	}
	close(fd);
	return TEST_OK;
}

static int test_write_after_submit(void)
{
	int fd = open(DEVICE_PATH, O_RDWR);
	if (tap_ok(fd < 0, "Failed to open device\n")) {

		return TEST_FAIL;
	}
	char *data = "guess";

	// Write a guess to the device
	if (tap_ok(write(fd, data, strlen(data)) < 0, "Write failed")) {

		close(fd);
		return TEST_FAIL;
	}

	// Submit the guess using ioctl
	if (tap_ok(ioctl(fd, WORDLE_IOC_SUBMIT_GUESS) < 0, "IOCTL_SUBMIT_GUESS failed")) {

		close(fd);
		return TEST_FAIL;
	}

	// Attempt to write again after submitting the guess
	if (tap_ok(write(fd, "new", 3) >= 0, "Test failed: Write succeeded after submit.\n")) {

		close(fd);
		return TEST_FAIL;
	}

	if (tap_ok(errno != EAGAIN, "Test failed: Unexpected errno %d\n")) {

		close(fd);
		return TEST_FAIL;
	}
	close(fd);
	return TEST_OK;
}

static int test_read_feedback(void)
{
	int fd = open_device();
	if (tap_ok(fd < 0, "Failed to open device\n")) {
		return TEST_FAIL;
	}
	const char *guess = "guess";
	write(fd, guess, strlen(guess));
	ioctl(fd, WORDLE_IOC_SUBMIT_GUESS);
	char buf[256];
	if (tap_ok(read(fd, buf, sizeof(buf)) < 0, "Failed to read feedback")) {

		close(fd);
		return TEST_FAIL;
	}
	close(fd);
	return TEST_OK;
}

static int test_submit_undefined_word(void)
{
	int fd = open(DEVICE_PATH, O_RDWR);
	if (tap_ok(fd < 0, "Failed to open device\n")) {

		return TEST_FAIL;
	}
	char *invalid = "aaaaa";

	// Write an invalid guess to the device
	if (tap_ok(write(fd, invalid, strlen(invalid)) < 0, "Write failed")) {

		close(fd);
		return TEST_FAIL;
	}

	// Attempt to submit the guess using ioctl
	if (tap_ok(ioctl(fd, WORDLE_IOC_SUBMIT_GUESS) >= 0, "Test failed: Submit succeeded for undefined word.\n")) {

		close(fd);
		return TEST_FAIL;
	}

	if (tap_ok(errno != EILSEQ, "Test failed: Unexpected errno %d\n")) {

		close(fd);
		return TEST_FAIL;
	}

	close(fd);
	return TEST_OK;
}

static int test_read_after_write(void)
{
	int fd = open_device();
	if (tap_ok(fd < 0, "Failed to open device\n")) {
		return TEST_FAIL;
	}
	const char *data = "guess";
	write(fd, data, strlen(data));
	char buf[256];
	if (tap_ok(read(fd, buf, sizeof(buf)) < 0 || strncmp(buf, data, strlen(data)) != 0, "Failed to read after write")) {

		close(fd);
		return TEST_FAIL;
	}
	close(fd);
	return TEST_OK;
}

static int test_read_small_buffer_size(void)
{
	int fd = open_device();
	if (tap_ok(fd < 0, "Failed to open device\n")) {
		return TEST_FAIL;
	}
	const char *guess = "guess";
	write(fd, guess, strlen(guess));
	char buf[3]; // Smaller than expected
	if (tap_ok(read(fd, buf, sizeof(buf)) < 0, "Failed to read with insufficient buffer size")) {

		close(fd);
		return TEST_FAIL;
	}
	close(fd);
	return TEST_OK;
}

static int test_read_twice(void)
{
	int fd = open_device();
	if (tap_ok(fd < 0, "Failed to open device\n")) {
		return TEST_FAIL;
	}
	const char *guess = "guess";
	write(fd, guess, strlen(guess));
	char buf[4]; // Smaller than expected
	if (tap_ok(read(fd, buf, sizeof(buf)) < 0 || strncmp(buf, guess, strlen(buf)) != 0, "Failed to read with insufficient buffer size")) {

		close(fd);
		return TEST_FAIL;
	}
	memset(buf, 0, 4);
	if (tap_ok(read(fd, buf, sizeof(buf)) < 0 || strncmp(buf, "ss", 2) != 0, "Failed to read Twice")) {

		close(fd);
		return TEST_FAIL;
	}
	close(fd);
	return TEST_OK;
}

static int test_read_after_clear_feedback(void)
{
	int fd = open_device();
	if (tap_ok(fd < 0, "Failed to open device\n")) {
		return TEST_FAIL;
	}
	const char *guess = "guess";
	write(fd, guess, strlen(guess));
	ioctl(fd, WORDLE_IOC_SUBMIT_GUESS);
	ioctl(fd, WORDLE_IOC_CLEAR_FEEDBACK);
	char buf[256];
	if (tap_ok(read(fd, buf, sizeof(buf)) < 0, "Failed to read after clearing feedback")) {

		close(fd);
		return TEST_FAIL;
	}
	close(fd);
	return TEST_OK;
}

static int test_read_feedback_color_coding(void)
{
	int fd = open_device();
	if (tap_ok(fd < 0, "Failed to open device\n")) {
		return TEST_FAIL;
	}
	const char *guess = "guess";
	write(fd, guess, strlen(guess));
	ioctl(fd, WORDLE_IOC_SUBMIT_GUESS);
	char buf[256];
	if (tap_ok(read(fd, buf, sizeof(buf)) < 0, "Failed to read feedback with color coding")) {

		close(fd);
		return TEST_FAIL;
	}
	// Simple check for color codes
	if (tap_ok(strstr(buf, "\033[0;32m") == NULL &&
		strstr(buf, "\033[0;33m") == NULL &&
		strstr(buf, "\033[0;31m") == NULL, "Feedback does not contain expected color codes\n")) {

		close(fd);
		return TEST_FAIL;
	}
	close(fd);
	return TEST_OK;
}

static int test_seek_valid(void)
{
	int fd = open_device();

	if (tap_ok(fd < 0, "Failed to open device\n")) {
		return TEST_FAIL;
	}
	const char *data = "guess";

	write(fd, data, strlen(data));
	if (tap_ok(lseek(fd, 2, SEEK_SET) < 0, "Failed to seek to a valid position")) {

		close(fd);
		return TEST_FAIL;
	}
	write(fd, "a", 1);
	char buf[256];

	if (tap_ok(read(fd, buf, sizeof(buf)) < 0 || strncmp(buf, "guass", strlen(data)) != 0, "Failed to seek to a valid position")) {

		close(fd);
		return TEST_FAIL;
	}
	close(fd);
	return TEST_OK;
}

static int test_seek_out_of_bounds(void)
{
	int fd = open_device();
	if (tap_ok(fd < 0, "Failed to open device\n")) {
		return TEST_FAIL;
	}
	const char *data = "guess";
	write(fd, data, strlen(data));
	if (tap_ok(lseek(fd, 100, SEEK_SET) != (off_t)-1 || errno != EINVAL, "Seek did not return -EINVAL for out of bounds position")) {

		close(fd);
		return TEST_FAIL;
	}
	close(fd);
	return TEST_OK;
}

static int test_ioctl_reset(void)
{
	int fd = open(DEVICE_PATH, O_RDWR);
	if (tap_ok(fd < 0, "Failed to open device\n")) {

		return TEST_FAIL;
	}

	if (tap_ok(ioctl(fd, WORDLE_IOC_RESET) < 0, "IOCTL_RESET_GAME failed")) {

		close(fd);
		return TEST_FAIL;
	}

	int stats[1] = {0};
	if (tap_ok(ioctl(fd, WORDLE_IOC_GET_STATS, stats, 1) < 0, "IOCTL_GET_STATS failed")) {

		close(fd);
		return TEST_FAIL;
	}

	if (tap_ok(stats[0] != 1, "Testing ioctl reset failed - did not have 1 game done")) {

		close(fd);
		return TEST_FAIL;
	}

	close(fd);
	return TEST_OK;
}

static int test_ioctl_set_get_word_length(void)
{
	int fd = open_device();
	if (tap_ok(fd < 0, "Failed to open device\n")) {
		return TEST_FAIL;
	}
	int len = 6;
	if (tap_ok(ioctl(fd, WORDLE_IOC_SET_LEN, &len) < 0, "Failed to set word length")) {

		close(fd);
		return TEST_FAIL;
	}
	int current_len;
	if (tap_ok(ioctl(fd, WORDLE_IOC_GET_LEN, &current_len) < 0, "Failed to get word length")) {

		close(fd);
		return TEST_FAIL;
	}
	if (tap_ok(current_len != len, "Word length mismatch: expected %d, got %d\n")) {

		close(fd);
		return TEST_FAIL;
	}
	close(fd);
	return TEST_OK;
}

static int test_ioctl_set_get_guesses(void)
{
	int fd = open_device();
	if (tap_ok(fd < 0, "Failed to open device\n")) {
		return TEST_FAIL;
	}
	int guesses = 10;
	if (tap_ok(ioctl(fd, WORDLE_IOC_SET_GUESSES, &guesses) < 0, "Failed to set number of guesses")) {

		close(fd);
		return TEST_FAIL;
	}
	int current_guesses;
	if (tap_ok(ioctl(fd, WORDLE_IOC_GET_GUESSES, &current_guesses) < 0, "Failed to get number of guesses")) {

		close(fd);
		return TEST_FAIL;
	}
	if (tap_ok(current_guesses != guesses, "Number of guesses mismatch: expected %d, got %d\n")) {

		close(fd);
		return TEST_FAIL;
	}
	close(fd);
	return TEST_OK;
}

static int test_ioctl_clear_feedback_no_feedback(void)
{
	int fd = open_device();
	if (tap_ok(fd < 0, "Failed to open device\n")) {
		return TEST_FAIL;
	}
	if (tap_ok(ioctl(fd, WORDLE_IOC_CLEAR_FEEDBACK) != -ENODATA, "Clear feedback did not return -ENODATA when no feedback is present")) {

		close(fd);
		return TEST_FAIL;
	}
	close(fd);
	return TEST_OK;
}

#define test_ioctl_get_stats_N 3
static int test_ioctl_get_stats(void)
{
	int fd = open_device();
	if (tap_ok(fd < 0, "Failed to open device\n")) {
		return TEST_FAIL;
	}
	int stats[test_ioctl_get_stats_N];
	if (tap_ok(ioctl(fd, WORDLE_IOC_GET_STATS, stats, test_ioctl_get_stats_N) < 0, "Failed to get stats")) {

		close(fd);
		return TEST_FAIL;
	}
	close(fd);
	return TEST_OK;
}
#undef test_ioctl_get_stats_N

static int test_ioctl_set_len_invalid(void)
{
	int fd = open_device();
	if (tap_ok(fd < 0, "Failed to open device\n")) {
		return TEST_FAIL;
	}
	int invalid_len = 0; // Invalid length
	if (tap_ok(ioctl(fd, WORDLE_IOC_SET_LEN, &invalid_len) != -EINVAL, "Set word length did not return -EINVAL for invalid value")) {

		close(fd);
		return TEST_FAIL;
	}
	close(fd);
	return TEST_OK;
}

static int test_ioctl_get_word_length(void)
{
	int fd = open_device();
	if (tap_ok(fd < 0, "Failed to open device\n")) {
		return TEST_FAIL;
	}
	int len;
	if (tap_ok(ioctl(fd, WORDLE_IOC_GET_LEN, &len) < 0, "Failed to get word length")) {

		close(fd);
		return TEST_FAIL;
	}
	close(fd);
	return TEST_OK;
}

static int test_ioctl_set_word_length(void)
{
	int fd = open_device();
	if (tap_ok(fd < 0, "Failed to open device\n")) {
		return TEST_FAIL;
	}
	int len = 6;
	if (tap_ok(ioctl(fd, WORDLE_IOC_SET_LEN, &len) < 0, "Failed to set word length")) {

		close(fd);
		return TEST_FAIL;
	}
	close(fd);
	return TEST_OK;
}

static int test_ioctl_get_stats_invalid_size(void)
{
	int fd = open_device();
	if (fd < 0) {
		return TEST_FAIL;
	}
	int stats[1];
	if (tap_ok(ioctl(fd, WORDLE_IOC_GET_STATS, stats, 0) != -EINVAL, "Get stats did not return -EINVAL for invalid buffer size")) {

		close(fd);
		return TEST_FAIL;
	}
	close(fd);
	return TEST_OK;
}

#define TEST_NAMES	{							\
	X(test_open)								\
	X(test_open_twice)							\
	X(test_write_invalid_chars)					\
	X(test_write_overflow)						\
	X(test_write_valid_input)					\
	X(test_write_partial_data)					\
	X(test_write_after_submit)					\
	X(test_read_feedback)						\
	X(test_submit_undefined_word)				\
	X(test_read_after_write)					\
	X(test_read_small_buffer_size)				\
	X(test_read_twice)							\
	X(test_read_after_clear_feedback)			\
	X(test_read_feedback_color_coding)			\
	X(test_seek_valid)							\
	X(test_seek_out_of_bounds)					\
	X(test_ioctl_reset)							\
	X(test_ioctl_set_get_word_length)			\
	X(test_ioctl_set_get_guesses)				\
	X(test_ioctl_clear_feedback_no_feedback)	\
	X(test_ioctl_get_stats)						\
	X(test_ioctl_set_len_invalid)				\
	X(test_ioctl_get_word_length)				\
	X(test_ioctl_set_word_length)				\
	X(test_ioctl_get_stats_invalid_size) }

typedef int (*test_func)(void);
#define X(name) name,
static test_func tests[] = TEST_NAMES;
#undef X

#define X(name) #name,
static const char *tests_names[] = TEST_NAMES;
#undef X

int main(void)
{
	tap_print_header();

	for (int i = 0; i < ntests; ++i) {
		run_test(tests[i]);
		tap_test_passed(tests_names[i]);
	}

	return 0;
}
