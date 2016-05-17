#ifndef SOFORT_H
#define SOFORT_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

#include "slibtool_api.h"

#ifdef __cplusplus
extern "C" {
#endif

/* pre-alpha */
#ifndef SLBT_APP
#ifndef SLBT_PRE_ALPHA
#error  libslibtool: pre-alpha: ABI is not final!
#error  to use the library, please pass -DSLBT_PRE_ALPHA to the compiler.
#endif
#endif

/* status codes */
#define SLBT_OK				0x00
#define SLBT_USAGE			0x01
#define SLBT_BAD_OPT			0x02
#define SLBT_BAD_OPT_VAL		0x03
#define SLBT_IO_ERROR			0xA0
#define SLBT_MAP_ERROR			0xA1

/* driver flags */
#define SLBT_DRIVER_VERBOSITY_NONE	0x0000
#define SLBT_DRIVER_VERBOSITY_ERRORS	0x0001
#define SLBT_DRIVER_VERBOSITY_STATUS	0x0002
#define SLBT_DRIVER_VERBOSITY_USAGE	0x0004
#define SLBT_DRIVER_CLONE_VECTOR	0x0008

#define SLBT_DRIVER_VERSION		0x0010
#define SLBT_DRIVER_DRY_RUN		0x0020
#define SLBT_DRIVER_CONFIG		0x0040
#define SLBT_DRIVER_DEBUG		0x0080
#define SLBT_DRIVER_FEATURES		0x0100
#define SLBT_DRIVER_DEPS		0x0200
#define SLBT_DRIVER_SILENT		0x0400
#define SLBT_DRIVER_VERBOSE		0x0800
#define SLBT_DRIVER_PRO_PIC		0x1000
#define SLBT_DRIVER_ANTI_PIC		0x2000
#define SLBT_DRIVER_SHARED		0x4000
#define SLBT_DRIVER_STATIC		0x8000

#define SLBT_DRIVER_HEURISTICS		0x010000
#define SLBT_DRIVER_STRICT		0x020000
#define SLBT_DRIVER_NO_UNDEFINED	0x040000
#define SLBT_DRIVER_MODULE		0x080000
#define SLBT_DRIVER_AVOID_VERSION	0x100000
#define SLBT_DRIVER_IMAGE_ELF		0x200000
#define SLBT_DRIVER_IMAGE_PE		0x400000
#define SLBT_DRIVER_IMAGE_MACHO		0x800000

#define SLBT_DRIVER_ALL_STATIC		0x01000000
#define SLBT_DRIVER_DISABLE_STATIC	0x02000000
#define SLBT_DRIVER_DISABLE_SHARED	0x04000000
#define SLBT_DRIVER_LEGABITS		0x08000000

#define SLBT_DRIVER_ANNOTATE_ALWAYS	0x10000000
#define SLBT_DRIVER_ANNOTATE_NEVER	0x20000000
#define SLBT_DRIVER_ANNOTATE_FULL	0x40000000

/* execution modes */
enum slbt_mode {
	SLBT_MODE_UNKNOWN,
	SLBT_MODE_CLEAN,
	SLBT_MODE_COMPILE,
	SLBT_MODE_EXECUTE,
	SLBT_MODE_FINISH,
	SLBT_MODE_INSTALL,
	SLBT_MODE_LINK,
	SLBT_MODE_UNINSTALL,
};

enum slbt_tag {
	SLBT_TAG_UNKNOWN,
	SLBT_TAG_CC,
	SLBT_TAG_CXX,
	SLBT_TAG_NASM,
};

enum slbt_warning_level {
	SLBT_WARNING_LEVEL_UNKNOWN,
	SLBT_WARNING_LEVEL_ALL,
	SLBT_WARNING_LEVEL_ERROR,
	SLBT_WARNING_LEVEL_NONE,
};

struct slbt_source_version {
	int		major;
	int		minor;
	int		revision;
	const char *	commit;
};

struct slbt_input {
	void *	addr;
	size_t	size;
};

struct slbt_exec_ctx {
	char *	program;
	char *	compiler;
	char **	cargv;
	char **	argv;
	char **	envp;
	char ** altv;
	char ** dpic;
	char ** fpic;
	char ** cass;
	char ** noundef;
	char ** soname;
	char ** lsoname;
	char ** symdefs;
	char ** symfile;
	char ** lout[2];
	char ** mout[2];
	char ** rpath[2];
	char ** sentinel;
	FILE *	fwrapper;
	FILE *	fdeps;
	char *	csrc;
	char *	ldirname;
	char *	lbasename;
	char *	lobjname;
	char *	aobjname;
	char *	ltobjname;
	char *	arfilename;
	char *	lafilename;
	char *	laifilename;
	char *	dsofilename;
	char *	dsolinkname;
	char *	deffilename;
	char *	rpathfilename;
	char *	dimpfilename;
	char *	pimpfilename;
	char *	vimpfilename;
	char *	exefilename;
	pid_t	pid;
	int	exitcode;
};

struct slbt_version_info {
	int				major;
	int				minor;
	int				revision;
	const char *			verinfo;
	const char *			vernumber;
};

struct slbt_host_params {
	const char *			host;
	const char *			flavor;
	const char *			ar;
	const char *			ranlib;
	const char *			dlltool;
	const char *			ldrpath;
};

struct slbt_flavor_settings {
	const char *			imagefmt;
	const char *			arprefix;
	const char *			arsuffix;
	const char *			dsoprefix;
	const char *			dsosuffix;
	const char *			exeprefix;
	const char *			exesuffix;
	const char *			impprefix;
	const char *			impsuffix;
	const char *			ldpathenv;
};

struct slbt_common_ctx {
	uint64_t			drvflags;
	uint64_t			actflags;
	uint64_t			fmtflags;
	struct slbt_host_params		host;
	struct slbt_host_params		cfgmeta;
	struct slbt_flavor_settings	settings;
	struct slbt_host_params		ahost;
	struct slbt_host_params		acfgmeta;
	struct slbt_flavor_settings	asettings;
	struct slbt_version_info	verinfo;
	enum slbt_mode			mode;
	enum slbt_tag			tag;
	enum slbt_warning_level		warnings;
	char **				cargv;
	char **				targv;
	char *				libname;
	const char *			target;
	const char *			output;
	const char *			shrext;
	const char *			rpath;
	const char *			release;
	const char *			symfile;
	const char *			regex;
	const char *			user;
};

struct slbt_driver_ctx {
	const char **			units;
	const char *			program;
	const char *			module;
	const struct slbt_common_ctx *	cctx;
	void *				any;
	int				status;
	int				nerrors;
};

struct slbt_unit_ctx {
	const char * const *		path;
	const struct slbt_input *	map;
	const struct slbt_common_ctx *	cctx;
	void *				any;
	int				status;
	int				nerrors;
};

/* package info */
slbt_api				const struct slbt_source_version * slbt_source_version(void);

/* driver api */
slbt_api int  slbt_get_driver_ctx	(char ** argv, char ** envp, uint32_t flags, struct slbt_driver_ctx **);
slbt_api int  slbt_create_driver_ctx	(const struct slbt_common_ctx *, struct slbt_driver_ctx **);
slbt_api void slbt_free_driver_ctx	(struct slbt_driver_ctx *);

slbt_api int  slbt_get_unit_ctx		(const struct slbt_driver_ctx *, const char * path, struct slbt_unit_ctx **);
slbt_api void slbt_free_unit_ctx	(struct slbt_unit_ctx *);

/* execution context api */
slbt_api int  slbt_get_exec_ctx		(const struct slbt_driver_ctx *, struct slbt_exec_ctx **);
slbt_api void slbt_free_exec_ctx	(struct slbt_exec_ctx *);
slbt_api void slbt_reset_arguments	(struct slbt_exec_ctx *);
slbt_api void slbt_reset_placeholders	(struct slbt_exec_ctx *);
slbt_api void slbt_disable_placeholders	(struct slbt_exec_ctx *);

/* core api */
slbt_api int  slbt_exec_compile		(const struct slbt_driver_ctx *, struct slbt_exec_ctx *);
slbt_api int  slbt_exec_execute		(const struct slbt_driver_ctx *, struct slbt_exec_ctx *);
slbt_api int  slbt_exec_install		(const struct slbt_driver_ctx *, struct slbt_exec_ctx *);
slbt_api int  slbt_exec_link		(const struct slbt_driver_ctx *, struct slbt_exec_ctx *);

slbt_api int  slbt_set_alternate_host	(const struct slbt_driver_ctx *, const char * host, const char * flavor);
slbt_api void slbt_reset_alternate_host	(const struct slbt_driver_ctx *);

/* helper api */
slbt_api int  slbt_map_input		(int fd, const char * path, int prot, struct slbt_input *);
slbt_api int  slbt_unmap_input		(struct slbt_input *);
slbt_api int  slbt_archive_import	(const struct slbt_driver_ctx *, struct slbt_exec_ctx *,
					 char * dstarchive, char * srcarchive);
slbt_api int  slbt_copy_file		(const struct slbt_driver_ctx *, struct slbt_exec_ctx *,
					 char * src, char * dst);
slbt_api int  slbt_dump_machine		(const char * compiler, char * machine, size_t bufsize);

/* utility api */
slbt_api int  slbt_output_config	(const struct slbt_driver_ctx *);
slbt_api int  slbt_output_exec		(const struct slbt_driver_ctx *, const struct slbt_exec_ctx *, const char *);
slbt_api int  slbt_output_compile	(const struct slbt_driver_ctx *, const struct slbt_exec_ctx *);
slbt_api int  slbt_output_execute	(const struct slbt_driver_ctx *, const struct slbt_exec_ctx *);
slbt_api int  slbt_output_install	(const struct slbt_driver_ctx *, const struct slbt_exec_ctx *);
slbt_api int  slbt_output_link		(const struct slbt_driver_ctx *, const struct slbt_exec_ctx *);

#ifdef __cplusplus
}
#endif

#endif
