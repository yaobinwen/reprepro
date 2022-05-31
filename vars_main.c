#include <fcntl.h>
#include <stdbool.h>
#include <string.h>
#include "config_option_owner.h"
#include "distribution.h"
#include "files.h"
#include "freespace.h"
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

int 	listmax = -1;
int 	listskip = 0;
int	delete = D_COPY;
bool	nothingiserror = false;
bool	nolistsdownload = false;
bool	keepunreferenced = false;
bool	keepunusednew = false;
bool	askforpassphrase = false;
bool	guessgpgtty = true;
bool	skipold = true;
size_t   waitforlock = 0;
enum exportwhen export = EXPORT_CHANGED;
int		verbose = 0;
bool	fast = false;
bool	verbosedatabase = false;
enum spacecheckmode spacecheckmode = scm_FULL;
/* default: 100 MB for database to grow */
off_t reserveddbspace = 1024*1024*100
/* 1MB safety margin for other filesystems */;
off_t reservedotherspace = 1024*1024;

/* define for each config value an owner, and only higher owners are allowed
 * to change something owned by lower owners. */
enum config_option_owner config_state,
#define O(x) owner_ ## x = CONFIG_OWNER_DEFAULT
O(fast), O(x_morguedir), O(x_outdir), O(x_basedir), O(x_distdir), O(x_dbdir), O(x_listdir), O(x_confdir), O(x_logdir), O(x_methoddir), O(x_section), O(x_priority), O(x_component), O(x_architecture), O(x_packagetype), O(nothingiserror), O(nolistsdownload), O(keepunusednew), O(keepunreferenced), O(keeptemporaries), O(keepdirectories), O(askforpassphrase), O(skipold), O(export), O(waitforlock), O(spacecheckmode), O(reserveddbspace), O(reservedotherspace), O(guessgpgtty), O(verbosedatabase), O(gunzip), O(bunzip2), O(unlzma), O(unxz), O(lunzip), O(gnupghome), O(listformat), O(listmax), O(listskip), O(onlysmalldeletes), O(endhook), O(outhook);
#undef O
