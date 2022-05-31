#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "action_handlers.h"
#include "aptmethod.h"
#include "archallflood.h"
#include "atoms.h"
#include "binaries.h"
#include "checkin.h"
#include "checkindeb.h"
#include "checkindsc.h"
#include "copypackages.h"
#include "debfile.h"
#include "descriptions.h"
#include "dirs.h"
#include "downloadcache.h"
#include "dpkgversions.h"
#include "filecntl.h"
#include "filelist.h"
#include "files.h"
#include "filterlist.h"
#include "globmatch.h"
#include "ignore.h"
#include "incoming.h"
#include "log.h"
#include "main_helpers.h"
#include "mprintf.h"
#include "names.h"
#include "needbuild.h"
#include "optionsfile.h"
#include "outhook.h"
#include "override.h"
#include "package.h"
#include "pool.h"
#include "printlistformat.h"
#include "pull.h"
#include "reference.h"
#include "release.h"
#include "signature.h"
#include "sizes.h"
#include "strlist.h"
#include "sourcecheck.h"
#include "sourceextraction.h"
#include "sources.h"
#include "target.h"
#include "termdecide.h"
#include "tracking.h"
#include "uncompression.h"
#include "upgradelist.h"
#include "updates.h"
#include "uploaderslist.h"
#include "vars_main.h"

ACTION_N(n, n, y, printargs) {
	int i;

	fprintf(stderr, "argc: %d\n", argc);
	for (i=0 ; i < argc ; i++) {
		fprintf(stderr, "%s\n", argv[i]);
	}
	return RET_OK;
}

ACTION_N(n, n, n, dumpuncompressors) {
	enum compression c;

	assert (argc == 1);
	for (c = 0 ; c < c_COUNT ; c++) {
		if (c == c_none)
			continue;
		printf("%s: ", uncompression_suffix[c]);
		if (uncompression_builtin(c)) {
			if (extern_uncompressors[c] != NULL)
				printf("built-in + '%s'\n",
						extern_uncompressors[c]);
			else
				printf("built-in\n");
		} else if (extern_uncompressors[c] != NULL)
			printf("'%s'\n", extern_uncompressors[c]);
		else switch (c) {
			case c_bzip2:
				printf(
"not supported (install bzip2 or use --bunzip2 to tell where bunzip2 is).\n");

				break;
			case c_lzma:
				printf(
"not supported (install lzma or use --unlzma to tell where unlzma is).\n");
				break;
			case c_xz:
				printf(
"not supported (install xz-utils or use --unxz to tell where unxz is).\n");
				break;
			case c_lunzip:
				printf(
"not supported (install lzip or use --lunzip to tell where lunzip is).\n");
				break;
			default:
				printf("not supported\n");
		}
	}
	return RET_OK;
}

ACTION_N(n, n, y, uncompress) {
	enum compression c;

	assert (argc == 4);
	c = c_none + 1;
	while (c < c_COUNT && strcmp(argv[1], uncompression_suffix[c]) != 0)
		c++;
	if (c >= c_COUNT) {
		fprintf(stderr, "Unknown compression format '%s'\n", argv[1]);
		return RET_ERROR;
	}
	if (!uncompression_supported(c)) {
		fprintf(stderr,
"Cannot uncompress format '%s'\nCheck __dumpuncompressors for more details.\n",
				argv[1]);
		return RET_ERROR;
	}
	return uncompress_file(argv[2], argv[3], c);
}

ACTION_N(n, n, y, extractcontrol) {
	retvalue result;
	char *control;

	assert (argc == 2);

	result = extractcontrol(&control, argv[1]);

	if (RET_IS_OK(result)) {
		puts(control);
		free(control);
	}
	return result;
}

ACTION_N(n, n, y, extractfilelist) {
	retvalue result;
	char *filelist;
	size_t fls, len;
	size_t lengths[256];
	const unsigned char *dirs[256];
	int depth = 0, i, j;

	assert (argc == 2);

	result = getfilelist(&filelist, &fls, argv[1]);
	if (RET_IS_OK(result)) {
		const unsigned char *p = (unsigned char*)filelist;
		while (*p != '\0') {
			unsigned char c = *(p++);
			if (c > 2) {
				if (depth >= c)
					depth -= c;
				else
					depth = 0;
			} else if (c == 2) {
				len = 0;
				while (*p == 255) {
					len +=255;
					p++;
				}
				len += *(p++);
				lengths[depth] = len;
				dirs[depth++] = p;
				p += len;
			} else {
				len = 0;
				while (*p == 255) {
					len +=255;
					p++;
				}
				len += *(p++);
				(void)putchar('/');
				for (i = 0 ; i < depth ; i++) {
					const unsigned char *n = dirs[i];
					j = lengths[i];
					while (j-- > 0)
						(void)putchar(*(n++));
					(void)putchar('/');
				}
				while (len-- > 0)
					(void)putchar(*(p++));
				(void)putchar('\n');
			}
		}
		free(filelist);
	}
	return result;
}

ACTION_N(n, n, y, extractsourcesection) {
	struct dsc_headers dsc;
	struct sourceextraction *extraction;
	char *section = NULL, *priority = NULL, *directory, *filename;
	retvalue result, r;
	bool broken;
	int i;

	assert (argc == 2);

	r = sources_readdsc(&dsc, argv[1], argv[1], &broken);
	if (!RET_IS_OK(r))
		return r;
	if (broken && !IGNORING(brokensignatures,
"'%s' contains only broken signatures.\n"
"This most likely means the file was damaged or edited improperly\n",
				argv[1]))
		return RET_ERROR;
	r = dirs_getdirectory(argv[1], &directory);
	if (RET_WAS_ERROR(r)) {
		sources_done(&dsc);
		return r;
	}
	assert (RET_IS_OK(r));

	extraction = sourceextraction_init(&section, &priority);
	if (FAILEDTOALLOC(extraction)) {
		sources_done(&dsc);
		return RET_ERROR_OOM;
	}
	for (i = 0 ; i < dsc.files.names.count ; i ++)
		sourceextraction_setpart(extraction, i,
				dsc.files.names.values[i]);
	result = RET_OK;
	while (sourceextraction_needs(extraction, &i)) {
		filename = calc_dirconcat(directory, dsc.files.names.values[i]);
		if (FAILEDTOALLOC(filename)) {
			result = RET_ERROR_OOM;
			break;
		}
		r = sourceextraction_analyse(extraction, filename);
		free(filename);
		if (RET_WAS_ERROR(r)) {
			result = r;
			break;
		}
	}
	free(directory);
	if (RET_WAS_ERROR(result)) {
		sourceextraction_abort(extraction);
	} else {
		r = sourceextraction_finish(extraction);
		RET_UPDATE(result, r);
	}
	if (RET_IS_OK(result)) {
		if (section != NULL)
			printf("Section: %s\n", section);
		if (priority != NULL)
			printf("Priority: %s\n", priority);
	}
	sources_done(&dsc);
	free(section);
	free(priority);
	return result;
}

ACTION_F(n, n, n, y, fakeemptyfilelist) {
	assert (argc == 2);
	return fakefilelist(argv[1]);
}

ACTION_F(n, n, n, y, generatefilelists) {
	assert (argc == 2 || argc == 3);

	if (argc == 2)
		return files_regenerate_filelist(false);
	if (strcmp(argv[1], "reread") == 0)
		return files_regenerate_filelist(true);

	fprintf(stderr, "Error: Unrecognized second argument '%s'\n"
			"Syntax: reprepro generatefilelists [reread]\n",
				argv[1]);
	return RET_ERROR;
}

ACTION_T(n, n, translatefilelists) {
	return database_translate_filelists();
}

ACTION_N(n, n, n, translatelegacychecksums) {

	assert (argc == 1);

	return database_translate_legacy_checksums(
			verbosedatabase || verbose > 10);
}


ACTION_F(n, n, n, n, addmd5sums) {
	char buffer[2000], *c, *m;
	retvalue result, r;

	result = RET_NOTHING;

	while (fgets(buffer, 1999, stdin) != NULL) {
		struct checksums *checksums;

		c = strchr(buffer, '\n');
		if (c == NULL) {
			fprintf(stderr, "Line too long\n");
			return RET_ERROR;
		}
		*c = '\0';
		m = strchr(buffer, ' ');
		if (m == NULL) {
			fprintf(stderr, "Malformed line\n");
			return RET_ERROR;
		}
		*m = '\0'; m++;
		if (*m == '\0') {
			fprintf(stderr, "Malformed line\n");
			return RET_ERROR;
		}
		r = checksums_setall(&checksums, m, strlen(m));
		if (RET_WAS_ERROR(r))
			return r;
		r = files_add_checksums(buffer, checksums);
		RET_UPDATE(result, r);
		checksums_free(checksums);

	}
	return result;
}

ACTION_R(n, n, n, y, removereferences) {
	assert (argc == 2);
	return references_remove(argv[1]);
}

ACTION_R(n, n, n, y, removereference) {
	assert (argc == 3);
	return references_decrement(argv[2], argv[1]);
}

ACTION_R(n, n, n, n, dumpreferences) {
	return references_dump();
}

static retvalue checkifreferenced(UNUSED(void *data), const char *filekey) {
	retvalue r;

	r = references_isused(filekey);
	if (r == RET_NOTHING) {
		printf("%s\n", filekey);
		return RET_OK;
	} else if (RET_IS_OK(r)) {
		return RET_NOTHING;
	} else
		return r;
}

ACTION_RF(n, n, n, n, dumpunreferenced) {
	retvalue result;

	result = files_foreach(checkifreferenced, NULL);
	return result;
}

static retvalue deleteifunreferenced(UNUSED(void *data), const char *filekey) {
	retvalue r;

	r = references_isused(filekey);
	if (r == RET_NOTHING) {
		r = pool_delete(filekey);
		return r;
	} else if (RET_IS_OK(r)) {
		return RET_NOTHING;
	} else
		return r;
}

ACTION_RF(n, n, n, n, deleteunreferenced) {
	retvalue result;

	if (keepunreferenced) {
		if (owner_keepunreferenced == CONFIG_OWNER_CMDLINE)
			fprintf(stderr,
"Calling deleteunreferenced with --keepunreferencedfiles does not really make sense, does it?\n");
		else
			fprintf(stderr,
"Error: deleteunreferenced called with option\n"
"'keepunreferencedfiles' activated. Please run\n"
"'reprepro --nokeepunreferencedfiles deleteunreferenced',\n"
"if you are sure you want to delete those files.\n");
		return RET_ERROR;
	}
	result = files_foreach(deleteifunreferenced, NULL);
	return result;
}

ACTION_RF(n, n, n, y, deleteifunreferenced) {
	char buffer[5000], *nl;
	int i;
	retvalue r, ret;

	ret = RET_NOTHING;
	if (argc > 1) {
		for (i = 1 ; i < argc ; i++) {
			r = deleteifunreferenced(NULL, argv[i]);
			RET_UPDATE(ret, r);
			if (r == RET_NOTHING && verbose >= 0)
				fprintf(stderr, "Not removing '%s'\n",
						argv[i]);
		}

	} else
		while (fgets(buffer, 4999, stdin) != NULL) {
			nl = strchr(buffer, '\n');
			if (nl == NULL) {
				return RET_ERROR;
			}
			*nl = '\0';
			r = deleteifunreferenced(NULL, buffer);
			RET_UPDATE(ret, r);
			if (r == RET_NOTHING && verbose >= 0)
				fprintf(stderr, "Not removing '%s'\n",
						buffer);
		}
	return ret;
}

ACTION_R(n, n, n, y, addreference) {
	assert (argc == 2 || argc == 3);
	return references_increment(argv[1], argv[2]);
}

ACTION_R(n, n, n, y, addreferences) {
	char buffer[5000], *nl;
	int i;
	retvalue r, ret;

	ret = RET_NOTHING;

	if (argc > 2) {
		for (i = 2 ; i < argc ; i++) {
			const char *filename = argv[i];
			r = references_increment(filename, argv[1]);
			RET_UPDATE(ret, r);
		}
	} else {
		while (fgets(buffer, 4999, stdin) != NULL) {
			nl = strchr(buffer, '\n');
			if (nl == NULL) {
				return RET_ERROR;
			}
			*nl = '\0';
			r = references_increment(buffer, argv[1]);
			RET_UPDATE(ret, r);
		}
	}

	return ret;
}

