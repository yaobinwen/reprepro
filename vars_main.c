#include <string.h>
#include "vars_main.h"

/* global options */
char /*@only@*/ /*@notnull@*/ // *g*
	*x_basedir = NULL,
	*x_outdir = NULL,
	*x_distdir = NULL,
	*x_dbdir = NULL,
	*x_listdir = NULL,
	*x_confdir = NULL,
	*x_logdir = NULL,
	*x_morguedir = NULL,
	*x_methoddir = NULL;

char /*@only@*/ /*@null@*/
	*x_section = NULL,
	*x_priority = NULL,
	*x_component = NULL,
	*x_architecture = NULL,
	*x_packagetype = NULL;

char /*@only@*/ /*@null@*/ *listformat = NULL;
char /*@only@*/ /*@null@*/ *endhook = NULL;
char /*@only@*/ /*@null@*/ *outhook = NULL;
char /*@only@*/
	*gunzip = NULL,
	*bunzip2 = NULL,
	*unlzma = NULL,
	*unxz = NULL,
	*lunzip = NULL,
	*gnupghome = NULL;