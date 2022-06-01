#ifndef REPREPRO_INDEXFILE_H
#define REPREPRO_INDEXFILE_H

#include "error.h"
#include "target.h"

struct indexfile;
struct package;

retvalue indexfile_open(/*@out@*/struct indexfile **, const char *, enum compression);
retvalue indexfile_close(/*@only@*/struct indexfile *);
bool indexfile_getnext(struct indexfile *, /*@out@*/struct package *, struct target *, bool allowwrongarchitecture);

#endif
