#ifndef REPREPRO_INCOMING_H
#define REPREPRO_INCOMING_H

#include "error.h"

retvalue process_incoming(struct distribution *distributions, const char *name, /*@null@*/const char *onlychangesfilename);
#endif
