#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "filecntl.h"
#include "hooks.h"
#include "main_helpers.h"
#include "names.h"
#include "pool.h"
#include "vars_main.h"

void myexit(int status) {
	free(x_dbdir);
	free(x_distdir);
	free(x_listdir);
	free(x_logdir);
	free(x_confdir);
	free(x_basedir);
	free(x_outdir);
	free(x_methoddir);
	free(x_component);
	free(x_architecture);
	free(x_packagetype);
	free(x_section);
	free(x_priority);
	free(x_morguedir);
	free(gnupghome);
	free(endhook);
	free(outhook);
	pool_free();
	exit(status);
}

void disallow_plus_prefix(const char *dir, const char *name, const char *allowed) {
	if (dir[0] != '+')
		return;
	if (dir[1] == '\0' || dir[2] != '/') {
		fprintf(stderr,
"Error: %s starts with +, but does not continue with '+b/'.\n",
				name);
		myexit(EXIT_FAILURE);
	}
	if (strchr(allowed, dir[1]) != NULL)
		return;
	fprintf(stderr, "Error: %s is not allowed to start with '+%c/'.\n"
"(if your directory is named like that, set it to './+%c/')\n",
			name, dir[1], dir[1]);
	myexit(EXIT_FAILURE);
}

char *expand_plus_prefix(/*@only@*/char *dir, const char *name, const char *allowed, bool freedir) {
	const char *fromdir;
	char *newdir;

	disallow_plus_prefix(dir, name, allowed);

	if (dir[0] == '/' || (dir[0] == '.' && dir[1] == '/'))
		return dir;
	if (dir[0] != '+') {
		fprintf(stderr,
"Warning: %s '%s'  does not start with '/', './', or '+'.\n"
"This currently means it is relative to the current working directory,\n"
"but that might change in the future or cause an error instead!\n",
				name, dir);
		return dir;
	}
	if (dir[1] == 'b') {
		fromdir = x_basedir;
	} else if (dir[1] == 'o') {
		fromdir = x_outdir;
	} else if (dir[1] == 'c') {
		fromdir = x_confdir;
	} else {
		abort();
		return dir;
	}
	if (dir[3] == '\0')
		newdir = strdup(fromdir);
	else
		newdir = calc_dirconcat(fromdir, dir + 3);
	if (FAILEDTOALLOC(newdir)) {
		(void)fputs("Out of Memory!\n", stderr);
		exit(EXIT_FAILURE);
	}
	if (freedir)
		free(dir);
	return newdir;
}

int callendhook(int status, char *argv[]) {
	char exitcode[4];

	/* Try to close all open fd but 0,1,2 */
	closefrom(3);

	if (snprintf(exitcode, 4, "%u", ((unsigned int)status)&255U) > 3)
		memcpy(exitcode, "255", 4);
	sethookenvironment(causingfile, NULL, NULL, exitcode);
	argv[0] = endhook,
	(void)execv(endhook, argv);
	fprintf(stderr, "Error executing '%s': %s\n", endhook,
				strerror(errno));
	return EXIT_RET(RET_ERROR);
}
