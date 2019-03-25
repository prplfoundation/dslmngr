/*
 * dslmngr_nl.c - converts netlink messages to UBUS events
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
#include <time.h>
#include <signal.h>
#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/genl.h>
#include <netlink/attr.h>
#include "libubox/blobmsg_json.h"
#include "libubus.h"

#define NETLINK_FAMILY_NAME "easysoc"
#define NETLINK_GROUP_NAME  "notify"

#define MAX_MSG 128

/* nl attributes */
enum {
	XDSL_NL_UNSPEC,
	XDSL_NL_MSG,
	__XDSL_NL_MAX,
};

static struct nla_policy nl_notify_policy[__XDSL_NL_MAX] = {
	[XDSL_NL_MSG] = { .type = NLA_STRING },
};

static struct nlattr *attrs[__XDSL_NL_MAX];

static int dslmngr_ubus_event(struct ubus_context *ctx, char *message)
{
	static struct blob_buf b;
	char event[32];
	char data[128];

	sscanf(message, "%s '%[^\n]s'", event, data);

	blob_buf_init(&b, 0);

	if (!blobmsg_add_json_from_string(&b, data)) {
		fprintf(stderr, "Failed to parse message data: %s\n", data);
		return -1;
	}

	return ubus_send_event(ctx, event, b.head);
}

static int dslmngr_nl_to_ubus_event(struct nl_msg *msg, void *arg)
{
	struct nlmsghdr *nlh = nlmsg_hdr(msg);
	struct ubus_context *ctx = (struct ubus_context *)arg;
	char *message;
	int ret;

	if (!genlmsg_valid_hdr(nlh, 0)){
		fprintf(stderr, "received invalid message\n");
		return 0;
	}

	ret = genlmsg_parse(nlh, 0, attrs, XDSL_NL_MSG, nl_notify_policy);
	if (!ret) {
		if (attrs[XDSL_NL_MSG] ) {
			message = nla_get_string(attrs[XDSL_NL_MSG]);
			dslmngr_ubus_event(ctx, message);
		}
	}
	return 0;
}

int dslmngr_nl_msgs_handler(struct ubus_context *ctx)
{
	struct nl_sock *sock;
	int grp;
	int err;

	sock = nl_socket_alloc();
	if(!sock){
		fprintf(stderr, "Error: nl_socket_alloc\n");
		return -1;
	}

	nl_socket_disable_seq_check(sock);
	err = nl_socket_modify_cb(sock, NL_CB_VALID, NL_CB_CUSTOM,
				dslmngr_nl_to_ubus_event, ctx);

	if ((err = genl_connect(sock)) < 0){
		fprintf(stderr, "Error: %s\n", nl_geterror(err));
		return -1;
	}

	if ((grp = genl_ctrl_resolve_grp(sock,
					NETLINK_FAMILY_NAME,
					NETLINK_GROUP_NAME)) < 0) {
		return -1;
	}

	nl_socket_add_membership(sock, grp);

	while (1) {
		err = nl_recvmsgs_default(sock);
		if (err < 0) {
			fprintf(stderr, "Error: %s (%s grp %s)\n",
					nl_geterror(err),
					NETLINK_FAMILY_NAME,
					NETLINK_GROUP_NAME);
		}
	}

	return 0;
}
