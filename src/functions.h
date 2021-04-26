int Copy_ch_file(const char *file, const char *tmpfile, regex_t *oldstring, char *newstring);

int Copy_ch_file2(const char *file, const char *tmpfile, char *oldstring, char *newstring);

int list(const char *name, const struct stat *status, int type);
 
void Usage();

void chg_quit(int signal);
