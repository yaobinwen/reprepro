#ifndef REPREPRO_GUESSCOMPONENT_H
#define REPREPRO_GUESSCOMPONENT_H

#include "error.h"
#include "atoms.h"

retvalue guess_component(const char * /*codename*/, const struct atomlist * /*components*/, const char * /*package*/, const char * /*section*/, component_t, /*@out@*/component_t *);

#endif
