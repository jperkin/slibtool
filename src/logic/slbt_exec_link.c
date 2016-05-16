/*******************************************************************/
/*  slibtool: a skinny libtool implementation, written in C        */
/*  Copyright (C) 2016  Z. Gilboa                                  */
/*  Released under the Standard MIT License; see COPYING.SLIBTOOL. */
/*******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#include <slibtool/slibtool.h>
#include "slibtool_spawn_impl.h"
#include "slibtool_mkdir_impl.h"
#include "slibtool_readlink_impl.h"
#include "slibtool_symlink_impl.h"

struct slbt_deps_meta {
	char ** altv;
	char *	args;
	int	depscnt;
	int	infolen;
};

/*******************************************************************/
/*                                                                 */
/* -o <ltlib>  switches              input   result                */
/* ----------  --------------------- -----   ------                */
/* libfoo.a    [-shared|-static]     bar.lo  libfoo.a              */
/*                                                                 */
/* ar cru libfoo.a bar.o                                           */
/* ranlib libfoo.a                                                 */
/*                                                                 */
/*******************************************************************/

/*******************************************************************/
/*                                                                 */
/* -o <ltlib>  switches              input   result                */
/* ----------  --------------------- -----   ------                */
/* libfoo.la   -shared               bar.lo  libfoo.la             */
/*                                           .libs/libfoo.a        */
/*                                           .libs/libfoo.la (lnk) */
/*                                                                 */
/* ar cru .libs/libfoo.a .libs/bar.o                               */
/* ranlib .libs/libfoo.a                                           */
/* (generate libfoo.la)                                            */
/* ln -s ../libfoo.la .libs/libfoo.la                              */
/*                                                                 */
/*******************************************************************/

/*******************************************************************/
/*                                                                 */
/* -o <ltlib>  switches              input   result                */
/* ----------  --------------------- -----   ------                */
/* libfoo.la   -static               bar.lo  libfoo.la             */
/*                                           .libs/libfoo.a        */
/*                                           .libs/libfoo.la (lnk) */
/*                                                                 */
/* ar cru .libs/libfoo.a bar.o                                     */
/* ranlib .libs/libfoo.a                                           */
/* (generate libfoo.la)                                            */
/* ln -s ../libfoo.la .libs/libfoo.la                              */
/*                                                                 */
/*******************************************************************/

static int slbt_exec_link_exit(
	struct slbt_deps_meta *	depsmeta,
	int			ret)
{
	if (depsmeta->altv)
		free(depsmeta->altv);

	if (depsmeta->args)
		free(depsmeta->args);

	return ret;
}

static int slbt_get_deps_meta(
	char *			libfilename,
	struct slbt_deps_meta *	depsmeta)
{
	int		ret;
	FILE *		fdeps;
	struct stat	st;
	char		depfile[4*PATH_MAX];
	char *		deplibs = depfile;

	/* -rpath */
	if ((size_t)snprintf(depfile,sizeof(depfile),"%s.slibtool.rpath",
				libfilename)
			>= sizeof(depfile))
		return -1;

	if (!(lstat(depfile,&st))) {
		/* -Wl,%s */
		depsmeta->infolen += st.st_size + 4;
		depsmeta->infolen++;
	}

	/* .deps */
	if ((size_t)snprintf(depfile,sizeof(depfile),"%s.slibtool.deps",
				libfilename)
			>= sizeof(depfile))
		return -1;

	if ((stat(depfile,&st)))
		return -1;

	if (!(fdeps = fopen(depfile,"r")))
		return -1;

	if ((size_t)st.st_size >= sizeof(depfile))
		if (!(deplibs = malloc(st.st_size+1))) {
			fclose(fdeps);
			return -1;
		}

	depsmeta->infolen += st.st_size;
	depsmeta->infolen++;

	while (fscanf(fdeps,"%s\n",deplibs) == 1)
		depsmeta->depscnt++;

	if (deplibs != depfile)
		free(deplibs);

	ret = ferror(fdeps) ? -1 : 0;
	fclose(fdeps);

	return ret;
}

static bool slbt_adjust_input_argument(
	char *		arg,
	const char *	osuffix,
	const char *	asuffix,
	bool		fpic)
{
	char *	slash;
	char *	dot;
	char	base[PATH_MAX];

	if (*arg == '-')
		return false;

	if (!(dot = strrchr(arg,'.')))
		return false;

	if (strcmp(dot,osuffix))
		return false;

	if (fpic) {
		if ((slash = strrchr(arg,'/')))
			slash++;
		else
			slash = arg;

		if ((size_t)snprintf(base,sizeof(base),"%s",
				slash) >= sizeof(base))
			return false;

		sprintf(slash,".libs/%s",base);
		dot = strrchr(arg,'.');
	}

	strcpy(dot,asuffix);
	return true;
}

static int slbt_adjust_linker_argument(
	char *		arg,
	bool		fpic,
	const char *	dsosuffix,
	const char *	arsuffix,
	struct slbt_deps_meta * depsmeta)
{
	int	fdlib;
	char *	slash;
	char *	dot;
	char	base[PATH_MAX];
	char	slnk[PATH_MAX];

	if (*arg == '-')
		return 0;

	if (!(dot = strrchr(arg,'.')))
		return 0;

	if (strcmp(dot,".la"))
		return 0;

