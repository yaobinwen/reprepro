#ifndef REPREPRO_DEBFILE_H
#define REPREPRO_DEBFILE_H

#include "error.h"

/* Read the control information of a .deb file */
retvalue extractcontrol(/*@out@*/char **, const char *);

/* Read a list of files from a .deb file */
retvalue getfilelist(/*@out@*/char **, /*@out@*/ size_t *, const char *);

#endif
