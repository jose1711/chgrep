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

#include	<stdio.h>
#include	<unistd.h>
#include	<string.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#include	<utime.h>
#include	<fcntl.h>
#include	<errno.h>
#include	<regex.h>
#include	<limits.h>
#include	<dirent.h>
#include	"ftw.h"

#define		MAXLINE	4096
#define		PERM	0666

extern int	verbose;
extern int	losttmp;
extern int	withoutregexp;
extern int	simulation_mode;
extern int	dont_ch_times;
extern int	quiet_mode;
extern regex_t	*oldstr;
extern char	*oldstr2;
extern char	*newstr;
extern char	*file_quit;
extern char	*tmpfile_quit;


int
Copy_ch_file(const char *file, const char *tmpfile, regex_t *oldstring, char *newstring)
{
	int		fdin, fdout;
	struct stat	tmpstate;
	struct utimbuf	tmputime;
	struct flock	fl;
	char		*buffin;
	char		*buffout;
	char		*buffintmp1;
	char		*buffintmp2;
	char		*buffouttmp;
	size_t		size_newstring, size_oldstring;
	ssize_t		nread, nwrite, nwrote, mem_al;
	ssize_t		counter = 0;
	unsigned int	loop = 0;
	char		*tmp;
	regmatch_t	pmatch[512];

	buffin = (char *) Malloc(BUFSIZ);
	buffout = (char *) Malloc(BUFSIZ * 5);
	mem_al = BUFSIZ * 5;
	file_quit = (char *) tmpfile;
	tmpfile_quit = (char *) file;

	fl.l_type = F_RDLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start = 0;
	fl.l_len = 0;

	size_newstring = strlen(newstring);

	if (memcmp(file, "stdin", strlen(file)) == 0) {
		fdin = STDIN_FILENO;
	} else {
		if ( (fdin = open(file, O_RDONLY, PERM)) == -1) {
			err_msg("%s\topen error:%s", file, strerror(errno));
			return(1);
		}
		if (fstat(fdin, &tmpstate)) {
			err_msg("%s\tfstat error:%s", file, strerror(errno));
			if (fdin != STDIN_FILENO)
				Close(fdin);
			return(1);
		}
	}
	if (memcmp(tmpfile, "stdout", strlen(tmpfile)) == 0) {
		fdout = STDOUT_FILENO;
	} else {
		if ( (fdout = open(tmpfile, O_WRONLY|O_CREAT|O_TRUNC, PERM)) == -1) {
			err_msg("%s\topen error:%s", tmpfile, strerror(errno));
			if (fdin != STDIN_FILENO)
				Close(fdin);
			return(1);
		}

		if (chmod(tmpfile, tmpstate.st_mode)) {
			err_msg("%s\tchmod error:%s", tmpfile, strerror(errno));
			if (fdin != STDIN_FILENO)
				Close(fdin);
			if (fdout != STDOUT_FILENO)
				Close(fdout);
			return(1);
		}
		if (chown(tmpfile, tmpstate.st_uid, tmpstate.st_gid)) {
			err_msg("%s\tchown error:%s", tmpfile, strerror(errno));
			if (fdin != STDIN_FILENO)
				Close(fdin);
			if (fdout != STDOUT_FILENO)
				Close(fdout);
			return(1);
		}
		if (fcntl(fdin, F_SETLK, &fl) == -1) {
			err_msg("%s\tfcntl error:%s", file, strerror(errno));
			if (fdin != STDIN_FILENO)
				Close(fdin);
			if (fdout != STDOUT_FILENO)
				Close(fdout);
			return(1);
		} 
	}
	if (verbose)
		Write(STDERR_FILENO, "\n", 1);

	while ( (nwrite = nread = Read(fdin, buffin, BUFSIZ - 1)) != 0) {
		buffin[nread] = '\0';
		nwrote = 0;
		if (verbose)
			err_msg("Loop %d: Read: %d, Counter: %d", ++loop, nread, counter);
		buffintmp1 = buffin;
		buffouttmp = buffout;

		/* Compare and change strings */
		while ( regexec(oldstring, buffintmp1, 1, pmatch, 0) == 0) {
			buffintmp2 = buffintmp1 + pmatch[0].rm_so;
			counter++;
			nwrote += buffintmp2 - buffintmp1;
			nwrite -= buffintmp2 - buffintmp1;
			if (nwrote > mem_al - BUFSIZ ) {
				buffout = (char *) Realloc(buffout, mem_al + BUFSIZ);
				mem_al += BUFSIZ;
			}
			memcpy(buffouttmp, buffintmp1, buffintmp2 - buffintmp1);
			buffouttmp += buffintmp2 - buffintmp1;
			nwrote += size_newstring;
			nwrite -= pmatch[0].rm_eo - pmatch[0].rm_so;
			if (nwrote > mem_al - BUFSIZ ) {
				buffout = (char *) Realloc(buffout, mem_al + BUFSIZ);
				mem_al += BUFSIZ;
			}
			memcpy(buffouttmp, newstring, size_newstring);
			buffouttmp += size_newstring;
			buffintmp1 = buffintmp2 + (pmatch[0].rm_eo - pmatch[0].rm_so);
		}

		memcpy(buffouttmp, buffintmp1, nwrite);
		nwrote += buffin + nread - buffintmp1;

		if (verbose)
			err_msg("\tWrite: %d, Counter: %d", nwrote, counter);

		if(simulation_mode)
			Write(fdout, buffin, nread);
		else
			Write(fdout, buffout, nwrote);

		buffout = (char *) Realloc(buffout, BUFSIZ * 5);
		mem_al = BUFSIZ * 5;
	}

	fl.l_type = F_UNLCK;
	if (fcntl(fdout, F_SETLK, &fl) == -1) {
		err_msg("%s\tfcntl error:%s", tmpfile, strerror(errno));
		if (fdin != STDIN_FILENO)
			Close(fdin);
		if (fdout != STDOUT_FILENO)
			Close(fdout);
		return(1);
	} 

	if (fdin != STDIN_FILENO)
		Close(fdin);

	if (fdout != STDOUT_FILENO) {
		Close(fdout);
		if(dont_ch_times) {
			tmputime.actime = tmpstate.st_atime;
			tmputime.modtime = tmpstate.st_mtime;
			Utime(tmpfile, &tmputime);
		}
	}
	
	if(! quiet_mode)
		printf(" %ld", counter);
	fflush(stdout);

	free(buffin);
	free(buffout);

	return 0;
}