static retvalue remove_from_target(struct distribution *distribution, struct trackingdata *trackingdata, struct target *target, int count, const char * const *names, int *todo, bool *gotremoved) {
	retvalue result, r;
	int i;

	result = RET_NOTHING;
	for (i = 0 ; i < count ; i++){
		r = target_removepackage(target, distribution->logger,
				names[i], trackingdata);
		RET_UPDATE(distribution->status, r);
		if (RET_IS_OK(r)) {
			if (!gotremoved[i])
				(*todo)--;
			gotremoved[i] = true;
		}
		RET_UPDATE(result, r);
	}
	return result;
}

ACTION_D(y, n, y, remove) {
	retvalue result, r;
	struct distribution *distribution;
	struct target *t;
	bool *gotremoved;
	int todo;

	trackingdb tracks;
	struct trackingdata trackingdata;

	r = distribution_get(alldistributions, argv[1], true, &distribution);
	assert (r != RET_NOTHING);
	if (RET_WAS_ERROR(r))
		return r;

	if (distribution->readonly) {
		fprintf(stderr,
"Cannot remove packages from read-only distribution '%s'\n",
				distribution->codename);
		return RET_ERROR;
	}

	r = distribution_prepareforwriting(distribution);
	if (RET_WAS_ERROR(r))
		return r;

	if (distribution->tracking != dt_NONE) {
		r = tracking_initialize(&tracks, distribution, false);
		if (RET_WAS_ERROR(r)) {
			return r;
		}
		r = trackingdata_new(tracks, &trackingdata);
		if (RET_WAS_ERROR(r)) {
			(void)tracking_done(tracks);
			return r;
		}
	}

	todo = argc-2;
	gotremoved = nzNEW(argc - 2, bool);
	result = RET_NOTHING;
	if (FAILEDTOALLOC(gotremoved))
		result = RET_ERROR_OOM;
	else for (t = distribution->targets ; t != NULL ; t = t->next) {
		 if (!target_matches(t, components, architectures, packagetypes))
			 continue;
		 r = target_initpackagesdb(t, READWRITE);
		 RET_UPDATE(result, r);
		 if (RET_WAS_ERROR(r))
			 break;
		 r = remove_from_target(distribution,
				 (distribution->tracking != dt_NONE)
				 	? &trackingdata
					: NULL,
				 t, argc-2, argv+2,
				 &todo, gotremoved);
		 RET_UPDATE(result, r);
		 r = target_closepackagesdb(t);
		 RET_UPDATE(distribution->status, r);
		 RET_UPDATE(result, r);
		 if (RET_WAS_ERROR(result))
			 break;
	}

	if (distribution->tracking != dt_NONE) {
		if (RET_WAS_ERROR(result))
			trackingdata_done(&trackingdata);
		else
			trackingdata_finish(tracks, &trackingdata);
		r = tracking_done(tracks);
		RET_ENDUPDATE(result, r);
	}
	if (verbose >= 0 && !RET_WAS_ERROR(result) && todo > 0) {
		int i = argc - 2;

		(void)fputs("Not removed as not found: ", stderr);
		while (i > 0) {
			i--;
			assert(gotremoved != NULL);
			if (!gotremoved[i]) {
				(void)fputs(argv[2 + i], stderr);
				todo--;
				if (todo > 0)
					(void)fputs(", ", stderr);
			}
		}
		(void)fputc('\n', stderr);
	}
	free(gotremoved);
	return result;
}

struct removesrcdata {
	const char *sourcename;
	const char /*@null@*/ *sourceversion;
	bool found;
};

static retvalue package_source_fits(struct package *package, void *data) {
	struct removesrcdata *d = data;
	retvalue r;

	r = package_getsource(package);
	if (!RET_IS_OK(r))
		return r;
	for (; d->sourcename != NULL ; d++) {
		if (strcmp(package->source, d->sourcename) != 0)
			continue;
		if (d->sourceversion == NULL)
			break;
		if (strcmp(package->sourceversion, d->sourceversion) == 0)
			break;
	}
	if (d->sourcename == NULL)
		return RET_NOTHING;
	else {
		d->found = true;
		return RET_OK;
	}
}

static retvalue remove_packages(struct distribution *distribution, struct removesrcdata *toremove) {
	trackingdb tracks;
	retvalue result, r;

	r = distribution_prepareforwriting(distribution);
	if (RET_WAS_ERROR(r))
		return r;

	if (distribution->tracking != dt_NONE) {
		r = tracking_initialize(&tracks, distribution, false);
		if (RET_WAS_ERROR(r)) {
			return r;
		}
		if (r == RET_NOTHING)
			tracks = NULL;
	} else
		tracks = NULL;
	result = RET_NOTHING;
	if (tracks != NULL) {
		result = RET_NOTHING;
		for (; toremove->sourcename != NULL ; toremove++) {
			r = tracking_removepackages(tracks, distribution,
					toremove->sourcename,
					toremove->sourceversion);
			RET_UPDATE(result, r);
			if (r == RET_NOTHING) {
				if (verbose >= -2) {
					if (toremove->sourceversion == NULL)
						fprintf(stderr,
"Nothing about source package '%s' found in the tracking data of '%s'!\n"
"This either means nothing from this source in this version is there,\n"
"or the tracking information might be out of date.\n",
							toremove->sourcename,
							distribution->codename);
					else
						fprintf(stderr,
"Nothing about '%s' version '%s' found in the tracking data of '%s'!\n"
"This either means nothing from this source in this version is there,\n"
"or the tracking information might be out of date.\n",
							toremove->sourcename,
							toremove->sourceversion,
							distribution->codename);
				}
			}
		}
		r = tracking_done(tracks);
		RET_ENDUPDATE(result, r);
		return result;
	}
	return package_remove_each(distribution,
			// TODO: why not arch comp pt here?
			atom_unknown, atom_unknown, atom_unknown,
			package_source_fits, NULL,
			toremove);
}

ACTION_D(n, n, y, removesrc) {
	retvalue r;
	struct distribution *distribution;
	struct removesrcdata data[2];

	r = distribution_get(alldistributions, argv[1], true, &distribution);
	assert (r != RET_NOTHING);
	if (RET_WAS_ERROR(r))
		return r;

	if (distribution->readonly) {
		fprintf(stderr,
"Error: Cannot remove packages from read-only distribution '%s'\n",
				distribution->codename);
		return RET_ERROR;
	}

	data[0].found = false;
	data[0].sourcename = argv[2];
	if (argc <= 3)
		data[0].sourceversion = NULL;
	else
		data[0].sourceversion = argv[3];
	if (index(data[0].sourcename, '=') != NULL && verbose >= 0) {
		fputs(
"Warning: removesrc treats '=' as normal character.\n"
"Did you want to use removesrcs?\n",
			stderr);
	}
	data[1].sourcename = NULL;
	data[1].sourceversion = NULL;
	return remove_packages(distribution, data);
}

ACTION_D(n, n, y, removesrcs) {
	retvalue r;
	struct distribution *distribution;
	struct removesrcdata data[argc-1];
	int i;

	r = distribution_get(alldistributions, argv[1], true, &distribution);
	assert (r != RET_NOTHING);
	if (RET_WAS_ERROR(r))
		return r;

	if (distribution->readonly) {
		fprintf(stderr,
"Error: Cannot remove packages from read-only distribution '%s'\n",
				distribution->codename);
		return RET_ERROR;
	}
	for (i = 0 ; i < argc-2 ; i++) {
		data[i].found = false;
		data[i].sourcename = argv[2 + i];
		data[i].sourceversion = index(data[i].sourcename, '=');
		if (data[i].sourceversion != NULL) {
			if (index(data[i].sourceversion+1, '=') != NULL) {
				fprintf(stderr,
"Cannot parse '%s': more than one '='\n",
						data[i].sourcename);
				data[i].sourcename = NULL;
				r = RET_ERROR;
			} else if (data[i].sourceversion[1] == '\0') {
				fprintf(stderr,
"Cannot parse '%s': no version after '='\n",
						data[i].sourcename);
				data[i].sourcename = NULL;
				r = RET_ERROR;
			} else if (data[i].sourceversion == data[i].sourcename) {
				fprintf(stderr,
"Cannot parse '%s': no source name found before the '='\n",
						data[i].sourcename);
				data[i].sourcename = NULL;
				r = RET_ERROR;
			} else {
				data[i].sourcename = strndup(data[i].sourcename,
						data[i].sourceversion
						- data[i].sourcename);
				if (FAILEDTOALLOC(data[i].sourcename))
					r = RET_ERROR_OOM;
				else
					r = RET_OK;
			}
			if (RET_WAS_ERROR(r)) {
				for (i-- ; i >= 0 ; i--) {
					if (data[i].sourceversion != NULL)
						free((char*)data[i].sourcename);
				}
				return r;
			}
			data[i].sourceversion++;
		}
	}
	data[i].sourcename = NULL;
	data[i].sourceversion= NULL;
	r = remove_packages(distribution, data);
	for (i = 0 ; i < argc-2 ; i++) {
		if (verbose >= 0 && !data[i].found) {
			if (data[i].sourceversion != NULL)
				fprintf(stderr,
"No package from source '%s', version '%s' found.\n",
						data[i].sourcename,
						data[i].sourceversion);
			else
				fprintf(stderr,
"No package from source '%s' (any version) found.\n",
						data[i].sourcename);
		}
		if (data[i].sourceversion != NULL)
			free((char*)data[i].sourcename);
	}
	return r;
}

static retvalue package_matches_condition(struct package *package, void *data) {
	term *condition = data;

	return term_decidepackage(condition, package, package->target);
}

ACTION_D(y, n, y, removefilter) {
	retvalue result, r;
	struct distribution *distribution;
	trackingdb tracks;
	struct trackingdata trackingdata;
	term *condition;

	assert (argc == 3);

	r = distribution_get(alldistributions, argv[1], true, &distribution);
	assert (r != RET_NOTHING);
	if (RET_WAS_ERROR(r))
		return r;

	if (distribution->readonly) {
		fprintf(stderr,
"Error: Cannot remove packages from read-only distribution '%s'\n",
				distribution->codename);
		return RET_ERROR;
	}

	result = term_compilefortargetdecision(&condition, argv[2]);
	if (RET_WAS_ERROR(result))
		return result;

	r = distribution_prepareforwriting(distribution);
	if (RET_WAS_ERROR(r)) {
		term_free(condition);
		return r;
	}

	if (distribution->tracking != dt_NONE) {
		r = tracking_initialize(&tracks, distribution, false);
		if (RET_WAS_ERROR(r)) {
			term_free(condition);
			return r;
		}
		if (r == RET_NOTHING)
			tracks = NULL;
		else {
			r = trackingdata_new(tracks, &trackingdata);
			if (RET_WAS_ERROR(r)) {
				(void)tracking_done(tracks);
				term_free(condition);
				return r;
			}
		}
	} else
		tracks = NULL;

	result = package_remove_each(distribution,
			components, architectures, packagetypes,
			package_matches_condition,
			(tracks != NULL)?&trackingdata:NULL,
			condition);
	if (tracks != NULL) {
		trackingdata_finish(tracks, &trackingdata);
		r = tracking_done(tracks);
		RET_ENDUPDATE(result, r);
	}
	term_free(condition);
	return result;
}

static retvalue package_matches_glob(struct package *package, void *data) {
	if (globmatch(package->name, data))
		return RET_OK;
	else
		return RET_NOTHING;
}

ACTION_D(y, n, y, removematched) {
	retvalue result, r;
	struct distribution *distribution;
	trackingdb tracks;
	struct trackingdata trackingdata;

	assert (argc == 3);

	r = distribution_get(alldistributions, argv[1], true, &distribution);
	assert (r != RET_NOTHING);
	if (RET_WAS_ERROR(r))
		return r;

	if (distribution->readonly) {
		fprintf(stderr,
"Error: Cannot remove packages from read-only distribution '%s'\n",
				distribution->codename);
		return RET_ERROR;
	}

	r = distribution_prepareforwriting(distribution);
	if (RET_WAS_ERROR(r))
		return r;

	if (distribution->tracking != dt_NONE) {
		r = tracking_initialize(&tracks, distribution, false);
		if (RET_WAS_ERROR(r))
			return r;
		if (r == RET_NOTHING)
			tracks = NULL;
		else {
			r = trackingdata_new(tracks, &trackingdata);
			if (RET_WAS_ERROR(r)) {
				(void)tracking_done(tracks);
				return r;
			}
		}
	} else
		tracks = NULL;

	result = package_remove_each(distribution,
			components, architectures, packagetypes,
			package_matches_glob,
			(tracks != NULL)?&trackingdata:NULL,
			(void*)argv[2]);
	if (tracks != NULL) {
		trackingdata_finish(tracks, &trackingdata);
		r = tracking_done(tracks);
		RET_ENDUPDATE(result, r);
	}
	return result;
}

