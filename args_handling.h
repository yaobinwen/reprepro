#ifndef REPREPRO_ARGS_HANDLING_H
#define REPREPRO_ARGS_HANDLING_H

#include "atoms.h"
#include "distribution.h"
#include "retvalue.h"

#define NEED_REFERENCES 1
/* FILESDB now includes REFERENCED... */
#define NEED_FILESDB 2
#define NEED_DEREF 4
#define NEED_DATABASE 8
#define NEED_CONFIG 16
#define NEED_NO_PACKAGES 32
#define IS_RO 64
#define MAY_UNUSED 128
#define NEED_ACT 256
#define NEED_SP 512
#define NEED_DELNEW 1024
#define NEED_RESTRICT 2048

struct action {
	const char *name;
	retvalue (*start)(
			/*@null@*/struct distribution *,
			/*@null@*/const char *priority,
			/*@null@*/const char *section,
			/*@null@*/const struct atomlist *,
			/*@null@*/const struct atomlist *,
			/*@null@*/const struct atomlist *,
			int argc, const char *argv[]);
	int needs;
	int minargs, maxargs;
	const char *wrongargmessage;
};

extern const struct action all_actions[];

retvalue callaction(command_t command, const struct action *action, int argc, const char *argv[]);

void setexport(const char *argument);

unsigned long long parse_number(const char *name, const char *argument, long long max);

void handle_option(int c, const char *argument);

#endif
