/*
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
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <net/if.h>
#include <stdbool.h>
#include <syslog.h>

#define INCLUDE_DSL_CPE_API_VRX // This is needed by drv_dsl_cpe_api.h
#include "drv_dsl_cpe_api/drv_dsl_cpe_api_ioctl.h"
#include "drv_dsl_cpe_api/drv_dsl_cpe_api.h"
#include "dsl-fapi/dsl_fapi.h"

#include "../xdsl.h"
#include "../utils.h"

/**
 * Mappings among string values and enum ones
 */
struct str_enum_map {
	char *val_str;
	int val_enum;
};

const struct dsl_ops xdsl_ops = {
	.get_line_info = dsl_get_line_info,
	.get_line_stats = dsl_get_line_stats,
	.get_line_stats_interval = dsl_get_line_stats_interval,
	.get_channel_info = dsl_get_channel_info,
	.get_channel_stats = dsl_get_channel_stats,
	.get_channel_stats_interval = dsl_get_channel_stats_interval
};

// TODO: this needs to be updated when supporting DSL bonding
static int max_line_num = 1;
static int max_chan_num = 1;

int dsl_get_line_number(void)
{
	return max_line_num;
}

int dsl_get_channel_number(void)
{
	return max_chan_num;
}

/**
	This function converts a string value to the corresponding enum value.

	\param mappings
		The mapping arrays whose element contains a string value and an enum one.
		This parameter must end with { NULL, -1 }.

	\param str_value
		The string value to be searched.

	\return
		Returns 0 on success. Otherwise a negative value is returned.
*/
static int dsl_get_enum_value(const struct str_enum_map *mappings, const char *str_value)
{
	const struct str_enum_map *element;

	for (element = mappings; element->val_str != NULL; element++) {
		if (strcasecmp(element->val_str, str_value) == 0)
			return element->val_enum;
	}

	return -1;
}

#define OPEN_DSL_FAPI_CTX(dev_num) do { \
				if (dev_num < 0 || dev_num >= max_line_num) \
					return -1; \
				fapi_ctx = fapi_dsl_open(0); \
				if (!fapi_ctx) { \
					LIBDSL_LOG(LOG_ERR, "fapi_dsl_open() failed\n"); \
					return -1; \
				} \
			} while (0)

static const struct str_enum_map if_status[] = {
	{ "Up", IF_UP },
	{ "Down", IF_DOWN },
	{ "Unknown", IF_UNKNOWN },
	{ "Dormant", IF_DORMANT },
	{ "NotPresent", IF_NOTPRESENT },
	{ "LowerLayerDown", IF_LLDOWN },
	{ "Error", IF_ERROR },
	{ NULL, -1 }
};

static enum dsl_if_status ifstatus_str2enum(const char *status_str)
{
	int status = dsl_get_enum_value(if_status, status_str);

	if (status >= 0)
		return (enum dsl_if_status)status;

	return IF_ERROR;
}

static const struct str_enum_map link_status[] = {
	{ "Up", LINK_UP },
	{ "Initializing", LINK_INITIALIZING },
	{ "EstablishingLink", LINK_ESTABLISHING },
	{ "NoSignal", LINK_NOSIGNAL },
	{ "Disabled", LINK_DISABLED },
	{ "Error", LINK_ERROR },
	{ NULL, -1 }
};

static enum dsl_link_status linkstatus_str2enum(const char *status_str)
{
	int status = dsl_get_enum_value(link_status, status_str);

	if (status >= 0)
		return (enum dsl_link_status)status;

	return LINK_ERROR;
}

static const struct str_enum_map profiles[] = {
	{ "8a", VDSL2_8a },
	{ "8b", VDSL2_8b },
	{ "8c", VDSL2_8c },
	{ "8d", VDSL2_8d },
	{ "12a", VDSL2_12a },
	{ "12b", VDSL2_12b },
	{ "17a", VDSL2_17a },
	{ "30a", VDSL2_30a },
	{ "35b", VDSL2_35b },
	{ NULL, -1 }
};

static enum dsl_profile profile_str2enum(const char *prof_str)
{
	int profile = dsl_get_enum_value(profiles, prof_str);

	if (profile >= 0)
		return (enum dsl_profile)profile;

	return (enum dsl_profile)0;
}

