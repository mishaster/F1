
#include <linux/cdev.h>
#include <linux/errname.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kdev_t.h>
#include <linux/module.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/list.h>

#include "game_of_life.h"

/* Define kernel macros */
#if defined(pr_fmt)
#undef pr_fmt
#endif

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

struct gol_info {
	char live_cell;
	char dead_cell;
	int board[ROWS][COLUMNS];
};

struct thread_node {
	pid_t thread_id;
	struct list_head list;
};

// Define a linked list head and a mutex for synchronization
LIST_HEAD(working_threads);
struct mutex working_threads_mutex;

/* Device file operations */
int goldev_open(struct inode *inode, struct file *filep)
{
	pr_info("open called by thread %d\n", current->pid);

	// Check if thread is on.
	struct thread_node *pos;

	mutex_lock(&working_threads_mutex);
	list_for_each_entry(pos, &working_threads, list) {
		if (current->pid == pos->thread_id) {
			pr_info("Thread %d already opened the device\n", current->pid);
			mutex_unlock(&working_threads_mutex);
			return -EBUSY;
		}
	}
	mutex_unlock(&working_threads_mutex);

	// Since the current thread isn't working add it to the list.
	struct thread_node *new_node = kmalloc(sizeof(*new_node), GFP_KERNEL);

	if (!new_node)
		return -ENOMEM;

	new_node->thread_id = current->pid;
	mutex_lock(&working_threads_mutex);
	list_add(&new_node->list, &working_threads);
	mutex_unlock(&working_threads_mutex);


	struct gol_info *data = kmalloc(sizeof(struct gol_info), GFP_KERNEL);

	if (!data) {
		printk(KERN_ERR "Memory allocation failed\n");
		return -ENOMEM;
	}

	data->live_cell = '*';
	data->dead_cell = ' ';
	for (int i = 0; i < ROWS; ++i) {
		for (int j = 0; j < COLUMNS; ++j)
			data->board[i][j] = 0;
	}

	filep->private_data = (void *)(data);
	return 0;
}

int goldev_close(struct inode *inode, struct file *filep)
{
	if (filep->private_data) {
		pr_info("Device released by thread\n");

		// Free the memory allocated for this thread's open
		kfree(filep->private_data);
		filep->private_data = NULL;
	}

	struct thread_node *node, *tmp;

	mutex_lock(&working_threads_mutex);
	list_for_each_entry_safe(node, tmp, &working_threads, list) {
		if (node->thread_id == current->pid) {
			// Remove the node from the list
			list_del(&node->list);
			// Free the memory allocated for the node
			kfree(node);
			break;  // Exit after deleting the first matching node
		}
	}
	mutex_unlock(&working_threads_mutex);
	return 0;
}

ssize_t goldev_read(struct file *filep, char *__user buf, size_t count,
		    loff_t *fpos)
{
	struct gol_info *data = (struct gol_info *)filep->private_data;
	int idx = 0;
	char _buf[HEIGHT * WIDTH + 1];

	for (int j = 0; j < (WIDTH - 1); ++j)
		_buf[idx++] = '-';
	_buf[idx++] = '\n';

	for (int i = 0; i < ROWS; ++i) {
		_buf[idx++] = '|';
		for (int j = 0; j < COLUMNS; ++j)
			_buf[idx++] = data->board[i][j] ? data->live_cell : data->dead_cell;
		_buf[idx++] = '|';
		_buf[idx++] = '\n';
	}

	for (int j = 0; j < (WIDTH - 1); ++j)
		_buf[idx++] = '-';
	_buf[idx++] = '\n';
	_buf[idx++] = '\0';

	size_t bytes_not_written = copy_to_user(buf, _buf, idx);

	if (bytes_not_written)
		return -EFAULT;

	return idx;
}

ssize_t goldev_write(struct file *filep, const char *__user buf, size_t count,
		     loff_t *fpos)
{
	pr_info("write called with %llu\n", *fpos);
	struct gol_info *data = (struct gol_info *)filep->private_data;

	if ((*fpos) < 0 || 511 < (*fpos))
		return -EPERM;

	int row = (*fpos) / COLUMNS;
	int col = (*fpos) % COLUMNS;

	data->board[row][col] = 1 - data->board[row][col];
	(*fpos) += 1;
	return 0;
}

static loff_t goldev_llseek(struct file *filep, loff_t off, int whence)
{
	pr_info("lseek called with %llu\n", off);
	loff_t retval;

	switch (whence) {
	case SEEK_SET:
		retval = off;
		break;
	case SEEK_CUR:
		retval = filep->f_pos + off;
		break;
	case SEEK_END:
		retval = 511 + off;
		break;
	default:
		return -EINVAL;
	}

	if (retval > 511)
		return -EPERM;

	return (filep->f_pos = retval);
}

