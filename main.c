/*  This file is part of "reprepro"
 *  Copyright (C) 2003,2004,2005,2006,2007,2008,2009,2011,2012,2016 Bernhard R. Link
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02111-1301  USA
 */
#include <config.h>

#include <errno.h>
#include <assert.h>
#include <limits.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <strings.h>
#include <fcntl.h>
#include <signal.h>
#include "error.h"
#include "action_handlers.h"
#include "config_option_owner.h"
#include "downloadcache.h"
#include "filecntl.h"
#include "files.h"
#include "filterlist.h"
#include "global_config.h"
#include "ignore.h"
#include "interrupt.h"
#include "log.h"
#include "macros_config.h"
#include "macros_std.h"
#include "macros_llong_max.h"
#include "main_helpers.h"
#include "signature.h"
#include "termdecide.h"
#include "optionsfile.h"
#include "outhook.h"
#include "package_info.h"
#include "pool.h"
#include "uncompression.h"
#include "vars_main.h"

#include "args_handling.h"
#include "enum_longopts.h"

const char * package_name(void) {
	return PACKAGE;
}

const char * package_version(void) {
	return VERSION;
}

int main(int argc, char *argv[]) {
	static struct option longopts[] = {
		{"delete", no_argument, &longoption, LO_DELETE},
		{"nodelete", no_argument, &longoption, LO_NODELETE},
		{"basedir", required_argument, NULL, 'b'},
		{"ignore", required_argument, NULL, 'i'},
		{"unignore", required_argument, &longoption, LO_UNIGNORE},
		{"noignore", required_argument, &longoption, LO_UNIGNORE},
		{"methoddir", required_argument, &longoption, LO_METHODDIR},
		{"outdir", required_argument, &longoption, LO_OUTDIR},
		{"distdir", required_argument, &longoption, LO_DISTDIR},
		{"dbdir", required_argument, &longoption, LO_DBDIR},
		{"listdir", required_argument, &longoption, LO_LISTDIR},
		{"confdir", required_argument, &longoption, LO_CONFDIR},
		{"logdir", required_argument, &longoption, LO_LOGDIR},
		{"section", required_argument, NULL, 'S'},
		{"priority", required_argument, NULL, 'P'},
		{"component", required_argument, NULL, 'C'},
		{"architecture", required_argument, NULL, 'A'},
		{"type", required_argument, NULL, 'T'},
		{"help", no_argument, NULL, 'h'},
		{"verbose", no_argument, NULL, 'v'},
		{"silent", no_argument, NULL, 's'},
		{"version", no_argument, &longoption, LO_VERSION},
		{"nothingiserror", no_argument, &longoption, LO_NOTHINGISERROR},
		{"nolistsdownload", no_argument, &longoption, LO_NOLISTDOWNLOAD},
		{"keepunreferencedfiles", no_argument, &longoption, LO_KEEPUNREFERENCED},
		{"keepunusednewfiles", no_argument, &longoption, LO_KEEPUNUSEDNEW},
		{"keepunneededlists", no_argument, &longoption, LO_KEEPUNNEEDEDLISTS},
		{"onlysmalldeletes", no_argument, &longoption, LO_ONLYSMALLDELETES},
		{"keepdirectories", no_argument, &longoption, LO_KEEPDIRECTORIES},
		{"keeptemporaries", no_argument, &longoption, LO_KEEPTEMPORARIES},
		{"ask-passphrase", no_argument, &longoption, LO_ASKPASSPHRASE},
		{"nonothingiserror", no_argument, &longoption, LO_NONOTHINGISERROR},
		{"nonolistsdownload", no_argument, &longoption, LO_LISTDOWNLOAD},
		{"listsdownload", no_argument, &longoption, LO_LISTDOWNLOAD},
		{"nokeepunreferencedfiles", no_argument, &longoption, LO_NOKEEPUNREFERENCED},
		{"nokeepunusednewfiles", no_argument, &longoption, LO_NOKEEPUNUSEDNEW},
		{"nokeepunneededlists", no_argument, &longoption, LO_NOKEEPUNNEEDEDLISTS},
		{"noonlysmalldeletes", no_argument, &longoption, LO_NOONLYSMALLDELETES},
		{"nokeepdirectories", no_argument, &longoption, LO_NOKEEPDIRECTORIES},
		{"nokeeptemporaries", no_argument, &longoption, LO_NOKEEPTEMPORARIES},
		{"noask-passphrase", no_argument, &longoption, LO_NOASKPASSPHRASE},
		{"guessgpgtty", no_argument, &longoption, LO_GUESSGPGTTY},
		{"noguessgpgtty", no_argument, &longoption, LO_NOGUESSGPGTTY},
		{"nonoguessgpgtty", no_argument, &longoption, LO_GUESSGPGTTY},
		{"fast", no_argument, &longoption, LO_FAST},
		{"nofast", no_argument, &longoption, LO_NOFAST},
		{"verbosedb", no_argument, &longoption, LO_VERBOSEDB},
		{"noverbosedb", no_argument, &longoption, LO_NOVERBOSEDB},
		{"verbosedatabase", no_argument, &longoption, LO_VERBOSEDB},
		{"noverbosedatabase", no_argument, &longoption, LO_NOVERBOSEDB},
		{"skipold", no_argument, &longoption, LO_SKIPOLD},
		{"noskipold", no_argument, &longoption, LO_NOSKIPOLD},
		{"nonoskipold", no_argument, &longoption, LO_SKIPOLD},
		{"force", no_argument, NULL, 'f'},
		{"export", required_argument, &longoption, LO_EXPORT},
		{"waitforlock", required_argument, &longoption, LO_WAITFORLOCK},
		{"checkspace", required_argument, &longoption, LO_SPACECHECK},
		{"spacecheck", required_argument, &longoption, LO_SPACECHECK},
		{"safetymargin", required_argument, &longoption, LO_SAFETYMARGIN},
		{"dbsafetymargin", required_argument, &longoption, LO_DBSAFETYMARGIN},
		{"gunzip", required_argument, &longoption, LO_GUNZIP},
		{"bunzip2", required_argument, &longoption, LO_BUNZIP2},
		{"unlzma", required_argument, &longoption, LO_UNLZMA},
		{"unxz", required_argument, &longoption, LO_UNXZ},
		{"lunzip", required_argument, &longoption, LO_LZIP},
		{"gnupghome", required_argument, &longoption, LO_GNUPGHOME},
		{"list-format", required_argument, &longoption, LO_LISTFORMAT},
		{"list-skip", required_argument, &longoption, LO_LISTSKIP},
		{"list-max", required_argument, &longoption, LO_LISTMAX},
		{"morguedir", required_argument, &longoption, LO_MORGUEDIR},
		{"show-percent", no_argument, &longoption, LO_SHOWPERCENT},
		{"restrict", required_argument, &longoption, LO_RESTRICT_SRC},
		{"restrict-source", required_argument, &longoption, LO_RESTRICT_SRC},
		{"restrict-src", required_argument, &longoption, LO_RESTRICT_SRC},
		{"restrict-binary", required_argument, &longoption, LO_RESTRICT_BIN},
		{"restrict-file", required_argument, &longoption, LO_RESTRICT_FILE_SRC},
		{"restrict-file-source", required_argument, &longoption, LO_RESTRICT_FILE_SRC},
		{"restrict-file-src", required_argument, &longoption, LO_RESTRICT_FILE_SRC},
		{"restrict-file-binary", required_argument, &longoption, LO_RESTRICT_FILE_BIN},
		{"endhook", required_argument, &longoption, LO_ENDHOOK},
		{"outhook", required_argument, &longoption, LO_OUTHOOK},
		{NULL, 0, NULL, 0}
	};
	const struct action *a;
	retvalue r;
	int c;
	struct sigaction sa;
	char *tempconfdir;

	sigemptyset(&sa.sa_mask);
#if defined(SA_ONESHOT)
	sa.sa_flags = SA_ONESHOT;
#elif defined(SA_RESETHAND)
	sa.sa_flags = SA_RESETHAND;
#elif !defined(SPLINT)
#       error "missing argument to sigaction!"
#endif
	sa.sa_handler = g_fn_interrupt_signaled;
	(void)sigaction(SIGTERM, &sa, NULL);
	(void)sigaction(SIGABRT, &sa, NULL);
	(void)sigaction(SIGINT, &sa, NULL);
	(void)sigaction(SIGQUIT, &sa, NULL);

	(void)signal(SIGPIPE, SIG_IGN);

	programname = argv[0];

	config_state = CONFIG_OWNER_DEFAULT;
	CONFIGDUP(x_basedir, STD_BASE_DIR);
	CONFIGDUP(x_confdir, "+b/conf");
	CONFIGDUP(x_methoddir, STD_METHOD_DIR);
	CONFIGDUP(x_outdir, "+b/");
	CONFIGDUP(x_distdir, "+o/dists");
	CONFIGDUP(x_dbdir, "+b/db");
	CONFIGDUP(x_logdir, "+b/logs");
	CONFIGDUP(x_listdir, "+b/lists");

	config_state = CONFIG_OWNER_CMDLINE;
	if (interrupted())
		exit(EXIT_RET(RET_ERROR_INTERRUPTED));

	while ((c = getopt_long(argc, argv, "+fVvshb:P:i:A:C:S:T:", longopts, NULL)) != -1) {
		handle_option(c, optarg);
	}
	if (optind >= argc) {
		fputs(
"No action given. (see --help for available options and actions)\n", stderr);
		exit(EXIT_FAILURE);
	}
	if (interrupted())
		exit(EXIT_RET(RET_ERROR_INTERRUPTED));

	/* only for this CONFIG_OWNER_ENVIRONMENT is a bit stupid,
	 * but perhaps it gets more... */
	config_state = CONFIG_OWNER_ENVIRONMENT;
	if (getenv("REPREPRO_BASE_DIR") != NULL) {
		CONFIGDUP(x_basedir, getenv("REPREPRO_BASE_DIR"));
	}
	if (getenv("REPREPRO_CONFIG_DIR") != NULL) {
		CONFIGDUP(x_confdir, getenv("REPREPRO_CONFIG_DIR"));
	}

	disallow_plus_prefix(x_basedir, "basedir", "");
	tempconfdir = expand_plus_prefix(x_confdir, "confdir", "b", false);

	config_state = CONFIG_OWNER_FILE;
	optionsfile_parse(tempconfdir, longopts, handle_option);
	if (tempconfdir != x_confdir)
		free(tempconfdir);

	disallow_plus_prefix(x_basedir, "basedir", "");
	disallow_plus_prefix(x_methoddir, "methoddir", "");
	x_confdir = expand_plus_prefix(x_confdir, "confdir", "b", true);
	x_outdir = expand_plus_prefix(x_outdir, "outdir", "bc", true);
	x_logdir = expand_plus_prefix(x_logdir, "logdir", "boc", true);
	x_dbdir = expand_plus_prefix(x_dbdir, "dbdir", "boc", true);
	x_distdir = expand_plus_prefix(x_distdir, "distdir", "boc", true);
	x_listdir = expand_plus_prefix(x_listdir, "listdir", "boc", true);
	if (x_morguedir != NULL)
		x_morguedir = expand_plus_prefix(x_morguedir, "morguedir",
				"boc", true);
	if (endhook != NULL) {
		if (endhook[0] == '+' || endhook[0] == '/' ||
				(endhook[0] == '.' && endhook[1] == '/')) {
			endhook = expand_plus_prefix(endhook, "endhook", "boc",
					true);
		} else {
			char *h;

			h = calc_dirconcat(x_confdir, endhook);
			free(endhook);
			endhook = h;
			if (endhook == NULL)
				exit(EXIT_RET(RET_ERROR_OOM));
		}
	}
	if (outhook != NULL) {
		if (outhook[0] == '+' || outhook[0] == '/' ||
				(outhook[0] == '.' && outhook[1] == '/')) {
			outhook = expand_plus_prefix(outhook, "outhook", "boc",
					true);
		} else {
			char *h;

			h = calc_dirconcat(x_confdir, outhook);
			free(outhook);
			outhook = h;
			if (outhook == NULL)
				exit(EXIT_RET(RET_ERROR_OOM));
		}
	}

	if (guessgpgtty && (getenv("GPG_TTY")==NULL) && isatty(0)) {
		static char terminalname[1024];
		ssize_t len;

		len = readlink("/proc/self/fd/0", terminalname, 1023);
		if (len > 0 && len < 1024) {
			terminalname[len] = '\0';
			setenv("GPG_TTY", terminalname, 0);
		} else if (verbose > 10) {
			fprintf(stderr,
"Could not readlink /proc/self/fd/0 (error was %s), not setting GPG_TTY.\n",
					strerror(errno));
		}
	}

	if (delete < D_COPY)
		delete = D_COPY;
	if (interrupted())
		exit(EXIT_RET(RET_ERROR_INTERRUPTED));
	global.basedir = x_basedir;
	global.dbdir = x_dbdir;
	global.outdir = x_outdir;
	global.confdir = x_confdir;
	global.distdir = x_distdir;
	global.logdir = x_logdir;
	global.methoddir = x_methoddir;
	global.listdir = x_listdir;
	global.morguedir = x_morguedir;

	if (gunzip != NULL && gunzip[0] == '+')
		gunzip = expand_plus_prefix(gunzip, "gunzip", "boc", true);
	if (bunzip2 != NULL && bunzip2[0] == '+')
		bunzip2 = expand_plus_prefix(bunzip2, "bunzip2", "boc", true);
	if (unlzma != NULL && unlzma[0] == '+')
		unlzma = expand_plus_prefix(unlzma, "unlzma", "boc", true);
	if (unxz != NULL && unxz[0] == '+')
		unxz = expand_plus_prefix(unxz, "unxz", "boc", true);
	if (lunzip != NULL && lunzip[0] == '+')
		lunzip = expand_plus_prefix(lunzip, "lunzip", "boc", true);
	uncompressions_check(gunzip, bunzip2, unlzma, unxz, lunzip);
	free(gunzip);
	free(bunzip2);
	free(unlzma);
	free(unxz);
	free(lunzip);

	a = all_actions;
	while (a->name != NULL) {
		a++;
	}
	r = atoms_init(a - all_actions);
	if (r == RET_ERROR_OOM)
		(void)fputs("Out of Memory!\n", stderr);
	if (RET_WAS_ERROR(r))
		exit(EXIT_RET(r));
	for (a = all_actions; a->name != NULL ; a++) {
		atoms_commands[1 + (a - all_actions)] = a->name;
	}

	if (gnupghome != NULL) {
		gnupghome = expand_plus_prefix(gnupghome,
				"gnupghome", "boc", true);
		if (setenv("GNUPGHOME", gnupghome, 1) != 0) {
			int e = errno;

			fprintf(stderr,
"Error %d setting GNUPGHOME to '%s': %s\n",
					e, gnupghome, strerror(e));
			myexit(EXIT_FAILURE);
		}
	}

	a = all_actions;
	while (a->name != NULL) {
		if (strcasecmp(a->name, argv[optind]) == 0) {
			signature_init(askforpassphrase);
			r = callaction(1 + (a - all_actions), a,
					argc-optind, (const char**)argv+optind);
			/* yeah, freeing all this stuff before exiting is
			 * stupid, but it makes valgrind logs easier
			 * readable */
			signatures_done();
			free_known_keys();
			if (RET_WAS_ERROR(r)) {
				if (r == RET_ERROR_OOM)
					(void)fputs("Out of Memory!\n", stderr);
				else if (verbose >= 0)
					(void)fputs(
"There have been errors!\n",
						stderr);
			}
			if (endhook != NULL) {
				assert (optind > 0);
				/* only returns upon error: */
				r = callendhook(EXIT_RET(r), argv + optind - 1);
			}
			myexit(EXIT_RET(r));
		} else
			a++;
	}

	fprintf(stderr,
"Unknown action '%s'. (see --help for available options and actions)\n",
			argv[optind]);
	signatures_done();
	myexit(EXIT_FAILURE);
}