static const struct str_enum_map power_states[] = {
	{ "L0", DSL_L0},
	{ "L1", DSL_L1},
	{ "L2", DSL_L2},
	{ "L3", DSL_L3},
	{ "L4", DSL_L4},
	{ NULL, -1 }
};

static enum dsl_power_state powerstate_str2enum(const char *power_state)
{
	int state = dsl_get_enum_value(power_states, power_state);

	if (state >= 0)
		return (enum dsl_power_state)state;

	return (enum dsl_power_state)0;
}

int dsl_get_line_info(int line_num, struct dsl_line *line)
{
	int retval = 0;
	struct fapi_dsl_ctx *fapi_ctx;
	struct dsl_fapi_line_obj obj;
	char *token, *saveptr;
	int num, i;

	// Open the DSL FAPI context
	OPEN_DSL_FAPI_CTX(line_num);

	// Get the data
	memset(&obj, 0, sizeof(obj));
	if (fapi_dsl_line_get(fapi_ctx, &obj) != FAPI_DSL_STATUS_SUCCESS) {
		LIBDSL_LOG(LOG_ERR, "fapi_dsl_line_get() failed\n");
		retval = -1;
		goto __ret;
	}

	// Initialize the output buffer
	memset(line, 0, sizeof(*line));

	/**
	 *  Convert the values
	 */
	line->status = ifstatus_str2enum(obj.status);
	line->upstream = obj.upstream;
	strncpy(line->firmware_version, obj.firmware_version, sizeof(line->firmware_version) - 1);
	line->link_status = linkstatus_str2enum(obj.link_status);

	// Supported standards and currently used standard
	line->standard_supported.use_xtse = true;
	memcpy(line->standard_supported.xtse, obj.xtse, sizeof(line->standard_supported.xtse));
	line->standard_used.use_xtse = true;
	memcpy(line->standard_used.xtse, obj.xtse_used, sizeof(line->standard_used.xtse));

	// Line encoding, it is always DMT for Intel
	line->line_encoding = LE_DMT;

	// Allowed profiles and the currently used profile
	token = strtok_r(obj.allowed_profiles, ",", &saveptr);
	while (token != NULL) {
		line->allowed_profiles |= profile_str2enum(token);
		// Move to the next token
		token = strtok_r(NULL, ",", &saveptr);
	}
	line->current_profile = profile_str2enum(obj.current_profile);

	line->power_management_state = powerstate_str2enum(obj.power_management_state);
	line->success_failure_cause = obj.success_failure_cause;

	// upbokler_pb
	token = strtok_r(obj.upbokler_pb, ",", &saveptr);
	for (num = sizeof (line->upbokler_pb.array) / sizeof (line->upbokler_pb.array[0]);
		 line->upbokler_pb.count < num && token != NULL;
		 line->upbokler_pb.count++, token = strtok_r(NULL, ",", &saveptr)) {
		line->upbokler_pb.array[line->upbokler_pb.count] = atoi(token);
	}

	// rxthrsh_ds
	token = strtok_r(obj.rxthrsh_ds, ",", &saveptr);
	for (num = sizeof (line->rxthrsh_ds.array) / sizeof (line->rxthrsh_ds.array[0]);
		 line->rxthrsh_ds.count < num && token != NULL;
		 line->rxthrsh_ds.count++, token = strtok_r(NULL, ",", &saveptr)) {
		line->rxthrsh_ds.array[line->rxthrsh_ds.count] = atoi(token);
	}

	line->act_ra_mode.us = obj.act_ra_mode_us;
	line->act_ra_mode.ds = obj.act_ra_mode_ds;

	line->snr_mroc_us = obj.snr_mroc_us;

	line->last_state_transmitted.us = obj.last_state_transmitted_upstream;
	line->last_state_transmitted.ds = obj.last_state_transmitted_downstream;

	line->us0_mask = obj.us0_mask;

	line->trellis.us = obj.trellis_us;
	line->trellis.ds = obj.trellis_ds;

	line->act_snr_mode.us = obj.act_snr_mode_us;
	line->act_snr_mode.ds = obj.act_snr_mode_ds;

	line->line_number = obj.line_number;

	/* Note that there is a mistake in DSL FAPI which uses bps instead of kbps */
	line->max_bit_rate.us = obj.upstream_max_bit_rate / 1000;
	line->max_bit_rate.ds = obj.downstream_max_bit_rate / 1000;

	line->noise_margin.us = obj.upstream_noise_margin;
	line->noise_margin.ds = obj.downstream_noise_margin;

	// snr_mpb_us
	token = strtok_r(obj.snr_mpb_us, ",", &saveptr);
	for (num = sizeof (line->snr_mpb_us.array) / sizeof (line->snr_mpb_us.array[0]);
		 line->snr_mpb_us.count < num && token != NULL;
		 line->snr_mpb_us.count++, token = strtok_r(NULL, ",", &saveptr)) {
		line->snr_mpb_us.array[line->snr_mpb_us.count] = atoi(token);
	}

	// snr_mpb_ds
	token = strtok_r(obj.snr_mpb_ds, ",", &saveptr);
	for (num = sizeof (line->snr_mpb_ds.array) / sizeof (line->snr_mpb_ds.array[0]);
		 line->snr_mpb_ds.count < num && token != NULL;
		 line->snr_mpb_ds.count++, token = strtok_r(NULL, ",", &saveptr)) {
		line->snr_mpb_ds.array[line->snr_mpb_ds.count] = atoi(token);
	}

	line->attenuation.us = obj.upstream_attenuation;
	line->attenuation.ds = obj.downstream_attenuation;

	line->power.us = obj.upstream_power;
	line->power.ds = obj.downstream_power;

	// XTU-R vendor ID
	num = (sizeof(line->xtur_vendor) / 2) < sizeof(obj.xtur_vendor) ?
			(sizeof(line->xtur_vendor) / 2) : sizeof(obj.xtur_vendor);
	for (i = 0; i < num; i++)
		sprintf(line->xtur_vendor + i * 2, "%02X", obj.xtur_vendor[i]);

	// XTU-R country
	num = (sizeof(line->xtur_country) / 2) < sizeof(obj.xtur_country) ?
			(sizeof(line->xtur_country) / 2) : sizeof(obj.xtur_country);
	for (i = 0; i < num; i++)
		sprintf(line->xtur_country + i * 2, "%02X", obj.xtur_country[i]);

	line->xtur_ansi_std = obj.xtur_ansi_std;
	line->xtur_ansi_rev = obj.xtur_ansi_rev;

	// XTU-C vendor ID
	num = (sizeof(line->xtuc_vendor) / 2) < (sizeof(obj.xtuc_vendor) / sizeof(obj.xtuc_vendor[0]))?
			(sizeof(line->xtuc_vendor) / 2) : (sizeof(obj.xtuc_vendor) / sizeof(obj.xtuc_vendor[0]));
	for (i = 0; i < num; i++)
		sprintf(line->xtuc_vendor + i * 2, "%02X", obj.xtuc_vendor[i]);

	// XTU-C country
	num = (sizeof(line->xtuc_country) / 2) < sizeof(obj.xtuc_country) ?
			(sizeof(line->xtuc_country) / 2) : (sizeof(obj.xtuc_country) / sizeof(obj.xtuc_country[0]));
	for (i = 0; i < num; i++)
		sprintf(line->xtuc_country + i * 2, "%02X", obj.xtuc_country[i]);

	line->xtuc_ansi_std = obj.xtuc_ansi_std;
	line->xtuc_ansi_rev = obj.xtuc_ansi_rev;

__ret:
	if (fapi_ctx)
		fapi_dsl_close(fapi_ctx);
	return retval;
}