	if (fpic) {
		if ((slash = strrchr(arg,'/')))
			slash++;
		else
			slash = arg;

		if ((size_t)snprintf(base,sizeof(base),"%s",
				slash) >= sizeof(base))
			return 0;

		sprintf(slash,".libs/%s",base);
		dot = strrchr(arg,'.');
	}

	/* shared library dependency? */
	if (fpic) {
		sprintf(dot,"%s",dsosuffix);

		if (!slbt_readlink(arg,slnk,sizeof(slnk))
				&& !(strcmp(slnk,"/dev/null")))
			sprintf(dot,"%s",arsuffix);
		else if ((fdlib = open(arg,O_RDONLY)) >= 0)
			close(fdlib);
		else
			sprintf(dot,"%s",arsuffix);

		return slbt_get_deps_meta(arg,depsmeta);
	}

	/* input archive */
	sprintf(dot,"%s",arsuffix);
	return 0;
}

static int slbt_exec_link_adjust_argument_vector(
	const struct slbt_driver_ctx *	dctx,
	struct slbt_exec_ctx *		ectx,
	struct slbt_deps_meta *		depsmeta,
	const char *			cwd,
	bool				flibrary)
{
	char ** carg;
	char ** aarg;
	char *	slash;
	char *	mark;
	char *	darg;
	char *	dot;
	FILE *	fdeps;
	char *	dpath;
	bool	freqd;
	int	argc;
	char	arg[PATH_MAX];
	char	lib[PATH_MAX];
	char	rpathdir[PATH_MAX];
	char	rpathlnk[PATH_MAX];
	bool	fwholearchive = false;
	struct	stat st;

	for (argc=0,carg=ectx->cargv; *carg; carg++)
		argc++;

	if (!(depsmeta->args = calloc(1,depsmeta->infolen)))
		return -1;

	argc *= 3;
	argc += depsmeta->depscnt;

	if (!(depsmeta->altv = calloc(argc,sizeof(char *))))
		return -1;

	carg = ectx->cargv;
	aarg = depsmeta->altv;
	darg = depsmeta->args;

	for (; *carg; ) {
		dpath = 0;
		freqd = false;

		if (!strcmp(*carg,"-Wl,--whole-archive"))
			fwholearchive = true;
		else if (!strcmp(*carg,"-Wl,--no-whole-archive"))
			fwholearchive = false;



		/* output annotation */
		if (carg == ectx->lout[0]) {
			ectx->mout[0] = &aarg[0];
			ectx->mout[1] = &aarg[1];
		}

		/* argument translation */
		if (**carg == '-') {
			*aarg++ = *carg++;

		} else if (!(dot = strrchr(*carg,'.'))) {
			*aarg++ = *carg++;

		} else if (!(strcmp(dot,".a"))) {
			if (flibrary && !fwholearchive)
				*aarg++ = "-Wl,--whole-archive";

			dpath = lib;
			sprintf(lib,"%s.slibtool.deps",*carg);
			*aarg++ = *carg++;

			if (flibrary && !fwholearchive)
				*aarg++ = "-Wl,--no-whole-archive";

		} else if (strcmp(dot,dctx->cctx->settings.dsosuffix)) {
			*aarg++ = *carg++;

		} else if (carg == ectx->lout[1]) {
			/* ^^^hoppla^^^ */
			*aarg++ = *carg++;
		} else {
			/* -rpath */
			sprintf(rpathlnk,"%s.slibtool.rpath",*carg);

			if (!(lstat(rpathlnk,&st))) {
				if (slbt_readlink(
						rpathlnk,\
						rpathdir,
						sizeof(rpathdir)))
					return -1;

				sprintf(darg,"-Wl,%s",rpathdir);
				*aarg++ = "-Wl,-rpath";
				*aarg++ = darg;
				darg   += strlen(darg);
				darg++;
			}

			dpath = lib;
			freqd = true;
			sprintf(lib,"%s.slibtool.deps",*carg);

			/* account for {'-','L','-','l'} */
			if ((size_t)snprintf(arg,sizeof(arg),"%s",
					*carg) >= (sizeof(arg) - 4))
				return -1;

			if ((slash = strrchr(arg,'/'))) {
				sprintf(*carg,"-L%s",arg);

				mark   = strrchr(*carg,'/');
				*mark  = 0;

				if (ectx->fwrapper) {
					*slash = 0;

					if (fprintf(ectx->fwrapper,
							"DL_PATH=\"$DL_PATH$COLON%s/%s\"\n"
							"COLON=':'\n\n",
							cwd,arg) < 0)
						return -1;
				}

				*aarg++ = *carg++;
				*aarg++ = ++mark;

				++slash;
				slash += strlen(dctx->cctx->settings.dsoprefix);

				sprintf(mark,"-l%s",slash);
				dot  = strrchr(mark,'.');
				*dot = 0;
			} else {
				*aarg++ = *carg++;
			}
		}

		if (dpath) {
			*aarg = darg;

			if ((fdeps = fopen(dpath,"r"))) {
				while (fscanf(fdeps,"%s\n",darg) == 1) {
					*aarg++ = darg;
					darg   += strlen(darg);
					darg++;
				}

				if (ferror(fdeps)) {
					free(depsmeta->altv);
					free(depsmeta->args);
					fclose(fdeps);
					return -1;
				} else {
					fclose(fdeps);
				}
			} else if (freqd) {
				free(depsmeta->altv);
				free(depsmeta->args);
				return -1;
			}
		}
	}

	return 0;
}

