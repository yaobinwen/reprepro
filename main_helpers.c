#include <stdlib.h>
#include "main_helpers.h"
#include "pool.h"
#include "vars_main.h"

void myexit(int status) {
	free(x_dbdir);
	free(x_distdir);
	free(x_listdir);
	free(x_logdir);
	free(x_confdir);
	free(x_basedir);
	free(x_outdir);
	free(x_methoddir);
	free(x_component);
	free(x_architecture);
	free(x_packagetype);
	free(x_section);
	free(x_priority);
	free(x_morguedir);
	free(gnupghome);
	free(endhook);
	free(outhook);
	pool_free();
	exit(status);
}
