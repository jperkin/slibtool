#include "slibtool_driver_impl.h"
#include "argv/argv.h"

const struct argv_option slbt_default_options[] = {
	{"version",		0,TAG_VERSION,ARGV_OPTARG_NONE,0,0,0,
				"show version information"},

	{"help",		'h',TAG_HELP,ARGV_OPTARG_OPTIONAL,0,0,0,
				"show usage information"},

	{"help-all",		'h',TAG_HELP_ALL,ARGV_OPTARG_NONE,0,0,0,
				"show comprehensive help information"},

	{"mode",		0,TAG_MODE,ARGV_OPTARG_REQUIRED,0,
				"clean|compile|execute|finish"
				"|install|link|uninstall",0,
				"set the execution mode, where <mode> "
				"is one of {%s}. of the above modes, "
				"'finish' is not needed and is therefore "
				"a no-op; 'clean' and 'uninstall' are "
				"currently not implemented, however "
				"their addition is expected before "
				"the next release"},

	{"dry-run",		'n',TAG_DRY_RUN,ARGV_OPTARG_NONE,0,0,0,
				"do not spawn any processes, "
				"do not make any changes to the file system"},

	{"tag",			0,TAG_TAG,ARGV_OPTARG_REQUIRED,0,
				"CC|CXX|NASM|disable-static|disable-shared",0,
				"a universal playground game; "
				"currently accepted tags are {%s}"},

	{"config",		0,TAG_CONFIG,ARGV_OPTARG_NONE,0,0,0,
				"display configuration information"},

	{"debug",		0,TAG_DEBUG,ARGV_OPTARG_NONE,0,0,0,
				"display internal debug information"},

	{"features",		0,TAG_FEATURES,ARGV_OPTARG_NONE,0,0,0,
				"show feature information"},

	{"legabits",		0,TAG_LEGABITS,ARGV_OPTARG_OPTIONAL,0,
				"enabled|disabled",0,
				"enable/disable legacy bits, i.e. "
				"the installation of an .la wrapper "
				"along with its associated shared library "
				"and/or static archive. option syntax is "
				"--legabits[=%s]"},

	{"no-warnings",		0,TAG_WARNINGS,ARGV_OPTARG_NONE,0,0,0,""},

	{"preserve-dup-deps",	0,TAG_DEPS,ARGV_OPTARG_NONE,0,0,0,
				"leave the dependency list alone"},

	{"annotate",		0,TAG_ANNOTATE,ARGV_OPTARG_REQUIRED,0,
				"always|never|minimal|full",0,
				"modify default annotation options; "
				"the defautls are full annotation when "
				"stdout is a tty, and no annotation "
				"at all otherwise; accepted modes "
				"are {%s}"},

	{"quiet",		0,TAG_SILENT,ARGV_OPTARG_NONE,0,0,0,
				"do not say anything"},

	{"silent",		0,TAG_SILENT,ARGV_OPTARG_NONE,0,0,0,
				"say absolutely nothing"},

	{"verbose",		0,TAG_VERBOSE,ARGV_OPTARG_NONE,0,0,0,
				"generate lots of informational messages "
				"that nobody can understand"},

	{"host",		0,TAG_HOST,ARGV_OPTARG_REQUIRED,0,0,"<host>",
				"set an explicit (cross-)host"},

	{"flavor",		0,TAG_FLAVOR,ARGV_OPTARG_REQUIRED,0,
				"bsd|cygwin|darwin|linux|midipix|mingw",
				0,"explicitly specify the host's flavor; "
				"currently accepted flavors are {%s}"},

	{"ar",			0,TAG_AR,ARGV_OPTARG_REQUIRED,0,0,"<ar>",
				"explicitly specify the archiver to be used"},

	{"ranlib",		0,TAG_RANLIB,ARGV_OPTARG_REQUIRED,0,0,"<ranlib>",
				"explicitly specify the librarian to be used"},

	{"dlltool",		0,TAG_DLLTOOL,ARGV_OPTARG_REQUIRED,0,0,"<dlltool>",
				"explicitly specify the PE import library generator "
				"to be used"},

	{"warnings",		0,TAG_WARNINGS,ARGV_OPTARG_REQUIRED,0,
				"all|none|error",0,
				"set the warning reporting level; "
				"accepted levels are {%s}"},

	{"W",			0,TAG_WARNINGS,ARGV_OPTARG_REQUIRED,
				ARGV_OPTION_HYBRID_ONLY|ARGV_OPTION_HYBRID_JOINED,
				"all|none|error","",
				"convenient shorthands for the above"},

	{"output",		'o',TAG_OUTPUT,ARGV_OPTARG_REQUIRED,0,0,"<file>",
				"write output to %s"},

	{"target",		0,TAG_TARGET,ARGV_OPTARG_REQUIRED,0,0,"<target>",
				"set an explicit (cross-)target, then pass it to "
				"the compiler"},

	{"R",			0,TAG_LDRPATH,ARGV_OPTARG_REQUIRED,
				ARGV_OPTION_HYBRID_ONLY|ARGV_OPTION_HYBRID_CIRCUS,0,0,
				"encode library path into the executable image "
				"[currently a no-op]"},

	{"bindir",		0,TAG_BINDIR,ARGV_OPTARG_REQUIRED,
				ARGV_OPTION_HYBRID_ONLY|ARGV_OPTION_HYBRID_SPACE,0,0,
				"override destination directory when installing "
				"PE shared libraries. Not needed, ignored for "
				"compatibility"},

	{"shrext",		0,TAG_SHREXT,ARGV_OPTARG_REQUIRED,
				ARGV_OPTION_HYBRID_ONLY|ARGV_OPTION_HYBRID_SPACE,0,0,
				"shared library extention, including the dot, that "
				"should override the default for the target system. "
				"regardless of build target and type (native/cross), "
				"you should never need to use this option."},

	{"rpath",		0,TAG_RPATH,ARGV_OPTARG_REQUIRED,
				ARGV_OPTION_HYBRID_ONLY|ARGV_OPTION_HYBRID_SPACE,0,0,
				"where a library should eventually be "
				"installed, relative to $(DESTDIR)$(PREFIX)"},

	{"release",		0,TAG_RELEASE,ARGV_OPTARG_REQUIRED,
				ARGV_OPTION_HYBRID_ONLY|ARGV_OPTION_HYBRID_SPACE,0,0,
				"specify release information; this will result "
				"in the generation of libfoo-x.y.z.so, "
				"followed by the creation of libfoo.so "
				"as a symlink thereto"},

	{"export-symbols",	0,TAG_EXPSYM_FILE,ARGV_OPTARG_REQUIRED,
				ARGV_OPTION_HYBRID_ONLY|ARGV_OPTION_HYBRID_SPACE,
				0,"<symfile>",
				"only export the symbols that are listed in %s "
				"[currently a no-op]"},

	{"export-symbols-regex",0,TAG_EXPSYM_REGEX,ARGV_OPTARG_REQUIRED,
				ARGV_OPTION_HYBRID_ONLY|ARGV_OPTION_HYBRID_SPACE,
				0,"<regex>",
				"only export symbols mathing the regex expression %s "
				"[currently a no-op]"},

	{"module",		0,TAG_MODULE,ARGV_OPTARG_NONE,
				ARGV_OPTION_HYBRID_ONLY,0,0,
				"create a shared object that will only be "
				"loaded at runtime via dlopen(3). the shared "
				"object name need not follow the platform's "
				"library naming conventions"},

	{"all-static",		0,TAG_ALL_STATIC,ARGV_OPTARG_NONE,
				ARGV_OPTION_HYBRID_ONLY,0,0,
				"for programs, create a zero-dependency, "
				"statically linked binary; for libraries, "
				"only create an archive of non-pic objects"},

	{"disable-static",	0,TAG_DISABLE_STATIC,ARGV_OPTARG_NONE,
				ARGV_OPTION_HYBRID_ONLY,0,0,
				"for libraries, only create a shared library, "
				"and accordingly do not create an archive "
				"containing the individual object files. "
				"[currently a no-op]"},

	{"disable-shared",	0,TAG_DISABLE_SHARED,ARGV_OPTARG_NONE,
				ARGV_OPTION_HYBRID_ONLY,0,0,
				"for libraries, only create an archive "
				"containing the individual object files, and "
				"accordingly do not create a shared library"},

	{"avoid-version",	0,TAG_AVOID_VERSION,ARGV_OPTARG_NONE,
				ARGV_OPTION_HYBRID_ONLY,0,0,
				"do not store version information, "
				"do not create version-specific symlinks"},

	{"version-info",	0,TAG_VERSION_INFO,ARGV_OPTARG_REQUIRED,
				ARGV_OPTION_HYBRID_ONLY|ARGV_OPTION_HYBRID_SPACE,0,
				"<current>[:<revision>[:<age>]]",
				"specify version information using a %s syntax"},

	{"version-number",	0,TAG_VERSION_NUMBER,ARGV_OPTARG_REQUIRED,
				ARGV_OPTION_HYBRID_ONLY|ARGV_OPTION_HYBRID_SPACE,0,
				"<major>[:<minor>[:<revision>]]",
				"specify version information using a %s syntax"},

	{"no-suppress",		0,TAG_NO_SUPPRESS,ARGV_OPTARG_NONE,
				ARGV_OPTION_HYBRID_ONLY,0,0,
				"transparently forward all "
				"compiler-generated output"},

	{"no-install",		0,TAG_NO_INSTALL,ARGV_OPTARG_NONE,
				ARGV_OPTION_HYBRID_ONLY,0,0,
				"consider the executable image wrapper "
				"to be optional "
				"[currently a no-op]"},

	{"prefer-pic",		0,TAG_PREFER_PIC,ARGV_OPTARG_NONE,
				ARGV_OPTION_HYBRID_ONLY,0,0,
				"pick on non-PIC objects"},

	{"prefer-non-pic",	0,TAG_PREFER_NON_PIC,ARGV_OPTARG_NONE,
				ARGV_OPTION_HYBRID_ONLY,0,0,
				"pick on PIC objects"},

	{"shared",		0,TAG_SHARED,ARGV_OPTARG_NONE,
				ARGV_OPTION_HYBRID_ONLY,0,0,
				"only build .libs/srcfile.o"},

	{"static",		0,TAG_STATIC,ARGV_OPTARG_NONE,
				ARGV_OPTION_HYBRID_ONLY,0,0,
				"only build ./srcfile.o"},

	{"Wc",			0,TAG_COMPILER_FLAG,ARGV_OPTARG_REQUIRED,
				ARGV_OPTION_HYBRID_ONLY|ARGV_OPTION_HYBRID_COMMA,
				0,"<flag>[,<flag]...",
				"pass comma-separated flags to the compiler; "
				"syntax: -Wc,%s"},

	{"Xcompiler",		0,TAG_VERBATIM_FLAG,ARGV_OPTARG_REQUIRED,
				ARGV_OPTION_HYBRID_ONLY|ARGV_OPTION_HYBRID_SPACE,
				0,"<flag>",
				"pass a raw flag to the compiler"},

	{"no-undefined",	0,TAG_NO_UNDEFINED,ARGV_OPTARG_NONE,
				ARGV_OPTION_HYBRID_ONLY,0,0,
				"disallow unresolved references"},

	/*
	 * Unimplemented options that we simply discard for now
	 */
	{"thread-safe",		0,TAG_UNIMPLEMENTED,ARGV_OPTARG_NONE,
				ARGV_OPTION_HYBRID_ONLY,0,0,
				"pass thread_safe_flag_spec (unimplemented)"},

	{0,0,0,0,0,0,0,0}
};
