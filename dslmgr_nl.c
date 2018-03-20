/*
 * dslmgr_nl.c - converts netlink messages to UBUS events
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
#include <time.h>
#include <signal.h>

#include <netlink/netlink.h>
#include <netlink/socket.h>
#include <netlink/genl/ctrl.h>
#include <netlink/genl/genl.h>

#include <netlink/attr.h>

#include <libubox/blobmsg_json.h>
#include "libubus.h"

#define PRPL_GENL_NAME	"prpl"
#define PRPL_GENL_GRP	"notify"

/* nl attributes */
enum {
	PRPL_NL_UNSPEC,
	PRPL_NL_MSG,
	__PRPL_NL_MAX,
};
#define PRPL_NL_MAX (__PRPL_NL_MAX - 1)
#define MAX_MSG 128

static struct blob_buf b;
static struct ubus_context *nlctx = NULL;

static struct nla_policy nl_notify_policy[__PRPL_NL_MAX] = {
	[PRPL_NL_MSG] = { .type = NLA_STRING },
};

static struct nlattr *attrs[__PRPL_NL_MAX];

static int dslmgr_ubus_event(char *data)
{
	blob_buf_init(&b, 0);

	if (!blobmsg_add_json_from_string(&b, data)) {
		fprintf(stderr, "Failed to parse message data\n");
		return -1;
	}

	return ubus_send_event(nlctx, NULL, b.head);
}

static int dslmgr_nl_to_ubus_event(struct nl_msg *msg, void *arg)
{
	struct nlmsghdr *nlh = nlmsg_hdr(msg);
	char *data;
	int ret;
	char cmd[MAX_MSG];

	if (!genlmsg_valid_hdr(nlh, 0)){
		fprintf(stderr, "got invalid message\n");
		return 0;
	}

	ret = genlmsg_parse(nlh, 0, attrs, PRPL_NL_MSG, nl_notify_policy);
	if (!ret) {
		if (attrs[PRPL_NL_MSG] ) {
			data = nla_get_string(attrs[PRPL_NL_MSG]);
			dslmgr_ubus_event(data);
		}
	}
	return 0;
}

int dslmgr_nl_msgs_handler(struct ubus_context *ctx)
{
	struct nl_sock *sock;
	int grp;
	int err;

	nlctx = ctx;

	sock = nl_socket_alloc();
	if(!sock){
		fprintf(stderr, "Error: nl_socket_alloc\n");
		return -1;
	}

	nl_socket_disable_seq_check(sock);
	err = nl_socket_modify_cb(sock, NL_CB_VALID, NL_CB_CUSTOM,
				dslmgr_nl_to_ubus_event, NULL);

	if ((err = genl_connect(sock)) < 0){
		fprintf(stderr, "Error: %s\n", nl_geterror(err));
		return -1;
	}

	if ((grp = genl_ctrl_resolve_grp(sock,
					PRPL_GENL_NAME,
					PRPL_GENL_GRP)) < 0) {
		/* fprintf(stderr, "Error: %s (%s grp %s)\n",
				nl_geterror(grp),
				PRPL_GENL_NAME,
				PRPL_GENL_GRP); */
		return -1;
	}

	nl_socket_add_membership(sock, grp);

	while (1) {
		err = nl_recvmsgs_default(sock);
		if (err < 0) {
			fprintf(stderr, "Error: %s (%s grp %s)\n",
					nl_geterror(err),
					PRPL_GENL_NAME,
					PRPL_GENL_GRP);
		}
	}

	return 0;
}