ACTION_B(y, n, y, buildneeded) {
	retvalue r;
	struct distribution *distribution;
	const char *glob;
	architecture_t arch;
	bool anyarchitecture;

	if (architectures != NULL) {
		fprintf(stderr,
"Error: build-needing cannot be used with --architecture!\n");
		return RET_ERROR;
	}
	if (packagetypes != NULL) {
		fprintf(stderr,
"Error: build-needing cannot be used with --packagetype!\n");
		return RET_ERROR;
	}

	if (argc == 4)
		glob = argv[3];
	else
		glob = NULL;

	if (strcmp(argv[2], "any") == 0) {
		anyarchitecture = true;
	} else {
		anyarchitecture = false;
		arch = architecture_find(argv[2]);
		if (!atom_defined(arch)) {
			fprintf(stderr,
"Error: Architecture '%s' is not known!\n", argv[2]);
			return RET_ERROR;
		}
		if (arch == architecture_source) {
			fprintf(stderr,
"Error: Architecture '%s' makes no sense for build-needing!\n", argv[2]);
			return RET_ERROR;
		}
	}
	r = distribution_get(alldistributions, argv[1], false, &distribution);
	assert (r != RET_NOTHING);
	if (RET_WAS_ERROR(r))
		return r;

	if (!atomlist_in(&distribution->architectures, architecture_source)) {
		fprintf(stderr,
"Error: Architecture '%s' does not contain sources. build-needing cannot be used!\n",
				distribution->codename);
		return RET_ERROR;
	}
	if (anyarchitecture) {
		retvalue result;
		int i;

		result = find_needs_build(distribution,
				architecture_all,
				components, glob, true);

		for (i = 0 ; i < distribution->architectures.count ; i++) {
			architecture_t a = distribution->architectures.atoms[i];

			if (a == architecture_source || a == architecture_all)
				continue;
			r = find_needs_build(distribution, a,
					components, glob, true);
			RET_UPDATE(result, r);
		}
		return result;
	} else {
		if (!atomlist_in(&distribution->architectures, arch) &&
				arch != architecture_all) {
			fprintf(stderr,
"Error: Architecture '%s' not found in distribution '%s'!\n", argv[2],
					distribution->codename);
			return RET_ERROR;
		}

		return find_needs_build(distribution, arch, components,
				glob, false);
	}
}

static retvalue list_in_target(struct target *target, const char *packagename) {
	retvalue r, result;
	struct package pkg;

	if (listmax == 0)
		return RET_NOTHING;

	result = package_get(target, packagename, NULL, &pkg);
	if (RET_IS_OK(result)) {
		if (listskip <= 0) {
			r = listformat_print(listformat, &pkg);
			RET_UPDATE(result, r);
			if (listmax > 0)
				listmax--;
		} else
			listskip--;
		package_done(&pkg);
	}
	return result;
}

static retvalue list_package(struct package *package, UNUSED(void *dummy3)) {
	if (listmax == 0)
		return RET_NOTHING;

	if (listskip <= 0) {
		if (listmax > 0)
			listmax--;
		return listformat_print(listformat, package);
	} else {
		listskip--;
		return RET_NOTHING;
	}
}

ACTION_B(y, n, y, list) {
	retvalue result = RET_NOTHING, r;
	struct distribution *distribution;
	struct target *t;

	assert (argc >= 2);

	r = distribution_get(alldistributions, argv[1], false, &distribution);
	assert (r != RET_NOTHING);
	if (RET_WAS_ERROR(r))
		return r;

	if (argc == 2)
		return package_foreach(distribution,
			components, architectures, packagetypes,
			list_package, NULL, NULL);
	else for (t = distribution->targets ; t != NULL ; t = t->next) {
		if (!target_matches(t, components, architectures, packagetypes))
			continue;
		r = list_in_target(t, argv[2]);
		if (RET_WAS_ERROR(r))
			return r;
		RET_UPDATE(result, r);
	}
	return result;
}

struct lsversion {
	/*@null@*/struct lsversion *next;
	char *version;
	struct atomlist architectures;
};
struct lspart {
	struct lspart *next;
	const char *codename;
	const char *component;
	struct lsversion *versions;
};

static retvalue newlsversion(struct lsversion **versions_p, struct package *package, architecture_t architecture) {
	struct lsversion *v, **v_p;

	for (v_p = versions_p ; (v = *v_p) != NULL ; v_p = &v->next) {
		if (strcmp(v->version, package->version) != 0)
			continue;
		return atomlist_add_uniq(&v->architectures, architecture);
	}
	v = zNEW(struct lsversion);
	if (FAILEDTOALLOC(v))
		return RET_ERROR_OOM;
	*v_p = v;
	v->version = package_dupversion(package);
	if (FAILEDTOALLOC(v->version))
		return RET_ERROR_OOM;
	return atomlist_add(&v->architectures, architecture);
}

static retvalue ls_in_target(struct target *target, const char *packagename, struct lsversion **versions_p) {
	retvalue r, result;
	struct package pkg;

	result = package_get(target, packagename, NULL, &pkg);
	if (RET_IS_OK(result)) {
		r = package_getversion(&pkg);
		if (RET_IS_OK(r))
			r = newlsversion(versions_p, &pkg,
					target->architecture);
		package_done(&pkg);
		RET_UPDATE(result, r);
	}
	return result;
}

static inline retvalue printlsparts(const char *pkgname, struct lspart *parts) {
	int versionlen, codenamelen, componentlen;
	struct lspart *p;
	retvalue result = RET_NOTHING;

	versionlen = 0; codenamelen = 0; componentlen = 0;
	for (p = parts ; p->codename != NULL ; p = p->next) {
		struct lsversion *v;
		int l;

		l = strlen(p->codename);
		if (l > codenamelen)
			codenamelen = l;
		if (p->component != NULL) {
			l = strlen(p->component);
			if (l > componentlen)
				componentlen = l;
		}
		for (v = p->versions ; v != NULL ; v = v->next) {
			l = strlen(v->version);
			if (l > versionlen)
				versionlen = l;
		}
	}
	while (parts->codename != NULL) {
		p = parts;
		parts = parts->next;
		while (p->versions != NULL) {
			architecture_t a; int i;
			struct lsversion *v;

			v = p->versions;
			p->versions = v->next;

			result = RET_OK;
			printf("%s | %*s | %*s | ", pkgname,
					versionlen, v->version,
					codenamelen, p->codename);
			if (componentlen > 0 && p->component != NULL)
				printf("%*s | ", componentlen, p->component);
			for (i = 0 ; i + 1 < v->architectures.count ; i++) {
				a = v->architectures.atoms[i];
				printf("%s, ", atoms_architectures[a]);
			}
			a = v->architectures.atoms[i];
			puts(atoms_architectures[a]);

			free(v->version);
			atomlist_done(&v->architectures);
			free(v);
		}
		free(p);
	}
	free(parts);
	return result;
}

ACTION_B(y, n, y, ls) {
	retvalue r;
	struct distribution *d;
	struct target *t;
	struct lspart *first, *last;

	assert (argc == 2);

	first = zNEW(struct lspart);
	last = first;

	for (d = alldistributions ; d != NULL ; d = d->next) {
		for (t = d->targets ; t != NULL ; t = t->next) {
			if (!target_matches(t, components, architectures,
						packagetypes))
				continue;
			r = ls_in_target(t, argv[1], &last->versions);
			if (RET_WAS_ERROR(r))
				return r;
		}
		if (last->versions != NULL) {
			last->codename = d->codename;
			last->next = zNEW(struct lspart);
			last = last->next;
		}
	}
	return printlsparts(argv[1], first);
}

ACTION_B(y, n, y, lsbycomponent) {
	retvalue r;
	struct distribution *d;
	struct target *t;
	struct lspart *first, *last;
	int i;

	assert (argc == 2);

	first = zNEW(struct lspart);
	last = first;

	for (d = alldistributions ; d != NULL ; d = d->next) {
		for (i = 0 ; i < d->components.count ; i ++) {
			component_t component = d->components.atoms[i];

			if (limitations_missed(components, component))
				continue;
			for (t = d->targets ; t != NULL ; t = t->next) {
				if (t->component != component)
					continue;
				if (limitations_missed(architectures,
							t->architecture))
					continue;
				if (limitations_missed(packagetypes,
							t->packagetype))
					continue;
				r = ls_in_target(t, argv[1], &last->versions);
				if (RET_WAS_ERROR(r))
					return r;
			}
			if (last->versions != NULL) {
				last->codename = d->codename;
				last->component = atoms_components[component];
				last->next = zNEW(struct lspart);
				last = last->next;
			}
		}
	}
	return printlsparts(argv[1], first);
}

static retvalue listfilterprint(struct package *package, void *data) {
	term *condition = data;
	retvalue r;

	if (listmax == 0)
		return RET_NOTHING;

	r = term_decidepackage(condition, package, package->target);
	if (RET_IS_OK(r)) {
		if (listskip <= 0) {
			if (listmax > 0)
				listmax--;
			r = listformat_print(listformat, package);
		} else {
			listskip--;
			r = RET_NOTHING;
		}
	}
	return r;
}

ACTION_B(y, n, y, listfilter) {
	retvalue r, result;
	struct distribution *distribution;
	term *condition;

	assert (argc == 3);

	r = distribution_get(alldistributions, argv[1], false, &distribution);
	assert (r != RET_NOTHING);
	if (RET_WAS_ERROR(r)) {
		return r;
	}
	result = term_compilefortargetdecision(&condition, argv[2]);
	if (RET_WAS_ERROR(result)) {
		return result;
	}

	result = package_foreach(distribution,
			components, architectures, packagetypes,
			listfilterprint, NULL, condition);
	term_free(condition);
	return result;
}

static retvalue listmatchprint(struct package *package, void *data) {
	const char *glob = data;

	if (listmax == 0)
		return RET_NOTHING;

	if (globmatch(package->name, glob)) {
		if (listskip <= 0) {
			if (listmax > 0)
				listmax--;
			return listformat_print(listformat, package);
		} else {
			listskip--;
			return RET_NOTHING;
		}
	} else
		return RET_NOTHING;
}

ACTION_B(y, n, y, listmatched) {
	retvalue r, result;
	struct distribution *distribution;

	assert (argc == 3);

	r = distribution_get(alldistributions, argv[1], false, &distribution);
	assert (r != RET_NOTHING);
	if (RET_WAS_ERROR(r)) {
		return r;
	}
	result = package_foreach(distribution,
			components, architectures, packagetypes,
			listmatchprint, NULL, (void*)argv[2]);
	return result;
}

ACTION_F(n, n, n, y, detect) {
	char buffer[5000], *nl;
	int i;
	retvalue r, ret;

	ret = RET_NOTHING;
	if (argc > 1) {
		for (i = 1 ; i < argc ; i++) {
			r = files_detect(argv[i]);
			RET_UPDATE(ret, r);
		}

	} else
		while (fgets(buffer, 4999, stdin) != NULL) {
			nl = strchr(buffer, '\n');
			if (nl == NULL) {
				return RET_ERROR;
			}
			*nl = '\0';
			r = files_detect(buffer);
			RET_UPDATE(ret, r);
		}
	return ret;
}

ACTION_F(n, n, n, y, forget) {
	char buffer[5000], *nl;
	int i;
	retvalue r, ret;

	ret = RET_NOTHING;
	if (argc > 1) {
		for (i = 1 ; i < argc ; i++) {
			r = files_remove(argv[i]);
			RET_UPDATE(ret, r);
		}

	} else
		while (fgets(buffer, 4999, stdin) != NULL) {
			nl = strchr(buffer, '\n');
			if (nl == NULL) {
				return RET_ERROR;
			}
			*nl = '\0';
			r = files_remove(buffer);
			RET_UPDATE(ret, r);
		}
	return ret;
}

