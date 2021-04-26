/*
 *  Copyright (C) 2002  Bartek Krajnik <bartek@bicom.pl>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include        <stdio.h>
#include        <unistd.h>
#include        <fcntl.h>
#include        <sys/types.h>
#include        <sys/stat.h>
#include        <utime.h>
#include        <errno.h>
#include        <signal.h>
#include        "error.h"
#include        "wrappers.h"

void
Close(int fd)
{
        if (close(fd) == -1)
                err_sys("close error");
}

int
Fcntl(int fd, int cmd, struct flock *lock)
{
        int     n;

        if ( (n = fcntl(fd, cmd, lock)) == -1)
                err_sys("fcntl error");
        return(n);
}

ssize_t
Read(int fd, void *ptr, size_t nbytes)
{
        ssize_t         n;

        if ( (n = read(fd, ptr, nbytes)) == -1)
                err_sys("read error");
        return(n);
}

void
Unlink(const char *pathname)
{
        if (unlink(pathname) == -1)
                err_sys("unlink error for %s", pathname);
}

ssize_t
Write(int fd, void *ptr, size_t nbytes)
{
	ssize_t	n;

        if ( (n = write(fd, ptr, nbytes)) == -1 || n < nbytes)
                err_sys("write error");
	return(n);
	
}

int
Open(const char *pathname, int oflag, mode_t mode)
{
        int             fd;

        if ( (fd = open(pathname, oflag, mode)) == -1)
                err_sys("open error for %s", pathname);
        return(fd);
}

off_t
Lseek(int fd, off_t offset, int whence)
{
	off_t	n;

	if ( (n = lseek(fd, offset, whence)) == -1)
		err_sys("lseek error");

	return(n);
}

int
Fstat(int fd, struct stat *buf)
{
	int	n;

	if ( (n = fstat(fd, buf)) == -1)
		err_sys("fstat error");

	return(n);
}

int
Chmod(const char *file, mode_t newmode)
{
	int n;

	if ( (n = chmod(file, newmode)) == -1)
		err_sys("chmod error");

	return(n);
}

int
Rename(const char *oldpath, const char *newpath)
{
	int n;
	
	if ( (n = rename(oldpath, newpath)) == -1)
		err_sys("rename error");

	return(n);
}

int
Utime(const char *pathname, const struct utimbuf *times)
{
	int n;
	
	if ( (n = utime(pathname, times)) == -1)
		err_sys("utime error");

	return(n);
}

int
Chown(const char *path, uid_t owner, gid_t group)
{
	int n;

	if ( (n = chown(path, owner, group)) == -1)
		err_sys("chown error");

	return(n);
}

Sigfunc *
signal(int signo, Sigfunc *func)
{
	struct sigaction	act, oact;

	act.sa_handler = func;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if (signo == SIGALRM) {
#ifdef	SA_INTERRUPT
		act.sa_flags |= SA_INTERRUPT;	/* SunOS 4.x */
#endif
	} else {
#ifdef	SA_RESTART
		act.sa_flags |= SA_RESTART;		/* SVR4, 44BSD */
#endif
	}
	if (sigaction(signo, &act, &oact) < 0)
		return(SIG_ERR);
	return(oact.sa_handler);
}
/* end signal */

Sigfunc *
Signal(int signo, Sigfunc *func)	/* for our signal() function */
{
	Sigfunc	*sigfunc;

	if ( (sigfunc = signal(signo, func)) == SIG_ERR)
		err_sys("signal error");
	return(sigfunc);
}

void *
Malloc(size_t size)
{
        void    *ptr;

        if ( (ptr = (void *) malloc(size)) == NULL)
                err_sys("malloc error");
        return(ptr);
}

void *
Realloc(void *ptr, size_t size)
{
        if ( (ptr = (void *) realloc(ptr, size)) == NULL)
                err_sys("realloc error");
        return(ptr);
}

