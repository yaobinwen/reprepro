#ifndef REPREPRO_GLOBALS_GLOBAL_CONFIG_H
#define REPREPRO_GLOBALS_GLOBAL_CONFIG_H

struct global_config {
	const char *basedir;
	const char *dbdir;
	const char *outdir;
	const char *distdir;
	const char *confdir;
	const char *methoddir;
	const char *logdir;
	const char *listdir;
	const char *morguedir;
	/* flags: */
	bool keepdirectories;
	bool keeptemporaries;
	bool onlysmalldeletes;
	/* verbosity of downloading statistics */
	int showdownloadpercent;
};

#endif