int dsl_get_line_stats(int line_num, struct dsl_line_channel_stats *stats)
{
	int retval = 0;
	struct fapi_dsl_ctx *fapi_ctx;
	struct dsl_fapi_line_stats_obj obj;

	// Open the DSL FAPI context
	OPEN_DSL_FAPI_CTX(line_num);

	// Get the data
	memset(&obj, 0, sizeof(obj));
	if (fapi_dsl_line_stats_get(fapi_ctx, &obj) != FAPI_DSL_STATUS_SUCCESS) {
		LIBDSL_LOG(LOG_ERR, "fapi_dsl_line_stats_get() failed\n");
		retval = -1;
		goto __ret;
	}

	// Initialize the output buffer
	memset(stats, 0, sizeof(*stats));

	// Fill in the output parameter
	stats->total_start = obj.total_start;
	stats->showtime_start = obj.showtime_start;
	stats->last_showtime_start = obj.last_showtime_start;
	stats->current_day_start = obj.current_day_start;
	stats->quarter_hour_start = obj.quarter_hour_start;

__ret:
	if (fapi_ctx)
		fapi_dsl_close(fapi_ctx);
	return retval;
}

int dsl_get_line_stats_interval(int line_num, enum dsl_stats_type type, struct dsl_line_stats_interval *stats)
{
	int retval = 0;
	struct fapi_dsl_ctx *fapi_ctx;
	struct dsl_fapi_line_stats_interval_obj obj;
	enum fapi_dsl_status status;

	// Open the DSL FAPI context
	OPEN_DSL_FAPI_CTX(line_num);

	// Get the data
	memset(&obj, 0, sizeof(obj));
	switch (type) {
	case DSL_STATS_TOTAL:
		status = fapi_dsl_line_stats_total_get(fapi_ctx, &obj);
		break;
	case DSL_STATS_SHOWTIME:
		status = fapi_dsl_line_stats_showtime_get(fapi_ctx, &obj);
		break;
	case DSL_STATS_LASTSHOWTIME:
		status = fapi_dsl_line_stats_last_showtime_get(fapi_ctx, &obj);
		break;
	case DSL_STATS_CURRENTDAY:
		status = fapi_dsl_line_stats_current_day_get(fapi_ctx, &obj);
		break;
	case DSL_STATS_QUARTERHOUR:
		status = fapi_dsl_line_stats_quarter_hour_get(fapi_ctx, &obj);
		break;
	default:
		LIBDSL_LOG(LOG_ERR, "Unknown interval type for DSL line statistics, %d\n", type);
		retval = -1;
		goto __ret;
	}

	if (status != FAPI_DSL_STATUS_SUCCESS) {
		LIBDSL_LOG(LOG_ERR,
			"Failed to call DSL FAPI to retrieve the DSL line statistics interval %d\n", type);
		retval = -1;
		goto __ret;
	}

	// Initialize the output buffer
	memset(stats, 0, sizeof(*stats));

	// Fill in the output parameter
	stats->errored_secs = obj.errored_secs;
	stats->severely_errored_secs = obj.severely_errored_secs;

__ret:
	if (fapi_ctx)
		fapi_dsl_close(fapi_ctx);
	return retval;
}

