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
	DSL_PM_DataPathCountersTotal_t data_path_counters_total;
	DSL_PM_ChannelCountersTotal_t channel_counters_total;
	DSL_PM_LineSecCountersTotal_t line_counters_total;
	DSL_SystemInterfaceStatus_t system_interface_status;
	DSL_G997_FramingParameterStatus_t framing_param;
	DSL_DslModeSelection_t dsl_mode = DSL_MODE_NA;
	DSL_G997_ChannelStatus_t channel_status;
	DSL_G997_XTUSystemEnabling_t xtu_status;
	DSL_AutobootStatus_t autoboot_status;
	DSL_BandPlanStatus_t profiles_status;
	DSL_G997_LineStatus_t line_status;
	DSL_LineFeature_t line_feature;
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
	line = &info->line[0];

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

	/* trellis coding */
	memset(&line_feature, 0x00, sizeof(DSL_LineFeature_t));
	line_feature.nDirection = DSL_DOWNSTREAM;
	ret = ioctl(fd, DSL_FIO_LINE_FEATURE_STATUS_GET, (int)&line_feature);
	if ((ret < 0) && (line_feature.accessCtl.nReturn < DSL_SUCCESS)) {
		fprintf(stderr, "Trellis get ds: error code %d\n",
					line_feature.accessCtl.nReturn);
	} else {
		link->trellis_enabled.ds = line_feature.data.bTrellisEnable;
	}

	memset(&line_feature, 0x00, sizeof(DSL_LineFeature_t));
	line_feature.nDirection = DSL_UPSTREAM;
	ret = ioctl(fd, DSL_FIO_LINE_FEATURE_STATUS_GET, (int)&line_feature);
	if ((ret < 0) && (line_feature.accessCtl.nReturn < DSL_SUCCESS)) {
		fprintf(stderr, "Trellis get us: error code %d\n",
					line_feature.accessCtl.nReturn);
	} else {
		link->trellis_enabled.us = line_feature.data.bTrellisEnable;
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

	/* TPS-TC */
	memset(&system_interface_status, 0x00, sizeof(DSL_SystemInterfaceStatus_t));
	ret = ioctl(fd, DSL_FIO_SYSTEM_INTERFACE_STATUS_GET, (int)&system_interface_status);
	if ((ret < 0) && (system_interface_status.accessCtl.nReturn < DSL_SUCCESS)) {
		fprintf(stderr, "Link encapsulation used? error code %d\n",
				system_interface_status.accessCtl.nReturn);
	} else {
		switch (system_interface_status.data.nTcLayer) {
		case DSL_TC_ATM:
			link->tc_type = TC_ATM;
			break;

		case DSL_TC_EFM:
		case DSL_TC_EFM_FORCED:
			link->tc_type = TC_PTM;
			break;

		case DSL_TC_AUTO:
			link->tc_type = TC_AUTO;
			break;

		default:
			link->tc_type = TC_RAW;
		}
	}

	/* INP, INPRein, delay, current rate */
	memset(&channel_status, 0x00, sizeof(DSL_G997_ChannelStatus_t));
	channel_status.nChannel = 0;
	channel_status.nDirection = DSL_DOWNSTREAM;
	ret = ioctl(fd, DSL_FIO_G997_CHANNEL_STATUS_GET, (int)&channel_status);
	if ((ret < 0) && (channel_status.accessCtl.nReturn < DSL_SUCCESS)) {
		fprintf(stderr, "G997_CHANNEL_STATUS get ds: error code %d\n",
					channel_status.accessCtl.nReturn);
	} else {
		line->param.delay.ds = channel_status.data.ActualInterleaveDelay;
		line->rate_kbps.ds = channel_status.data.ActualDataRate;
		line->param.inp.ds = channel_status.data.ActualImpulseNoiseProtection;
		line->param.inprein.ds = channel_status.data.ActualImpulseNoiseProtectionRein;
	}

	memset(&channel_status, 0x00, sizeof(DSL_G997_ChannelStatus_t));
	channel_status.nChannel = 0;
	channel_status.nDirection = DSL_UPSTREAM;
	ret = ioctl(fd, DSL_FIO_G997_CHANNEL_STATUS_GET, (int)&channel_status);
	if ((ret < 0) && (channel_status.accessCtl.nReturn < DSL_SUCCESS)) {
		fprintf(stderr, "G997_CHANNEL_STATUS get us: error code %d\n",
					channel_status.accessCtl.nReturn);
	} else {
		line->param.delay.us = channel_status.data.ActualInterleaveDelay;
		line->rate_kbps.us = channel_status.data.ActualDataRate;
		line->param.inp.us = channel_status.data.ActualImpulseNoiseProtection;
		line->param.inprein.us = channel_status.data.ActualImpulseNoiseProtectionRein;
	}

	/* line params - n, r, l, d */
	memset(&framing_param, 0x00, sizeof(DSL_G997_FramingParameterStatus_t));
	framing_param.nChannel = 0;
	framing_param.nDirection = DSL_DOWNSTREAM;
	ret = ioctl(fd, DSL_FIO_G997_FRAMING_PARAMETER_STATUS_GET,
					(int)&framing_param);
	if ((ret < 0) && (framing_param.accessCtl.nReturn < DSL_SUCCESS)) {
		fprintf(stderr, "G997_FRAMING_PARAM get ds: error code %d\n",
				framing_param.accessCtl.nReturn);
	} else {
		line->param.d.ds = framing_param.data.nINTLVDEPTH;
		line->param.n.ds = framing_param.data.nNFEC;
		line->param.r.ds = framing_param.data.nRFEC;
		line->param.l.ds = framing_param.data.nLSYMB;
	}

	memset(&framing_param, 0x00, sizeof(DSL_G997_FramingParameterStatus_t));
	framing_param.nChannel = 0;
	framing_param.nDirection = DSL_UPSTREAM;
	ret = ioctl(fd, DSL_FIO_G997_FRAMING_PARAMETER_STATUS_GET,
					(int)&framing_param);
	if ((ret < 0) && (framing_param.accessCtl.nReturn < DSL_SUCCESS)) {
		fprintf(stderr, "G997_FRAMING_PARAM get us: error code %d\n",
				framing_param.accessCtl.nReturn);
	} else {
		line->param.d.us = framing_param.data.nINTLVDEPTH;
		line->param.n.us = framing_param.data.nNFEC;
		line->param.r.us = framing_param.data.nRFEC;
		line->param.l.us = framing_param.data.nLSYMB;
	}

	/* Performance counters (total) */
	link->perf_cnts.type = STAT_TOTAL;
	memset(&channel_counters_total, 0x00, sizeof(DSL_PM_ChannelCountersTotal_t));
	channel_counters_total.nChannel = 0;
	channel_counters_total.nDirection = DSL_NEAR_END;
	ret = ioctl(fd, DSL_FIO_PM_CHANNEL_COUNTERS_TOTAL_GET,
		    (int)&channel_counters_total);
	if ((ret < 0) && (channel_counters_total.accessCtl.nReturn < DSL_SUCCESS)) {
		fprintf(stderr, "Channel counters near end: error code %d\n",
				channel_counters_total.accessCtl.nReturn);
	} else {
		link->perf_cnts.secs =
			channel_counters_total.total.nElapsedTime;

		line->err.fec.us = channel_counters_total.data.nFEC;
		line->err.crc.us = channel_counters_total.data.nCodeViolations;
	}

	memset(&line_counters_total, 0x00, sizeof(DSL_PM_LineSecCountersTotal_t));
	line_counters_total.nDirection = DSL_NEAR_END;
	ret = ioctl(fd, DSL_FIO_PM_LINE_SEC_COUNTERS_TOTAL_GET,
		    (int)&line_counters_total);
	if ((ret < 0) && (line_counters_total.accessCtl.nReturn < DSL_SUCCESS)) {
		fprintf(stderr, "Stats total near end: error code %d\n",
				line_counters_total.accessCtl.nReturn);
	} else {
		link->perf_cnts.es.us =
			line_counters_total.data.nES;
		link->perf_cnts.ses.us =
			line_counters_total.data.nSES;
	}

	memset(&line_counters_total, 0x00, sizeof(DSL_PM_LineSecCountersTotal_t));
	line_counters_total.nDirection = DSL_FAR_END;
	ret = ioctl(fd, DSL_FIO_PM_LINE_SEC_COUNTERS_TOTAL_GET,
		    (int)&line_counters_total);
	if ((ret < 0) && (line_counters_total.accessCtl.nReturn < DSL_SUCCESS)) {
		fprintf(stderr, "Stats total far end: error code %d\n",
				line_counters_total.accessCtl.nReturn);
	} else {
		link->perf_cnts.es.ds =
			line_counters_total.data.nES;
		link->perf_cnts.ses.ds =
			line_counters_total.data.nSES;
	}

	memset(&channel_counters_total, 0x00, sizeof(DSL_PM_ChannelCountersTotal_t));
	channel_counters_total.nChannel = 0;
	channel_counters_total.nDirection = DSL_FAR_END;
	ret = ioctl(fd, DSL_FIO_PM_CHANNEL_COUNTERS_TOTAL_GET, (int)&channel_counters_total);
	if ((ret < 0) && (channel_counters_total.accessCtl.nReturn < DSL_SUCCESS)) {
		fprintf(stderr, "FEC, CRC far end: error code %d\n",
				channel_counters_total.accessCtl.nReturn);
	} else {
		line->err.fec.ds = channel_counters_total.data.nFEC;
		line->err.crc.us = channel_counters_total.data.nCodeViolations;
	}

	memset(&data_path_counters_total, 0x00, sizeof(DSL_PM_DataPathCountersTotal_t));
	data_path_counters_total.nChannel = 0;
	data_path_counters_total.nDirection = DSL_NEAR_END;
	ret = ioctl(fd, DSL_FIO_PM_DATA_PATH_COUNTERS_TOTAL_GET,
		    (int)&data_path_counters_total);

	if ((ret < 0) && (data_path_counters_total.accessCtl.nReturn < DSL_SUCCESS)) {
		fprintf(stderr, "HEC near end: error code %d\n",
				data_path_counters_total.accessCtl.nReturn);
	} else {
		line->err.hec.us = data_path_counters_total.data.nHEC;
	}

	memset(&data_path_counters_total, 0x00, sizeof(DSL_PM_DataPathCountersTotal_t));
	data_path_counters_total.nChannel = 0;
	data_path_counters_total.nDirection = DSL_FAR_END;
	ret = ioctl(fd, DSL_FIO_PM_DATA_PATH_COUNTERS_TOTAL_GET,
		    (int)&data_path_counters_total);
	if ((ret < 0) && (data_path_counters_total.accessCtl.nReturn < DSL_SUCCESS)) {
		fprintf(stderr, "HEC far end: error code %d\n",
				data_path_counters_total.accessCtl.nReturn);
	} else {
		line->err.hec.ds = data_path_counters_total.data.nHEC;
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
