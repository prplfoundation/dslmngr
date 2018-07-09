/*
 * dsl.c - glue file for backend implementations
 *
 * Copyright (C) 2018 Inteno Broadband Technology AB. All rights reserved.
 *
 * Author: anjan.chanda@inteno.se
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "xdsl.h"

#ifdef INTELCPE
extern const struct dsl_ops intel_xdsl_ops;
#endif

const struct dsl_ops *dsl_backends[] = {
#ifdef INTELCPE
	&intel_xdsl_ops,
#endif
};

const struct dsl_ops *get_dsl_backend(char *name)
{
	int i;

	for (i = 0; i < sizeof(dsl_backends)/sizeof(dsl_backends[0]); i++)
		if (!strncmp(dsl_backends[i]->name, name,
					strlen(dsl_backends[i]->name)))
			return dsl_backends[i];

	return NULL;
}

int dsl_start(char *name, struct dsl_config *cfg)
{
	const struct dsl_ops *dsl = get_dsl_backend(name);

	if (dsl && dsl->start)
		return dsl->start(name, cfg);

	return -1;
}

int dsl_stop(char *name)
{
	const struct dsl_ops *dsl = get_dsl_backend(name);

	if (dsl && dsl->stop)
		return dsl->stop(name);

	return -1;
}

int dsl_get_max_bitrate(char *name, dsl_long_t *max_rate_kbps)
{
	const struct dsl_ops *dsl = get_dsl_backend(name);

	if (dsl && dsl->get_max_bitrate)
		return dsl->get_max_bitrate(name, max_rate_kbps);

	return -1;
}

int dsl_get_bitrate(char *name, dsl_long_t *rate_kbps)
{
	const struct dsl_ops *dsl = get_dsl_backend(name);

	if (dsl && dsl->get_bitrate)
		return dsl->get_bitrate(name, rate_kbps);

	return -1;
}

int dsl_get_status(char *name, struct dsl_info *info)
{
	const struct dsl_ops *dsl = get_dsl_backend(name);

	if (dsl && dsl->get_status)
		return dsl->get_status(name, info);

	return -1;
}

int dsl_get_stats(char *name, enum dsl_stattype type,
				struct dsl_perfcounters *c)
{
	const struct dsl_ops *dsl = get_dsl_backend(name);

	if (dsl && dsl->get_stats)
		return dsl->get_stats(name, type, c);

	return -1;
}
