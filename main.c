/*
 * main.c - dslmgr application
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

#include "dslmgr.h"

#define DSLMGR_EVENT_THREAD	"dslmgr_eventd"

static struct ubus_context *ctx = NULL;

static void dslmgr_cmd_main(struct ubus_context *ctx)
{
	int ret;

	ret = ubus_add_object(ctx, &dsl_object);
	if (ret)
		fprintf(stderr, "Failed to add 'xdsl' object: %s\n",
				ubus_strerror(ret));

	uloop_run();
}

void *dslmgr_event_main(void *arg)
{
	pthread_t me = pthread_self();
	pthread_setname_np(me, DSLMGR_EVENT_THREAD);

	dslmgr_nl_msgs_handler(ctx);
	return NULL;
}

int main(int argc, char **argv)
{
	const char *ubus_socket = NULL;
	int ch;
	pthread_t evtid;

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

	uloop_init();
	ctx = ubus_connect(ubus_socket);
	if (!ctx) {
		fprintf(stderr, "Failed to connect to ubus\n");
		return -1;
	}

	ubus_add_uloop(ctx);

	if (pthread_create(&evtid, NULL, &dslmgr_event_main, NULL) != 0)
		fprintf(stderr, "pthread_create error!\n");

	dslmgr_cmd_main(ctx);

	uloop_run();
	ubus_free(ctx);
	uloop_done();

	return 0;
}
