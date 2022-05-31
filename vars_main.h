#ifndef REPREPRO_VARS_MAIN_H
#define REPREPRO_VARS_MAIN_H

/* global options */
extern char /*@only@*/ /*@notnull@*/ // *g*
	*x_basedir,
	*x_outdir,
	*x_distdir,
	*x_dbdir,
	*x_listdir,
	*x_confdir,
	*x_logdir,
	*x_morguedir,
	*x_methoddir;

extern char /*@only@*/ /*@null@*/
	*x_section,
	*x_priority,
	*x_component,
	*x_architecture,
	*x_packagetype;

extern char /*@only@*/ /*@null@*/ *listformat;
extern char /*@only@*/ /*@null@*/ *endhook;
extern char /*@only@*/ /*@null@*/ *outhook;
extern char /*@only@*/
	*gunzip,
	*bunzip2,
	*unlzma,
	*unxz,
	*lunzip,
	*gnupghome;

#endif