int
Copy_ch_file2(const char *file, const char *tmpfile, char *oldstring, char *newstring)
{
	int	fdin, fdout;
	struct stat	tmpstate;
	struct utimbuf	tmputime;
	struct flock	fl;
	char	*buffin;
	char	*buffout;
	char	*buffintmp1;
	char	*buffintmp2;
	char	*buffouttmp;
	size_t	size_newstring, size_oldstring;
	ssize_t	nread, nwrite, nwrote, mem_al;
	ssize_t	counter = 0;
	unsigned int    loop = 0;
	char	*tmp;

	size_newstring = strlen(newstring);
	size_oldstring = strlen(oldstring);

	if(size_newstring == size_oldstring) {
		buffin = (char *) Malloc(BUFSIZ);
		buffout = (char *) Malloc(BUFSIZ);
		mem_al = BUFSIZ;
	} else {
		buffin = (char *) Malloc(BUFSIZ);
		buffout = (char *) Malloc(BUFSIZ * 5);
		mem_al = BUFSIZ * 5;
	}

	file_quit = (char *) tmpfile;
	tmpfile_quit = (char *) file;

	fl.l_type = F_RDLCK;
	fl.l_whence = SEEK_SET;
	fl.l_start = 0;
	fl.l_len = 0;

	if (memcmp(file, "stdin", strlen(file)) == 0) {
		fdin = STDIN_FILENO;
	} else {
		if ( (fdin = open(file, O_RDONLY, PERM)) == -1) {
			err_msg("%s\topen error:%s", file, strerror(errno));
			return(1);
		}
		if (fstat(fdin, &tmpstate)) {
			err_msg("%s\tfstat error:%s", file, strerror(errno));
			if (fdin != STDIN_FILENO)
				Close(fdin);
			return(1);
		}
	}
	if (memcmp(tmpfile, "stdout", strlen(tmpfile)) == 0) {
		fdout = STDOUT_FILENO;
	} else {
		if ( (fdout = open(tmpfile, O_WRONLY|O_CREAT|O_TRUNC, PERM)) == -1) {
			err_msg("%s\topen error:%s", tmpfile, strerror(errno));
			if (fdin != STDIN_FILENO)
				Close(fdin);
			return(1);
		}

		if (chmod(tmpfile, tmpstate.st_mode)) {
			err_msg("%s\tchmod error:%s", tmpfile, strerror(errno));
			if (fdin != STDIN_FILENO)
				Close(fdin);
			if (fdout != STDOUT_FILENO)
				Close(fdout);
			return(1);
		}
		if (chown(tmpfile, tmpstate.st_uid, tmpstate.st_gid)) {
			err_msg("%s\tchown error:%s", tmpfile, strerror(errno));
			if (fdin != STDIN_FILENO)
				Close(fdin);
			if (fdout != STDOUT_FILENO)
				Close(fdout);
			return(1);
		}
		if (fcntl(fdin, F_SETLK, &fl) == -1) {
			err_msg("%s\tfcntl error:%s", file, strerror(errno));
			if (fdin != STDIN_FILENO)
				Close(fdin);
			if (fdout != STDOUT_FILENO)
				Close(fdout);
			return(1);
		} 
	}

	if (verbose)
		Write(STDERR_FILENO, "\n", 1);

        while ( (nwrite = nread = Read(fdin, buffin, BUFSIZ - 1)) != 0) {
                buffin[nread] = '\0';
                nwrote = 0;
                if (verbose)
                        err_msg("Loop %d: Read: %d, Counter: %d", ++loop, nread, counter);
                buffintmp1 = buffin;
                buffouttmp = buffout;

                /* Compare and change strings */
                while ( (buffintmp2 = strstr(buffintmp1, oldstring)) != NULL) {
                        counter++;
                        nwrote += buffintmp2 - buffintmp1;
                        nwrite -= buffintmp2 - buffintmp1;
                        if ( (size_oldstring != size_newstring) && (nwrote > mem_al - BUFSIZ) ) {
                                buffout = (char *) Realloc(buffout, mem_al + BUFSIZ);
                                mem_al += BUFSIZ;
                        }
                        memcpy(buffouttmp, buffintmp1, buffintmp2 - buffintmp1);                        buffouttmp += buffintmp2 - buffintmp1;
                        nwrote += size_newstring;
                        nwrite -= size_oldstring;
                        if ( (size_oldstring != size_newstring) && (nwrote > mem_al - BUFSIZ) ) {
                                buffout = (char *) Realloc(buffout, mem_al + BUFSIZ);
                                mem_al += BUFSIZ;
                        }
                        memcpy(buffouttmp, newstring, size_newstring);
                        buffouttmp += size_newstring;
                        buffintmp1 = buffintmp2 + size_oldstring;
                }

                memcpy(buffouttmp, buffintmp1, nwrite);
                nwrote += buffin + nread - buffintmp1;

                if (verbose)
                        err_msg("\tWrite: %d, Counter: %d", nwrote, counter);

		if(simulation_mode)
                	Write(fdout, buffin, nread);
		else
                	Write(fdout, buffout, nwrote);

		if (size_oldstring != size_newstring) {
                	buffout = (char *) Realloc(buffout, BUFSIZ * 5);
                	mem_al = BUFSIZ * 5;
		}
        }

	fl.l_type = F_UNLCK;
	if (fcntl(fdout, F_SETLK, &fl) == -1) {
		err_msg("%s\tfcntl error:%s", tmpfile, strerror(errno));
		if (fdin != STDIN_FILENO)
			Close(fdin);
		if (fdout != STDOUT_FILENO)
			Close(fdout);
		return(1);
	} 

	if (fdin != STDIN_FILENO)
		Close(fdin);

	if (fdout != STDOUT_FILENO) {
		Close(fdout);
		if(dont_ch_times) {
			tmputime.actime = tmpstate.st_atime;
			tmputime.modtime = tmpstate.st_mtime;
			Utime(tmpfile, &tmputime);
		}
	}
	
	if(! quiet_mode)
		printf(" %ld", counter);
	fflush(stdout);

	free(buffin);
	free(buffout);

	return 0;
}

