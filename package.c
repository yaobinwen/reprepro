#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "package.h"
#include "target.h"

void package_done(struct package *pkg) {
	free(pkg->pkgname);
	free(pkg->pkgchunk);
	free(pkg->pkgversion);
	free(pkg->pkgsource);
	free(pkg->pkgsrcversion);
	memset(pkg, 0, sizeof(*pkg));
}

char *package_dupversion(struct package *package) {
	assert (package->version != NULL);
	if (package->pkgversion == NULL)
		return strdup(package->version);
	else {
		// steal version from package
		// (caller must ensure it is not freed while still needed)
		char *v = package->pkgversion;
		package->pkgversion = NULL;
		return v;
	}
}

retvalue package_newcontrol_by_cursor(struct package_cursor *cursor, const char *newcontrol, size_t newcontrollen) {
	return cursor_replace(cursor->target->packages, cursor->cursor,
			newcontrol, newcontrollen);
}
