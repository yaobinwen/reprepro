#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#include "args_handling.h"
#include "action_handlers.h"
#include "enum_longopts.h"
#include "filterlist.h"
#include "freespace.h"
#include "hooks.h"
#include "ignore.h"
#include "log.h"
#include "macros_config.h"
#include "macros_llong_max.h"
#include "outhook.h"
#include "package_info.h"
#include "pool.h"
#include "var_global.h"
#include "vars_main.h"

// TODO: this has become an utter mess and needs some serious cleaning...
#define A_N(w) action_n_n_n_ ## w, 0
#define A_C(w) action_c_n_n_ ## w, NEED_CONFIG
#define A_ROB(w) action_b_n_n_ ## w, NEED_DATABASE|IS_RO
#define A_ROBact(w) action_b_y_n_ ## w, NEED_ACT|NEED_DATABASE|IS_RO
#define A_L(w) action_l_n_n_ ## w, NEED_DATABASE
#define A_B(w) action_b_n_n_ ## w, NEED_DATABASE
#define A_Bact(w) action_b_y_n_ ## w, NEED_ACT|NEED_DATABASE
#define A_F(w) action_f_n_n_ ## w, NEED_DATABASE|NEED_FILESDB
#define A_Fact(w) action_f_y_n_ ## w, NEED_ACT|NEED_DATABASE|NEED_FILESDB
#define A_R(w) action_r_n_n_ ## w, NEED_DATABASE|NEED_REFERENCES
#define A__F(w) action_f_n_n_ ## w, NEED_DATABASE|NEED_FILESDB|NEED_NO_PACKAGES
#define A__R(w) action_r_n_n_ ## w, NEED_DATABASE|NEED_REFERENCES|NEED_NO_PACKAGES
#define A__T(w) action_t_n_n_ ## w, NEED_DATABASE|NEED_NO_PACKAGES|MAY_UNUSED
#define A_RF(w) action_rf_n_n_ ## w, NEED_DATABASE|NEED_FILESDB|NEED_REFERENCES
#define A_RFact(w) action_rf_y_n_ ## w, NEED_ACT|NEED_DATABASE|NEED_FILESDB|NEED_REFERENCES
/* to dereference files, one needs files and references database: */
#define A_D(w) action_d_n_n_ ## w, NEED_DATABASE|NEED_FILESDB|NEED_REFERENCES|NEED_DEREF
#define A_Dact(w) action_d_y_n_ ## w, NEED_ACT|NEED_DATABASE|NEED_FILESDB|NEED_REFERENCES|NEED_DEREF
#define A_Dactsp(w) action_d_y_y_ ## w, NEED_ACT|NEED_SP|NEED_DATABASE|NEED_FILESDB|NEED_REFERENCES|NEED_DEREF