static int slbt_exec_link_remove_file(
	const struct slbt_driver_ctx *	dctx,
	struct slbt_exec_ctx *		ectx,
	const char *			target)
{
	(void)ectx;

	/* remove target (if any) */
	if (!(unlink(target)) || (errno == ENOENT))
		return 0;

	if (!(dctx->cctx->drvflags & SLBT_DRIVER_SILENT))
		strerror(errno);

	return -1;
}

static int slbt_exec_link_create_dep_file(
	struct slbt_exec_ctx *	ectx,
	char **			altv,
	const char *		libfilename)
{
	char **	parg;
	char *	popt;
	char *	plib;
	const char *prefix;
	char	cwd[PATH_MAX];
	char	depfile[PATH_MAX];
	bool    pathmode;

	if (ectx->fdeps)
		fclose(ectx->fdeps);

	if (!getcwd(cwd,sizeof(cwd)))
		return -1;

	if ((size_t)snprintf(depfile,sizeof(depfile),"%s.slibtool.deps",
				libfilename)
			>= sizeof(depfile))
		return -1;

	if (!(ectx->fdeps = fopen(depfile,"w")))
		return -1;

	for (parg=altv; *parg; parg++) {
		popt = 0;
		plib = 0;

		if (!strcmp(*parg,"-L")) {
			pathmode = true;
			prefix = "-L";
			popt = *parg++;
			plib = *parg;
		} else if (!strncmp(*parg,"-L",2)) {
			pathmode = true;
			prefix = "-L";
			popt = *parg;
			plib = popt + 2;
		} else if (!strcmp(*parg,"-l")) {
			pathmode = false;
			prefix = "-l";
			popt = *parg++;
			plib = *parg;
		} else if (!strcmp(*parg,"--library")) {
			pathmode = false;
			prefix = "-l";
			popt = *parg++;
			plib = *parg;
		} else if (!strncmp(*parg,"-l",2)) {
			pathmode = false;
			prefix = "-l";
			popt = *parg;
			plib = popt + 2;
		} else if (!strncmp(*parg,"--library=",10)) {
			pathmode = false;
			prefix = "-l";
			popt = *parg;
			plib = popt + 10;
		}

		if (plib) {
			if (!pathmode || plib[0] == '/') {
				if (fprintf(ectx->fdeps,"%s%s\n",prefix,plib) < 0)
					return -1;
			} else {
				if (fprintf(ectx->fdeps,"%s%s/%s\n",prefix,cwd,plib) < 0)
					return -1;
			}
		}

		if ((popt = strrchr(*parg,'.')) && !strcmp(popt,".la")) {
			slbt_adjust_input_argument(*parg,".la",".la",true);
			if ((plib = strrchr(*parg,'/'))) {
				*plib++ = '\0';
				if (fprintf(ectx->fdeps,"-L%s/%s\n",cwd,*parg) < 0)
					return -1;
			} else {
				if (fprintf(ectx->fdeps,"-L%s\n",cwd) < 0)
					return -1;
			}
			/* libfoo.la -> -lfoo */
			plib += 3;
			popt = strrchr(plib, '.');
			*popt = '\0';
			if (fprintf(ectx->fdeps,"-l%s\n",plib) < 0)
				return -1;
		}
	}

	if (fflush(ectx->fdeps))
		return -1;

	return 0;
}

static int slbt_exec_link_create_import_library(
	const struct slbt_driver_ctx *	dctx,
	struct slbt_exec_ctx *		ectx,
	char *				impfilename,
	char *				deffilename,
	char *				soname,
	bool				ftag)
{
	char *	slash;
	char *	dlltool[8];
	char	program[PATH_MAX];
	char	hosttag[PATH_MAX];
	char	hostlnk[PATH_MAX];

	/* libfoo.so.def.{flavor} */
	if (ftag) {
		if ((size_t)snprintf(hosttag,sizeof(hosttag),"%s.%s",
				deffilename,
				dctx->cctx->host.flavor) >= sizeof(hosttag))
			return -1;

		if ((size_t)snprintf(hostlnk,sizeof(hostlnk),"%s.host",
				deffilename) >= sizeof(hostlnk))
			return -1;

		/* libfoo.so.def is under .libs/ */
		if (!(slash = strrchr(deffilename,'/')))
			return -1;

		if (slbt_create_symlink(
				dctx,ectx,
				deffilename,
				hosttag,
				false))
			return -1;

		/* libfoo.so.def.{flavor} is under .libs/ */
		if (!(slash = strrchr(hosttag,'/')))
			return -1;

		if (slbt_create_symlink(
				dctx,ectx,
				++slash,
				hostlnk,
				false))
			return -1;
	}

	/* dlltool argv */
	if ((size_t)snprintf(program,sizeof(program),"%s",
			dctx->cctx->host.dlltool) >= sizeof(program))
		return -1;

	dlltool[0] = program;
	dlltool[1] = "-l";
	dlltool[2] = impfilename;
	dlltool[3] = "-d";
	dlltool[4] = deffilename;
	dlltool[5] = "-D";
	dlltool[6] = soname;
	dlltool[7] = 0;

	/* alternate argument vector */
	ectx->argv    = dlltool;
	ectx->program = program;

	/* step output */
	if (!(dctx->cctx->drvflags & SLBT_DRIVER_SILENT))
		if (slbt_output_link(dctx,ectx))
			return -1;

	/* dlltool spawn */
	if ((slbt_spawn(ectx,true) < 0) || ectx->exitcode)
		return -1;

	return 0;
}