ACTION_F(n, n, n, n, listmd5sums) {
	return files_printmd5sums();
}

ACTION_F(n, n, n, n, listchecksums) {
	return files_printchecksums();
}

ACTION_B(n, n, n, dumpcontents) {
	retvalue result, r;
	struct table *packages;
	const char *package, *chunk;
	struct cursor *cursor;

	assert (argc == 2);

	result = database_openpackages(argv[1], true, &packages);
	if (RET_WAS_ERROR(result))
		return result;
	r = table_newglobalcursor(packages, &cursor);
	assert (r != RET_NOTHING);
	if (RET_WAS_ERROR(r)) {
		(void)table_close(packages);
		return r;
	}
	result = RET_NOTHING;
	while (cursor_nexttemp(packages, cursor, &package, &chunk)) {
		printf("'%s' -> '%s'\n", package, chunk);
		result = RET_OK;
	}
	r = cursor_close(packages, cursor);
	RET_ENDUPDATE(result, r);
	r = table_close(packages);
	RET_ENDUPDATE(result, r);
	return result;
}

ACTION_F(n, n, y, y, export) {
	retvalue result, r;
	struct distribution *d;

	if (export == EXPORT_NEVER || export == EXPORT_SILENT_NEVER) {
		fprintf(stderr,
"Error: reprepro export incompatible with --export=never\n");
		return RET_ERROR;
	}

	result = distribution_match(alldistributions, argc-1, argv+1, true, READWRITE);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result))
		return result;
	result = RET_NOTHING;
	for (d = alldistributions ; d != NULL ; d = d->next) {
		if (!d->selected)
			continue;

		if (d->exportoptions[deo_noexport]) {
			/* if explicitly selected, warn if not used: */
			if (argc > 1 && verbose >= 0 ) {
				printf("No exporting %s (as it has the noexport option set).\n", d->codename);
			}
			continue;
		}

		if (verbose > 0) {
			printf("Exporting %s...\n", d->codename);
		}
		r = distribution_fullexport(d);
		if (RET_IS_OK(r))
			/* avoid being exported again */
			d->lookedat = false;
		RET_UPDATE(result, r);
		if (RET_WAS_ERROR(r) && export != EXPORT_FORCE) {
			return r;
		}
	}
	return result;
}

/***********************update********************************/

ACTION_D(y, n, y, update) {
	retvalue result;
	struct update_pattern *patterns;
	struct update_distribution *u_distributions;

	result = dirs_make_recursive(global.listdir);
	if (RET_WAS_ERROR(result)) {
		return result;
	}

	result = distribution_match(alldistributions, argc-1, argv+1, true, READWRITE);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result))
		return result;

	result = updates_getpatterns(&patterns);
	if (RET_WAS_ERROR(result))
		return result;
	assert (RET_IS_OK(result));

	result = updates_calcindices(patterns, alldistributions,
			components, architectures, packagetypes,
			&u_distributions);
	if (!RET_IS_OK(result)) {
		if (result == RET_NOTHING) {
			if (argc == 1)
				fputs(
"Nothing to do, because no distribution has an Update: field.\n", stderr);
			else
				fputs(
"Nothing to do, because none of the selected distributions has an Update: field.\n",
					stderr);
		}
		updates_freepatterns(patterns);
		return result;
	}
	assert (RET_IS_OK(result));

	if (!RET_WAS_ERROR(result))
		result = updates_update(u_distributions,
				nolistsdownload, skipold,
				spacecheckmode, reserveddbspace,
				reservedotherspace);
	updates_freeupdatedistributions(u_distributions);
	updates_freepatterns(patterns);
	return result;
}

ACTION_D(y, n, y, predelete) {
	retvalue result;
	struct update_pattern *patterns;
	struct update_distribution *u_distributions;

	result = dirs_make_recursive(global.listdir);
	if (RET_WAS_ERROR(result)) {
		return result;
	}

	result = distribution_match(alldistributions, argc-1, argv+1, true, READWRITE);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result))
		return result;

	result = updates_getpatterns(&patterns);
	if (RET_WAS_ERROR(result)) {
		return result;
	}
	assert (RET_IS_OK(result));

	result = updates_calcindices(patterns, alldistributions,
			components, architectures, packagetypes,
			&u_distributions);
	if (!RET_IS_OK(result)) {
		if (result == RET_NOTHING) {
			if (argc == 1)
				fputs(
"Nothing to do, because no distribution has an Update: field.\n", stderr);
			else
				fputs(
"Nothing to do, because none of the selected distributions has an Update: field.\n",
					stderr);
		}
		updates_freepatterns(patterns);
		return result;
	}
	assert (RET_IS_OK(result));

	if (!RET_WAS_ERROR(result))
		result = updates_predelete(u_distributions,
				nolistsdownload, skipold);
	updates_freeupdatedistributions(u_distributions);
	updates_freepatterns(patterns);
	return result;
}

ACTION_B(y, n, y, checkupdate) {
	retvalue result;
	struct update_pattern *patterns;
	struct update_distribution *u_distributions;

	result = dirs_make_recursive(global.listdir);
	if (RET_WAS_ERROR(result)) {
		return result;
	}

	result = distribution_match(alldistributions, argc-1, argv+1, false, READONLY);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result))
		return result;

	result = updates_getpatterns(&patterns);
	if (RET_WAS_ERROR(result)) {
		return result;
	}

	result = updates_calcindices(patterns, alldistributions,
			components, architectures, packagetypes,
			&u_distributions);
	if (!RET_IS_OK(result)) {
		if (result == RET_NOTHING) {
			if (argc == 1)
				fputs(
"Nothing to do, because no distribution has an Updates: field.\n", stderr);
			else
				fputs(
"Nothing to do, because none of the selected distributions has an Update: field.\n",
					stderr);
		}
		updates_freepatterns(patterns);
		return result;
	}

	result = updates_checkupdate(u_distributions,
			nolistsdownload, skipold);

	updates_freeupdatedistributions(u_distributions);
	updates_freepatterns(patterns);

	return result;
}

ACTION_B(y, n, y, dumpupdate) {
	retvalue result;
	struct update_pattern *patterns;
	struct update_distribution *u_distributions;

	result = dirs_make_recursive(global.listdir);
	if (RET_WAS_ERROR(result)) {
		return result;
	}

	result = distribution_match(alldistributions, argc-1, argv+1, false, READONLY);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result))
		return result;

	result = updates_getpatterns(&patterns);
	if (RET_WAS_ERROR(result)) {
		return result;
	}

	result = updates_calcindices(patterns, alldistributions,
			components, architectures, packagetypes,
			&u_distributions);
	if (!RET_IS_OK(result)) {
		if (result == RET_NOTHING) {
			if (argc == 1)
				fputs(
"Nothing to do, because no distribution has an Updates: field.\n", stderr);
			else
				fputs(
"Nothing to do, because none of the selected distributions has an Update: field.\n",
					stderr);
		}
		updates_freepatterns(patterns);
		return result;
	}

	result = updates_dumpupdate(u_distributions,
			nolistsdownload, skipold);

	updates_freeupdatedistributions(u_distributions);
	updates_freepatterns(patterns);

	return result;
}

ACTION_L(n, n, n, n, cleanlists) {
	retvalue result;
	struct update_pattern *patterns;

	assert (argc == 1);

	if (!isdirectory(global.listdir))
		return RET_NOTHING;

	result = updates_getpatterns(&patterns);
	if (RET_WAS_ERROR(result))
		return result;

	result = updates_cleanlists(alldistributions, patterns);
	updates_freepatterns(patterns);
	return result;
}

/***********************migrate*******************************/

ACTION_D(y, n, y, pull) {
	retvalue result;
	struct pull_rule *rules;
	struct pull_distribution *p;

	result = distribution_match(alldistributions, argc-1, argv+1,
			true, READWRITE);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result))
		return result;

	result = pull_getrules(&rules);
	if (RET_WAS_ERROR(result)) {
		return result;
	}
	assert (RET_IS_OK(result));

	result = pull_prepare(alldistributions, rules, fast,
			components, architectures, packagetypes, &p);
	if (RET_WAS_ERROR(result)) {
		pull_freerules(rules);
		return result;
	}
	result = pull_update(p);

	pull_freerules(rules);
	pull_freedistributions(p);
	return result;
}

ACTION_B(y, n, y, checkpull) {
	retvalue result;
	struct pull_rule *rules;
	struct pull_distribution *p;

	result = distribution_match(alldistributions, argc-1, argv+1,
			false, READONLY);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result))
		return result;

	result = pull_getrules(&rules);
	if (RET_WAS_ERROR(result)) {
		return result;
	}
	assert (RET_IS_OK(result));

	result = pull_prepare(alldistributions, rules, fast,
			components, architectures, packagetypes, &p);
	if (RET_WAS_ERROR(result)) {
		pull_freerules(rules);
		return result;
	}
	result = pull_checkupdate(p);

	pull_freerules(rules);
	pull_freedistributions(p);

	return result;
}

ACTION_B(y, n, y, dumppull) {
	retvalue result;
	struct pull_rule *rules;
	struct pull_distribution *p;

	result = distribution_match(alldistributions, argc-1, argv+1,
			false, READONLY);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result))
		return result;

	result = pull_getrules(&rules);
	if (RET_WAS_ERROR(result)) {
		return result;
	}
	assert (RET_IS_OK(result));

	result = pull_prepare(alldistributions, rules, fast,
			components, architectures, packagetypes, &p);
	if (RET_WAS_ERROR(result)) {
		pull_freerules(rules);
		return result;
	}
	result = pull_dumpupdate(p);

	pull_freerules(rules);
	pull_freedistributions(p);

	return result;
}

ACTION_D(y, n, y, copy) {
	struct distribution *destination, *source;
	retvalue result;

	result = distribution_get(alldistributions, argv[1], true, &destination);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result))
		return result;
	result = distribution_get(alldistributions, argv[2], false, &source);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result))
		return result;

	if (destination->readonly) {
		fprintf(stderr,
"Cannot copy packages to read-only distribution '%s'.\n",
				destination->codename);
		return RET_ERROR;
	}
	result = distribution_prepareforwriting(destination);
	if (RET_WAS_ERROR(result))
		return result;

	return copy_by_name(destination, source, argc-3, argv+3,
			components, architectures, packagetypes);
}

ACTION_D(y, n, y, copysrc) {
	struct distribution *destination, *source;
	retvalue result;

	result = distribution_get(alldistributions, argv[1], true, &destination);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result))
		return result;
	result = distribution_get(alldistributions, argv[2], false, &source);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result))
		return result;
	if (destination->readonly) {
		fprintf(stderr,
"Cannot copy packages to read-only distribution '%s'.\n",
				destination->codename);
		return RET_ERROR;
	}
	result = distribution_prepareforwriting(destination);
	if (RET_WAS_ERROR(result))
		return result;

	return copy_by_source(destination, source, argc-3, argv+3,
			components, architectures, packagetypes);
	return result;
}

ACTION_D(y, n, y, copyfilter) {
	struct distribution *destination, *source;
	retvalue result;

	assert (argc == 4);

	result = distribution_get(alldistributions, argv[1], true, &destination);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result))
		return result;
	result = distribution_get(alldistributions, argv[2], false, &source);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result))
		return result;
	if (destination->readonly) {
		fprintf(stderr,
"Cannot copy packages to read-only distribution '%s'.\n",
				destination->codename);
		return RET_ERROR;
	}
	result = distribution_prepareforwriting(destination);
	if (RET_WAS_ERROR(result))
		return result;

	return copy_by_formula(destination, source, argv[3],
			components, architectures, packagetypes);
}

ACTION_D(y, n, y, copymatched) {
	struct distribution *destination, *source;
	retvalue result;

	assert (argc == 4);

	result = distribution_get(alldistributions, argv[1], true, &destination);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result))
		return result;
	result = distribution_get(alldistributions, argv[2], false, &source);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result))
		return result;
	if (destination->readonly) {
		fprintf(stderr,
"Cannot copy packages to read-only distribution '%s'.\n",
				destination->codename);
		return RET_ERROR;
	}
	result = distribution_prepareforwriting(destination);
	if (RET_WAS_ERROR(result))
		return result;

	return copy_by_glob(destination, source, argv[3],
			components, architectures, packagetypes);
}