const struct action all_actions[] = {
	{"__d", 		A_N(printargs),
		-1, -1, NULL},
	{"__dumpuncompressors",	A_N(dumpuncompressors),
		0, 0, "__dumpuncompressors"},
	{"__uncompress",	A_N(uncompress),
		3, 3, "__uncompress .gz|.bz2|.lzma|.xz|.lz <compressed-filename> <into-filename>"},
	{"__extractsourcesection", A_N(extractsourcesection),
		1, 1, "__extractsourcesection <.dsc-file>"},
	{"__extractcontrol",	A_N(extractcontrol),
		1, 1, "__extractcontrol <.deb-file>"},
	{"__extractfilelist",	A_N(extractfilelist),
		1, 1, "__extractfilelist <.deb-file>"},
	{"__checkuploaders",	A_C(checkuploaders),
		1, -1,	"__checkuploaders <codenames>"},
	{"_versioncompare",	A_N(versioncompare),
		2, 2, "_versioncompare <version> <version>"},
	{"_detect", 		A__F(detect),
		-1, -1, NULL},
	{"_forget", 		A__F(forget),
		-1, -1, NULL},
	{"_listmd5sums",	A__F(listmd5sums),
		0, 0, "_listmd5sums"},
	{"_listchecksums",	A__F(listchecksums),
		0, 0, "_listchecksums"},
	{"_addchecksums",	A__F(addmd5sums),
		0, 0, "_addchecksums < data"},
	{"_addmd5sums",		A__F(addmd5sums),
		0, 0, "_addmd5sums < data"},
	{"_dumpcontents", 	A_ROB(dumpcontents)|MAY_UNUSED,
		1, 1, "_dumpcontents <identifier>"},
	{"_removereferences", 	A__R(removereferences),
		1, 1, "_removereferences <identifier>"},
	{"_removereference", 	A__R(removereference),
		2, 2, "_removereferences <identifier>"},
	{"_addreference", 	A__R(addreference),
		2, 2, "_addreference <reference> <referee>"},
	{"_addreferences", 	A__R(addreferences),
		1, -1, "_addreferences <referee> <references>"},
	{"_fakeemptyfilelist",	A__F(fakeemptyfilelist),
		1, 1, "_fakeemptyfilelist <filekey>"},
	{"_addpackage",		A_Dact(addpackage),
		3, -1, "-C <component> -A <architecture> -T <packagetype> _addpackage <distribution> <filename> <package-names>"},
	{"remove", 		A_Dact(remove),
		2, -1, "[-C <component>] [-A <architecture>] [-T <type>] remove <codename> <package-names>"},
	{"removesrc", 		A_D(removesrc),
		2, 3, "removesrc <codename> <source-package-names> [<source-version>]"},
	{"removesrcs", 		A_D(removesrcs),
		2, -1, "removesrcs <codename> (<source-package-name>[=<source-version>])+"},
	{"ls", 		A_ROBact(ls),
		1, 1, "[-C <component>] [-A <architecture>] [-T <type>] ls <package-name>"},
	{"lsbycomponent",	A_ROBact(lsbycomponent),
		1, 1, "[-C <component>] [-A <architecture>] [-T <type>] lsbycomponent <package-name>"},
	{"list", 		A_ROBact(list),
		1, 2, "[-C <component>] [-A <architecture>] [-T <type>] list <codename> [<package-name>]"},
	{"listfilter", 		A_ROBact(listfilter),
		2, 2, "[-C <component>] [-A <architecture>] [-T <type>] listfilter <codename> <term to describe which packages to list>"},
	{"removefilter", 	A_Dact(removefilter),
		2, 2, "[-C <component>] [-A <architecture>] [-T <type>] removefilter <codename> <term to describe which packages to remove>"},
	{"listmatched", 	A_ROBact(listmatched),
		2, 2, "[-C <component>] [-A <architecture>] [-T <type>] listmatched <codename> <glob to describe packages>"},
	{"removematched", 	A_Dact(removematched),
		2, 2, "[-C <component>] [-A <architecture>] [-T <type>] removematched <codename> <glob to describe packages>"},
	{"createsymlinks", 	A_C(createsymlinks),
		0, -1, "createsymlinks [<distributions>]"},
	{"export", 		A_F(export),
		0, -1, "export [<distributions>]"},
	{"check", 		A_RFact(check),
		0, -1, "check [<distributions>]"},
	{"sizes", 		A_RF(sizes),
		0, -1, "check [<distributions>]"},
	{"reoverride", 		A_Fact(reoverride),
		0, -1, "[-T ...] [-C ...] [-A ...] reoverride [<distributions>]"},
	{"repairdescriptions", 	A_Fact(repairdescriptions),
		0, -1, "[-C ...] [-A ...] repairdescriptions [<distributions>]"},
	{"forcerepairdescriptions", 	A_Fact(repairdescriptions),
		0, -1, "[-C ...] [-A ...] [force]repairdescriptions [<distributions>]"},
	{"redochecksums", 	A_Fact(redochecksums),
		0, -1, "[-T ...] [-C ...] [-A ...] redo [<distributions>]"},
	{"collectnewchecksums", A_F(collectnewchecksums),
		0, 0, "collectnewchecksums"},
	{"checkpool", 		A_F(checkpool),
		0, 1, "checkpool [fast]"},
	{"rereference", 	A_R(rereference),
		0, -1, "rereference [<distributions>]"},
	{"dumpreferences", 	A_R(dumpreferences)|MAY_UNUSED,
		0, 0, "dumpreferences", },
	{"dumpunreferenced", 	A_RF(dumpunreferenced),
		0, 0, "dumpunreferenced", },
	{"deleteifunreferenced", A_RF(deleteifunreferenced),
		0, -1, "deleteifunreferenced"},
	{"deleteunreferenced", 	A_RF(deleteunreferenced),
		0, 0, "deleteunreferenced", },
	{"retrack",	 	A_D(retrack),
		0, -1, "retrack [<distributions>]"},
	{"dumptracks",	 	A_ROB(dumptracks)|MAY_UNUSED,
		0, -1, "dumptracks [<distributions>]"},
	{"removealltracks",	A_D(removealltracks)|MAY_UNUSED,
		1, -1, "removealltracks <distributions>"},
	{"tidytracks",		A_D(tidytracks),
		0, -1, "tidytracks [<distributions>]"},
	{"removetrack",		A_D(removetrack),
		3, 3, "removetrack <distribution> <sourcename> <version>"},
	{"update",		A_Dact(update)|NEED_RESTRICT,
		0, -1, "update [<distributions>]"},
	{"checkupdate",		A_Bact(checkupdate)|NEED_RESTRICT,
		0, -1, "checkupdate [<distributions>]"},
	{"dumpupdate",		A_Bact(dumpupdate)|NEED_RESTRICT,
		0, -1, "dumpupdate [<distributions>]"},
	{"predelete",		A_Dact(predelete),
		0, -1, "predelete [<distributions>]"},
	{"pull",		A_Dact(pull)|NEED_RESTRICT,
		0, -1, "pull [<distributions>]"},
	{"copy",		A_Dact(copy),
		3, -1, "[-C <component> ] [-A <architecture>] [-T <packagetype>] copy <destination-distribution> <source-distribution> <package-names to pull>"},
	{"copysrc",		A_Dact(copysrc),
		3, -1, "[-C <component> ] [-A <architecture>] [-T <packagetype>] copysrc <destination-distribution> <source-distribution> <source-package-name> [<source versions>]"},
	{"copymatched",		A_Dact(copymatched),
		3, 3, "[-C <component> ] [-A <architecture>] [-T <packagetype>] copymatched <destination-distribution> <source-distribution> <glob>"},
	{"copyfilter",		A_Dact(copyfilter),
		3, 3, "[-C <component> ] [-A <architecture>] [-T <packagetype>] copyfilter <destination-distribution> <source-distribution> <formula>"},
	{"restore",		A_Dact(restore),
		3, -1, "[-C <component> ] [-A <architecture>] [-T <packagetype>] restore <distribution> <snapshot-name> <package-names to restore>"},
	{"restoresrc",		A_Dact(restoresrc),
		3, -1, "[-C <component> ] [-A <architecture>] [-T <packagetype>] restoresrc <distribution> <snapshot-name> <source-package-name> [<source versions>]"},
	{"restorematched",		A_Dact(restorematched),
		3, 3, "[-C <component> ] [-A <architecture>] [-T <packagetype>] restorematched <distribution> <snapshot-name> <glob>"},
	{"restorefilter",		A_Dact(restorefilter),
		3, 3, "[-C <component> ] [-A <architecture>] [-T <packagetype>] restorefilter <distribution> <snapshot-name> <formula>"},
	{"dumppull",		A_Bact(dumppull)|NEED_RESTRICT,
		0, -1, "dumppull [<distributions>]"},
	{"checkpull",		A_Bact(checkpull)|NEED_RESTRICT,
		0, -1, "checkpull [<distributions>]"},
	{"includedeb",		A_Dactsp(includedeb)|NEED_DELNEW,
		2, -1, "[--delete] includedeb <distribution> <.deb-file>"},
	{"includeudeb",		A_Dactsp(includedeb)|NEED_DELNEW,
		2, -1, "[--delete] includeudeb <distribution> <.udeb-file>"},
	{"includedsc",		A_Dactsp(includedsc)|NEED_DELNEW,
		2, 2, "[--delete] includedsc <distribution> <package>"},
	{"include",		A_Dactsp(include)|NEED_DELNEW,
		2, 2, "[--delete] include <distribution> <.changes-file>"},
	{"generatefilelists",	A_F(generatefilelists),
		0, 1, "generatefilelists [reread]"},
	{"translatefilelists",	A__T(translatefilelists),
		0, 0, "translatefilelists"},
	{"translatelegacychecksums",	A_N(translatelegacychecksums),
		0, 0, "translatelegacychecksums"},
	{"_listconfidentifiers",	A_C(listconfidentifiers),
		0, -1, "_listconfidentifiers"},
	{"_listdbidentifiers",	A_ROB(listdbidentifiers)|MAY_UNUSED,
		0, -1, "_listdbidentifiers"},
	{"clearvanished",	A_D(clearvanished)|MAY_UNUSED,
		0, 0, "[--delete] clearvanished"},
	{"processincoming",	A_D(processincoming)|NEED_DELNEW,
		1, 2, "processincoming <rule-name> [<.changes file>]"},
	{"gensnapshot",		A_R(gensnapshot),
		2, 2, "gensnapshot <distribution> <date or other name>"},
	{"unreferencesnapshot",	A__R(unreferencesnapshot),
		2, 2, "gensnapshot <distribution> <name of snapshot>"},
	{"rerunnotifiers",	A_Bact(rerunnotifiers),
		0, -1, "rerunnotifiers [<distributions>]"},
	{"cleanlists",		A_L(cleanlists),
		0, 0,  "cleanlists"},
	{"build-needing", 	A_ROBact(buildneeded),
		2, 3, "[-C <component>] build-needing <codename> <architecture> [<glob>]"},
	{"flood", 		A_Dact(flood)|MAY_UNUSED,
		1, 2, "[-C <component> ] [-A <architecture>] [-T <packagetype>] flood <codename> [<architecture>]"},
	{"unusedsources",	A_B(unusedsources),
		0, -1, "unusedsources [<codenames>]"},
	{"sourcemissing",	A_B(sourcemissing),
		0, -1, "sourcemissing [<codenames>]"},
	{"reportcruft",		A_B(reportcruft),
		0, -1, "reportcruft [<codenames>]"},
	{NULL, NULL , 0, 0, 0, NULL}
};

