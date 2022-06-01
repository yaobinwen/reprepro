#ifndef REPREPRO_CHECKINDSC_H
#define REPREPRO_CHECKINDSC_H

#include "error.h"
#include "database.h"
#include "distribution.h"
#include "sources.h"

/* insert the given .dsc into the mirror in <component> in the <distribution>
 * if component is NULL, guess it from the section. */
retvalue dsc_add(component_t, /*@null@*/const char * /*forcesection*/, /*@null@*/const char * /*forcepriority*/, struct distribution *, const char * /*dscfilename*/, int /*delete*/, /*@null@*/trackingdb);

/* in two steps:
 * If basename, filekey and directory are != NULL, then they are used instead
 * of being newly calculated.
 * (And all files are expected to already be in the pool),
 * delete should be D_INPLACE then
 */

retvalue dsc_addprepared(const struct dsc_headers *, component_t, const struct strlist * /*filekeys*/, struct distribution *, /*@null@*/struct trackingdata *);

#endif
