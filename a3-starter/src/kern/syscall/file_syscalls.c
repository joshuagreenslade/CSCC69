/* BEGIN A3 SETUP */
/* This file existed for A1 and A2, but has been completely replaced for A3.
 * We have kept the dumb versions of sys_read and sys_write to support early
 * testing, but they should be replaced with proper implementations that 
 * use your open file table to find the correct vnode given a file descriptor
 * number.  All the "dumb console I/O" code should be deleted.
 */

#include <types.h>
#include <kern/errno.h>
#include <lib.h>
#include <thread.h>
#include <current.h>
#include <syscall.h>
#include <vfs.h>
#include <vnode.h>
#include <uio.h>
#include <kern/fcntl.h>
#include <kern/unistd.h>
#include <kern/limits.h>
#include <kern/stat.h>
#include <copyinout.h>
#include <synch.h>
#include <file.h>
#include <kern/seek.h>


/*
 * mk_useruio
 * sets up the uio for a USERSPACE transfer. 
 */
static
void
mk_useruio(struct iovec *iov, struct uio *u, userptr_t buf, 
	   size_t len, off_t offset, enum uio_rw rw)
{

	iov->iov_ubase = buf;
	iov->iov_len = len;
	u->uio_iov = iov;
	u->uio_iovcnt = 1;
	u->uio_offset = offset;
	u->uio_resid = len;
	u->uio_segflg = UIO_USERSPACE;
	u->uio_rw = rw;
	u->uio_space = curthread->t_addrspace;
}

/*
 * sys_open
 * just copies in the filename, then passes work to file_open.
 * You have to write file_open.
 * 
 */
int
sys_open(userptr_t filename, int flags, int mode, int *retval)
{
	char *fname;
	int result;

	if ( (fname = (char *)kmalloc(__PATH_MAX)) == NULL) {
		return ENOMEM;
	}

	result = copyinstr(filename, fname, __PATH_MAX, NULL);
	if (result) {
		kfree(fname);
		return result;
	}

	result =  file_open(fname, flags, mode, retval);
	kfree(fname);
	return result;
}

/* 
 * sys_close
 * You have to write file_close.
 */
int
sys_close(int fd)
{
	return file_close(fd);
}

/*
 * sys_dup2
 *
 */
int
sys_dup2(int oldfd, int newfd, int *retval)
{
	int error;
	struct openfiles *oldfile;

	//check that both fds ae valid, and that oldfd isn't null
	error = check_fd(oldfd);
	if(error)
		return error;

	error = check_fd(newfd);
	if(error)
		return error;

	oldfile = curthread->t_filetable->file[oldfd];
	if(oldfile == NULL)
		return EBADF;

	//if both fd's are the same, do nothing
	if(oldfd == newfd) {
		*retval = newfd;
		return 0;
	}

	//if the newfd has a file in it close it
	if(curthread->t_filetable->file[newfd] != NULL)
		error = file_close(newfd);
		if(error)
			return error;

	//increase the oldfile's links count
	lock_acquire(oldfile->file_lock);
	oldfile->links++;
	lock_release(oldfile->file_lock);

	//link file[newfd] to oldfile
	curthread->t_filetable->file[newfd] = oldfile;

	*retval = newfd;

	return 0;
}

/*
 * sys_read
 * calls VOP_READ.
 * 
 * A3: This is the "dumb" implementation of sys_write:
 * it only deals with file descriptors 1 and 2, and 
 * assumes they are permanently associated with the 
 * console vnode (which must have been previously initialized).
 *
 * In your implementation, you should use the file descriptor
 * to find a vnode from your file table, and then read from it.
 *
 * Note that any problems with the address supplied by the
 * user as "buf" will be handled by the VOP_READ / uio code
 * so you do not have to try to verify "buf" yourself.
 *
 * Most of this code should be replaced.
 */