#undef A_N
#undef A_B
#undef A_ROB
#undef A_C
#undef A_F
#undef A_R
#undef A_RF
#undef A_F
#undef A__T

retvalue callaction(command_t command, const struct action *action, int argc, const char *argv[]) {
	retvalue result, r;
	struct distribution *alldistributions = NULL;
	bool deletederef, deletenew;
	int needs;
	struct atomlist as, *architectures = NULL;
	struct atomlist cs, *components = NULL;
	struct atomlist ps, *packagetypes = NULL;

	assert(action != NULL);

	causingcommand = command;

	if (action->minargs >= 0 && argc < 1 + action->minargs) {
		fprintf(stderr,
"Error: Too few arguments for command '%s'!\nSyntax: reprepro %s\n",
				argv[0], action->wrongargmessage);
		return RET_ERROR;
	}
	if (action->maxargs >= 0 && argc > 1 + action->maxargs) {
		fprintf(stderr,
"Error: Too many arguments for command '%s'!\nSyntax: reprepro %s\n",
				argv[0], action->wrongargmessage);
		return RET_ERROR;
	}
	needs = action->needs;

	if (!ISSET(needs, NEED_ACT) && (x_architecture != NULL)) {
		if (!IGNORING(unusedoption,
"Action '%s' cannot be restricted to an architecture!\n"
"neither --archiecture nor -A make sense here.\n",
				action->name))
			return RET_ERROR;
	}
	if (!ISSET(needs, NEED_ACT) && (x_component != NULL)) {
		if (!IGNORING(unusedoption,
"Action '%s' cannot be restricted to a component!\n"
"neither --component nor -C make sense here.\n",
				action->name))
			return RET_ERROR;
	}
	if (!ISSET(needs, NEED_ACT) && (x_packagetype != NULL)) {
		if (!IGNORING(unusedoption,
"Action '%s' cannot be restricted to a packagetype!\n"
"neither --packagetype nor -T make sense here.\n",
				action->name))
			return RET_ERROR;
	}

	if (!ISSET(needs, NEED_SP) && (x_section != NULL)) {
		if (!IGNORING(unusedoption,
"Action '%s' cannot take a section option!\n"
"neither --section nor -S make sense here.\n",
				action->name))
			return RET_ERROR;
	}
	if (!ISSET(needs, NEED_SP) && (x_priority != NULL)) {
		if (!IGNORING(unusedoption,
"Action '%s' cannot take a priority option!\n"
"neither --priority nor -P make sense here.\n",
				action->name))
			return RET_ERROR;
	}
	if (!ISSET(needs, NEED_RESTRICT) && (cmdline_bin_filter.set
				|| cmdline_src_filter.set)) {
		if (!IGNORING(unusedoption,
"Action '%s' cannot take a --restrict-* option!\n",
				action->name))
			return RET_ERROR;
	}

	if (ISSET(needs, NEED_DATABASE))
		needs |= NEED_CONFIG;
	if (ISSET(needs, NEED_CONFIG)) {
		r = distribution_readall(&alldistributions);
		if (RET_WAS_ERROR(r))
			return r;
	}

	if (!ISSET(needs, NEED_DATABASE)) {
		assert ((needs & ~NEED_CONFIG) == 0);

		result = action->start(alldistributions,
				x_section, x_priority,
				atom_unknown, atom_unknown, atom_unknown,
				argc, argv);
		logger_wait();

		if (!RET_WAS_ERROR(result)) {
			r = distribution_exportlist(export, alldistributions);
			RET_ENDUPDATE(result, r);
		}

		r = distribution_freelist(alldistributions);
		RET_ENDUPDATE(result, r);
		return result;
	}

	if (ISSET(needs, NEED_ACT)) {
		const char *unknownitem;
		if (x_architecture != NULL) {
			r = atomlist_filllist(at_architecture, &as,
					x_architecture, &unknownitem);
			if (r == RET_NOTHING) {
				fprintf(stderr,
"Error: Architecture '%s' as given to --architecture is not know.\n"
"(it does not appear as architecture in %s/distributions (did you mistype?))\n",
					unknownitem, global.confdir);
				r = RET_ERROR;
			}
			if (RET_WAS_ERROR(r)) {
				(void)distribution_freelist(alldistributions);
				return r;
			}
			architectures = &as;
		} else {
			atomlist_init(&as);
		}
		if (x_component != NULL) {
			r = atomlist_filllist(at_component, &cs,
					x_component, &unknownitem);
			if (r == RET_NOTHING) {
				fprintf(stderr,
"Error: Component '%s' as given to --component is not know.\n"
"(it does not appear as component in %s/distributions (did you mistype?))\n",
					unknownitem, global.confdir);
				r = RET_ERROR;
			}
			if (RET_WAS_ERROR(r)) {
				(void)distribution_freelist(alldistributions);
				return r;
			}
			components = &cs;
		} else {
			atomlist_init(&cs);
		}
		if (x_packagetype != NULL) {
			r = atomlist_filllist(at_packagetype, &ps,
					x_packagetype, &unknownitem);
			if (r == RET_NOTHING) {
				fprintf(stderr,
"Error: Packagetype '%s' as given to --packagetype is not know.\n"
"(only dsc, deb, udeb and combinations of those are allowed)\n",
					unknownitem);
				r = RET_ERROR;
			}
			if (RET_WAS_ERROR(r)) {
				(void)distribution_freelist(alldistributions);
				return r;
			}
			packagetypes = &ps;
		} else {
			atomlist_init(&ps);
		}
		if (ps.count == 1 && ps.atoms[0] == pt_dsc &&
				limitations_missed(architectures,
					architecture_source)) {
			fprintf(stderr,
"Error: -T dsc is not possible with -A not including source!\n");
			return RET_ERROR;
		}
		if (as.count == 1 && as.atoms[0] == architecture_source &&
				limitations_missed(packagetypes, pt_dsc)) {
			fprintf(stderr,
"Error: -A source is not possible with -T not including dsc!\n");
			return RET_ERROR;
		}
	}

	deletederef = ISSET(needs, NEED_DEREF) && !keepunreferenced;
	deletenew = ISSET(needs, NEED_DELNEW) && !keepunusednew;

	result = database_create(alldistributions,
			fast, ISSET(needs, NEED_NO_PACKAGES),
			ISSET(needs, MAY_UNUSED), ISSET(needs, IS_RO),
			waitforlock, verbosedatabase || (verbose >= 30));
	if (!RET_IS_OK(result)) {
		(void)distribution_freelist(alldistributions);
		return result;
	}

	/* adding files may check references to see if they were added */
	if (ISSET(needs, NEED_FILESDB))
		needs |= NEED_REFERENCES;

	if (ISSET(needs, NEED_REFERENCES))
		result = database_openreferences();

	assert (result != RET_NOTHING);
	if (RET_IS_OK(result)) {

		if (ISSET(needs, NEED_FILESDB))
			result = database_openfiles();

		if (RET_IS_OK(result)) {
			if (outhook != NULL) {
				r = outhook_start();
				RET_UPDATE(result, r);
			}
		}

		assert (result != RET_NOTHING);
		if (RET_IS_OK(result)) {

			if (deletederef) {
				assert (ISSET(needs, NEED_REFERENCES));
			}

			if (!interrupted()) {
				result = action->start(alldistributions,
					x_section, x_priority,
					architectures, components, packagetypes,
					argc, argv);
				/* wait for package specific loggers */
				logger_wait();

				/* remove files added but not used */
				pool_tidyadded(deletenew);

				/* tell an outhook about added files */
				if (outhook != NULL)
					pool_sendnewfiles();
				/* export changed/lookedat distributions */
				if (!RET_WAS_ERROR(result)) {
					r = distribution_exportlist(export,
							alldistributions);
					RET_ENDUPDATE(result, r);
				}

				/* delete files losing references, or
				 * tell how many lost their references */

				// TODO: instead check if any distribution that
				// was not exported lost files
				// (and in a far future do not remove references
				// before the index is written)
				if (deletederef && RET_WAS_ERROR(result)) {
					deletederef = false;
					if (pool_havedereferenced) {
						fprintf(stderr,
"Not deleting possibly left over files due to previous errors.\n"
"(To keep the files in the still existing index files from vanishing)\n"
"Use dumpunreferenced/deleteunreferenced to show/delete files without references.\n");
					}
				}
				r = pool_removeunreferenced(deletederef);
				RET_ENDUPDATE(result, r);

				if (outhook != NULL) {
					if (interrupted())
						r = RET_ERROR_INTERRUPTED;
					else
						r = outhook_call(outhook);
					RET_ENDUPDATE(result, r);
				}
			}
		}
	}
	if (!interrupted()) {
		logger_wait();
	}
	if (ISSET(needs, NEED_ACT)) {
		atomlist_done(&as);
		atomlist_done(&cs);
		atomlist_done(&ps);
	}
	logger_warn_waiting();
	r = database_close();
	RET_ENDUPDATE(result, r);
	r = distribution_freelist(alldistributions);
	RET_ENDUPDATE(result, r);
	return result;
}

