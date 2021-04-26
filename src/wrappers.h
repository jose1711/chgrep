void Close(int fd);

int Fcntl(int fd, int cmd, struct flock *lock);

ssize_t Read(int fd, void *ptr, size_t nbytes);

void Unlink(const char *pathname);

ssize_t rite(int fd, void *ptr, size_t nbytes);

int Open(const char *pathname, int oflag, mode_t mode);

off_t Lseek(int fd, off_t offset, int whence);

int Fstat(int fd, struct stat *buf);

int Chmod(const char *file, mode_t newmode);

int Rename(const char *oldpath, const char *newpath);

int Utime(const char *pathname, const struct utimbuf *times);

int Chown(const char *path, uid_t owner, gid_t group);

typedef void    Sigfunc(int);   /* for signal handlers */

Sigfunc *Signal(int signo, Sigfunc *func);

void *Malloc(size_t size);
