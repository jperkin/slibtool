/*******************************************************************/
/*  slibtool: a skinny libtool implementation, written in C        */
/*  Copyright (C) 2016  Z. Gilboa                                  */
/*  Released under the Standard MIT License; see COPYING.SLIBTOOL. */
/*******************************************************************/

#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <slibtool/slibtool.h>

#define SLBT_ARGV_SPARE_PTRS	16

struct slbt_exec_ctx_impl {
	int			argc;
	char *			args;
	struct slbt_exec_ctx	ctx;
	char *			buffer[];
};


static size_t slbt_parse_comma_separated_flags(
	const char *	str,
	int *		argc)
{
	const char * ch;

	for (ch=str; *ch; ch++)
		if (*ch == ',')
			(*argc)++;

	return ch - str;
}


static char * slbt_source_file(char ** argv)
{
	char **	parg;
	char *	ch;

	for (parg=argv; *parg; parg++)
		if ((ch = strrchr(*parg,'.')))
			if ((!(strcmp(++ch,"s")))
					|| (!(strcmp(ch,"S")))
					|| (!(strcmp(ch,"c")))
					|| (!(strcmp(ch,"cc")))
					|| (!(strcmp(ch,"cxx"))))
				return *parg;
	return 0;
}


static struct slbt_exec_ctx_impl * slbt_exec_ctx_alloc(
	const struct slbt_driver_ctx *	dctx)
{
	struct slbt_exec_ctx_impl *	ictx;
	size_t				size;
	int				argc;
	char *				args;
	char *				csrc;
	char **				parg;

	argc = 0;
	csrc = 0;

	/* clerical [worst-case] buffer size (guard, .libs, version) */
	size  = strlen(".lo") + sizeof('\0');
	size +=  6 * (strlen(".libs/") + sizeof('\0'));
	size += 36 * (strlen(".0000") + sizeof('\0'));

	/* buffer size (cargv, -Wc) */
	for (parg=dctx->cctx->cargv; *parg; parg++, argc++)
		if (!(strncmp("-Wc,",*parg,4)))
			size += sizeof('\0') + slbt_parse_comma_separated_flags(
					parg[4],&argc);
		else
			size += sizeof('\0') + strlen(*parg);

	/* buffer size (ldirname, lbasename, lobjname, aobjname, ltobjname) */
	if (dctx->cctx->output)
		size += 4*strlen(dctx->cctx->output);
	else if ((csrc = slbt_source_file(dctx->cctx->cargv)))
		size += 4*strlen(csrc);

	/* alloc */
	if (!(args = malloc(size)))
		return 0;

	size = sizeof(*ictx) + (argc+SLBT_ARGV_SPARE_PTRS)*sizeof(char *);

	if (!(ictx = calloc(1,size))) {
		free(args);
		return 0;
	}

	ictx->args = args;
	ictx->argc = argc;

	ictx->ctx.csrc = csrc;

	return ictx;
}


int  slbt_get_exec_ctx(
	const struct slbt_driver_ctx *	dctx,
	struct slbt_exec_ctx **		ectx)
{
	struct slbt_exec_ctx_impl *	ictx;
	char **				parg;
	char *				ch;
	char *				slash;
	const char *			ref;
	int				i;

	/* alloc */
	if (!(ictx = slbt_exec_ctx_alloc(dctx)))
		return -1;

	/* init with guard for later .lo check */
	ch                = ictx->args + strlen(".lo");
	ictx->ctx.argv    = ictx->buffer;

	/* <compiler> */
	ictx->ctx.program = dctx->cctx->cargv[0];

	/* ldirname, lbasename */
	ref = (dctx->cctx->output)
		? dctx->cctx->output
		: ictx->ctx.csrc;

	if (ref && !ictx->ctx.csrc && (slash = strrchr(ref,'/'))) {
		ictx->ctx.ldirname = ch;
		strcpy(ch,ref);
		ch += slash - ref;
		ch += sprintf(ch,"%s","/.libs/");
		ch++;

		ictx->ctx.lbasename = ch;
		ch += sprintf(ch,"%s",++slash);
		ch++;
	} else if (ref) {
		ictx->ctx.ldirname = ch;
		ch += sprintf(ch,"%s",".libs/");
		ch++;

		ictx->ctx.lbasename = ch;
		slash = strrchr(ref,'/');
		ch += sprintf(ch,"%s",slash ? ++slash : ref);
		ch++;
	}