int
list(const char *name, const struct stat *status, int type)
{
	char	tmpfile[1024];
	char	*tmp;
	int	fdin, fdout;

        if (type != FTW_F)
                return 0;

	if (strlen(name) > 4) { /* don't work on files.lock */
		tmp = (char *) &name[strlen(name) - 5];
		if (strstr(tmp, ".lock")) {
			err_msg("%s\tfile didn't change! Try -t option!", name);
			return 0;
		}
	}

	strcat(strcpy(tmpfile, name), ".lock");

	if ( (fdout = open(tmpfile, O_RDONLY, PERM)) != -1) {
		Close(fdout);
		tmpfile[strlen(tmpfile)-5] = '\0';
		err_msg("%s\tfile didn't change! %s.lock exists. Try later!", tmpfile, tmpfile);
		return 0;
	}

	Rename(name, tmpfile);

	if(! quiet_mode)
		Write(STDOUT_FILENO, (char *) name, strlen(name));

	if (withoutregexp) {
		if (Copy_ch_file2(tmpfile, name, oldstr2, newstr))
			Rename(tmpfile, name);
	} else {
		if (Copy_ch_file(tmpfile, name, oldstr, newstr))
			Rename(tmpfile, name);
	}

	if(! quiet_mode)
		Write(STDOUT_FILENO, "\n", 1);

 	if(losttmp)
		Unlink(tmpfile);
	
	return 0;

}
 
