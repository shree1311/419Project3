#define _GNU_SOURCE
#include <string.h>
#include <stdarg.h>	// handle variable args in open
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>	// for setting an error code
#include <dlfcn.h>	// for dynamic loading

static int (*real_open) (const char *pathname, int flags, ...);
static struct dirent *(*real_readdir)(DIR *dirp);

// interpose the open call
// open takes an optional third argument - the permissions for creating a file
// we don't care what it's set to but will have to handle the variable # of args
//	int open(const char *pathname, int flags)
//	int open(const char *pathname, int flags, mode_t mode)
//

static int contains_any(const char *str, const char *list){
	if (!list || !str) return 0;

	char *list_copy = strdup(list);
	char *token = strtok(list_copy, ":");

	while (token) {
		if (strstr(str, token)){
			free(list_copy);
			return 1;
		}
		token = strtok(NULL, ":");
	}
	
	free(list_copy);
	return 0;
}
int open(const char *pathname, int flags, ...)
{
	if (!real_open) {
		real_open = dlsym(RTLD_NEXT, "open");
	}

	//need to check if file needs to be blocked
	if (contains_any(pathname, getenv("BLOCKED"))){
		errno = EACCES;
		return -1;
	}
	
	va_list args;
	va_start(args, flags);
	int mode  = va_arg(args, int);
	va_end(args);

	return real_open(pathname, flags, mode);
}

// interpose the readdir call
// This isn't the system call but is used by many programs that need to read directories, 
// like find and ls.

struct dirent *readdir(DIR *dirp)
{
	if (!real_readdir){
		real_readdir = dlsym(RTLD_NEXT, "readdir");
	}

	struct dirent *entry;

	//skip non hidden files
	do{
		entry = real_readdir(dirp);
	} while (entry && contains_any(entry -> d_name, getenv("HIDDEN")));

	return entry;
}