static int count_neighbors(struct gol_info *data, int row, int col)
{
	int ret = 0;

	for (int i = row - 1; i <= row + 1; ++i) {
		int ii = (i < 0 ? ROWS + i : i) % ROWS;

		for (int j = col - 1; j <= col + 1; ++j) {
			int jj = (j < 0 ? COLUMNS + j : j) % COLUMNS;

			if (i == row && j == col)
				continue;

			if (data->board[ii][jj])
				++ret;
		}
	}
	return ret;
}

int new_board[ROWS][COLUMNS];
static void update_board(struct gol_info *data)
{
	for (int i = 0; i < ROWS; ++i) {
		for (int j = 0; j < COLUMNS; ++j) {
			int neighbors = count_neighbors(data, i, j);

			if (data->board[i][j]) {
				// Live Cell
				if (neighbors < 2) {
					// Under-population
					new_board[i][j] = 0;
				} else if (neighbors > 3) {
					// Over-population
					new_board[i][j] = 0;
				} else {
					new_board[i][j] = 1;
				}
			} else {
				// Dead Cell
				if (neighbors == 3) {
					// Reproduction
					new_board[i][j] = 1;
				} else {
					new_board[i][j] = 0;
				}
			}
		}
	}

	for (int i = 0; i < ROWS; ++i) {
		for (int j = 0; j < COLUMNS; ++j)
			data->board[i][j] = new_board[i][j];
	}
}

static long goldev_ioctl(struct file *filep, unsigned int cmd,
						 unsigned long arg)
{
	pr_info("ioctl called\n");
	struct gol_info *data = (struct gol_info *)filep->private_data;

	switch (cmd) {
	case GOL_TICK:
		update_board(data);
		break;
	case GOL_LIVE:
		if (arg < 0x21 || 0x7e < arg)
			return -EINVAL;
		data->live_cell = (char)arg;
		break;
	default:
		return -EPERM;
	}
	return 0;
}

struct file_operations goldev_fops = {
	.owner = THIS_MODULE,
	.open = goldev_open,
	.read = goldev_read,
	.write = goldev_write,
	.llseek = goldev_llseek,
	.release = goldev_close,
	.unlocked_ioctl = goldev_ioctl,
};

#if !defined(errname)
#define errname(err) #err
#endif

/* Device class */
static struct goldev {
	dev_t devnum;
	struct cdev cdev;
	struct class *class;
	struct device *device;
} goldev;

static char *goldev_devnode(const struct device *dev, umode_t *mode)
{
	if (mode)
		*mode = 0666;
	return NULL;
}

int __init goldev_init(void)
{
	pr_info("init\n");

	// Initialize the mutex
	mutex_init(&working_threads_mutex);

	int ret = alloc_chrdev_region(&goldev.devnum, 0, 1, DEVNAME);

	if (ret) {
		pr_err("Unable to allocate major/minor: %d (%s)\n", ret, errname(ret));
		goto err_alloc_chrdev_region;
	}

	goldev.class = class_create(DEVNAME);
	if (IS_ERR(goldev.class)) {
		ret = PTR_ERR(goldev.class);
		pr_err("failed to create device class: %s\n", errname(ret));
		goto err_class_create;
	}
	goldev.class->devnode = goldev_devnode;

	cdev_init(&goldev.cdev, &goldev_fops);
	ret = cdev_add(&goldev.cdev, goldev.devnum, 1);
	if (ret) {
		pr_err("failed to add kkey cdev: %s\n", errname(ret));
		goto err_cdev_add;
	}

	struct device *dev_i = device_create(
		goldev.class, NULL, MKDEV(MAJOR(goldev.devnum), 0), NULL, DEVNAME);

	if (IS_ERR(dev_i)) {
		ret = PTR_ERR(dev_i);
		pr_err("failed to create kkey device: %s\n", errname(ret));
		goto err_device_create;
	}
	goldev.device = dev_i;
	return 0;

err_device_create:
	device_destroy(goldev.class, MKDEV(MAJOR(goldev.devnum), 0));
err_cdev_add:
	class_destroy(goldev.class);
err_class_create:
	unregister_chrdev_region(goldev.devnum, 1);
err_alloc_chrdev_region:
	return ret;
}

void goldev_cleanup(void)
{
	pr_info("cleanup\n");

	device_destroy(goldev.class, MKDEV(MAJOR(goldev.devnum), 0));
	cdev_del(&goldev.cdev);
	class_destroy(goldev.class);
	unregister_chrdev_region(goldev.devnum, 10);
}

module_init(goldev_init);
module_exit(goldev_cleanup);
MODULE_LICENSE("GPL");