int
sys_read(int fd, userptr_t buf, size_t size, int *retval)
{
	struct openfiles *file;
	struct uio user_uio;
	struct iovec user_iov;
	int result;
	int error;

	if(buf == NULL)
		return EFAULT;

	//check if the fd is valid
	error = check_fd(fd);
	if (error)
		return error;

	file = curthread->t_filetable->file[fd];
	if(file == NULL)
		return EBADF;

	lock_acquire(file->file_lock);

	//check that the file is open for reading
	if(file->flag == O_WRONLY){
		lock_release(file->file_lock);
		return EBADF;
	}

	/* set up a uio with the buffer, its size, and the current offset */
	mk_useruio(&user_iov, &user_uio, buf, size, file->offset, UIO_READ);

	/* does the read */
	result = VOP_READ(file->vn, &user_uio);
	if (result) {
		lock_release(file->file_lock);
		return result;
	}

	//set the offset
	file->offset = user_uio.uio_offset;
	lock_release(file->file_lock);

	/*
	 * The amount read is the size of the buffer originally, minus
	 * how much is left in it.
	 */
	*retval = size - user_uio.uio_resid;

	return 0;
}

/*
 * sys_write
 * calls VOP_WRITE.
 *
 * A3: This is the "dumb" implementation of sys_write:
 * it only deals with file descriptors 1 and 2, and
 * assumes they are permanently associated with the
 * console vnode (which must have been previously initialized).
 *
 * In your implementation, you should use the file descriptor
 * to find a vnode from your file table, and then read from it.
 *
 * Note that any problems with the address supplied by the
 * user as "buf" will be handled by the VOP_READ / uio code
 * so you do not have to try to verify "buf" yourself.
 *
 * Most of this code should be replaced.
 */

int
sys_write(int fd, userptr_t buf, size_t len, int *retval)
{
	struct openfiles *file;
        struct uio user_uio;
        struct iovec user_iov;
        int result;
	int error;

	if(buf == NULL)
		return EFAULT;

        //check if the fd is valid
	error = check_fd(fd);
        if(error)
        	return error;

	file = curthread->t_filetable->file[fd];
	if(file == NULL)
		return EBADF;

	lock_acquire(file->file_lock);

	//check that the file is open for writing
	if(file->flag == O_RDONLY){
		lock_release(file->file_lock);
		return EBADF;
	}

        /* set up a uio with the buffer, its size, and the current offset */
        mk_useruio(&user_iov, &user_uio, buf, len, file->offset, UIO_WRITE);

        /* does the write */
        result = VOP_WRITE(file->vn, &user_uio);
        if (result) {
		lock_release(file->file_lock);
                return result;
        }

	//set the offset
	file->offset = user_uio.uio_offset;
	lock_release(file->file_lock);

        /*
         * the amount written is the size of the buffer originally,
         * minus how much is left in it.
         */
        *retval = len - user_uio.uio_resid;

        return 0;
}

/*
 * sys_lseek
 *
 */
int
sys_lseek(int fd, off_t offset, int whence, off_t *retval)
{
	int error;
	struct stat info;

	//check that fd is valid
	error = check_fd(fd);
	if(error)
		return error;

	if(curthread->t_filetable->file[fd] == NULL)
		return EBADF;

	lock_acquire(curthread->t_filetable->file[fd]->file_lock);

	//get the new offset value
	if(whence == SEEK_SET)
		*retval = offset;
	else if(whence == SEEK_CUR)
		*retval = curthread->t_filetable->file[fd]->offset + offset;
	else if(whence == SEEK_END) {
		error = VOP_STAT(curthread->t_filetable->file[fd]->vn, &info);
		if(error)
			return error;

		*retval = info.st_size + offset;
	}
	else {
		lock_release(curthread->t_filetable->file[fd]->file_lock);
		return EINVAL;
	}

	//check that the new offset isn't negative
	if(*retval < 0){
		lock_release(curthread->t_filetable->file[fd]->file_lock);
		return EINVAL;
	}

	//check that fd is not a console device
	error = VOP_TRYSEEK(curthread->t_filetable->file[fd]->vn, *retval);
	if(error) {
		lock_release(curthread->t_filetable->file[fd]->file_lock);
		return ESPIPE;
	}

	curthread->t_filetable->file[fd]->offset = *retval;
	lock_release(curthread->t_filetable->file[fd]->file_lock);
	return 0;
}


/* really not "file" calls, per se, but might as well put it here */

