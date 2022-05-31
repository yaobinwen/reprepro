#ifndef REPREPRO_MAIN_HELPERS_H
#define REPREPRO_MAIN_HELPERS_H

#include <stdbool.h>

void myexit(int) __attribute__((__noreturn__));

void disallow_plus_prefix(const char *dir, const char *name, const char *allowed);

char *expand_plus_prefix(/*@only@*/char *dir, const char *name, const char *allowed, bool freedir);

int callendhook(int status, char *argv[]);

#endif