ACTION_D(y, n, y, restore) {
	struct distribution *destination;
	retvalue result;

	result = distribution_get(alldistributions, argv[1], true, &destination);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result))
		return result;
	if (destination->readonly) {
		fprintf(stderr,
"Cannot copy packages to read-only distribution '%s'.\n",
				destination->codename);
		return RET_ERROR;
	}
	result = distribution_prepareforwriting(destination);
	if (RET_WAS_ERROR(result))
		return result;

	return restore_by_name(destination,
			components, architectures, packagetypes, argv[2],
			argc-3, argv+3);
}

ACTION_D(y, n, y, restoresrc) {
	struct distribution *destination;
	retvalue result;

	result = distribution_get(alldistributions, argv[1], true, &destination);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result))
		return result;
	if (destination->readonly) {
		fprintf(stderr,
"Cannot copy packages to read-only distribution '%s'.\n",
				destination->codename);
		return RET_ERROR;
	}
	result = distribution_prepareforwriting(destination);
	if (RET_WAS_ERROR(result))
		return result;

	return restore_by_source(destination,
			components, architectures, packagetypes, argv[2],
			argc-3, argv+3);
}

ACTION_D(y, n, y, restorematched) {
	struct distribution *destination;
	retvalue result;

	assert (argc == 4);

	result = distribution_get(alldistributions, argv[1], true, &destination);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result))
		return result;
	if (destination->readonly) {
		fprintf(stderr,
"Cannot copy packages to read-only distribution '%s'.\n",
				destination->codename);
		return RET_ERROR;
	}
	result = distribution_prepareforwriting(destination);
	if (RET_WAS_ERROR(result))
		return result;

	return restore_by_glob(destination,
			components, architectures, packagetypes, argv[2],
			argv[3]);
}

ACTION_D(y, n, y, restorefilter) {
	struct distribution *destination;
	retvalue result;

	assert (argc == 4);

	result = distribution_get(alldistributions, argv[1], true, &destination);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result))
		return result;
	if (destination->readonly) {
		fprintf(stderr,
"Cannot copy packages to read-only distribution '%s'.\n",
				destination->codename);
		return RET_ERROR;
	}
	result = distribution_prepareforwriting(destination);
	if (RET_WAS_ERROR(result))
		return result;

	return restore_by_formula(destination,
			components, architectures, packagetypes, argv[2],
			argv[3]);
}

ACTION_D(y, n, y, addpackage) {
	struct distribution *destination;
	retvalue result;
	architecture_t architecture = atom_unknown;
	component_t component = atom_unknown;
	packagetype_t packagetype = atom_unknown;

	if (packagetypes != NULL) {
		if (packagetypes->count > 1) {
			fprintf(stderr,
"_addpackage can only cope with one packagetype at a time!\n");
			return RET_ERROR;
		}
		packagetype = packagetypes->atoms[0];
	}
	if (architectures != NULL) {
		if (architectures->count > 1) {
			fprintf(stderr,
"_addpackage can only cope with one architecture at a time!\n");
			return RET_ERROR;
		}
		architecture = architectures->atoms[0];
	}
	if (components != NULL) {
		if (components->count > 1) {
			fprintf(stderr,
"_addpackage can only cope with one component at a time!\n");
			return RET_ERROR;
		}
		component = components->atoms[0];
	}

	if (!atom_defined(packagetype) && atom_defined(architecture) &&
			architecture == architecture_source)
		packagetype = pt_dsc;
	if (atom_defined(packagetype) && !atom_defined(architecture) &&
			packagetype == pt_dsc)
		architecture = architecture_source;
	// TODO: some more guesses based on components and udebcomponents

	if (!atom_defined(architecture) || !atom_defined(component) ||
			!atom_defined(packagetype)) {
		fprintf(stderr, "_addpackage needs -C and -A and -T set!\n");
		return RET_ERROR;
	}

	result = distribution_get(alldistributions, argv[1], true, &destination);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result))
		return result;
	if (destination->readonly) {
		fprintf(stderr,
"Cannot add packages to read-only distribution '%s'.\n",
				destination->codename);
		return RET_ERROR;
	}
	result = distribution_prepareforwriting(destination);
	if (RET_WAS_ERROR(result))
		return result;

	return copy_from_file(destination,
			component, architecture, packagetype, argv[2],
			argc-3, argv+3);
}

/***********************rereferencing*************************/
ACTION_R(n, n, y, y, rereference) {
	retvalue result, r;
	struct distribution *d;
	struct target *t;

	result = distribution_match(alldistributions, argc-1, argv+1, false, READONLY);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result)) {
		return result;
	}
	result = RET_NOTHING;
	for (d = alldistributions ; d != NULL ; d = d->next) {
		if (!d->selected)
			continue;

		if (verbose > 0) {
			printf("Referencing %s...\n", d->codename);
		}
		for (t = d->targets ; t != NULL ; t = t->next) {
			r = target_rereference(t);
			RET_UPDATE(result, r);
		}
		r = tracking_rereference(d);
		RET_UPDATE(result, r);
		if (RET_WAS_ERROR(r))
			break;
	}

	return result;
}
/***************************retrack****************************/
ACTION_D(n, n, y, retrack) {
	retvalue result, r;
	struct distribution *d;

	result = distribution_match(alldistributions, argc-1, argv+1, false, READONLY);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result)) {
		return result;
	}
	result = RET_NOTHING;
	for (d = alldistributions ; d != NULL ; d = d->next) {
		if (!d->selected)
			continue;
		if (d->tracking == dt_NONE) {
			if (argc > 1) {
				fprintf(stderr,
"Cannot retrack %s: Tracking not activated for this distribution!\n",
					d->codename);
				RET_UPDATE(result, RET_ERROR);
			}
			continue;
		}
		r = tracking_retrack(d, true);
		RET_ENDUPDATE(result, r);
		if (RET_WAS_ERROR(result))
			break;
	}
	return result;
}

ACTION_D(n, n, y, removetrack) {
	retvalue result, r;
	struct distribution *distribution;
	trackingdb tracks;

	assert (argc == 4);

	result = distribution_get(alldistributions, argv[1], false, &distribution);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result))
		return result;
	r = tracking_initialize(&tracks, distribution, false);
	if (RET_WAS_ERROR(r)) {
		return r;
	}

	result = tracking_remove(tracks, argv[2], argv[3]);

	r = tracking_done(tracks);
	RET_ENDUPDATE(result, r);
	return result;
}

ACTION_D(n, n, y, removealltracks) {
	retvalue result, r;
	struct distribution *d;
	const char *codename;
	int i;

	if (delete <= 0)
		for (i = 1 ; i < argc ; i ++) {
			codename = argv[i];

			d = alldistributions;
			while (d != NULL && strcmp(codename, d->codename) != 0)
				d = d->next;
			if (d != NULL && d->tracking != dt_NONE) {
				fprintf(stderr,
"Error: Requested removing of all tracks of distribution '%s',\n"
"which still has tracking enabled. Use --delete to delete anyway.\n",
						codename);
				return RET_ERROR;
			}
		}
	result = RET_NOTHING;
	for (i = 1 ; i < argc ; i ++) {
		codename = argv[i];

		if (verbose >= 0) {
			printf("Deleting all tracks for %s...\n", codename);
		}

		r = tracking_drop(codename);
		RET_UPDATE(result, r);
		if (RET_WAS_ERROR(result))
			break;
		if (r == RET_NOTHING) {
			d = alldistributions;
			while (d != NULL && strcmp(codename, d->codename) != 0)
				d = d->next;
			if (d == NULL) {
				fprintf(stderr,
"Warning: There was no tracking information to delete for '%s',\n"
"which is also not found in conf/distributions. Either this was already\n"
"deleted earlier, or you might have mistyped.\n", codename);
			}
		}
	}
	return result;
}

ACTION_D(n, n, y, tidytracks) {
	retvalue result, r;
	struct distribution *d;

	result = distribution_match(alldistributions, argc-1, argv+1,
			false, READONLY);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result)) {
		return result;
	}
	result = RET_NOTHING;
	for (d = alldistributions ; d != NULL ; d = d->next) {
		trackingdb tracks;

		if (!d->selected)
			continue;

		if (d->tracking == dt_NONE) {
			r = tracking_drop(d->codename);
			RET_UPDATE(result, r);
			if (RET_WAS_ERROR(r))
				break;
			continue;
		}

		if (verbose >= 0) {
			printf("Looking for old tracks in %s...\n",
					d->codename);
		}
		r = tracking_initialize(&tracks, d, false);
		if (RET_WAS_ERROR(r)) {
			RET_UPDATE(result, r);
			if (RET_WAS_ERROR(r))
				break;
			continue;
		}
		r = tracking_tidyall(tracks);
		RET_UPDATE(result, r);
		r = tracking_done(tracks);
		RET_ENDUPDATE(result, r);
		if (RET_WAS_ERROR(result))
			break;
	}
	return result;
}

ACTION_B(n, n, y, dumptracks) {
	retvalue result, r;
	struct distribution *d;

	result = distribution_match(alldistributions, argc-1, argv+1,
			false, READONLY);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result)) {
		return result;
	}
	result = RET_NOTHING;
	for (d = alldistributions ; d != NULL ; d = d->next) {
		trackingdb tracks;

		if (!d->selected)
			continue;

		r = tracking_initialize(&tracks, d, true);
		if (RET_WAS_ERROR(r)) {
			RET_UPDATE(result, r);
			if (RET_WAS_ERROR(r))
				break;
			continue;
		}
		if (r == RET_NOTHING)
			continue;
		r = tracking_printall(tracks);
		RET_UPDATE(result, r);
		r = tracking_done(tracks);
		RET_ENDUPDATE(result, r);
		if (RET_WAS_ERROR(result))
			break;
	}
	return result;
}

/***********************checking*************************/

ACTION_RF(y, n, y, y, check) {
	retvalue result, r;
	struct distribution *d;

	result = distribution_match(alldistributions, argc-1, argv+1,
			false, READONLY);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result)) {
		return result;
	}
	result = RET_NOTHING;
	for (d = alldistributions ; d != NULL ; d = d->next) {
		if (!d->selected)
			continue;

		if (verbose > 0) {
			printf("Checking %s...\n", d->codename);
		}

		r = package_foreach(d,
				components, architectures, packagetypes,
				package_check, NULL, NULL);
		RET_UPDATE(result, r);
		if (RET_WAS_ERROR(r))
			break;
	}
	return result;
}

ACTION_F(n, n, n, y, checkpool) {

	if (argc == 2 && strcmp(argv[1], "fast") != 0) {
		fprintf(stderr, "Error: Unrecognized second argument '%s'\n"
				"Syntax: reprepro checkpool [fast]\n",
				argv[1]);
		return RET_ERROR;
	}

	return files_checkpool(argc == 2);
}

/* Update checksums of existing files */

ACTION_F(n, n, n, n, collectnewchecksums) {

	return files_collectnewchecksums();
}
/*****************reapplying override info***************/

ACTION_F(y, n, y, y, reoverride) {
	retvalue result, r;
	struct distribution *d;

	result = distribution_match(alldistributions, argc-1, argv+1,
			true, READWRITE);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result)) {
		return result;
	}
	result = RET_NOTHING;
	for (d = alldistributions ; d != NULL ; d = d->next) {

		if (!d->selected)
			continue;

		if (verbose > 0) {
			fprintf(stderr, "Reapplying override to %s...\n",
					d->codename);
		}

		r = distribution_loadalloverrides(d);
		if (RET_IS_OK(r)) {
			struct target *t;

			for (t = d->targets ; t != NULL ; t = t->next) {
				if (!target_matches(t,
				      components, architectures, packagetypes))
					continue;
				r = target_reoverride(t, d);
				RET_UPDATE(result, r);
				// TODO: how to separate this in those
				// affecting d and those that do not?
				RET_UPDATE(d->status, r);
			}
			distribution_unloadoverrides(d);
		} else if (r == RET_NOTHING) {
			fprintf(stderr,
"No override files, thus nothing to do for %s.\n",
					d->codename);
		} else {
			RET_UPDATE(result, r);
		}
		if (RET_WAS_ERROR(result))
			break;
	}
	return result;
}

/*****************retrieving Description data from .deb files***************/

