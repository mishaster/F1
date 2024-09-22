#ifndef GAME_OF_LIFE_H
#define GAME_OF_LIFE_H

/*
 *  <----! Game of Life written in C ---->
 *
 *	  open(2) - creates new grid
 *	  read(2) - reads whole grid
 *	  write(2) - toggles cell specified by the seek pointer
 *	  lseek(2) - sets the seek point to the given cell
 *	  ioctl(2) - op 0 replaces * with another printable character
 *				 op 1 calculates the next generation
 *	  close(2) - resets grid
 */

#ifdef __KERNEL__
#include <linux/ioctl.h>
#else /* userspace */
#include <sys/ioctl.h>
#endif

#define GOL_MAGIC ('g')
#define GOL_TICK _IO(GOL_MAGIC, 0x01)
#define GOL_LIVE _IOW(GOL_MAGIC, 0x02, char)

#define ROWS (16)
#define COLUMNS (32)

#define HEIGHT (ROWS + 2)
#define WIDTH (COLUMNS + 2 + 1)

#define DEVNAME "game_of_life"
#define DEVPATH "/dev/" DEVNAME

#define GRID_STRING_SIZE (HEIGHT * WIDTH + 1)

#endif  // GAME_OF_LIFE_H