static int slbt_exec_link_create_archive(
	const struct slbt_driver_ctx *	dctx,
	struct slbt_exec_ctx *		ectx,
	const char *			arfilename,
	bool				fpic,
	bool				fprimary)
{
	char ** 	aarg;
	char ** 	parg;
	char *		base;
	char *		mark;
	char *		slash;
	char *		ranlib[3];
	char		program[PATH_MAX];
	char		output [PATH_MAX];
	char		arfile [PATH_MAX];
	char		arlink [PATH_MAX];

	/* initial state */
	slbt_reset_arguments(ectx);

	/* placeholders */
	slbt_reset_placeholders(ectx);

	/* alternate program (ar, ranlib) */
	ectx->program = program;

	/* output */
	if ((size_t)snprintf(output,sizeof(output),"%s",
			arfilename) >= sizeof(output))
		return -1;

	/* ar alternate argument vector */
	if ((size_t)snprintf(program,sizeof(program),"%s",
			dctx->cctx->host.ar) >= sizeof(program))
		return -1;

	aarg    = ectx->altv;
	*aarg++ = program;
	*aarg++ = "cru";
	*aarg++ = output;

	/* input argument adjustment */
	for (parg=ectx->cargv; *parg; parg++)
		if (slbt_adjust_input_argument(*parg,".lo",".o",fpic))
			*aarg++ = *parg;

	*aarg = 0;
	ectx->argv = ectx->altv;

	/* step output */
	if (!(dctx->cctx->drvflags & SLBT_DRIVER_SILENT))
		if (slbt_output_link(dctx,ectx))
			return -1;

	/* remove old archive as needed */
	if (slbt_exec_link_remove_file(dctx,ectx,output))
		return -1;

	/* .deps */
	if (slbt_exec_link_create_dep_file(ectx,ectx->cargv,arfilename))
		return -1;

	/* ar spawn */
	if ((slbt_spawn(ectx,true) < 0) || ectx->exitcode)
		return -1;

#if 0
	/* input objects associated with .la archives */
	for (parg=ectx->cargv; *parg; parg++)
		if (slbt_adjust_input_argument(*parg,".la",".a",fpic))
			if (slbt_archive_import(dctx,ectx,output,*parg))
				return -1;
#endif

	/* ranlib argv */
	if ((size_t)snprintf(program,sizeof(program),"%s",
			dctx->cctx->host.ranlib) >= sizeof(program))
		return -1;

	ranlib[0] = program;
	ranlib[1] = output;
	ranlib[2] = 0;
	ectx->argv = ranlib;

	/* step output */
	if (!(dctx->cctx->drvflags & SLBT_DRIVER_SILENT))
		if (slbt_output_link(dctx,ectx))
			return -1;

	/* ranlib spawn */
	if ((slbt_spawn(ectx,true) < 0) || ectx->exitcode)
		return -1;

	if (fprimary && (dctx->cctx->drvflags & SLBT_DRIVER_DISABLE_SHARED)) {
		strcpy(arlink,output);
		mark  = strrchr(arlink,'/');
		*mark = 0;

		base  = output + (mark - arlink);
		base++;

		if ((slash = strrchr(arlink,'/')))
			slash++;
		else
			slash = arlink;

		strcpy(slash,base);
		sprintf(arfile,".libs/%s",base);

		if (slbt_exec_link_remove_file(dctx,ectx,arlink))
			return -1;

		if (symlink(arfile,arlink))
			return -1;
	}

	return 0;
}