static retvalue repair_descriptions(struct target *target) {
        struct package_cursor iterator;
        retvalue result, r;

        assert(target->packages == NULL);
	assert(target->packagetype == pt_deb || target->packagetype == pt_udeb);

        if (verbose > 2) {
                printf(
"Redoing checksum information for packages in '%s'...\n",
                                target->identifier);
        }

        r = package_openiterator(target, READWRITE, &iterator);
        if (!RET_IS_OK(r))
                return r;
        result = RET_NOTHING;
        while (package_next(&iterator)) {
                char *newcontrolchunk = NULL;

		if (interrupted()) {
			result = RET_ERROR_INTERRUPTED;
			break;
		}
		/* replace it by itself to normalize the Description field */
                r = description_addpackage(target, iterator.current.name,
				iterator.current.control,
				iterator.current.control, NULL,
				&newcontrolchunk);
                RET_UPDATE(result, r);
                if (RET_WAS_ERROR(r))
                        break;
                if (RET_IS_OK(r)) {
			if (verbose >= 0) {
				printf(
"Fixing description for '%s'...\n", iterator.current.name);
			}
			r = package_newcontrol_by_cursor(&iterator,
                                newcontrolchunk, strlen(newcontrolchunk));
                        free(newcontrolchunk);
                        if (RET_WAS_ERROR(r)) {
                                result = r;
                                break;
                        }
                        target->wasmodified = true;
                }
        }
        r = package_closeiterator(&iterator);
        RET_ENDUPDATE(result, r);
        return result;
}

ACTION_F(y, n, y, y, repairdescriptions) {
	retvalue result, r;
	struct distribution *d;

	result = distribution_match(alldistributions, argc-1, argv+1,
			true, READWRITE);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result)) {
		return result;
	}
	result = RET_NOTHING;
	for (d = alldistributions ; d != NULL ; d = d->next) {
		struct target *t;

		if (!d->selected)
			continue;

		if (verbose > 0) {
			printf(
"Looking for 'Description's to repair in %s...\n", d->codename);
		}

		for (t = d->targets ; t != NULL ; t = t->next) {
			if (interrupted()) {
				result = RET_ERROR_INTERRUPTED;
				break;
			}
			if (!target_matches(t, components, architectures, packagetypes))
				continue;
			if (t->packagetype == pt_dsc)
				continue;
			r = repair_descriptions(t);
			RET_UPDATE(result, r);
			RET_UPDATE(d->status, r);
			if (RET_WAS_ERROR(r))
				break;
		}
	}
	return result;
}

/*****************adding checkums of files again*****************/

ACTION_F(y, n, y, y, redochecksums) {
	retvalue result, r;
	struct distribution *d;

	result = distribution_match(alldistributions, argc-1, argv+1,
			true, READWRITE);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result)) {
		return result;
	}
	result = RET_NOTHING;
	for (d = alldistributions ; d != NULL ; d = d->next) {
		struct target *t;

		if (!d->selected)
			continue;

		if (verbose > 0) {
			fprintf(stderr,
"Readding checksum information to packages in %s...\n", d->codename);
		}

		for (t = d->targets ; t != NULL ; t = t->next) {
			if (!target_matches(t,
			      components, architectures, packagetypes))
				continue;
			r = target_redochecksums(t, d);
			RET_UPDATE(result, r);
			RET_UPDATE(d->status, r);
			if (RET_WAS_ERROR(r))
				break;
		}
		if (RET_WAS_ERROR(result))
			break;
	}
	return result;
}

/*******************sizes of distributions***************/

ACTION_RF(n, n, y, y, sizes) {
	retvalue result;

	result = distribution_match(alldistributions, argc-1, argv+1,
			false, READONLY);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result)) {
		return result;
	}
	return sizes_distributions(alldistributions, argc > 1);
}

/***********************include******************************************/

ACTION_D(y, y, y, includedeb) {
	retvalue result, r;
	struct distribution *distribution;
	packagetype_t packagetype;
	trackingdb tracks;
	int i = 0;
	component_t component = atom_unknown;

	if (components != NULL) {
		if (components->count > 1) {
			fprintf(stderr,
"Error: Only one component is allowed with %s!\n",
					argv[0]);
			return RET_ERROR;
		}
		assert(components->count > 0);
		component = components->atoms[0];
	}

	if (architectures != NULL)
		if (!atomlist_hasexcept(architectures, architecture_source)) {
			fprintf(stderr,
"Error: -A source is not possible with includedeb!\n");
			return RET_ERROR;
		}
	if (strcmp(argv[0], "includeudeb") == 0) {
		packagetype = pt_udeb;
		if (limitations_missed(packagetypes, pt_udeb)) {
			fprintf(stderr,
"Calling includeudeb with a -T not containing udeb makes no sense!\n");
			return RET_ERROR;
		}
	} else if (strcmp(argv[0], "includedeb") == 0) {
		packagetype = pt_deb;
		if (limitations_missed(packagetypes, pt_deb)) {
			fprintf(stderr,
"Calling includedeb with a -T not containing deb makes no sense!\n");
			return RET_ERROR;
		}

	} else {
		fprintf(stderr, "Internal error while parding command!\n");
		return RET_ERROR;
	}

	for (i = 2 ; i < argc ; i++) {
		const char *filename = argv[i];

		if (packagetype == pt_udeb) {
			if (!endswith(filename, ".udeb") && !IGNORING(extension,
"includeudeb called with file '%s' not ending with '.udeb'\n", filename))
				return RET_ERROR;
		} else {
			if (!endswith(filename, ".deb") && !IGNORING(extension,
"includedeb called with file '%s' not ending with '.deb'\n", filename))
				return RET_ERROR;
		}
	}

	result = distribution_get(alldistributions, argv[1], true, &distribution);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result)) {
		return result;
	}
	if (distribution->readonly) {
		fprintf(stderr, "Cannot add packages to read-only distribution '%s'.\n",
				distribution->codename);
		return RET_ERROR;
	}

	if (packagetype == pt_udeb)
		result = override_read(distribution->udeb_override,
				&distribution->overrides.udeb, false);
	else
		result = override_read(distribution->deb_override,
				&distribution->overrides.deb, false);
	if (RET_WAS_ERROR(result)) {
		return result;
	}

	// TODO: same for component? (depending on type?)
	if (architectures != NULL) {
		architecture_t missing = atom_unknown;

		if (!atomlist_subset(&distribution->architectures,
				architectures, &missing)){
			fprintf(stderr,
"Cannot force into the architecture '%s' not available in '%s'!\n",
				atoms_architectures[missing],
				distribution->codename);
			return RET_ERROR;
		}
	}

	r = distribution_prepareforwriting(distribution);
	if (RET_WAS_ERROR(r)) {
		return RET_ERROR;
	}

	if (distribution->tracking != dt_NONE) {
		result = tracking_initialize(&tracks, distribution, false);
		if (RET_WAS_ERROR(result)) {
			return result;
		}
	} else {
		tracks = NULL;
	}
	result = RET_NOTHING;
	for (i = 2 ; i < argc ; i++) {
		const char *filename = argv[i];

		r = deb_add(component, architectures,
				section, priority, packagetype,
				distribution, filename,
				delete, tracks);
		RET_UPDATE(result, r);
	}

	distribution_unloadoverrides(distribution);

	r = tracking_done(tracks);
	RET_ENDUPDATE(result, r);
	return result;
}


ACTION_D(y, y, y, includedsc) {
	retvalue result, r;
	struct distribution *distribution;
	trackingdb tracks;
	component_t component = atom_unknown;

	if (components != NULL) {
		if (components->count > 1) {
			fprintf(stderr,
"Error: Only one component is allowed with %s!\n",
					argv[0]);
			return RET_ERROR;
		}
		assert(components->count > 0);
		component = components->atoms[0];
	}


	assert (argc == 3);

	if (limitations_missed(architectures, architecture_source)) {
		fprintf(stderr,
"Cannot put a source package anywhere else than in architecture 'source'!\n");
		return RET_ERROR;
	}
	if (limitations_missed(packagetypes, pt_dsc)) {
		fprintf(stderr,
"Cannot put a source package anywhere else than in type 'dsc'!\n");
		return RET_ERROR;
	}
	if (!endswith(argv[2], ".dsc") && !IGNORING(extension,
"includedsc called with a file not ending with '.dsc'\n"))
		return RET_ERROR;

	result = distribution_get(alldistributions, argv[1], true, &distribution);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result))
		return result;
	if (distribution->readonly) {
		fprintf(stderr,
"Cannot add packages to read-only distribution '%s'.\n",
				distribution->codename);
		return RET_ERROR;
	}
	result = override_read(distribution->dsc_override,
			&distribution->overrides.dsc, true);
	if (RET_WAS_ERROR(result)) {
		return result;
	}
	result = distribution_prepareforwriting(distribution);
	if (RET_WAS_ERROR(result)) {
		return result;
	}

	if (distribution->tracking != dt_NONE) {
		result = tracking_initialize(&tracks, distribution, false);
		if (RET_WAS_ERROR(result)) {
			return result;
		}
	} else {
		tracks = NULL;
	}

	result = dsc_add(component, section, priority,
			distribution, argv[2], delete, tracks);
	logger_wait();

	distribution_unloadoverrides(distribution);
	r = tracking_done(tracks);
	RET_ENDUPDATE(result, r);
	return result;
}

ACTION_D(y, y, y, include) {
	retvalue result, r;
	struct distribution *distribution;
	trackingdb tracks;
	component_t component = atom_unknown;

	if (components != NULL) {
		if (components->count > 1) {
			fprintf(stderr,
"Error: Only one component is allowed with %s!\n",
					argv[0]);
			return RET_ERROR;
		}
		assert(components->count > 0);
		component = components->atoms[0];
	}

	assert (argc == 3);

	if (!endswith(argv[2], ".changes") && !IGNORING(extension,
"include called with a file not ending with '.changes'\n"
"(Did you mean includedeb or includedsc?)\n"))
		return RET_ERROR;

	result = distribution_get(alldistributions, argv[1], true, &distribution);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result))
		return result;
	if (distribution->readonly) {
		fprintf(stderr,
"Cannot add packages to read-only distribution '%s'.\n",
				distribution->codename);
		return RET_ERROR;
	}

	result = distribution_loadalloverrides(distribution);
	if (RET_WAS_ERROR(result)) {
		return result;
	}

	if (distribution->tracking != dt_NONE) {
		result = tracking_initialize(&tracks, distribution, false);
		if (RET_WAS_ERROR(result)) {
			return result;
		}
	} else {
		tracks = NULL;
	}
	result = distribution_loaduploaders(distribution);
	if (RET_WAS_ERROR(result)) {
		r = tracking_done(tracks);
		RET_ENDUPDATE(result, r);
		return result;
	}
	result = changes_add(tracks, packagetypes, component, architectures,
			section, priority, distribution,
			argv[2], delete);
	if (RET_WAS_ERROR(result))
		RET_UPDATE(distribution->status, result);

	distribution_unloadoverrides(distribution);
	distribution_unloaduploaders(distribution);
	r = tracking_done(tracks);
	RET_ENDUPDATE(result, r);
	return result;
}

/***********************createsymlinks***********************************/

static bool mayaliasas(const struct distribution *alldistributions, const char *part, const char *cnpart) {
	const struct distribution *d;

	/* here it is only checked whether there is something that could
	 * cause this link to exist. No tests whether this really will
	 * cause it to be created (or already existing). */

	for (d = alldistributions ; d != NULL ; d = d->next) {
		if (d->suite == NULL)
			continue;
		if (strcmp(d->suite, part) == 0 &&
				strcmp(d->codename, cnpart) == 0)
			return true;
		if (strcmp(d->codename, part) == 0 &&
				strcmp(d->suite, cnpart) == 0)
			return true;
	}
	return false;
}

