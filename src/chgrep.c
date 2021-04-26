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
 *      This program changes OLDPATTERN into NEWPATTERN in files or in stdin.
 *      Usage: chgrep [OPTION]... OLDPATTERN NEWPATTERN [FILE] ...
 */

#include	<stdio.h>
#include	<sys/stat.h>
#include	<sys/types.h>
#include	<utime.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<string.h>
#include	<stdarg.h>
#include	<syslog.h>
#include	<errno.h>
#include	<dirent.h>
#include	<signal.h>
#include	<regex.h>
#include	<limits.h>

#include	"error.h"
#include	"wrappers.h"
#include	"functions.h"


#define		MAXLINE	4096
#define		PERM	0666

int		verbose = 0;
int		losttmp = 1;
int		withoutregexp = 0;
int		quiet_mode = 0;
int		simulation_mode = 0;
int		dont_ch_times = 0;
regex_t		*oldstr;
char		*oldstr2;
char		*newstr;
char		*file_quit;
char		*tmpfile_quit;

int
main(int argc, char **argv)
{
	int	list(const char *, const struct stat *, int);
	char	tmpfile[MAXLINE];	/* 4096 */
	char	*tmp;
	int	c, opttmp, fdin, fdout, t, i;
	struct flock	lockf;
	int	istmpfilename = 0, recursive = 0;
	int	regcopt = 0, newline = 0, ignorecase = 0, extendedregexp = 0;	/* regex */
	DIR	*dp;
	regex_t re_old;
	regex_t	re_new;
	char	buffer[MAXLINE];

	newstr = malloc(MAXLINE);
	oldstr2 = malloc(MAXLINE);
	for (i=0; i < MAXLINE; i++)
		newstr[i] = 0;

	opterr=0;
	while( (c = getopt(argc, argv, "dehilnqrst:vw")) != -1) {
		switch(c) {
			case 'd': 
				dont_ch_times = 1;
				break;
			case 'e': 
				extendedregexp = 1;
				break;
			case 'h':
				Usage();
			case 'i': 
				ignorecase = 1;
				break;
			case 'l': 
				losttmp = 0;
				break;
			case 'n': 
				newline = 1;
				break;
			case 'q': 
				quiet_mode = 1;
				break;
			case 'r': 
				recursive = 1;
				break;
			case 's': 
				simulation_mode = 1;
				break;
			case 't': 
				istmpfilename = 1;
				strcpy(tmpfile, optarg);
				break;
			case 'v': 
				verbose = 1;
				break;
			case 'w':
				withoutregexp = 1;
				break;
			case '?':
				err_msg("?? wrong option ??\n");
				Usage();
			default:
				err_quit("?? getopt returned character code 0%o ??\n", c);
		}
	}

	if (argc == 1 || (argc-optind) < 2)
		Usage();

	if (withoutregexp && extendedregexp || withoutregexp && newline)
		err_quit("Options -w is incompatible with -e and/or -n !");

	if (strlen(argv[optind]) > MAXLINE || strlen(argv[optind+1]) > MAXLINE)
		err_quit("Use not so long PATTERN !");

	Signal(SIGINT, chg_quit);

	if (ignorecase)
		regcopt |= REG_ICASE;

	if (newline)
		regcopt |= REG_NEWLINE;

	if(extendedregexp)
		regcopt |= REG_EXTENDED;

	if(! withoutregexp) {
	  if ((t=regcomp( &re_old, argv[optind], regcopt)) != 0) {
		regerror(t, &re_old, buffer, sizeof buffer);
		err_sys("Error in regular expression: %s (%s)\n", buffer, argv[optind]);
	  }
	  oldstr = &re_old;
	} else
	  strcpy(oldstr2, argv[optind]);

	if(strcmp(argv[optind+1], "NULL") == 0)
		newstr[0] = 0;
	else
		strcpy(newstr, argv[optind+1]);

	if ( (argc-optind) == 2) {	/* From STDIN to STDOUT */
		if (verbose)
			Write(STDOUT_FILENO, "STDIN\n", 6);

		if(withoutregexp)
			Copy_ch_file2("stdin", "stdout", oldstr2, newstr);
		else
			Copy_ch_file("stdin", "stdout", &re_old, newstr);

		if (verbose)
			Write(STDOUT_FILENO, "\n", 1);

		return 0;
	}

	opttmp = optind+2;
	while (opttmp < argc) {
		file_quit = NULL;
		tmpfile_quit = NULL;

		if (recursive) {
			ftw(argv[opttmp], list, sysconf(_SC_OPEN_MAX));
			return 0;
		}

		if ( ! istmpfilename) {	/* if we don't use -t filename.tmp */
			strcat(strcpy(tmpfile, argv[opttmp]), ".lock");
			if ( (fdout = open(tmpfile, O_RDONLY, PERM)) != -1) {
				err_msg("%s\tfile didn't change! %s exists. Try later!", argv[opttmp], tmpfile);
				opttmp++;
				continue;
			}

			if (strlen(argv[opttmp]) > 4) { /* don't work on files.lock */
				tmp = argv[opttmp];
				tmp += strlen(argv[opttmp]) - 5;
				if (strstr(tmp, ".lock")) {
					opttmp++;
					continue;
				}
			}

		}

		if ( (dp = opendir(argv[opttmp])) != NULL) {
			err_msg("%s\tis directory", argv[opttmp]);
			closedir(dp);
			opttmp++;
			continue;
		}

		Rename(argv[opttmp], tmpfile);
		if(! quiet_mode)
			Write(STDOUT_FILENO, argv[opttmp], strlen(argv[opttmp]));

		if(withoutregexp) {
			if (Copy_ch_file2(tmpfile, argv[opttmp], oldstr2, newstr))
				Rename(tmpfile, argv[opttmp]);
		} else {
			if (Copy_ch_file(tmpfile, argv[opttmp], &re_old, newstr))
				Rename(tmpfile, argv[opttmp]);
		}

		if(! quiet_mode)
			Write(STDOUT_FILENO, "\n", 1);

		if(losttmp)
			Unlink(tmpfile);

		opttmp++;
	}

	return 0;
}
