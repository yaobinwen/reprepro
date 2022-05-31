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

extern int 	listmax;
extern int 	listskip;
extern int	delete;
extern bool	nothingiserror;
extern bool	nolistsdownload;
extern bool	keepunreferenced;
extern bool	keepunusednew;
extern bool	askforpassphrase;
extern bool	guessgpgtty;
extern bool	skipold;
extern size_t   waitforlock;
extern enum exportwhen export;
extern int		verbose;
extern bool	fast;
extern bool	verbosedatabase;
extern enum spacecheckmode spacecheckmode;
extern off_t reserveddbspace;
extern off_t reservedotherspace;

/* define for each config value an owner, and only higher owners are allowed
 * to change something owned by lower owners. */
extern enum config_option_owner config_state,
#define O(x) owner_ ## x
O(fast), O(x_morguedir), O(x_outdir), O(x_basedir), O(x_distdir), O(x_dbdir), O(x_listdir), O(x_confdir), O(x_logdir), O(x_methoddir), O(x_section), O(x_priority), O(x_component), O(x_architecture), O(x_packagetype), O(nothingiserror), O(nolistsdownload), O(keepunusednew), O(keepunreferenced), O(keeptemporaries), O(keepdirectories), O(askforpassphrase), O(skipold), O(export), O(waitforlock), O(spacecheckmode), O(reserveddbspace), O(reservedotherspace), O(guessgpgtty), O(verbosedatabase), O(gunzip), O(bunzip2), O(unlzma), O(unxz), O(lunzip), O(gnupghome), O(listformat), O(listmax), O(listskip), O(onlysmalldeletes), O(endhook), O(outhook);
#undef O

extern int longoption;
extern const char *programname;

#endif
