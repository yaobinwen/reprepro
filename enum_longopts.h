#ifndef REPREPRO_ENUM_LONGOPTS_H
#define REPREPRO_ENUM_LONGOPTS_H

enum {
	LO_DELETE=1,
	LO_KEEPUNREFERENCED,
	LO_KEEPUNUSEDNEW,
	LO_KEEPUNNEEDEDLISTS,
	LO_NOTHINGISERROR,
	LO_NOLISTDOWNLOAD,
	LO_ASKPASSPHRASE,
	LO_ONLYSMALLDELETES,
	LO_KEEPDIRECTORIES,
	LO_KEEPTEMPORARIES,
	LO_FAST,
	LO_SKIPOLD,
	LO_GUESSGPGTTY,
	LO_NODELETE,
	LO_NOKEEPUNREFERENCED,
	LO_NOKEEPUNUSEDNEW,
	LO_NOKEEPUNNEEDEDLISTS,
	LO_NONOTHINGISERROR,
	LO_LISTDOWNLOAD,
	LO_NOASKPASSPHRASE,
	LO_NOONLYSMALLDELETES,
	LO_NOKEEPDIRECTORIES,
	LO_NOKEEPTEMPORARIES,
	LO_NOFAST,
	LO_NOSKIPOLD,
	LO_NOGUESSGPGTTY,
	LO_VERBOSEDB,
	LO_NOVERBOSEDB,
	LO_EXPORT,
	LO_OUTDIR,
	LO_DISTDIR,
	LO_DBDIR,
	LO_LOGDIR,
	LO_LISTDIR,
	LO_OVERRIDEDIR,
	LO_CONFDIR,
	LO_METHODDIR,
	LO_VERSION,
	LO_WAITFORLOCK,
	LO_SPACECHECK,
	LO_SAFETYMARGIN,
	LO_DBSAFETYMARGIN,
	LO_GUNZIP,
	LO_BUNZIP2,
	LO_UNLZMA,
	LO_UNXZ,
	LO_LZIP,
	LO_GNUPGHOME,
	LO_LISTFORMAT,
	LO_LISTSKIP,
	LO_LISTMAX,
	LO_MORGUEDIR,
	LO_SHOWPERCENT,
	LO_RESTRICT_BIN,
	LO_RESTRICT_SRC,
	LO_RESTRICT_FILE_BIN,
	LO_RESTRICT_FILE_SRC,
	LO_ENDHOOK,
	LO_OUTHOOK,
	LO_UNIGNORE
};

#endif