void setexport(const char *argument) {
	if (strcasecmp(argument, "silent-never") == 0) {
		CONFIGSET(export, EXPORT_SILENT_NEVER);
		return;
	}
	if (strcasecmp(argument, "never") == 0) {
		CONFIGSET(export, EXPORT_NEVER);
		return;
	}
	if (strcasecmp(argument, "changed") == 0) {
		CONFIGSET(export, EXPORT_CHANGED);
		return;
	}
	if (strcasecmp(argument, "normal") == 0) {
		CONFIGSET(export, EXPORT_NORMAL);
		return;
	}
	if (strcasecmp(argument, "lookedat") == 0) {
		CONFIGSET(export, EXPORT_NORMAL);
		return;
	}
	if (strcasecmp(argument, "force") == 0) {
		CONFIGSET(export, EXPORT_FORCE);
		return;
	}
	fprintf(stderr,
"Error: --export needs an argument of 'never', 'normal' or 'force', but got '%s'\n",
			argument);
	exit(EXIT_FAILURE);
}

unsigned long long parse_number(const char *name, const char *argument, long long max) {
	long long l;
	char *p;

	l = strtoll(argument, &p, 10);
	if (p==NULL || *p != '\0' || l < 0) {
		fprintf(stderr, "Invalid argument to %s: '%s'\n", name, argument);
		exit(EXIT_FAILURE);
	}
	if (l == LLONG_MAX  || l > max) {
		fprintf(stderr, "Too large argument for to %s: '%s'\n", name, argument);
		exit(EXIT_FAILURE);
	}
	return l;
}