static int slbt_exec_link_create_library(
	const struct slbt_driver_ctx *	dctx,
	struct slbt_exec_ctx *		ectx,
	const char *			dsofilename,
	const char *			dsolinkname)
{
	char ** parg;
	char	cwd    [PATH_MAX];
	char	output [PATH_MAX];
	char	soname [PATH_MAX];
	char	symfile[PATH_MAX];
	struct slbt_deps_meta depsmeta = {0,0,0,0};

	/* initial state */
	slbt_reset_arguments(ectx);

	/* placeholders */
	slbt_reset_placeholders(ectx);

	/* input argument adjustment */
	for (parg=ectx->cargv; *parg; parg++)
		slbt_adjust_input_argument(*parg,".lo",".o",true);

	/* linker argument adjustment */
	for (parg=ectx->cargv; *parg; parg++)
		if (slbt_adjust_linker_argument(
				*parg,true,
				dctx->cctx->settings.dsosuffix,
				dctx->cctx->settings.arsuffix,
				&depsmeta) < 0)
			return -1;

	/* --no-undefined */
	if (dctx->cctx->drvflags & SLBT_DRIVER_NO_UNDEFINED)
		*ectx->noundef = "-Wl,--no-undefined";

	/* -soname */
	if ((dctx->cctx->drvflags & SLBT_DRIVER_IMAGE_MACHO)) {
		(void)0;
	} else if ((dctx->cctx->drvflags & SLBT_DRIVER_AVOID_VERSION) ||
		  (!dctx->cctx->verinfo.verinfo && dctx->cctx->release)) {
		if ((size_t)snprintf(soname,sizeof(soname),"-Wl,%s%s%s%s%s",
					(dctx->cctx->drvflags & SLBT_DRIVER_MODULE)
					    ? "" : dctx->cctx->settings.dsoprefix,
					dctx->cctx->libname,
					(dctx->cctx->release) ? "-" : "",
					(dctx->cctx->release) ? dctx->cctx->release : "",
					dctx->cctx->settings.dsosuffix)
				>= sizeof(soname))
			return -1;

		*ectx->soname  = "-Wl,-soname";
		*ectx->lsoname = soname;
	} else {
		if ((size_t)snprintf(soname,sizeof(soname),"-Wl,%s%s%s%s%s.%d",
					(dctx->cctx->drvflags & SLBT_DRIVER_MODULE)
					    ? "" : dctx->cctx->settings.dsoprefix,
					dctx->cctx->libname,
					(dctx->cctx->release) ? "-" : "",
					(dctx->cctx->release) ? dctx->cctx->release : "",
					dctx->cctx->settings.dsosuffix,
					dctx->cctx->verinfo.major)
				>= sizeof(soname))
			return -1;

		*ectx->soname  = "-Wl,-soname";
		*ectx->lsoname = soname;

	}

	/* PE: --output-def */
	if (dctx->cctx->drvflags & SLBT_DRIVER_IMAGE_PE) {
		if ((size_t)snprintf(symfile,sizeof(symfile),"-Wl,%s",
					ectx->deffilename)
				>= sizeof(output))
			return -1;

		*ectx->symdefs = "-Wl,--output-def";
		*ectx->symfile = symfile;
	}

	/* shared/static */
	if (dctx->cctx->drvflags & SLBT_DRIVER_ALL_STATIC) {
		*ectx->dpic = "-static";
	} else {
		*ectx->dpic = "-shared";
		*ectx->fpic = "-fPIC";
	}

	/* output */
	if ((dctx->cctx->drvflags & SLBT_DRIVER_AVOID_VERSION) ||
	   (!dctx->cctx->verinfo.verinfo && dctx->cctx->release)) {
		strcpy(output,dsofilename);
	} else {
		if ((size_t)snprintf(output,sizeof(output),"%s.%d.%d.%d",
					dsofilename,
					dctx->cctx->verinfo.major,
					dctx->cctx->verinfo.minor,
					dctx->cctx->verinfo.revision)
				>= sizeof(output))
			return -1;
	}

	*ectx->lout[0] = "-o";
	*ectx->lout[1] = output;

	/* ldrpath */
	if (dctx->cctx->host.ldrpath) {
		if (slbt_exec_link_remove_file(dctx,ectx,ectx->rpathfilename))
			return -1;

		if (symlink(dctx->cctx->host.ldrpath,ectx->rpathfilename))
			return -1;
	}

	/* cwd */
	if (!getcwd(cwd,sizeof(cwd)))
		return -1;

	/* .libs/libfoo.so --> -L.libs -lfoo */
	if (slbt_exec_link_adjust_argument_vector(
			dctx,ectx,&depsmeta,cwd,true))
		return -1;

	/* using alternate argument vector */
	ectx->argv    = depsmeta.altv;
	ectx->program = depsmeta.altv[0];

	/* step output */
	if (!(dctx->cctx->drvflags & SLBT_DRIVER_SILENT))
		if (slbt_output_link(dctx,ectx))
			return slbt_exec_link_exit(&depsmeta,-1);

	/* .deps */
	if (slbt_exec_link_create_dep_file(ectx,ectx->argv,dsolinkname))
		return slbt_exec_link_exit(&depsmeta,-1);

	/* spawn */
	if ((slbt_spawn(ectx,true) < 0) || ectx->exitcode)
		return slbt_exec_link_exit(&depsmeta,-1);

	return slbt_exec_link_exit(&depsmeta,0);
}