/*
 * Given a path, change the curthread current working directory to the working directory
 * of the given path.
 * return 0 if succeeded, -1 if not 0
 * sys_chdir
 * 
 */
int
sys_chdir(userptr_t path)
{
        int result;
        struct vnode *dir;
        char *path_name;

        if ((path_name = (char *)kmalloc(__PATH_MAX)) == NULL)
		{
			return ENOMEM;
		}
		result = copyinstr(path, path_name, __PATH_MAX, NULL);

		if (result)
		{
			kfree(path_name);
			return result;
		}

		// if path is too long
		if (sizeof(path_name) > __PATH_MAX)		
		{
			return ENAMETOOLONG;
		}

		// find inode to wanted directory
		result = vfs_lookup(path_name, &dir);
		if (result)
		{
			return result;
		}
		// set to current directory
		result = vfs_setcurdir(dir);
		return result;

}

/*
 * Get the pathname of the current working directory
 * sys___getcwd
 * 
 */
int
sys___getcwd(userptr_t buf, size_t buflen, int *retval)
{
        struct uio user_uio;
        struct iovec user_iov;
        int result;

        // if the size is empty
        if (buflen == 0)
        {
        	return EINVAL;
        }

        mk_useruio(&user_iov, &user_uio, buf, buflen, 0, UIO_READ);
        result = vfs_getcwd(&user_uio);
        if (result)
        {
        	// we are returning -1 on error
        	*retval = -1;
        	return result;
        }

        // save the # of bytes read in return value
        *retval = buflen - user_uio.uio_resid;
        return 0;
}

/*
 * Get information about an open file associated with
 * the file descriptor file descriptors, and write it
 * to the area pointed to by the buffer.
 * sys_fstat
 */
int
sys_fstat(int fd, userptr_t statptr)
{
		// If it is a bad file descriptor
        if ( fd < 0 || fd >= __OPEN_MAX )
        {
        	return EBADF;
        }
        struct openfiles *file = curthread->t_filetable->file[fd];
        if (file == NULL)
        {
        	return EBADF;
        }

        // If it is a bad address
        if ( statptr == NULL || curthread->t_filetable->file[fd]->vn == NULL )
        {
        	return EFAULT;
        }

        struct vnode *file_vnode;
        struct stat stats;
        struct uio 	user_uio;
        struct iovec user_iov;
        int result;

        file_vnode = curthread->t_filetable->file[fd]->vn;

        // If file does not exist
        if ( !file_vnode )
        {
        	return EBADF;
        }

        result = VOP_STAT(file_vnode, &stats);
        if (result)
        {
        	return result;
        }

        //Point statptr to stats
        mk_useruio(&user_iov, &user_uio, statptr, sizeof(stats), 0, UIO_READ);
        result = uiomove(&stats, sizeof(stats), &user_uio);
        return result;
}

/*
 * Read filename from directory
 * sys_getdirentry
 */
int
sys_getdirentry(int fd, userptr_t buf, size_t buflen, int *retval)
{
        // If it is a bad file descriptor
        if ( fd < 0 || fd >= __OPEN_MAX )
        {
        	return EBADF;
        }
        if ( curthread->t_filetable->file[fd] == NULL )
        {
        	return EBADF;
        }
        // If it is a bad address
        if ( buf == NULL || curthread->t_filetable->file[fd]->vn == NULL )
        {
        	return EFAULT;
        }
        struct uio user_uio;
        struct iovec user_iov;
        struct vnode *file_vnode;
        int result;

        file_vnode = curthread->t_filetable->file[fd]->vn;

        // If file does not exist
        if (!file_vnode)
        {
        	*retval = -1;
        	return EBADF;
        }

        mk_useruio(&user_iov, &user_uio, buf, buflen, curthread->t_filetable->file[fd]->offset, UIO_READ);
        result = VOP_GETDIRENTRY(file_vnode, &user_uio);

        // if VOP_GETDIRENTRY fails
        if ( result )
        {
        	return result;
        }

        // return the size of file name 
        *retval = buflen - user_uio.uio_resid;

        // better take this out into a function
        curthread->t_filetable->file[fd]->offset = user_uio.uio_offset;
        return 0;
}

/* END A3 SETUP */