void handle_option(int c, const char *argument) {
	retvalue r;
	int i;

	switch (c) {
		case 'h':
			printf(
"reprepro - Produce and Manage a Debian package repository\n\n"
"options:\n"
" -h, --help:                        Show this help\n"
" -i  --ignore <flag>:               Ignore errors of type <flag>.\n"
"     --keepunreferencedfiles:       Do not delete files no longer needed.\n"
"     --delete:                      Delete included files if reasonable.\n"
" -b, --basedir <dir>:               Base directory\n"
"     --outdir <dir>:                Set pool and dists base directory\n"
"     --distdir <dir>:               Override dists directory.\n"
"     --dbdir <dir>:                 Directory to place the database in.\n"
"     --listdir <dir>:               Directory to place downloaded lists in.\n"
"     --confdir <dir>:               Directory to search configuration in.\n"
"     --logdir <dir>:                Directory to put requeted log files in.\n"
"     --methodir <dir>:              Use instead of /usr/lib/apt/methods/\n"
" -S, --section <section>:           Force include* to set section.\n"
" -P, --priority <priority>:         Force include* to set priority.\n"
" -C, --component <component>: 	     Add,list or delete only in component.\n"
" -A, --architecture <architecture>: Add,list or delete only to architecture.\n"
" -T, --type <type>:                 Add,list or delete only type (dsc,deb,udeb).\n"
"\n"
"actions (selection, for more see manpage):\n"
" dumpreferences:    Print all saved references\n"
" dumpunreferenced:   Print registered files without reference\n"
" deleteunreferenced: Delete and forget all unreferenced files\n"
" checkpool:          Check if all files in the pool are still in proper shape.\n"
" check [<distributions>]\n"
"       Check for all needed files to be registered properly.\n"
" export [<distributions>]\n"
"	Force (re)generation of Packages.gz/Packages/Sources.gz/Release\n"
" update [<distributions>]\n"
"	Update the given distributions from the configured sources.\n"
" remove <distribution> <packagename>\n"
"       Remove the given package from the specified distribution.\n"
" include <distribution> <.changes-file>\n"
"       Include the given upload.\n"
" includedeb <distribution> <.deb-file>\n"
"       Include the given binary package.\n"
" includeudeb <distribution> <.udeb-file>\n"
"       Include the given installer binary package.\n"
" includedsc <distribution> <.dsc-file>\n"
"       Include the given source package.\n"
" list <distribution> <package-name>\n"
"       List all packages by the given name occurring in the given distribution.\n"
" listfilter <distribution> <condition>\n"
"       List all packages in the given distribution matching the condition.\n"
" clearvanished\n"
"       Remove everything no longer referenced in the distributions config file.\n"
"\n");
			exit(EXIT_SUCCESS);
		case '\0':
			switch (longoption) {
				case LO_UNIGNORE:
					r = set_ignore(argument, false, config_state);
					if (RET_WAS_ERROR(r)) {
						exit(EXIT_FAILURE);
					}
					break;
				case LO_SHOWPERCENT:
					global.showdownloadpercent++;
					break;
				case LO_DELETE:
					delete++;
					break;
				case LO_NODELETE:
					delete--;
					break;
				case LO_KEEPUNREFERENCED:
					CONFIGSET(keepunreferenced, true);
					break;
				case LO_NOKEEPUNREFERENCED:
					CONFIGSET(keepunreferenced, false);
					break;
				case LO_KEEPUNUSEDNEW:
					CONFIGSET(keepunusednew, true);
					break;
				case LO_NOKEEPUNUSEDNEW:
					CONFIGSET(keepunusednew, false);
					break;
				case LO_KEEPUNNEEDEDLISTS:
					/* this is the only option now and ignored
					 * for compatibility reasond */
					break;
				case LO_NOKEEPUNNEEDEDLISTS:
					fprintf(stderr,
"Warning: --nokeepuneededlists no longer exists.\n"
"Use cleanlists to clean manually.\n");
					break;
				case LO_KEEPTEMPORARIES:
					CONFIGGSET(keeptemporaries, true);
					break;
				case LO_NOKEEPTEMPORARIES:
					CONFIGGSET(keeptemporaries, false);
					break;
				case LO_ONLYSMALLDELETES:
					CONFIGGSET(onlysmalldeletes, true);
					break;
				case LO_NOONLYSMALLDELETES:
					CONFIGGSET(onlysmalldeletes, false);
					break;
				case LO_KEEPDIRECTORIES:
					CONFIGGSET(keepdirectories, true);
					break;
				case LO_NOKEEPDIRECTORIES:
					CONFIGGSET(keepdirectories, false);
					break;
				case LO_NOTHINGISERROR:
					CONFIGSET(nothingiserror, true);
					break;
				case LO_NONOTHINGISERROR:
					CONFIGSET(nothingiserror, false);
					break;
				case LO_NOLISTDOWNLOAD:
					CONFIGSET(nolistsdownload, true);
					break;
				case LO_LISTDOWNLOAD:
					CONFIGSET(nolistsdownload, false);
					break;
				case LO_ASKPASSPHRASE:
					CONFIGSET(askforpassphrase, true);
					break;
				case LO_NOASKPASSPHRASE:
					CONFIGSET(askforpassphrase, false);
					break;
				case LO_GUESSGPGTTY:
					CONFIGSET(guessgpgtty, true);
					break;
				case LO_NOGUESSGPGTTY:
					CONFIGSET(guessgpgtty, false);
					break;
				case LO_SKIPOLD:
					CONFIGSET(skipold, true);
					break;
				case LO_NOSKIPOLD:
					CONFIGSET(skipold, false);
					break;
				case LO_FAST:
					CONFIGSET(fast, true);
					break;
				case LO_NOFAST:
					CONFIGSET(fast, false);
					break;
				case LO_VERBOSEDB:
					CONFIGSET(verbosedatabase, true);
					break;
				case LO_NOVERBOSEDB:
					CONFIGSET(verbosedatabase, false);
					break;
				case LO_EXPORT:
					setexport(argument);
					break;
				case LO_OUTDIR:
					CONFIGDUP(x_outdir, argument);
					break;
				case LO_DISTDIR:
					CONFIGDUP(x_distdir, argument);
					break;
				case LO_DBDIR:
					CONFIGDUP(x_dbdir, argument);
					break;
				case LO_LISTDIR:
					CONFIGDUP(x_listdir, argument);
					break;
				case LO_CONFDIR:
					CONFIGDUP(x_confdir, argument);
					break;
				case LO_LOGDIR:
					CONFIGDUP(x_logdir, argument);
					break;
				case LO_METHODDIR:
					CONFIGDUP(x_methoddir, argument);
					break;
				case LO_MORGUEDIR:
					CONFIGDUP(x_morguedir, argument);
					break;
				case LO_VERSION:
					fprintf(stderr, "%s: This is %s version %s\n",
						programname, package_name(), package_version());
					exit(EXIT_SUCCESS);
				case LO_WAITFORLOCK:
					CONFIGSET(waitforlock, parse_number(
							"--waitforlock",
							argument, LONG_MAX));
					break;
				case LO_SPACECHECK:
					if (strcasecmp(argument, "none") == 0) {
						CONFIGSET(spacecheckmode, scm_NONE);
					} else if (strcasecmp(argument, "full") == 0) {
						CONFIGSET(spacecheckmode, scm_FULL);
					} else {
						fprintf(stderr,
"Unknown --spacecheck argument: '%s'!\n", argument);
						exit(EXIT_FAILURE);
					}
					break;
				case LO_SAFETYMARGIN:
					CONFIGSET(reservedotherspace, parse_number(
							"--safetymargin",
							argument, LONG_MAX));
					break;
				case LO_DBSAFETYMARGIN:
					CONFIGSET(reserveddbspace, parse_number(
							"--dbsafetymargin",
							argument, LONG_MAX));
					break;
				case LO_GUNZIP:
					CONFIGDUP(gunzip, argument);
					break;
				case LO_BUNZIP2:
					CONFIGDUP(bunzip2, argument);
					break;
				case LO_UNLZMA:
					CONFIGDUP(unlzma, argument);
					break;
				case LO_UNXZ:
					CONFIGDUP(unxz, argument);
					break;
				case LO_LZIP:
					CONFIGDUP(lunzip, argument);
					break;
				case LO_GNUPGHOME:
					CONFIGDUP(gnupghome, argument);
					break;
				case LO_ENDHOOK:
					CONFIGDUP(endhook, argument);
					break;
				case LO_OUTHOOK:
					CONFIGDUP(outhook, argument);
					break;
				case LO_LISTMAX:
					i = parse_number("--list-max",
							argument, INT_MAX);
					if (i == 0)
						i = -1;
					CONFIGSET(listmax, i);
					break;
				case LO_LISTSKIP:
					i = parse_number("--list-skip",
							argument, INT_MAX);
					CONFIGSET(listskip, i);
					break;
				case LO_LISTFORMAT:
					if (strcmp(argument, "NONE") == 0) {
						CONFIGSET(listformat, NULL);
					} else
						CONFIGDUP(listformat, argument);
					break;
				case LO_RESTRICT_BIN:
					r = filterlist_cmdline_add_pkg(false,
							argument);
					if (RET_WAS_ERROR(r))
						exit(EXIT_FAILURE);
					break;
				case LO_RESTRICT_SRC:
					r = filterlist_cmdline_add_pkg(true,
							argument);
					if (RET_WAS_ERROR(r))
						exit(EXIT_FAILURE);
					break;
				case LO_RESTRICT_FILE_BIN:
					r = filterlist_cmdline_add_file(false,
							argument);
					if (RET_WAS_ERROR(r))
						exit(EXIT_FAILURE);
					break;
				case LO_RESTRICT_FILE_SRC:
					r = filterlist_cmdline_add_file(true,
							argument);
					if (RET_WAS_ERROR(r))
						exit(EXIT_FAILURE);
					break;
				default:
					fputs(
"Error parsing arguments!\n", stderr);
					exit(EXIT_FAILURE);
			}
			longoption = 0;
			break;
		case 's':
			verbose--;
			break;
		case 'v':
			verbose++;
			break;
		case 'V':
			verbose+=5;
			break;
		case 'f':
			fprintf(stderr,
"Ignoring no longer existing option -f/--force!\n");
			break;
		case 'b':
			CONFIGDUP(x_basedir, argument);
			break;
		case 'i':
			r = set_ignore(argument, true, config_state);
			if (RET_WAS_ERROR(r)) {
				exit(EXIT_FAILURE);
			}
			break;
		case 'C':
			if (x_component != NULL &&
					strcmp(x_component, argument) != 0) {
				fprintf(stderr,
"Multiple '-%c' are not supported!\n", 'C');
				exit(EXIT_FAILURE);
			}
			CONFIGDUP(x_component, argument);
			break;
		case 'A':
			if (x_architecture != NULL &&
					strcmp(x_architecture, argument) != 0) {
				fprintf(stderr,
"Multiple '-%c' are not supported!\n", 'A');
				exit(EXIT_FAILURE);
			}
			CONFIGDUP(x_architecture, argument);
			break;
		case 'T':
			if (x_packagetype != NULL &&
					strcmp(x_packagetype, argument) != 0) {
				fprintf(stderr,
"Multiple '-%c' are not supported!\n", 'T');
				exit(EXIT_FAILURE);
			}
			CONFIGDUP(x_packagetype, argument);
			break;
		case 'S':
			if (x_section != NULL &&
					strcmp(x_section, argument) != 0) {
				fprintf(stderr,
"Multiple '-%c' are not supported!\n", 'S');
				exit(EXIT_FAILURE);
			}
			CONFIGDUP(x_section, argument);
			break;
		case 'P':
			if (x_priority != NULL &&
					strcmp(x_priority, argument) != 0) {
				fprintf(stderr,
"Multiple '-%c' are not supported!\n", 'P');
				exit(EXIT_FAILURE);
			}
			CONFIGDUP(x_priority, argument);
			break;
		case '?':
			/* getopt_long should have already given an error msg */
			exit(EXIT_FAILURE);
		default:
			fprintf(stderr, "Not supported option '-%c'\n", c);
			exit(EXIT_FAILURE);
	}
}
