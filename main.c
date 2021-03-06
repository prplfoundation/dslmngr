/*
 * main.c - dslmngr application
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <pthread.h>
#include <libubox/blobmsg.h>
#include <libubox/blobmsg_json.h>
#include <libubox/uloop.h>
#include <libubox/ustream.h>
#include <libubox/utils.h>
#include <libubus.h>

#include "dslmngr.h"

#if 0 // Converting netlink to UBUS is done by inbd for now
void *dslmngr_event_main(void *arg)
{
	struct ubus_context *ctx = (struct ubus_context *)arg;

	pthread_t me = pthread_self();
	pthread_setname_np(me, "dslmngr_eventd");

	dslmngr_nl_msgs_handler(ctx);
	return NULL;
}
#endif

int main(int argc, char **argv)
{
	const char *ubus_socket = NULL;
	struct ubus_context *ctx = NULL;
	int ch, ret;
#if 0
	pthread_t evtid;
	pthread_attr_t attr;
#endif

	while ((ch = getopt(argc, argv, "cs:")) != -1) {
		switch (ch) {
		case 's':
			ubus_socket = optarg;
			break;
		default:
			break;
		}
	}
	argc -= optind;
	argv += optind;

#if 0 // Converting netlink to UBUS is done by inbd for now
	if (pthread_attr_init(&attr) == 0) {
		struct sched_param sp;

		pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
		pthread_attr_setschedpolicy(&attr, SCHED_RR);
		sp.sched_priority = 99;
		pthread_attr_setschedparam(&attr, &sp);
	}

	if (pthread_create(&evtid, &attr, &dslmngr_event_main, ctx) != 0)
		fprintf(stderr, "pthread_create error!\n");

	dslmngr_cmd_main(ctx);
#endif

	uloop_init();
	ctx = ubus_connect(ubus_socket);
	if (!ctx) {
		DSLMNGR_LOG(LOG_ERR, "Failed to connect to ubus\n");
		return -1;
	}

	ubus_add_uloop(ctx);

	if (dsl_add_ubus_objects(ctx) != 0)
		goto __ret;

	uloop_run();

__ret:
	ubus_free(ctx);
	uloop_done();

	return ret;
}
