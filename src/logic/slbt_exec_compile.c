/*******************************************************************/
/*  slibtool: a skinny libtool implementation, written in C        */
/*  Copyright (C) 2016  Z. Gilboa                                  */
/*  Released under the Standard MIT License; see COPYING.SLIBTOOL. */
/*******************************************************************/

#include <string.h>
#include <stdbool.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/stat.h>

#include <slibtool/slibtool.h>
#include "slibtool_spawn_impl.h"
#include "slibtool_mkdir_impl.h"

static int slbt_exec_compile_remove_file(
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

int  slbt_exec_compile(
	const struct slbt_driver_ctx *	dctx,
	struct slbt_exec_ctx *		ectx)
{
	int			ret;
	FILE *			fout;
	struct slbt_exec_ctx *	actx = 0;
	const struct slbt_source_version * verinfo;

	/* dry run */
	if (dctx->cctx->drvflags & SLBT_DRIVER_DRY_RUN)
		return 0;

	/* context */
	if (ectx)
		slbt_reset_placeholders(ectx);
	else if ((ret = slbt_get_exec_ctx(dctx,&ectx)))
		return ret;
	else
		actx = ectx;

	/* remove old .lo wrapper */
	if (slbt_exec_compile_remove_file(dctx,ectx,ectx->ltobjname))
		return -1;

	/* .libs directory */
	if (dctx->cctx->drvflags & SLBT_DRIVER_SHARED)
		if (slbt_mkdir(ectx->ldirname)) {
			slbt_free_exec_ctx(actx);
			return -1;
		}

	/* compile mode */
	ectx->program = ectx->compiler;
	ectx->argv    = ectx->cargv;

	/* shared library object */
	if (dctx->cctx->drvflags & SLBT_DRIVER_SHARED) {
		if (!(dctx->cctx->drvflags & SLBT_DRIVER_ANTI_PIC)) {
			*ectx->dpic = "-DPIC";
			*ectx->fpic = "-fPIC";
		}

		switch (dctx->cctx->tag) {
			case SLBT_TAG_NASM:
				break;

			default:
				*ectx->cass = "-c";
				break;
		}

		*ectx->lout[0] = "-o";
		*ectx->lout[1] = ectx->lobjname;

		if (!(dctx->cctx->drvflags & SLBT_DRIVER_SILENT)) {
			if (slbt_output_compile(dctx,ectx)) {
				slbt_free_exec_ctx(actx);
				return -1;
			}
		}

		if (((ret = slbt_spawn(ectx,true)) < 0) || ectx->exitcode) {
			slbt_free_exec_ctx(actx);
			return -1;
		}
	}

	/* static archive object */
	if (dctx->cctx->drvflags & SLBT_DRIVER_STATIC) {
		slbt_reset_placeholders(ectx);

		if (dctx->cctx->drvflags & SLBT_DRIVER_PRO_PIC) {
			*ectx->dpic = "-DPIC";
			*ectx->fpic = "-fPIC";
		}

		switch (dctx->cctx->tag) {
			case SLBT_TAG_NASM:
				break;

			default:
				*ectx->cass = "-c";
				break;
		}

		*ectx->lout[0] = "-o";
		*ectx->lout[1] = ectx->aobjname;

		if (!(dctx->cctx->drvflags & SLBT_DRIVER_SILENT)) {
			if (slbt_output_compile(dctx,ectx)) {
				slbt_free_exec_ctx(actx);
				return -1;
			}
		}

		if (((ret = slbt_spawn(ectx,true)) < 0) || ectx->exitcode) {
			slbt_free_exec_ctx(actx);
			return -1;
		}
	}

	/* libtool object */
	if (!(fout = fopen(ectx->ltobjname,"w"))) {
		slbt_free_exec_ctx(actx);
		return -1;
	}

	verinfo = slbt_source_version();

	ret = fprintf(fout,
		"# %s - a libtool object file\n"
		"# Generated by %s (slibtool %d.%d.%d)\n"
		"# [commit reference: %s]\n"
		"#\n"
		"# Please DO NOT delete this file!\n"
		"# It is necessary for linking the library.\n\n"

		"# Name of the PIC object.\n"
		"pic_object='%s'\n\n"

		"# Name of the non-PIC object\n"
		"non_pic_object='%s'\n\n",

		ectx->ltobjname,
		dctx->program,
		verinfo->major,verinfo->minor,verinfo->revision,
		verinfo->commit,

		(dctx->cctx->drvflags & SLBT_DRIVER_DISABLE_SHARED)
			? "none"
			: ectx->lobjname,
		(dctx->cctx->drvflags & SLBT_DRIVER_DISABLE_STATIC)
			? "none"
			: ectx->aobjname);

	fclose(fout);
	slbt_free_exec_ctx(actx);

	return (ret > 0) ? 0 : -1;
}