static const struct str_enum_map link_encaps[] = {
	{ "G.992.3_Annex_K_ATM", G_992_3_ANNEK_K_ATM },
	{ "G.992.3_Annex_K_PTM", G_992_3_ANNEK_K_PTM },
	{ "G.993.2_Annex_K_ATM", G_993_2_ANNEK_K_ATM },
	{ "G.993.2_Annex_K_PTM", G_993_2_ANNEK_K_PTM},
	{ "G.994.1 (Auto)", G_994_1_AUTO },
	{ NULL, -1 }
};

static enum dsl_link_encapsulation linkencap_str2enum(const char *link_encap)
{
	int encap = dsl_get_enum_value(link_encaps, link_encap);

	if (encap >= 0)
		return (enum dsl_link_encapsulation)encap;

	return (enum dsl_link_encapsulation)0;
}

int dsl_get_channel_info(int chan_num, struct dsl_channel *channel)
{
	int retval = 0;
	struct fapi_dsl_ctx *fapi_ctx;
	struct dsl_fapi_channel_obj obj;
	char *token, *saveptr;

	// Open the DSL FAPI context
	OPEN_DSL_FAPI_CTX(chan_num);

	// Get the data
	memset(&obj, 0, sizeof(obj));
	if (fapi_dsl_channel_get(fapi_ctx, &obj) != FAPI_DSL_STATUS_SUCCESS) {
		LIBDSL_LOG(LOG_ERR, "fapi_dsl_channel_get() failed\n");
		retval = -1;
		goto __ret;
	}

	// Initialize the output buffer
	memset(channel, 0, sizeof(*channel));

	/**
	 *  Fill in the output parameter
	 */
	channel->status = ifstatus_str2enum(obj.status);

	// link_encapsulation_supported and link_encapsulation_used
	token = strtok_r(obj.link_encapsulation_supported, ",", &saveptr);
	while (token != NULL) {
		channel->link_encapsulation_supported |= linkencap_str2enum(token);;
		// Move to the next token
		token = strtok_r(NULL, ",", &saveptr);
	}
	channel->link_encapsulation_used = linkencap_str2enum(obj.link_encapsulation_used);

	channel->lpath = obj.lpath;
	channel->intlvdepth = obj.intlvdepth;
	channel->intlvblock = obj.intlvblock;
	channel->actual_interleaving_delay = obj.actual_interleaving_delay;
	channel->actinp = obj.actinp;
	channel->inpreport = obj.inpreport;
	channel->nfec = obj.nfec;
	channel->rfec = obj.rfec;
	channel->lsymb = obj.lsymb;

	/* Note that there is a mistake in DSL FAPI which uses bps instead of kbps */
	channel->curr_rate.us = obj.upstream_curr_rate / 1000;
	channel->curr_rate.ds = obj.downstream_curr_rate / 1000;
	channel->actndr.us = obj.actndr_us / 1000;
	channel->actndr.ds = obj.actndr_ds / 1000;

	channel->actinprein.us = obj.actinprein_us;
	channel->actinprein.ds = obj.actinprein_ds;

__ret:
	if (fapi_ctx)
		fapi_dsl_close(fapi_ctx);
	return retval;
}

