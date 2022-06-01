#ifndef REPREPRO_CHECKIN_H
#define REPREPRO_CHECKIN_H

#include "error.h"
#include "database.h"
#include "distribution.h"
#include "atoms.h"

/* insert the given .changes into the mirror in the <distribution>
 * if forcecomponent, forcesection or forcepriority is NULL
 * get it from the files or try to guess it.
 * if dereferencedfilekeys is != NULL, add filekeys that lost reference,
 * if tracks != NULL, update/add tracking information there... */
retvalue changes_add(/*@null@*/trackingdb, const struct atomlist * /*packagetypes*/, component_t, const struct atomlist * /*forcearchitecture*/, /*@null@*/const char * /*forcesection*/, /*@null@*/const char * /*forcepriority*/, struct distribution *, const char * /*changesfilename*/, int /*delete*/);

#endif

