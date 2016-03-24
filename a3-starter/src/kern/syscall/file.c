/* BEGIN A3 SETUP */
/*
 * File handles and file tables.
 * New for ASST3
 */

#include <types.h>
#include <kern/errno.h>
#include <kern/limits.h>
#include <kern/stat.h>
#include <kern/unistd.h>
#include <file.h>
#include <syscall.h>
#include <vfs.h>
#include <lib.h>
#include <synch.h>
#include <thread.h>
#include <current.h>
#include <vnode.h>
#include <uio.h>
#include <kern/fcntl.h>

/*** openfile functions ***/

/*
 * file_open
 * opens a file, places it in the filetable, sets RETFD to the file
 * descriptor. the pointer arguments must be kernel pointers.
 * NOTE -- the passed in filename must be a mutable string.
 * 
 * A3: As per the OS/161 man page for open(), you do not need 
 * to do anything with the "mode" argument.
 */
int
file_open(char *filename, int flags, int mode, int *retfd)
{
	int error;
	struct vnode *vn;
	struct openfiles *file;

	//open the file
	error = vfs_open(filename, flags, mode, &vn);
	if(error)
		return error;

	//allocate space for the file info
	file = kmalloc(sizeof(struct openfiles));
	if(file == NULL) {
		vfs_close(vn);
		return ENOMEM;
	}

	//initalize the file table entry
	file->filename = filename;
	file->flag = flags;
	file->offset = 0;
	file->links = 1;
	file->vn = vn;

	file->file_lock = lock_create("file_lock");
	if(file->file_lock == NULL) {
		vfs_close(vn);
		kfree(file);
		return ENOMEM;
	}

	//put the file into the lowest fd in the filetable
	error = insert_file(file, retfd);
	if(error) {
		vfs_close(vn);
		lock_destroy(file->file_lock);
		kfree(file);
		return error;
	}
	return 0;
}


/* 
 * file_close
 * Called when a process closes a file descriptor.  Think about how you plan
 * to handle fork, and what (if anything) is shared between parent/child after
 * fork.  Your design decisions will affect what you should do for close.
 */
int
file_close(int fd)
{
	struct openfiles *file;
	int error;

	//check that the fd is valid
	error = check_fd(fd);
	if(error)
		return error;

	file = curthread->t_filetable->file[fd];
	if(file == NULL)
		return EBADF;

	lock_acquire(file->file_lock);

    //decrease the files link
	file->links--;

	//if the link is zero so close the file
	if(file->links < 0) {
		if(file->vn != NULL)
			vfs_close(file->vn);
		lock_release(file->file_lock);
		lock_destroy(file->file_lock);
		kfree(file);
	}
	else
		lock_release(file->file_lock);

	curthread->t_filetable->file[fd] = NULL;

	return 0;
}

/*** filetable functions ***/

/* 
 * filetable_init
 * pretty straightforward -- allocate the space, set up 
 * first 3 file descriptors for stdin, stdout and stderr,
 * and initialize all other entries to NULL.
 * 
 * Should set curthread->t_filetable to point to the
 * newly-initialized filetable.
 * 
 * Should return non-zero error code on failure.  Currently
 * does nothing but returns success so that loading a user
 * program will succeed even if you haven't written the
 * filetable initialization yet.
 */

int
filetable_init(void)
{
	int error;
	char *string;

	//allocate space for filetable
	curthread->t_filetable = kmalloc(sizeof(struct filetable));
	if(curthread->t_filetable == NULL)
		return ENOMEM;

	//initalize the table
	for(int i=0; i < __OPEN_MAX; i++)
		curthread->t_filetable->file[i] = NULL;

	//set stdin, stdout, stderr
	for(int i=0; i < 3; i++) {
		if(i == STDIN_FILENO) {
			strcpy(string, "con:");
			error = file_open(string, O_RDONLY, 0, &i);
			if(error)
				return error;
		}
		else if(i == STDOUT_FILENO) {
			strcpy(string, "con:");
			error = file_open(string, O_WRONLY, 0, &i);
			if(error)
				return error;
		}
		else if(i == STDERR_FILENO) {
			strcpy(string, "con:");
			error = file_open(string, O_WRONLY, 0, &i);
			if(error)
				return error;
		}
	}
	return 0;
}

/*
 * filetable_destroy
 * closes the files in the file table, frees the table.
 * This should be called as part of cleaning up a process (after kill
 * or exit).
 */
void
filetable_destroy(struct filetable *ft)
{
        //call close on all tables that arn't null
	for(int i=0; i < __OPEN_MAX; i++) {
		if(ft->file[i] != NULL)
			file_close(i);
	}

	//free the table
	kfree(ft);
	ft = NULL;
}


/* 
 * You should add additional filetable utility functions here as needed
 * to support the system calls.  For example, given a file descriptor
 * you will want some sort of lookup function that will check if the fd is 
 * valid and return the associated vnode (and possibly other information like
 * the current file position) associated with that open file.
 */

/*
 * Inserts the given openfile into the filetable and returns the fd that it was
 * inserted into. Returns EMFILE if the file table was full.
 */
int
insert_file(struct openfiles *file, int *retfd)
{
	//look for the first avaliable fd in the filetable
	for(int i=0; i < __OPEN_MAX; i++) {
		if(curthread->t_filetable->file[i] == NULL) {
			curthread->t_filetable->file[i] = file;
			*retfd = i;
			return 0;
		}
	}
	return EMFILE;
}


/*
 * Checks if the fd is valid and is in the filetable. Return the openfile if it
 * is, null otherwise.
 */
int
check_fd(int fd)
{
	if(fd < 0 || fd >= __OPEN_MAX)
		return EBADF;

	return 0;
}


/*
 * Duplicates the current threads filetable and puts it into duplicate.
 */
int
duplicate_filetable(struct filetable **duplicate)
{
	if(curthread->t_filetable == NULL) {
		*duplicate = NULL;
		return 0;
	}

	//allocate space for the filetable
	*duplicate = kmalloc(sizeof(struct filetable));
	if(duplicate == NULL)
		return ENOMEM;

	//copy all entries to the duplicate table and increase all the link counts
	//by 1
	for(int i=0; i < __OPEN_MAX; i++) {
		if(curthread->t_filetable->file[i] == NULL)
			(*duplicate)->file[i] = NULL;
		else {
			lock_acquire(curthread->t_filetable->file[i]->file_lock);
			curthread->t_filetable->file[i]->links++;
			lock_release(curthread->t_filetable->file[i]->file_lock);
			(*duplicate)->file[i] = curthread->t_filetable->file[i];
		}
	}
	return 0;
}

/* END A3 SETUP */
