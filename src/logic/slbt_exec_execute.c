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

int  slbt_exec_execute(
	const struct slbt_driver_ctx *	dctx,
	struct slbt_exec_ctx *		ectx)
{
	int			ret;
	char *			program;
	char *			script;
	char *			base;
	char *			mark;
	char			exeref [PATH_MAX];
	char			wrapper[PATH_MAX];
	struct slbt_exec_ctx *	actx = 0;

	/* dry run */
	if (dctx->cctx->drvflags & SLBT_DRIVER_DRY_RUN)
		return 0;

	/* context */
	if (ectx)
		slbt_disable_placeholders(ectx);
	else if ((ret = slbt_get_exec_ctx(dctx,&ectx)))
		return ret;
	else {
		actx = ectx;
		slbt_disable_placeholders(ectx);
	}

	/* script, program */
	program = ectx->cargv[0];
	script  = ectx->cargv[1];

	/* wrapper */
	if ((size_t)snprintf(wrapper,sizeof(wrapper),"./%s.exe.wrapper",
				script)
			>= sizeof(wrapper)) {
		slbt_free_exec_ctx(actx);
		return -1;
	}

	/* exeref */
	if ((base = strrchr(script,'/')))
		base++;
	else
		base = script;

	strcpy(exeref,script);
	mark = exeref + (base - script);
	sprintf(mark,".libs/%s",base);

	/* swap vector */
	ectx->cargv[0] = wrapper;
	ectx->cargv[1] = program;
	ectx->cargv[2] = exeref;

	/* execute mode */
	ectx->program = script;
	ectx->argv    = ectx->cargv;

	/* step output */
	if (!(dctx->cctx->drvflags & SLBT_DRIVER_SILENT))
		if (slbt_output_execute(dctx,ectx)) {
			slbt_free_exec_ctx(actx);
			return -1;
		}

	execvp(wrapper,ectx->argv);

	slbt_free_exec_ctx(actx);
	return -1;
}