ACTION_C(n, n, createsymlinks) {
	retvalue result, r;
	struct distribution *d, *d2;
	bool warned_slash = false;

	r = dirs_make_recursive(global.distdir);
	if (RET_WAS_ERROR(r))
		return r;

	result = distribution_match(alldistributions, argc-1, argv+1, false, READONLY);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result)) {
		return result;
	}
	result = RET_NOTHING;
	for (d = alldistributions ; d != NULL ; d = d->next) {
		char *linkname, *buffer;
		size_t bufsize;
		int ret;
		const char *separator_in_suite;

		if (!d->selected)
			continue;

		if (d->suite == NULL || strcmp(d->suite, d->codename) == 0)
			continue;
		r = RET_NOTHING;
		for (d2 = alldistributions ; d2 != NULL ; d2 = d2->next) {
			if (!d2->selected)
				continue;
			if (d!=d2 && d2->suite!=NULL &&
					strcmp(d->suite, d2->suite)==0) {
				fprintf(stderr,
"Not linking %s->%s due to conflict with %s->%s\n",
					d->suite, d->codename,
					d2->suite, d2->codename);
				r = RET_ERROR;
			} else if (strcmp(d->suite, d2->codename)==0) {
				fprintf(stderr,
"Not linking %s->%s due to conflict with %s\n",
					d->suite, d->codename, d2->codename);
				r = RET_ERROR;
			}
		}
		if (RET_WAS_ERROR(r)) {
			RET_UPDATE(result, r);
			continue;
		}

		separator_in_suite = strchr(d->suite, '/');
		if (separator_in_suite != NULL) {
			/* things with / in it are tricky:
			 * relative symbolic links are hard,
			 * perhaps something else already moved
			 * the earlier ones, ... */
			const char *separator_in_codename;
			size_t ofs_in_suite = separator_in_suite - d->suite;
			char *part = strndup(d->suite, ofs_in_suite);

			if (FAILEDTOALLOC(part))
				return RET_ERROR_OOM;

			/* check if this is some case we do not want to warn about: */

			separator_in_codename = strchr(d->codename, '/');
			if (separator_in_codename != NULL &&
			    strcmp(separator_in_codename,
			           separator_in_suite) == 0) {
				/* all but the first is common: */
				size_t cnofs = separator_in_codename - d->codename;
				char *cnpart = strndup(d->codename, cnofs);
				if (FAILEDTOALLOC(cnpart)) {
					free(part);
					return RET_ERROR_OOM;
				}
				if (mayaliasas(alldistributions, part, cnpart)) {
					if (verbose > 1)
					fprintf(stderr,
"Not creating '%s' -> '%s' because of the '/' in it.\n"
"Hopefully something else will link '%s' -> '%s' then this is not needed.\n",
						d->suite, d->codename,
						part, cnpart);
					free(part);
					free(cnpart);
					continue;
				}
				free(cnpart);
			}
			free(part);
			if (verbose >= 0 && !warned_slash) {
				fprintf(stderr,
"Creating symlinks with '/' in them is not yet supported:\n");
				warned_slash = true;
			}
			if (verbose >= 0)
				fprintf(stderr,
"Not creating '%s' -> '%s' because of '/'.\n", d->suite, d->codename);
				continue;
		}

		linkname = calc_dirconcat(global.distdir, d->suite);
		bufsize = strlen(d->codename)+10;
		buffer = calloc(1, bufsize);
		if (FAILEDTOALLOC(linkname) || FAILEDTOALLOC(buffer)) {
			free(linkname); free(buffer);
			(void)fputs("Out of Memory!\n", stderr);
			return RET_ERROR_OOM;
		}

		ret = readlink(linkname, buffer, bufsize - 4);
		if (ret < 0 && errno == ENOENT) {
			ret = symlink(d->codename, linkname);
			if (ret != 0) {
				int e = errno;
				r = RET_ERRNO(e);
				fprintf(stderr,
"Error %d creating symlink %s->%s: %s\n", e, linkname, d->codename, strerror(e));
				RET_UPDATE(result, r);
			} else {
				if (verbose > 0) {
					printf("Created %s->%s\n", linkname,
							d->codename);
				}
				RET_UPDATE(result, RET_OK);
			}
		} else if (ret >= 0) {
			buffer[ret] = '\0';
			if (ret >= ((int)bufsize) - 4) {
				buffer[bufsize-4]='.';
				buffer[bufsize-3]='.';
				buffer[bufsize-2]='.';
				buffer[bufsize-1]='\0';
			}
			if (strcmp(buffer, d->codename) == 0) {
				if (verbose > 2) {
					printf("Already ok: %s->%s\n",
							linkname, d->codename);
				}
				RET_UPDATE(result, RET_OK);
			} else {
				if (delete <= 0) {
					fprintf(stderr,
"Cannot create %s as already pointing to %s instead of %s,\n"
" use --delete to delete the old link before creating an new one.\n",
						linkname, buffer, d->codename);
					RET_UPDATE(result, RET_ERROR);
				} else {
					unlink(linkname);
					ret = symlink(d->codename, linkname);
					if (ret != 0) {
						int e = errno;
						r = RET_ERRNO(e);
						fprintf(stderr,
"Error %d creating symlink %s->%s: %s\n", e, linkname, d->codename, strerror(e));
						RET_UPDATE(result, r);
					} else {
						if (verbose > 0) {
							printf(
"Replaced %s->%s\n", linkname, d->codename);
						}
						RET_UPDATE(result, RET_OK);
					}

				}
			}
		} else {
			int e = errno;
			r = RET_ERRNO(e);
			fprintf(stderr,
"Error %d checking %s, perhaps not a symlink?: %s\n", e, linkname, strerror(e));
			RET_UPDATE(result, r);
		}
		free(linkname); free(buffer);

		RET_UPDATE(result, r);
	}
	return result;
}

/***********************checkuploaders***********************************/

/* Read a fake package description from stdin */
static inline retvalue read_package_description(char **sourcename, struct strlist *sections, struct strlist *binaries, struct strlist *byhands, struct atomlist *architectures, struct signatures **signatures, char **buffer_p, size_t *bufferlen_p) {
	retvalue r;
	ssize_t got;
	char *buffer, *v, *p;
	struct strlist *l;
	struct signatures *s;
	struct signature *sig;
	architecture_t architecture;

	if (isatty(0)) {
		puts(
"Please input the simulated package data to test.\n"
"Format: (source|section|binary|byhand|architecture|signature) <value>\n"
"some keys may be given multiple times");
	}
	while ((got = getline(buffer_p, bufferlen_p, stdin)) >= 0) {
		buffer = *buffer_p;
		if (got == 0 || buffer[got - 1] != '\n') {
			fputs("stdin is not text\n", stderr);
			return RET_ERROR;
		}
		buffer[--got] = '\0';
		if (strncmp(buffer, "source ", 7) == 0) {
			if (*sourcename != NULL) {
				fprintf(stderr,
"Source name only allowed once!\n");
				return RET_ERROR;
			}
			*sourcename = strdup(buffer + 7);
			if (FAILEDTOALLOC(*sourcename))
				return RET_ERROR_OOM;
			continue;
		} else if (strncmp(buffer, "signature ", 10) == 0) {
			v = buffer + 10;
			if (*signatures == NULL) {
				s = calloc(1, sizeof(struct signatures)
						+sizeof(struct signature));
				if (FAILEDTOALLOC(s))
					return RET_ERROR_OOM;
			} else {
				s = realloc(*signatures,
						sizeof(struct signatures)
						+ (s->count+1)
						  * sizeof(struct signature));
				if (FAILEDTOALLOC(s))
					return RET_ERROR_OOM;
			}
			*signatures = s;
			sig = s->signatures + s->count;
			s->count++;
			s->validcount++;
			sig->expired_key = false;
			sig->expired_signature = false;
			sig->revoced_key = false;
			sig->state = sist_valid;
			switch (*v) {
				case 'b':
					sig->state = sist_bad;
					s->validcount--;
					v++;
					break;
				case 'e':
					sig->state = sist_mostly;
					sig->expired_signature = true;
					s->validcount--;
					v++;
					break;
				case 'i':
					sig->state = sist_invalid;
					s->validcount--;
					v++;
					break;
			}
			p = v;
			while ((*p >= '0' && *p <= '9') || (*p >= 'a' && *p <= 'f'))
				p++;
			sig->keyid = strndup(v, p-v);
			sig->primary_keyid = NULL;
			if (FAILEDTOALLOC(sig->keyid))
				return RET_ERROR_OOM;
			if (*p == ':') {
				p++;
				v = p;
				while ((*p >= '0' && *p <= '9')
						|| (*p >= 'a' && *p <= 'f'))
					p++;
				if (*p != '\0') {
					fprintf(stderr,
"Invalid character in key id: '%c'!\n",
						*p);
					return RET_ERROR;
				}
				sig->primary_keyid = strdup(v);
			} else if (*p != '\0') {
				fprintf(stderr,
"Invalid character in key id: '%c'!\n",
						*p);
				return RET_ERROR;
			} else
				sig->primary_keyid = strdup(sig->keyid);
			if (FAILEDTOALLOC(sig->primary_keyid))
				return RET_ERROR_OOM;
			continue;
		} else if (strncmp(buffer, "section ", 8) == 0) {
			v = buffer + 8;
			l = sections;
		} else if (strncmp(buffer, "binary ", 7) == 0) {
			v = buffer + 7;
			l = binaries;
		} else if (strncmp(buffer, "byhand ", 7) == 0) {
			v = buffer + 7;
			l = byhands;
		} else if (strncmp(buffer, "architecture ", 13) == 0) {
			v = buffer + 13;
			r = architecture_intern(v, &architecture);
			if (RET_WAS_ERROR(r))
				return r;
			r = atomlist_add(architectures, architecture);
			if (RET_WAS_ERROR(r))
				return r;
			continue;
		} else if (strcmp(buffer, "finished") == 0) {
			break;
		} else {
			fprintf(stderr, "Unparseable line '%s'\n", buffer);
			return RET_ERROR;
		}
		r = strlist_add_dup(l, v);
		if (RET_WAS_ERROR(r))
			return r;
	}
	if (ferror(stdin)) {
		int e = errno;
		fprintf(stderr, "Error %d reading data from stdin: %s\n",
				e, strerror(e));
		return RET_ERRNO(e);
	}
	if (*sourcename == NULL) {
		fprintf(stderr, "No source name specified!\n");
		return RET_ERROR;
	}
	return RET_OK;
}

static inline void verifystrlist(struct upload_conditions *conditions, const struct strlist *list) {
	int i;
	for (i = 0 ; i < list->count ; i++) {
		if (!uploaders_verifystring(conditions, list->values[i]))
			break;
	}
}
static inline void verifyatomlist(struct upload_conditions *conditions, const struct atomlist *list) {
	int i;
	for (i = 0 ; i < list->count ; i++) {
		if (!uploaders_verifyatom(conditions, list->atoms[i]))
			break;
	}
}


ACTION_C(n, n, checkuploaders) {
	retvalue result, r;
	struct distribution *d;
	char *sourcename = NULL;
	struct strlist sections, binaries, byhands;
	struct atomlist architectures;
	struct signatures *signatures = NULL;
	struct upload_conditions *conditions;
	bool accepted, rejected;
	char *buffer = NULL;
	size_t bufferlen = 0;
	int i;

	r = distribution_match(alldistributions, argc-1, argv+1, false, READONLY);
	assert (r != RET_NOTHING);
	if (RET_WAS_ERROR(r)) {
		return r;
	}
	for (d = alldistributions ; d != NULL ; d = d->next) {
		if (!d->selected)
			continue;
		r = distribution_loaduploaders(d);
		if (RET_WAS_ERROR(r))
			return r;
	}

	strlist_init(&sections);
	strlist_init(&binaries);
	strlist_init(&byhands);
	atomlist_init(&architectures);

	r = read_package_description(&sourcename, &sections, &binaries,
			&byhands, &architectures, &signatures,
			&buffer, &bufferlen);
	free(buffer);
	if (RET_WAS_ERROR(r)) {
		free(sourcename);
		strlist_done(&sections);
		strlist_done(&byhands);
		atomlist_done(&architectures);
		signatures_free(signatures);
		return r;
	}

	result = RET_NOTHING;
	accepted = false;
	for (i = 1 ; !accepted && i < argc ; i++) {
		r = distribution_get(alldistributions, argv[i], false, &d);
		if (RET_WAS_ERROR(r)) {
			result = r;
			break;
		}
		r = distribution_loaduploaders(d);
		if (RET_WAS_ERROR(r)) {
			result = r;
			break;
		}
		if (d->uploaderslist == NULL) {
			printf(
"'%s' would have been accepted by '%s' (as it has no uploader restrictions)\n",
				sourcename, d->codename);
			accepted = true;
			break;
		}
		r = uploaders_permissions(d->uploaderslist, signatures,
				&conditions);
		if (RET_WAS_ERROR(r)) {
			result = r;
			break;
		}
		rejected = false;
		do switch (uploaders_nextcondition(conditions)) {
			case uc_ACCEPTED:
				accepted = true;
				break;
			case uc_REJECTED:
				rejected = true;
				break;
			case uc_CODENAME:
				uploaders_verifystring(conditions, d->codename);
				break;
			case uc_SOURCENAME:
				uploaders_verifystring(conditions, sourcename);
				break;
			case uc_SECTIONS:
				verifystrlist(conditions, &sections);
				break;
			case uc_ARCHITECTURES:
				verifyatomlist(conditions, &architectures);
				break;
			case uc_BYHAND:
				verifystrlist(conditions, &byhands);
				break;
			case uc_BINARIES:
				verifystrlist(conditions, &byhands);
				break;
		} while (!accepted && !rejected);
		free(conditions);

		if (accepted) {
			printf("'%s' would have been accepted by '%s'\n",
					sourcename, d->codename);
			break;
		}
	}
	if (!accepted)
		printf(
"'%s' would NOT have been accepted by any of the distributions selected.\n",
			sourcename);
	free(sourcename);
	strlist_done(&sections);
	strlist_done(&byhands);
	atomlist_done(&architectures);
	signatures_free(signatures);
	if (RET_WAS_ERROR(result))
		return result;
	else if (accepted)
		return RET_OK;
	else
		return RET_NOTHING;
}