static int slbt_exec_link_create_executable(
	const struct slbt_driver_ctx *	dctx,
	struct slbt_exec_ctx *		ectx,
	const char *			exefilename)
{
	char ** parg;
	char *	base;
	char	cwd    [PATH_MAX];
	char	output [PATH_MAX];
	char	wrapper[PATH_MAX];
	char	wraplnk[PATH_MAX];
	bool	fabspath;
	bool	fpic;
	const struct slbt_source_version * verinfo;
	struct slbt_deps_meta depsmeta = {0,0,0,0};

	/* initial state */
	slbt_reset_arguments(ectx);

	/* placeholders */
	slbt_reset_placeholders(ectx);

	/* fpic */
	fpic = !(dctx->cctx->drvflags & SLBT_DRIVER_ALL_STATIC);

	/* input argument adjustment */
	for (parg=ectx->cargv; *parg; parg++)
		slbt_adjust_input_argument(*parg,".lo",".o",fpic);

	/* linker argument adjustment */
	for (parg=ectx->cargv; *parg; parg++)
		if (slbt_adjust_linker_argument(
				*parg,true,
				dctx->cctx->settings.dsosuffix,
				dctx->cctx->settings.arsuffix,
				&depsmeta) < 0)
			return -1;

	/* --no-undefined */
	if (dctx->cctx->drvflags & SLBT_DRIVER_NO_UNDEFINED)
		*ectx->noundef = "-Wl,--no-undefined";

	/* executable wrapper: header */
	if ((size_t)snprintf(wrapper,sizeof(wrapper),"%s.wrapper.tmp",
				dctx->cctx->output)
			>= sizeof(wrapper))
		return -1;

	if (!(ectx->fwrapper = fopen(wrapper,"w")))
		return -1;

	verinfo = slbt_source_version();

	if (fprintf(ectx->fwrapper,
			"#!/bin/sh\n"
			"# libtool compatible executable wrapper\n"
			"# Generated by %s (slibtool %d.%d.%d)\n"
			"# [commit reference: %s]\n\n"

			"if [ -z \"$%s\" ]; then\n"
			"\tDL_PATH=\n"
			"\tCOLON=\n"
			"\tLCOLON=\n"
			"else\n"
			"\tDL_PATH=\n"
			"\tCOLON=\n"
			"\tLCOLON=':'\n"
			"fi\n\n",

			dctx->program,
			verinfo->major,verinfo->minor,verinfo->revision,
			verinfo->commit,
			dctx->cctx->settings.ldpathenv) < 0)
		return -1;

	/* output */
	if ((size_t)snprintf(output,sizeof(output),"%s",
				exefilename)
			>= sizeof(output))
		return -1;

	*ectx->lout[0] = "-o";
	*ectx->lout[1] = output;

	/* cwd */
	if (!getcwd(cwd,sizeof(cwd)))
		return -1;

	/* .libs/libfoo.so --> -L.libs -lfoo */
	if (slbt_exec_link_adjust_argument_vector(
			dctx,ectx,&depsmeta,cwd,false))
		return -1;

	/* using alternate argument vector */
	ectx->argv    = depsmeta.altv;
	ectx->program = depsmeta.altv[0];

	/* executable wrapper symlink */
	if ((size_t)snprintf(wraplnk,sizeof(wraplnk),"%s.exe.wrapper",
			dctx->cctx->output) >= sizeof(wraplnk))
		return slbt_exec_link_exit(&depsmeta,-1);

	/* executable wrapper: base name */
	if ((base = strrchr(wraplnk,'/')))
		base++;
	else
		base = wraplnk;

	/* executable wrapper: footer */
	fabspath = (exefilename[0] == '/');

	if (fprintf(ectx->fwrapper,
			"DL_PATH=\"$DL_PATH$LCOLON$%s\"\n\n"
			"export %s=$DL_PATH\n\n"
			"if [ `basename \"$0\"` = \"%s\" ]; then\n"
			"\tprogram=\"$1\"; shift\n"
			"\texec \"$program\" \"$@\"\n"
			"fi\n\n"
			"exec %s/%s \"$@\"\n",
			dctx->cctx->settings.ldpathenv,
			dctx->cctx->settings.ldpathenv,
			base,
			fabspath ? "" : cwd,
			fabspath ? &exefilename[1] : exefilename) < 0)
		return slbt_exec_link_exit(&depsmeta,-1);

	/* step output */
	if (!(dctx->cctx->drvflags & SLBT_DRIVER_SILENT))
		if (slbt_output_link(dctx,ectx))
			return slbt_exec_link_exit(&depsmeta,-1);

	/* spawn */
	if ((slbt_spawn(ectx,true) < 0) || ectx->exitcode)
		return slbt_exec_link_exit(&depsmeta,-1);

	/* executable wrapper: finalize */
	fclose(ectx->fwrapper);
	ectx->fwrapper = 0;

	if (slbt_create_symlink(
			dctx,ectx,
			dctx->cctx->output,wraplnk,
			false))
		return slbt_exec_link_exit(&depsmeta,-1);

	if (rename(wrapper,dctx->cctx->output))
		return slbt_exec_link_exit(&depsmeta,-1);

	if (chmod(dctx->cctx->output,0755))
		return slbt_exec_link_exit(&depsmeta,-1);

	return slbt_exec_link_exit(&depsmeta,0);
}

static int slbt_exec_link_create_library_symlink(
	const struct slbt_driver_ctx *	dctx,
	struct slbt_exec_ctx *		ectx,
	bool				fmajor)
{
	char	target[PATH_MAX];
	char	lnkname[PATH_MAX];

	if ((dctx->cctx->drvflags & SLBT_DRIVER_AVOID_VERSION) ||
	    (!dctx->cctx->verinfo.verinfo && dctx->cctx->release))
		strcpy(target,ectx->dsofilename);
	else
		sprintf(target,"%s.%d.%d.%d",
			ectx->dsofilename,
			dctx->cctx->verinfo.major,
			dctx->cctx->verinfo.minor,
			dctx->cctx->verinfo.revision);

	if (fmajor) {
		if ((dctx->cctx->drvflags & SLBT_DRIVER_AVOID_VERSION) ||
		    (!dctx->cctx->verinfo.verinfo && dctx->cctx->release))
			sprintf(lnkname,"%s.release", ectx->dsolinkname);
		else
			sprintf(lnkname,"%s.%d",
				ectx->dsofilename,
				dctx->cctx->verinfo.major);
	} else
		strcpy(lnkname,ectx->dsolinkname);

	if (fmajor && (dctx->cctx->drvflags & SLBT_DRIVER_IMAGE_PE))
		return slbt_copy_file(
			dctx,ectx,
			target,lnkname);
	else
		return slbt_create_symlink(
			dctx,ectx,
			target,lnkname,
			false);
}