int dsl_get_channel_stats(int chan_num, struct dsl_line_channel_stats *stats)
{
	int retval = 0;
	struct fapi_dsl_ctx *fapi_ctx;
	struct dsl_fapi_channel_stats_obj obj;

	// Open the DSL FAPI context
	OPEN_DSL_FAPI_CTX(chan_num);

	// Get the data
	memset(&obj, 0, sizeof(obj));
	if (fapi_dsl_channel_stats_get(fapi_ctx, &obj) != FAPI_DSL_STATUS_SUCCESS) {
		LIBDSL_LOG(LOG_ERR, "fapi_dsl_channel_stats_get() failed\n");
		retval = -1;
		goto __ret;
	}

	// Initialize the output buffer
	memset(stats, 0, sizeof(*stats));

	// Fill in the output parameter
	stats->total_start = obj.total_start;
	stats->showtime_start = obj.showtime_start;
	stats->last_showtime_start = obj.last_showtime_start;
	stats->current_day_start = obj.current_day_start;
	stats->quarter_hour_start = obj.quarter_hour_start;

__ret:
	if (fapi_ctx)
		fapi_dsl_close(fapi_ctx);
	return retval;
}

int dsl_get_channel_stats_interval(int chan_num, enum dsl_stats_type type, struct dsl_channel_stats_interval *stats)
{
	int retval = 0;
	struct fapi_dsl_ctx *fapi_ctx;
	struct dsl_fapi_channel_stats_interval_obj obj;
	enum fapi_dsl_status status;

	// Open the DSL FAPI context
	OPEN_DSL_FAPI_CTX(chan_num);

	// Get the data
	memset(&obj, 0, sizeof(obj));
	switch (type) {
	case DSL_STATS_TOTAL:
		status = fapi_dsl_channel_stats_total_get(fapi_ctx, &obj);
		break;
	case DSL_STATS_SHOWTIME:
		status = fapi_dsl_channel_stats_showtime_get(fapi_ctx, &obj);
		break;
	case DSL_STATS_LASTSHOWTIME:
		status = fapi_dsl_channel_stats_last_showtime_get(fapi_ctx, &obj);
		break;
	case DSL_STATS_CURRENTDAY:
		status = fapi_dsl_channel_stats_current_day_get(fapi_ctx, &obj);
		break;
	case DSL_STATS_QUARTERHOUR:
		status = fapi_dsl_channel_stats_quarter_hour_get(fapi_ctx, &obj);
		break;
	default:
		LIBDSL_LOG(LOG_ERR, "Unknown interval type for DSL channel statistics, %d\n", type);
		retval = -1;
		goto __ret;
	}

	if (status != FAPI_DSL_STATUS_SUCCESS) {
		LIBDSL_LOG(LOG_ERR,
			"Failed to call DSL FAPI to retrieve the channel statistics interval %d\n", type);
		retval = -1;
		goto __ret;
	}

	// Initialize the output buffer
	memset(stats, 0, sizeof(*stats));

	// Fill in the output parameter
	stats->xtur_fec_errors = obj.xtu_rfec_errors;
	stats->xtuc_fec_errors = obj.xtu_cfec_errors;

	stats->xtur_hec_errors = obj.xtu_rhec_errors;
	stats->xtuc_hec_errors = obj.xtu_chec_errors;

	stats->xtur_crc_errors = obj.xtu_rcrc_errors;
	stats->xtuc_crc_errors = obj.xtu_ccrc_errors;

__ret:
	if (fapi_ctx)
		fapi_dsl_close(fapi_ctx);
	return retval;
}