	/* lbasename suffix */
	if (ref && (dctx->cctx->mode == SLBT_MODE_COMPILE)) {
		if ((ch[-4] == '.') && (ch[-3] == 'l') && (ch[-2] == 'o')) {
			ch[-3] = 'o';
			ch[-2] = '\0';
			ch--;
		} else if (ictx->ctx.csrc) {
			if ((ch = strrchr(ictx->ctx.lbasename,'.'))) {
				*++ch = 'o';
				*++ch = '\0';
				ch++;
			}
		}
	}

	/* cargv, -Wc */
	for (i=0, parg=dctx->cctx->cargv; *parg; parg++, ch++) {
		if (!(strncmp("-Wc,",*parg,4))) {
			strcpy(ch,parg[4]);
			ictx->ctx.argv[i++] = ch;

			for (; *ch; ch++)
				if (*ch == ',') {
					*ch = '\0';
					ictx->ctx.argv[i++] = ch+1;
				}
		} else {
			ictx->ctx.argv[i++] = ch;
			ch += sprintf(ch,"%s",*parg);
		}
	}

	/* placeholders for -DPIC, -fPIC, -c, -o, <output> */
	ictx->ctx.dpic = &ictx->ctx.argv[i++];
	ictx->ctx.fpic = &ictx->ctx.argv[i++];
	ictx->ctx.cass = &ictx->ctx.argv[i++];

	ictx->ctx.lout[0] = &ictx->ctx.argv[i++];
	ictx->ctx.lout[1] = &ictx->ctx.argv[i++];

	/* output file name */
	if (ref) {
		*ictx->ctx.lout[0] = "-o";
		*ictx->ctx.lout[1] = ch;
		ictx->ctx.lobjname = ch;

		ch += sprintf(ch,"%s%s",
			ictx->ctx.ldirname,
			ictx->ctx.lbasename)
			+ sizeof('\0');

		ictx->ctx.aobjname = ch;

		ch += sprintf(ch,"%s",ictx->ctx.ldirname);
		ch -= strlen(".libs/");
		ch += sprintf(ch,"%s",
			ictx->ctx.lbasename)
			+ sizeof('\0');

		ictx->ctx.ltobjname = ch;
		strcpy(ch,ictx->ctx.aobjname);

		if ((ch = strrchr(ch,'.')))
			ch += sprintf(ch,"%s",
				(dctx->cctx->mode == SLBT_MODE_COMPILE)
					? ".lo"
					: ".la")
				+ sizeof('\0');
	}

	*ectx = &ictx->ctx;
	return 0;
}


static int slbt_free_exec_ctx_impl(
	struct slbt_exec_ctx_impl *	ictx,
	int				status)
{
	free(ictx->args);
	free (ictx);
	return status;
}


void slbt_free_exec_ctx(struct slbt_exec_ctx * ctx)
{
	struct slbt_exec_ctx_impl *	ictx;
	uintptr_t			addr;

	if (ctx) {
		addr = (uintptr_t)ctx - offsetof(struct slbt_exec_ctx_impl,ctx);
		ictx = (struct slbt_exec_ctx_impl *)addr;
		slbt_free_exec_ctx_impl(ictx,0);
	}
}


void slbt_reset_placeholders(struct slbt_exec_ctx * ectx)
{
	*ectx->dpic = "-DSLIBTOOL_PLACEHOLDER_DPIC";
	*ectx->fpic = "-DSLIBTOOL_PLACEHOLDER_FPIC";
	*ectx->cass = "-DSLIBTOOL_PLACEHOLDER_COMPILE_ASSEMBLE";

	*ectx->lout[0] = "-DSLIBTOOL_PLACEHOLDER_OUTPUT_SWITCH";
	*ectx->lout[1] = "-DSLIBTOOL_PLACEHOLDER_OUTPUT_FILE";
}