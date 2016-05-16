#ifndef SOFORT_DRIVER_IMPL_H
#define SOFORT_DRIVER_IMPL_H

#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>

#include <slibtool/slibtool.h>
#include "argv/argv.h"

extern int slibtool_main(int, char **, char **);
extern const struct argv_option slbt_default_options[];

enum app_tags {
	TAG_HELP,
	TAG_HELP_ALL,
	TAG_VERSION,
	TAG_CONFIG,
	TAG_DEBUG,
	TAG_DRY_RUN,
	TAG_FEATURES,
	TAG_LEGABITS,
	TAG_MODE,
	TAG_WARNINGS,
	TAG_ANNOTATE,
	TAG_DEPS,
	TAG_SILENT,
	TAG_TAG,
	TAG_VERBOSE,
	TAG_TARGET,
	TAG_HOST,
	TAG_FLAVOR,
	TAG_AR,
	TAG_RANLIB,
	TAG_DLLTOOL,
	TAG_OUTPUT,
	TAG_BINDIR,
	TAG_LDRPATH,
	TAG_SHREXT,
	TAG_RPATH,
	TAG_RELEASE,
	TAG_EXPSYM_FILE,
	TAG_EXPSYM_REGEX,
	TAG_VERSION_INFO,
	TAG_VERSION_NUMBER,
	TAG_NO_SUPPRESS,
	TAG_NO_INSTALL,
	TAG_PREFER_PIC,
	TAG_PREFER_NON_PIC,
	TAG_SHARED,
	TAG_STATIC,
	TAG_ALL_STATIC,
	TAG_DISABLE_STATIC,
	TAG_DISABLE_SHARED,
	TAG_NO_UNDEFINED,
	TAG_MODULE,
	TAG_AVOID_VERSION,
	TAG_COMPILER_FLAG,
	TAG_VERBATIM_FLAG,
	TAG_UNIMPLEMENTED,
};

struct slbt_host_strs {
	char *		host;
	char *		flavor;
	char *		ar;
	char *		ranlib;
	char *		dlltool;
};

struct slbt_driver_ctx_impl {
	struct slbt_common_ctx	cctx;
	struct slbt_driver_ctx	ctx;
	struct slbt_host_strs	host;
	struct slbt_host_strs	ahost;
	char *			libname;
	char **			targv;
	char **			cargv;
};

struct slbt_unit_ctx_impl {
	const char *		path;
	struct slbt_input	map;
	struct slbt_common_ctx	cctx;
	struct slbt_unit_ctx	uctx;
};

#endif
