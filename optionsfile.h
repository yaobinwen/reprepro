#ifndef REPREPRO_OPTIONSFILE_H
#define REPREPRO_OPTIONSFILE_H

#include <getopt.h>

#include "error.h"

void optionsfile_parse(const char * /*directory*/, const struct option *, void handle_option(int, const char *));

#endif /*REPREPRO_OPTIONSFILE_H*/
