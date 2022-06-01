#ifndef REPREPRO_NEEDBUILD_H
#define REPREPRO_NEEDBUILD_H

#include "atoms.h"
#include "database.h"
#include "distribution.h"

retvalue find_needs_build(struct distribution *, architecture_t, const struct atomlist *, /*@null@*/const char *glob, bool printarch);

#endif
