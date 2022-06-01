#ifndef REPREPRO_DOWNLOADLIST_H
#define REPREPRO_DOWNLOADLIST_H

#include "error.h"
#include "strlist.h"
#include "database.h"
#include "aptmethod.h"
#include "checksums.h"
#include "freespace.h"

struct downloaditem;

struct downloadcache {
	/*@null@*/struct downloaditem *items;
	/*@null@*/struct devices *devices;

	/* for showing what percentage was downloaded */
	long long size_todo, size_done;
	unsigned int last_percent;
};

/* Initialize a new download session */
retvalue downloadcache_initialize(enum spacecheckmode, off_t /*reserveddb*/, off_t /*reservedother*/, /*@out@*/struct downloadcache **);

/* free all memory */
retvalue downloadcache_free(/*@null@*//*@only@*/struct downloadcache *);

/* queue a new file to be downloaded:
 * results in RET_ERROR_WRONG_MD5, if someone else already asked
 * for the same destination with other md5sum created. */
retvalue downloadcache_add(struct downloadcache *, struct aptmethod *, const char * /*orig*/, const char * /*filekey*/, const struct checksums *);

/* some as above, only for more files... */
retvalue downloadcache_addfiles(struct downloadcache *, struct aptmethod *, const struct checksumsarray * /*origfiles*/, const struct strlist * /*filekeys*/);
#endif
