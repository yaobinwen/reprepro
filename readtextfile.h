#ifndef REPREPRO_READTEXTFILE
#define REPREPRO_READTEXTFILE

#include "error.h"
#include "globals.h"

retvalue readtextfilefd(int, const char *, /*@out@*/char **, /*@null@*//*@out@*/size_t *);
retvalue readtextfile(const char *, const char *, /*@out@*/char **, /*@null@*//*@out@*/size_t *);

#endif