/***********************clearvanished***********************************/

ACTION_D(n, n, n, clearvanished) {
	retvalue result, r;
	struct distribution *d;
	struct strlist identifiers, codenames;
	bool *inuse;
	int i;

	result = database_listpackages(&identifiers);
	if (!RET_IS_OK(result)) {
		return result;
	}

	inuse = nzNEW(identifiers.count, bool);
	if (FAILEDTOALLOC(inuse)) {
		strlist_done(&identifiers);
		return RET_ERROR_OOM;
	}
	for (d = alldistributions; d != NULL ; d = d->next) {
		struct target *t;
		for (t = d->targets; t != NULL ; t = t->next) {
			int ofs = strlist_ofs(&identifiers, t->identifier);
			if (ofs >= 0) {
				inuse[ofs] = true;
				if (verbose > 6)
					printf(
"Marking '%s' as used.\n", t->identifier);
			} else if (verbose > 3 && database_allcreated()){
				fprintf(stderr,
"Strange, '%s' does not appear in packages.db yet.\n", t->identifier);
			}
		}
	}
	for (i = 0 ; i < identifiers.count ; i ++) {
		const char *identifier = identifiers.values[i];
		const char *p, *q;

		if (inuse[i])
			continue;
		if (interrupted())
			return RET_ERROR_INTERRUPTED;
		if (delete <= 0) {
			r = database_haspackages(identifier);
			if (RET_IS_OK(r)) {
				fprintf(stderr,
"There are still packages in '%s', not removing (give --delete to do so)!\n", identifier);
				continue;
			}
		}
		if (interrupted())
			return RET_ERROR_INTERRUPTED;
		// TODO: if delete, check what is removed, so that tracking
		// information can be updated.
		printf(
"Deleting vanished identifier '%s'.\n", identifier);
		/* intern component and architectures, so parsing
		 * has no problems (actually only need component now) */
		p = identifier;
		if (strncmp(p, "u|", 2) == 0)
			p += 2;
		p = strchr(p, '|');
		if (p != NULL) {
			p++;
			q = strchr(p, '|');
			if (q != NULL) {
				atom_t dummy;

				char *component = strndup(p, q-p);
				q++;
				char *architecture = strdup(q);
				if (FAILEDTOALLOC(component) ||
						FAILEDTOALLOC(architecture)) {
					free(component);
					free(architecture);
					return RET_ERROR_OOM;
				}
				r = architecture_intern(architecture, &dummy);
				free(architecture);
				if (RET_WAS_ERROR(r)) {
					free(component);
					return r;
				}
				r = component_intern(component, &dummy);
				free(component);
				if (RET_WAS_ERROR(r))
					return r;
			}
		}
		/* derference anything left */
		references_remove(identifier);
		/* remove the database */
		database_droppackages(identifier);
	}
	free(inuse);
	strlist_done(&identifiers);
	if (interrupted())
		return RET_ERROR_INTERRUPTED;

	r = tracking_listdistributions(&codenames);
	RET_UPDATE(result, r);
	if (RET_IS_OK(r)) {
		for (d = alldistributions; d != NULL ; d = d->next) {
			strlist_remove(&codenames, d->codename);
		}
		for (i = 0 ; i < codenames.count ; i ++) {
			printf(
"Deleting tracking data for vanished distribution '%s'.\n",
					codenames.values[i]);
			r = tracking_drop(codenames.values[i]);
			RET_UPDATE(result, r);
		}
		strlist_done(&codenames);
	}

	return result;
}

ACTION_B(n, n, y, listdbidentifiers) {
	retvalue result;
	struct strlist identifiers;
	const struct distribution *d;
	int i;

	result = database_listpackages(&identifiers);
	if (!RET_IS_OK(result)) {
		return result;
	}
	result = distribution_match(alldistributions, argc-1, argv+1, false, READONLY);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result))
		return result;

	result = RET_NOTHING;
	for (i = 0 ; i < identifiers.count ; i++) {
		const char *p, *q, *identifier = identifiers.values[i];

		if (argc <= 1) {
			puts(identifier);
			result = RET_OK;
			continue;
		}
		p = identifier;
		if (strncmp(p, "u|", 2) == 0)
			p += 2;
		q = strchr(p, '|');
		if (q == NULL)
			q = strchr(p, '\0');
		for (d = alldistributions ; d != NULL ; d = d->next) {
			if (!d->selected)
				continue;
			if (strncmp(p, d->codename, q - p) == 0
			    && d->codename[q-p] == '\0') {
				puts(identifier);
				result = RET_OK;
				break;
			}
		}
	}
	strlist_done(&identifiers);
	return result;
}

ACTION_C(n, n, listconfidentifiers) {
	struct target *t;
	const struct distribution *d;
	retvalue result;

	result = distribution_match(alldistributions, argc-1, argv+1, false, READONLY);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result))
		return result;
	result = RET_NOTHING;
	for (d = alldistributions ; d != NULL ; d = d->next) {
		if (!d->selected)
			continue;

		for (t = d->targets; t != NULL ; t = t->next) {
			puts(t->identifier);
			result = RET_OK;
		}
	}
	return result;
}

ACTION_N(n, n, y, versioncompare) {
	retvalue r;
	int i;

	assert (argc == 3);

	r = properversion(argv[1]);
	if (RET_WAS_ERROR(r))
		fprintf(stderr, "'%s' is not a proper version!\n", argv[1]);
	r = properversion(argv[2]);
	if (RET_WAS_ERROR(r))
		fprintf(stderr, "'%s' is not a proper version!\n", argv[2]);
	r = dpkgversions_cmp(argv[1], argv[2], &i);
	if (RET_IS_OK(r)) {
		if (i < 0) {
			printf("'%s' is smaller than '%s'.\n",
						argv[1], argv[2]);
		} else if (i > 0) {
			printf("'%s' is larger than '%s'.\n",
					argv[1], argv[2]);
		} else
			printf("'%s' is the same as '%s'.\n",
					argv[1], argv[2]);
	}
	return r;
}
/***********************processincoming********************************/
ACTION_D(n, n, y, processincoming) {
	struct distribution *d;

	for (d = alldistributions ; d != NULL ; d = d->next)
		d->selected = true;

	return process_incoming(alldistributions, argv[1],
			(argc==3) ? argv[2] : NULL);
}
/***********************gensnapshot********************************/
ACTION_R(n, n, y, y, gensnapshot) {
	retvalue result;
	struct distribution *distribution;

	assert (argc == 3);

	result = distribution_get(alldistributions, argv[1], false, &distribution);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result))
		return result;

	return distribution_snapshot(distribution, argv[2]);
}

ACTION_R(n, n, n, y, unreferencesnapshot) {
	retvalue result;
	char *id;

	assert (argc == 3);

	id = mprintf("s=%s=%s", argv[1], argv[2]);
	if (FAILEDTOALLOC(id))
		return RET_ERROR_OOM;

	result = references_remove(id);

	free(id);

	return result;
}

/***********************rerunnotifiers********************************/
static retvalue rerunnotifiersintarget(struct target *target, UNUSED(void *dummy)) {
	if (!logger_rerun_needs_target(target->distribution->logger, target))
		return RET_NOTHING;
	return RET_OK;
}

ACTION_B(y, n, y, rerunnotifiers) {
	retvalue result, r;
	struct distribution *d;

	result = distribution_match(alldistributions, argc-1, argv+1, false, READONLY);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result))
		return result;

	result = RET_NOTHING;
	for (d = alldistributions ; d != NULL ; d = d->next) {
		if (!d->selected)
			continue;

		if (d->logger == NULL)
			continue;

		if (verbose > 0) {
			printf("Processing %s...\n", d->codename);
		}
		r = logger_prepare(d->logger);
		RET_UPDATE(result, r);
		if (RET_WAS_ERROR(r))
			break;

		r = package_foreach(d,
				components, architectures, packagetypes,
				package_rerunnotifiers,
				rerunnotifiersintarget, NULL);
		logger_wait();

		RET_UPDATE(result, r);
		if (RET_WAS_ERROR(r))
			break;
	}
	return result;
}

/*********************** flood ****************************/

ACTION_D(y, n, y, flood) {
	retvalue result, r;
	struct distribution *distribution;
	trackingdb tracks;
	component_t architecture = atom_unknown;

	result = distribution_get(alldistributions, argv[1], true, &distribution);
	assert (result != RET_NOTHING);
	if (RET_WAS_ERROR(result))
		return result;
	if (distribution->readonly) {
		fprintf(stderr,
"Cannot add packages to read-only distribution '%s'.\n",
				distribution->codename);
		return RET_ERROR;
	}

	if (argc == 3) {
		architecture = architecture_find(argv[2]);
		if (!atom_defined(architecture)) {
			fprintf(stderr, "Error: Unknown architecture '%s'!\n",
					argv[2]);
			return RET_ERROR;
		}
		if (architecture == architecture_source) {
			fprintf(stderr,
"Error: Architecture 'source' does not make sense with 'flood'!\n");
			return RET_ERROR;
		}
		if (!atomlist_in(&distribution->architectures, architecture)) {
			fprintf(stderr,
"Error: Architecture '%s' not part of '%s'!\n",
					argv[2], distribution->codename);
			return RET_ERROR;
		}
	}

	result = distribution_prepareforwriting(distribution);
	if (RET_WAS_ERROR(result)) {
		return result;
	}

	if (distribution->tracking != dt_NONE) {
		result = tracking_initialize(&tracks, distribution, false);
		if (RET_WAS_ERROR(result)) {
			return result;
		}
	} else
		tracks = NULL;
	result = flood(distribution, components, architectures, packagetypes,
			architecture, tracks);

	if (RET_WAS_ERROR(result))
		RET_UPDATE(distribution->status, result);

	if (tracks != NULL) {
		r = tracking_done(tracks);
		RET_ENDUPDATE(result, r);
	}
	return result;
}

/*********************** unusedsources ****************************/
ACTION_B(n, n, y, unusedsources) {
	retvalue r;

	r = distribution_match(alldistributions, argc-1, argv+1,
			false, READONLY);
	assert (r != RET_NOTHING);
	if (RET_WAS_ERROR(r))
		return r;
	return unusedsources(alldistributions);
}

/*********************** missingsource ****************************/
ACTION_B(n, n, y, sourcemissing) {
	retvalue r;

	r = distribution_match(alldistributions, argc-1, argv+1,
			false, READONLY);
	assert (r != RET_NOTHING);
	if (RET_WAS_ERROR(r))
		return r;
	return sourcemissing(alldistributions);
}
/*********************** reportcruft ****************************/
ACTION_B(n, n, y, reportcruft) {
	retvalue r;

	r = distribution_match(alldistributions, argc-1, argv+1,
			false, READONLY);
	assert (r != RET_NOTHING);
	if (RET_WAS_ERROR(r))
		return r;
	return reportcruft(alldistributions);
}
