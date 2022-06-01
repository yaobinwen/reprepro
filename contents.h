#ifndef REPREPRO_CONTENTS_H
#define REPREPRO_CONTENTS_H

#include "strlist.h"
#include "database.h"
#include "release.h"

struct contentsoptions {
	struct {
		bool enabled;
		bool udebs;
		bool nodebs;
		bool percomponent;
		bool allcomponents;
		bool compatsymlink;
	} flags;
	compressionset compressions;
};

struct distribution;
struct configiterator;

retvalue contentsoptions_parse(struct distribution *, struct configiterator *);
retvalue contents_generate(struct distribution *, struct release *, bool /*onlyneeded*/);

#endif
