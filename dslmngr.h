/*
 * dslmngr header file
 *
 * Copyright (C) 2019 iopsys Software Solutions AB. All rights reserved.
 *
 * Author: anjan.chanda@iopsys.eu
 *         yalu.zhang@iopsys.eu
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
#ifndef _DSLMNGR_H
#define _DSLMNGR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <syslog.h>
#include <libubus.h>

#define DSLMNGR_LOG(log_level, format...) fprintf(stderr, ##format)

#define CHECK_POINT() printf("Check point at %s@%s:%d\n", __func__, __FILE__, __LINE__)

int dsl_add_ubus_objects(struct ubus_context *ctx);

#ifdef __cplusplus
}
#endif
#endif /* _DSLMNGR_H */
