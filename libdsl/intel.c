/*
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
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <net/if.h>
#include <stdbool.h>

#include "xdsl.h"

#include <drv_dsl_cpe_api/drv_dsl_cpe_api_ioctl.h>
#include "intel_dsl_fapi.h"	// FIXME: using private copy now

#define UNUSED(var)	(void)(var)

#define MAX_DSL_LINE    1
static const char *intel_xdsl_device = "/dev/dsl_cpe_api";

static int intel_xdsl_dev_open(uint8_t line)
{
	int fd;
	char dev_name[128];

	if (line >= MAX_DSL_LINE)
		return -1;

	sprintf(dev_name, "%s/%d", intel_xdsl_device, line);
	fd = open(dev_name, O_RDWR);
	if (fd < 0)
		return -1;

	fcntl(fd, F_SETFD, FD_CLOEXEC);

	return fd;
}

static int intel_xdsl_dev_close(int fd)
{
	return close(fd);
}

int intel_xdsl_get_status(char *name, struct dsl_info *info)
{
	struct dsl_fapi_line_obj lineobj, *obj = &lineobj;
	unsigned long size = sizeof(struct dsl_fapi_line_obj);
	DSL_DslModeSelection_t dsl_mode = DSL_MODE_NA;
	DSL_G997_XTUSystemEnabling_t xtu_status;
	DSL_AutobootStatus_t autoboot_status;
	DSL_BandPlanStatus_t profiles_status;
	DSL_G997_LineStatus_t line_status;
	DSL_LineState_t line_state;
	struct dsl_line *line;
	struct dsl_link *link;
	int ret;
	int fd;
	int i;

	UNUSED(name);

	memset(obj, 0x00, sizeof(struct dsl_fapi_line_obj));
	link = &info->link;
	info->num_lines = 0;

	fd = intel_xdsl_dev_open(0);
	if (fd < 0) {
		fprintf(stderr, "Failed to open %s/0\n", intel_xdsl_device);
		return -1;
	}

	memset(&autoboot_status, 0x00, sizeof(DSL_AutobootStatus_t));
	ret = ioctl(fd, DSL_FIO_AUTOBOOT_STATUS_GET, (int)&autoboot_status);
	if ((ret < 0) && (autoboot_status.accessCtl.nReturn < DSL_SUCCESS)) {
		fprintf(stderr, "Autoboot Status: error code %d\n",
			       autoboot_status.accessCtl.nReturn);
		return -1;
	}
	if (autoboot_status.data.nStatus == DSL_AUTOBOOT_STATUS_DISABLED ||
		autoboot_status.data.nStatus == DSL_AUTOBOOT_STATUS_STOPPED) {
		return 0;	/* not enabled */
	}

	info->num_lines = 1;	/* TODO: probe */

	/* line status */
	memset(&line_state, 0x00, sizeof(DSL_LineState_t));
	ret = ioctl(fd, DSL_FIO_LINE_STATE_GET, (int)&line_state);
	if ((ret < 0) && (line_state.accessCtl.nReturn < DSL_SUCCESS)) {
		fprintf(stderr, "LINE_STATE get: error code %d\n",
			       line_state.accessCtl.nReturn);
	} else {
		switch (line_state.data.nLineState) {
		case DSL_LINESTATE_NOT_INITIALIZED:
		case DSL_LINESTATE_NOT_UPDATED:
		case DSL_LINESTATE_DISABLED:
		case DSL_LINESTATE_IDLE_REQUEST:
			link->linestate = LINE_DOWN;
			break;
		case DSL_LINESTATE_IDLE:
			link->linestate = LINE_DOWN;
			break;
		case DSL_LINESTATE_FULL_INIT:
		case DSL_LINESTATE_SHORT_INIT_ENTRY:
		case DSL_LINESTATE_DISCOVERY:
		case DSL_LINESTATE_TRAINING:
		case DSL_LINESTATE_ANALYSIS:
		case DSL_LINESTATE_EXCHANGE:
		case DSL_LINESTATE_SHOWTIME_NO_SYNC:
			link->linestate = LINE_TRAINING;
			break;
		case DSL_LINESTATE_SHOWTIME_TC_SYNC:
			link->linestate = LINE_SHOWTIME;
			break;
		case DSL_LINESTATE_EXCEPTION:
			link->linestate = LINE_UNKNOWN;
			break;
		default:
			link->linestate = LINE_DOWN;
			break;
		}
		link->training_status = link->linestate;
	}

	/* get current xDSL mode */
	memset(&xtu_status, 0x00, sizeof(DSL_G997_XTUSystemEnabling_t));
	ret = ioctl(fd, DSL_FIO_G997_XTU_SYSTEM_ENABLING_STATUS_GET, (int)&xtu_status);
	if ((ret < 0) && (xtu_status.accessCtl.nReturn < DSL_SUCCESS)) {
		fprintf(stderr, "XTSE status get: error code %d\n",
			       xtu_status.accessCtl.nReturn);
	} else {
		if (xtu_status.data.XTSE[7]) {
			dsl_mode = DSL_MODE_VDSL;
			link->mode = MOD_VDSL;
		} else {
			dsl_mode = DSL_MODE_ADSL;
			link->mode = MOD_ADSL2;	// TODO: parse xtse bits
		}
	}

	/* vdsl2 profiles active */
	memset(&profiles_status, 0x00, sizeof(DSL_BandPlanStatus_t));
	ret = ioctl(fd, DSL_FIO_BAND_PLAN_STATUS_GET, (int)&profiles_status);
	if ((ret < 0) && (profiles_status.accessCtl.nReturn < DSL_SUCCESS)) {
		fprintf(stderr, "active vdsl2 profiles get: error code %d\n",
			       profiles_status.accessCtl.nReturn);
	} else {
		link->vdsl2_profile = profiles_status.data.nProfile & 0x1ff;
	}

	/* Max bitrate, SNR, Power, Attn */
	memset(&line_status, 0x00, sizeof(DSL_G997_LineStatus_t));
	line_status.nDirection = DSL_UPSTREAM;
	line_status.nDeltDataType = DSL_DELT_DATA_SHOWTIME;
	ret = ioctl(fd, DSL_FIO_G997_LINE_STATUS_GET, (int)&line_status);
	if ((ret < 0) && (line_status.accessCtl.nReturn < DSL_SUCCESS)) {
		fprintf(stderr, "G997_LINE_STATUS get us: error code %d\n",
			       line_status.accessCtl.nReturn);
	} else {
		link->rate_kbps_max.us = line_status.data.ATTNDR;
		link->snr_margin.us = line_status.data.SNR;
		link->power.us = line_status.data.ACTATP;
		link->attn.us = line_status.data.LATN;
	}

	memset(&line_status, 0x00, sizeof(DSL_G997_LineStatus_t));
	line_status.nDirection = DSL_DOWNSTREAM;
	line_status.nDeltDataType = DSL_DELT_DATA_SHOWTIME;
	ret = ioctl(fd, DSL_FIO_G997_LINE_STATUS_GET, (int)&line_status);
	if ((ret < 0) && (line_status.accessCtl.nReturn < DSL_SUCCESS)) {
		fprintf(stderr, "G997_LINE_STATUS get ds: error code %d\n",
			       line_status.accessCtl.nReturn);
	} else {
		link->rate_kbps_max.ds = line_status.data.ATTNDR;
		link->snr_margin.ds = line_status.data.SNR;
		link->power.ds = line_status.data.ACTATP;
		link->attn.ds = line_status.data.LATN;
	}

	intel_xdsl_dev_close(fd);
	return 0;
}

int intel_xdsl_start(char *name, struct dsl_config *cfg)
{
	//TODO
	return 0;
}

int intel_xdsl_stop(char *name)
{
	//TODO
	return 0;
}

const struct dsl_ops intel_xdsl_ops = {
	.name = "intelcpe",
	.start = intel_xdsl_start,
	.stop = intel_xdsl_stop,
	.get_status = intel_xdsl_get_status,
};