int slbt_exec_link(
	const struct slbt_driver_ctx *	dctx,
	struct slbt_exec_ctx *		ectx)
{
	int			ret;
	const char *		output;
	const char *		header;
	const char *		base;
	char *			dot;
	FILE *			fout;
	struct slbt_exec_ctx *	actx;
	bool			fpic;
	bool			fstaticonly;
	bool			fnover;
	bool			fvernum;
	int			current;
	int			revision;
	int			age;
	char			soname[PATH_MAX];
	char			soxyz [PATH_MAX];
	char			solnk [PATH_MAX];
	char			arname[PATH_MAX];
	const struct slbt_source_version * verinfo;

	/* dry run */
	if (dctx->cctx->drvflags & SLBT_DRIVER_DRY_RUN)
		return 0;

	/* libfoo.so.x.y.z */
	if ((size_t)snprintf(soxyz,sizeof(soxyz),"%s%s%s%s%s.%d.%d.%d",
				(dctx->cctx->drvflags & SLBT_DRIVER_MODULE)
				    ? "" : dctx->cctx->settings.dsoprefix,
				dctx->cctx->libname,
				(dctx->cctx->release) ? "-" : "",
				(dctx->cctx->release) ? dctx->cctx->release : "",
				dctx->cctx->settings.dsosuffix,
				dctx->cctx->verinfo.major,
				dctx->cctx->verinfo.minor,
				dctx->cctx->verinfo.revision)
			>= sizeof(soxyz))
		return -1;

	/* libfoo.so.x */
	if ((dctx->cctx->drvflags & SLBT_DRIVER_AVOID_VERSION) ||
	    (!dctx->cctx->verinfo.verinfo && dctx->cctx->release))
		sprintf(soname,"%s%s%s%s%s",
			(dctx->cctx->drvflags & SLBT_DRIVER_MODULE)
			    ? "" : dctx->cctx->settings.dsoprefix,
			dctx->cctx->libname,
			(dctx->cctx->release) ? "-" : "",
			(dctx->cctx->release) ? dctx->cctx->release : "",
			dctx->cctx->settings.dsosuffix);
	else
		sprintf(soname,"%s%s%s%s%s.%d",
			(dctx->cctx->drvflags & SLBT_DRIVER_MODULE)
			    ? "" : dctx->cctx->settings.dsoprefix,
			dctx->cctx->libname,
			(dctx->cctx->release) ? "-" : "",
			(dctx->cctx->release) ? dctx->cctx->release : "",
			dctx->cctx->settings.dsosuffix,
			dctx->cctx->verinfo.major);

	/* libfoo.so */
	sprintf(solnk,"%s%s%s",
		(dctx->cctx->drvflags & SLBT_DRIVER_MODULE)
			? ""
			: dctx->cctx->settings.dsoprefix,
		dctx->cctx->libname,
		dctx->cctx->settings.dsosuffix);

	/* libfoo.a */
	sprintf(arname,"%s%s%s",
		(dctx->cctx->drvflags & SLBT_DRIVER_MODULE)
			? "" : dctx->cctx->settings.arprefix,
		dctx->cctx->libname,
		dctx->cctx->settings.arsuffix);

	/* context */
	if (ectx)
		actx = 0;
	else if ((ret = slbt_get_exec_ctx(dctx,&ectx)))
		return ret;
	else
		actx = ectx;

	/* output suffix */
	output = dctx->cctx->output;
	dot    = strrchr(output,'.');

	/* .libs directory */
	if (slbt_mkdir(ectx->ldirname)) {
		slbt_free_exec_ctx(actx);
		return -1;
	}

	/* non-pic libfoo.a */
	if (dot && !strcmp(dot,".a"))
		if (slbt_exec_link_create_archive(dctx,ectx,output,false,false)) {
			slbt_free_exec_ctx(actx);
			return -1;
		}

	/* fpic, fstaticonly */
	if (dctx->cctx->drvflags & SLBT_DRIVER_ALL_STATIC) {
		fstaticonly = true;
		fpic        = false;
	} else if (dctx->cctx->drvflags & SLBT_DRIVER_DISABLE_SHARED) {
		fstaticonly = true;
		fpic        = false;
	} else if (dctx->cctx->drvflags & SLBT_DRIVER_SHARED) {
		fstaticonly = false;
		fpic        = true;
	} else {
		fstaticonly = false;
		fpic        = false;
	}

	/* pic libfoo.a */
	if (dot && !strcmp(dot,".la"))
		if (slbt_exec_link_create_archive(
				dctx,ectx,
				ectx->arfilename,
				fpic,true)) {
			slbt_free_exec_ctx(actx);
			return -1;
		}

	/* -all-static library */
	if (fstaticonly && dctx->cctx->libname)
		if (slbt_create_symlink(
				dctx,ectx,
				"/dev/null",
				ectx->dsofilename,
				false))
			return -1;

