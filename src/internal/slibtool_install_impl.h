#ifndef SLIBTOOL_INSTALL_IMPL_H
#define SLIBTOOL_INSTALL_IMPL_H

#include "argv/argv.h"

extern const struct argv_option slbt_install_options[];

enum install_tags {
	TAG_INSTALL_HELP,
	TAG_INSTALL_COPY,
	TAG_INSTALL_MKDIR,
	TAG_INSTALL_TARGET_MKDIR,
	TAG_INSTALL_STRIP,
	TAG_INSTALL_PRESERVE,
	TAG_INSTALL_USER,
	TAG_INSTALL_GROUP,
	TAG_INSTALL_MODE,
	TAG_INSTALL_DSTDIR,
};

#endif