void
Usage()
{
	printf("This is chgrep, version %s\n", VERSION);
	printf("Copyright 2002-2004, Bartek Krajnik\n");
	printf("See GNU GENERAL PUBLIC LICENSE\n\n");

	printf("Usage: chgrep [OPTION]... OLDPATTERN NEWPATTERN [FILE] ...\n");
	printf("Search for OLDPATTERN in each FILE (or stdin) and change them to NEWPATTERN.\n\n");
	printf("Example: chgrep helloold hellonew filename.c\n\n");
	printf("Regexp selection and interpretation:\n");
	printf("  -d\t\tDon't change modification times of altered files\n");
	printf("  -e\t\tExtended-regexp\n");
	printf("  -h\t\tPrint this help and exit\n");
	printf("  -i\t\tIgnore differences in case at OLDPATTERN\n");
	printf("  -l\t\tLost old file unchanged as filename.lock\n");
	printf("  -n\t\tTreat <newline> as a regular character\n");
	printf("  -q\t\tQuiet mode (no output at all)\n");
	printf("  -r\t\tRecursively\n");
	printf("  -s\t\tSimulation mode\n");
	printf("  -t file.tmp\tUse not default file.lock for caching\n");
	printf("  -v\t\tVerbose mode\n");
	printf("  -w\t\tdoesn't recognize regexp in OLDPATTERN (goes much faster)\n");
	printf("You can write NEWPATTERN as \"NULL\" to clear OLDPATTERN.\n");
	printf("Press Ctrl-C or send SIGINT to stop operation.\n\n");


	exit(0);
}

void
chg_quit(int signal)
{
	if (file_quit != NULL && tmpfile_quit != NULL)
		Rename(tmpfile_quit, file_quit);

	err_quit(" recovered\nOperation stopped. You pressed Ctrl-C or received SIGINT.");

}
