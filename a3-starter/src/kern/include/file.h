/* BEGIN A3 SETUP */
/*
 * Declarations for file handle and file table management.
 * New for A3.
 */

#ifndef _FILE_H_
#define _FILE_H_

#include <kern/limits.h>

struct vnode;
struct lock;

/*
 * filetable struct
 * just an array, nice and simple.  
 * It is up to you to design what goes into the array.  The current
 * array of ints is just intended to make the compiler happy.
 */
struct filetable {
	struct openfiles *file[__OPEN_MAX];
};

struct openfiles {
	char *filename;
	int flag;		//the w/r flag
	off_t offset;		//the file offset
	int links;		//keep track of the number of links to the file

	struct lock *file_lock;	//to lock the file info
	struct vnode *vn;	//the files vnode
};

/* these all have an implicit arg of the curthread's filetable */
int filetable_init(void);
void filetable_destroy(struct filetable *ft);

/* opens a file (must be kernel pointers in the args) */
int file_open(char *filename, int flags, int mode, int *retfd);

/* closes a file */
int file_close(int fd);

/* A3: You should add additional functions that operate on
 * the filetable to help implement some of the filetable-related
 * system calls.
 */

/* inserts a file into the filetable */
int insert_file(struct openfiles *file, int *retfd);

/* checks that the fd is valid and is in the filetable */
struct openfiles* check_fd(int fd);

/* duplicates the current threads filetable */
int duplicate_filetable(struct filetable **duplicate);


#endif /* _FILE_H_ */

/* END A3 SETUP */
