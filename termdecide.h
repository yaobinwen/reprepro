#ifndef REPREPRO_TERMDECIDE_H
#define REPREPRO_TERMDECIDE_H

#include "terms.h"
#include "target.h"
#include "package.h"

retvalue term_compilefortargetdecision(/*@out@*/term **, const char *);
retvalue term_decidepackage(const term *, struct package *, struct target *);



#endif