	/* dynamic library */
	if (dot && !strcmp(dot,".la") && dctx->cctx->rpath && !fstaticonly) {
		/* linking: libfoo.so.x.y.z */
		if (slbt_exec_link_create_library(
				dctx,ectx,
				ectx->dsofilename,
				ectx->dsolinkname)) {
			slbt_free_exec_ctx(actx);
			return -1;
		}

		if (!(dctx->cctx->drvflags & SLBT_DRIVER_AVOID_VERSION)) {
			/* symlink: libfoo.so.x --> libfoo.so.x.y.z */
			if (slbt_exec_link_create_library_symlink(
					dctx,ectx,
					true)) {
				slbt_free_exec_ctx(actx);
				return -1;
			}

			/* symlink: libfoo.so --> libfoo.so.x.y.z */
			if (slbt_exec_link_create_library_symlink(
					dctx,ectx,
					false)) {
				slbt_free_exec_ctx(actx);
				return -1;
			}
		}

		/* PE import libraries */
		if (dctx->cctx->drvflags & SLBT_DRIVER_IMAGE_PE) {
			/* libfoo.x.lib.a */
			if (slbt_exec_link_create_import_library(
					dctx,ectx,
					ectx->pimpfilename,
					ectx->deffilename,
					soname,
					true))
				return -1;

			/* symlink: libfoo.lib.a --> libfoo.x.lib.a */
			if (slbt_create_symlink(
					dctx,ectx,
					ectx->pimpfilename,
					ectx->dimpfilename,
					false))
				return -1;

			/* libfoo.x.y.z.lib.a */
			if (slbt_exec_link_create_import_library(
					dctx,ectx,
					ectx->vimpfilename,
					ectx->deffilename,
					soxyz,
					false))
				return -1;
		}
	}

	/* executable */
	if (!dctx->cctx->rpath && !dctx->cctx->libname) {
		/* linking: .libs/exefilename */
		if (slbt_exec_link_create_executable(
				dctx,ectx,
				ectx->exefilename)) {
			slbt_free_exec_ctx(actx);
			return -1;
		}
	}

	/* no wrapper? */
	if (!dot || strcmp(dot,".la")) {
		slbt_free_exec_ctx(actx);
		return 0;
	}

	/* hey, yo, let's rap it up */
	if (!(fout = fopen(output,"w"))) {
		slbt_free_exec_ctx(actx);
		return -1;
	}

	/* compatible library wrapper */
	current  = 0;
	age      = 0;
	revision = 0;

	if (dctx->cctx->verinfo.verinfo)
		sscanf(dctx->cctx->verinfo.verinfo,"%d:%d:%d",
			&current,&revision,&age);

	fnover  = !!(dctx->cctx->drvflags & SLBT_DRIVER_AVOID_VERSION ||
		    (!dctx->cctx->verinfo.verinfo && dctx->cctx->release));
	fvernum = !!(dctx->cctx->verinfo.vernumber);
	verinfo = slbt_source_version();

	/* wrapper header */
	if (dctx->cctx->drvflags & SLBT_DRIVER_LEGABITS) {
		header = " - a libtool library file\n";

		if ((base = strrchr(output,'/')))
			base++;
		else
			base = output;
	} else {
		header = "libtool compatible library wrapper\n";
		base   = "";
	}

	ret = fprintf(fout,
		"# %s%s"
		"# Generated by %s (slibtool %d.%d.%d)\n"
		"# [commit reference: %s]\n\n"

		"dlname='%s'\n"
		"library_names='%s %s %s'\n"
		"old_library='%s'\n\n"

		"inherited_linker_flags='%s'\n"
		"dependency_libs='%s'\n"
		"weak_library_names='%s'\n\n"

		"current=%d\n"
		"age=%d\n"
		"revision=%d\n\n"

		"installed=%s\n"
		"shouldnotlink=%s\n\n"

		"dlopen='%s'\n"
		"dlpreopen='%s'\n\n"

		"libdir='%s'\n",

		/* wrapper header */
		base,header,

		/* nickname, verinfo */
		dctx->program,
		verinfo->major,verinfo->minor,verinfo->revision,
		verinfo->commit,

		/* dlname */
		soname,

		/* library_names */
		fnover ? soname : soxyz,
		soname,
		solnk,

		/* old_library */
		(dctx->cctx->drvflags & SLBT_DRIVER_DISABLE_STATIC) ? "" : arname,

		/* inherited_linker_flags, dependency_libs, weak_library_names */
		"","","",

		/* current, age, revision */
		fvernum ? dctx->cctx->verinfo.major : current,
		fvernum ? dctx->cctx->verinfo.minor : age,
		fvernum ? dctx->cctx->verinfo.major : revision,

		/* installed, shouldnotlink */
		"no","no",

		/* dlopen, dlpreopen */
		"","",

		/* libdir */
		dctx->cctx->rpath);

	/* wrapper symlink */
	if (ret > 0)
		ret = (slbt_create_symlink(
				dctx,ectx,
				output,
				ectx->lafilename,
				true));

	/* .lai wrapper symlink */
	if (ret == 0)
		ret = (slbt_create_symlink(
				dctx,ectx,
				output,
				ectx->laifilename,
				true));

	/* all done */
	fclose(fout);
	slbt_free_exec_ctx(actx);

	return ret;
}
