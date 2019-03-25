/*
 * dslmngr.c - provides "dsl" UBUS object
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
#include <libubox/blobmsg.h>
#include <libubox/blobmsg_json.h>
#include <libubox/uloop.h>
#include <libubox/ustream.h>
#include <libubox/utils.h>
#include <uci.h>

#include "xdsl.h"
#include "dslmngr.h"

#define DSL_OBJECT_LINE "line"
#define DSL_OBJECT_CHANNEL "channel"

struct value2text {
	int value;
	char *text;
};

enum {
	DSL_STATS_INTERVAL,
	__DSL_STATS_MAX,
};

static const struct blobmsg_policy dsl_stats_policy[__DSL_STATS_MAX] = {
	[DSL_STATS_INTERVAL] = { .name = "interval", .type = BLOBMSG_TYPE_STRING },
};

static const char *dsl_if_status_str(enum dsl_if_status status)
{
	switch (status) {
	case IF_UP: return "up";
	case IF_DOWN: return "down";
	case IF_DORMANT: return "dormant";
	case IF_NOTPRESENT: return "not_present";
	case IF_LLDOWN: return "lower_layer_down";
	case IF_ERROR: return "error";
	case IF_UNKNOWN:
	default: return "unknown";
	}
}

static const char *dsl_link_status_str(enum dsl_link_status status)
{
	switch (status) {
	case LINK_UP: return "up";
	case LINK_INITIALIZING: return "initializing";
	case LINK_ESTABLISHING: return "establishing";
	case LINK_NOSIGNAL: return "no_signal";
	case LINK_DISABLED: return "disabled";
	case LINK_ERROR: return "error";
	default: return "unknown";
	}
}

static const char *dsl_mod_str(enum dsl_modtype mod)
{
	switch (mod) {
	case MOD_G_922_1_ANNEX_A: return "gdmt_annexa";
	case MOD_G_922_1_ANNEX_B: return "gdmt_annexb";
	case MOD_G_922_1_ANNEX_C: return "gdmt_annexc";
	case MOD_T1_413: return "t1413";
	case MOD_T1_413i2: return "t1413_i2";
	case MOD_ETSI_101_388: return "etsi_101_388";
	case MOD_G_992_2: return "glite";
	case MOD_G_992_3_Annex_A: return "adsl2_annexa";
	case MOD_G_992_3_Annex_B: return "adsl2_annexb";
	case MOD_G_992_3_Annex_C: return "adsl2_annexc";
	case MOD_G_992_3_Annex_I: return "adsl2_annexi";
	case MOD_G_992_3_Annex_J: return "adsl2_annexj";
	case MOD_G_992_3_Annex_L: return "adsl2_annexl";
	case MOD_G_992_3_Annex_M: return "adsl2_annexm";
	case MOD_G_992_4: return "splitterless_adsl2";
	case MOD_G_992_5_Annex_A: return "adsl2p_annexa";
	case MOD_G_992_5_Annex_B: return "adsl2p_annexb";
	case MOD_G_992_5_Annex_C: return "adsl2p_annexc";
	case MOD_G_992_5_Annex_I: return "adsl2p_annexi";
	case MOD_G_992_5_Annex_J: return "adsl2p_annexj";
	case MOD_G_992_5_Annex_M: return "adsl2p_annexm";
	case MOD_G_993_1: return "vdsl";
	case MOD_G_993_1_Annex_A: return "vdsl_annexa";
	case MOD_G_993_2_Annex_A: return "vdsl2_annexa";
	case MOD_G_993_2_Annex_B: return "vdsl2_annexb";
	case MOD_G_993_2_Annex_C: return "vdsl2_annexc";
	default: return "unknown";
	}
}

static const char *dsl_line_encoding_str(enum dsl_line_encoding encoding)
{
	switch (encoding) {
	case LE_DMT: return "dmt";
	case LE_CAP: return "cap";
	case LE_2B1Q: return "2b1q";
	case LE_43BT: return "43bt";
	case LE_PAM: return "pam";
	case LE_QAM: return "qam";
	default: return "unknown";
	}
}

static const char *dsl_profile_str(enum dsl_profile profile)
{
	switch (profile) {
	case VDSL2_8a: return "8a";
	case VDSL2_8b: return "8b";
	case VDSL2_8c: return "8c";
	case VDSL2_8d: return "8b";
	case VDSL2_12a: return "12a";
	case VDSL2_12b: return "12b";
	case VDSL2_17a: return "17a";
	case VDSL2_30a: return "30a";
	case VDSL2_35b: return "35b";
	default: return "unknown";
	}
};

static const char *dsl_power_state_str(enum dsl_power_state power_state)
{
	switch (power_state) {
	case DSL_L0: return "l0";
	case DSL_L1: return "l1";
	case DSL_L2: return "l2";
	case DSL_L3: return "l3";
	case DSL_L4: return "l4";
	default: return "unknown";
	}
};

static void dsl_add_sequence_to_blob(const char *name, bool is_signed, size_t count,
				const long *head, struct blob_buf *bb)
{
	void *array;
	size_t i;

	array = blobmsg_open_array(bb, name);
	for (i = 0; i < count; i++) {
		if (is_signed)
			blobmsg_add_u32(bb, "", (uint32_t)head[i]);
		else
			blobmsg_add_u64(bb, "", (uint64_t)head[i]);
	}
	blobmsg_close_array(bb, array);
}

static void dsl_add_usds_to_blob(const char *name, bool is_signed, const long *head, struct blob_buf *bb)
{
	void *table;
	int i;

	table = blobmsg_open_table(bb, name);
	for (i = 0; i < 2; i++) {
		if (is_signed)
			blobmsg_add_u32(bb, i == 0 ? "us" : "ds", (uint32_t)head[i]);
		else
			blobmsg_add_u64(bb, i == 0 ? "us" : "ds", (uint64_t)head[i]);
	}
	blobmsg_close_table(bb, table);
}

static void dsl_add_int_to_blob(const char *name, long int value, struct blob_buf *bb)
{
	blobmsg_add_u32(bb, name, (uint32_t)value);
}

static void dsl_status_line_to_blob(const struct dsl_line *line, struct blob_buf *bb)
{
	void *array;
	int i;
	unsigned long opt;
	char str[64];

	/*
	 * Put most important information at the beginning
	 */
	blobmsg_add_string(bb, "status", dsl_if_status_str(line->status));
	blobmsg_add_u8(bb, "upstream", line->upstream);
	blobmsg_add_string(bb, "firmware_version", line->firmware_version);
	blobmsg_add_string(bb, "link_status", dsl_link_status_str(line->link_status));
	// standard_used
	if (line->standard_used.use_xtse) {
		array = blobmsg_open_array(bb, "xtse_used");
		for (i = 0; i < ARRAY_SIZE(line->standard_used.xtse); i++) {
			snprintf(str, sizeof(str), "%02x", line->standard_used.xtse[i]);
			blobmsg_add_string(bb, "", str);
		}
		blobmsg_close_array(bb, array);
	} else {
		for (opt = (unsigned long)MOD_G_922_1_ANNEX_A;
			 opt <= (unsigned long)MOD_G_993_2_Annex_C;
			 opt <<= 1) {
			if (line->standard_used.mode & opt) {
				blobmsg_add_string(bb, "standard_used", dsl_mod_str(opt));
				break;
			}
		}
	}
	// current_profile
	blobmsg_add_string(bb, "current_profile", dsl_profile_str(line->current_profile));
	blobmsg_add_string(bb, "power_management_state", dsl_power_state_str(line->power_management_state));
	// max_bit_rate
	unsigned long rates[] = { line->max_bit_rate.us, line->max_bit_rate.ds };
	dsl_add_usds_to_blob("max_bit_rate", false, rates, bb);

	blobmsg_add_string(bb, "line_encoding", dsl_line_encoding_str(line->line_encoding));

	// standards_supported
	if (line->standard_supported.use_xtse) {
		array = blobmsg_open_array(bb, "xtse");
		for (i = 0; i < ARRAY_SIZE(line->standard_supported.xtse); i++) {
			snprintf(str, sizeof(str), "%02x", line->standard_supported.xtse[i]);
			blobmsg_add_string(bb, "", str);
		}
		blobmsg_close_array(bb, array);
	} else {
		array = blobmsg_open_array(bb, "standards_supported");
		for (opt = (unsigned long)MOD_G_922_1_ANNEX_A;
			 opt <= (unsigned long)MOD_G_993_2_Annex_C;
			 opt <<= 1) {
			if (line->standard_supported.mode & opt)
				blobmsg_add_string(bb, "", dsl_mod_str(opt));
		}
		blobmsg_close_array(bb, array);
	}

	// allowed_profiles
	array = blobmsg_open_array(bb, "allowed_profiles");
	for (opt = (unsigned long)VDSL2_8a; opt <= (unsigned long)VDSL2_35b; opt <<= 1) {
		if (line->allowed_profiles & opt)
			blobmsg_add_string(bb, "", dsl_profile_str(opt));
	}
	blobmsg_close_array(bb, array);

	blobmsg_add_u32(bb, "success_failure_cause", line->success_failure_cause);

	// upbokler_pb
	dsl_add_sequence_to_blob("upbokler_pb", false, line->upbokler_pb.count,
			(const long *)line->upbokler_pb.array, bb);

	// rxthrsh_ds
	dsl_add_sequence_to_blob("rxthrsh_ds", false, line->rxthrsh_ds.count,
			(const long *)line->rxthrsh_ds.array, bb);

	// act_ra_mode
	unsigned long ra_modes[] = { line->act_ra_mode.us, line->act_ra_mode.ds };
	dsl_add_usds_to_blob("act_ra_mode", false, ra_modes, bb);

	blobmsg_add_u64(bb, "snr_mroc_us", line->snr_mroc_us);

	// last_state_transmitted
	unsigned long lst[] = { line->last_state_transmitted.us, line->last_state_transmitted.ds };
	dsl_add_usds_to_blob("last_state_transmitted", false, lst, bb);

	// us0_mask
	blobmsg_add_u64(bb, "us0_mask", line->us0_mask);

	// trellis
	long trellis[] = { line->trellis.us, line->trellis.ds };
	dsl_add_usds_to_blob("trellis", true, trellis, bb);

	// act_snr_mode
	unsigned long snr_modes[] = { line->act_snr_mode.us, line->act_snr_mode.ds };
	dsl_add_usds_to_blob("act_snr_mode", false, snr_modes, bb);

	// line_number
	dsl_add_int_to_blob("line_number", line->line_number, bb);

	// noise_margin
	long margins[] = { line->noise_margin.us, line->noise_margin.ds };
	dsl_add_usds_to_blob("noise_margin", true, margins, bb);

	// snr_mpb_us
	dsl_add_sequence_to_blob("snr_mpb_us", true, line->snr_mpb_us.count,
			(const long *)line->snr_mpb_us.array, bb);

	// snr_mpb_ds
	dsl_add_sequence_to_blob("snr_mpb_ds", true, line->snr_mpb_ds.count,
			(const long *)line->snr_mpb_ds.array, bb);

	// attenuation
	long attenuations[] = { line->attenuation.us, line->attenuation.ds };
	dsl_add_usds_to_blob("attenuation", true, attenuations, bb);

	// power
	long powers[] = { line->power.us, line->power.ds };
	dsl_add_usds_to_blob("power", true, powers, bb);

	blobmsg_add_string(bb, "xtur_vendor", line->xtur_vendor);
	blobmsg_add_string(bb, "xtur_country", line->xtur_country);
	blobmsg_add_u64(bb, "xtur_ansi_std", line->xtur_ansi_std);
	blobmsg_add_u64(bb, "xtur_ansi_rev", line->xtur_ansi_rev);

	blobmsg_add_string(bb, "xtuc_vendor", line->xtuc_vendor);
	blobmsg_add_string(bb, "xtuc_country", line->xtuc_country);
	blobmsg_add_u64(bb, "xtuc_ansi_std", line->xtuc_ansi_std);
	blobmsg_add_u64(bb, "xtuc_ansi_rev", line->xtuc_ansi_rev);
}

static const char *dsl_link_encap_str(enum dsl_link_encapsulation encap)
{
	switch (encap) {
	case G_992_3_ANNEK_K_ATM: return "adsl2_atm";
	case G_992_3_ANNEK_K_PTM: return "adsl2_ptm";
	case G_993_2_ANNEK_K_ATM: return "vdsl2_atm";
	case G_993_2_ANNEK_K_PTM: return "vdsl2_ptm";
	case G_994_1_AUTO: return "auto";
	default: return "unknown";
	}
};

static void dsl_status_channel_to_blob(const struct dsl_channel *channel, struct blob_buf *bb)
{
	void *array;
	unsigned long opt;

	blobmsg_add_string(bb, "status", dsl_if_status_str(channel->status));
	blobmsg_add_string(bb, "link_encapsulation_used", dsl_link_encap_str(channel->link_encapsulation_used));

	// curr_rate
	unsigned long curr_rates[] = { channel->curr_rate.us, channel->curr_rate.ds };
	dsl_add_usds_to_blob("curr_rate", false, curr_rates, bb);

	// actndr
	unsigned long act_rates[] = { channel->actndr.us, channel->actndr.ds };
	dsl_add_usds_to_blob("actndr", false, act_rates, bb);

	// link_encapsulation_supported
	array = blobmsg_open_array(bb, "link_encapsulation_supported");
	for (opt = (unsigned long)G_992_3_ANNEK_K_ATM; opt <= (unsigned long)G_994_1_AUTO; opt <<= 1) {
		if (channel->link_encapsulation_supported & opt)
			blobmsg_add_string(bb, "", dsl_link_encap_str(opt));
	}
	blobmsg_close_array(bb, array);

	blobmsg_add_u64(bb, "lpath", channel->lpath);
	blobmsg_add_u64(bb, "intlvdepth", channel->intlvdepth);
	dsl_add_int_to_blob("intlvblock", channel->intlvblock, bb);
	blobmsg_add_u64(bb, "actual_interleaving_delay", channel->actual_interleaving_delay);
	dsl_add_int_to_blob("actinp", channel->actinp, bb);
	blobmsg_add_u8(bb, "inpreport", channel->inpreport);
	dsl_add_int_to_blob("nfec", channel->nfec, bb);
	dsl_add_int_to_blob("rfec", channel->rfec, bb);
	dsl_add_int_to_blob("lsymb", channel->lsymb, bb);

	// actinprein
	unsigned long act_inps[] = { channel->actinprein.us, channel->actinprein.ds };
	dsl_add_usds_to_blob("actinprein", false, act_inps, bb);
}

static int dsl_status_all(struct ubus_context *ctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method, struct blob_attr *msg)
{
	static struct blob_buf bb;
	struct dsl_line line;
	struct dsl_channel channel;
	int retval = UBUS_STATUS_OK;
	int i, max_line;
	void *array_line, *array_chan, *table_line, *table_chan;

	// Initialize the buffer
	memset(&bb, 0, sizeof(bb));
	blob_buf_init(&bb, 0);

	array_line = blobmsg_open_array(&bb, DSL_OBJECT_LINE);
	for (i = 0, max_line = dsl_get_line_number(); i < max_line; i++) {
		if (xdsl_ops.get_line_info == NULL || (*xdsl_ops.get_line_info)(i, &line) != 0 ||
			xdsl_ops.get_channel_info == NULL || (*xdsl_ops.get_channel_info)(i, &channel) != 0) {
			retval = UBUS_STATUS_UNKNOWN_ERROR;
			goto __ret;
		}

		// Line table
		table_line = blobmsg_open_table(&bb, "");

		// Line parameters
		blobmsg_add_u32(&bb, "id", (unsigned int)i);
		dsl_status_line_to_blob(&line, &bb);

		// Embed channel(s) inside a line in the format channel: [{},{}...]
		array_chan = blobmsg_open_array(&bb, DSL_OBJECT_CHANNEL);
		table_chan = blobmsg_open_table(&bb, "");
		// Channel parameters
		blobmsg_add_u32(&bb, "id", 0);
		dsl_status_channel_to_blob(&channel, &bb);
		blobmsg_close_table(&bb, table_chan);
		blobmsg_close_array(&bb, array_chan);

		blobmsg_close_table(&bb, table_line);
	}
	blobmsg_close_array(&bb, array_line);

	// Send the reply
	ubus_send_reply(ctx, req, bb.head);

__ret:
	blob_buf_free(&bb);
	return retval;
}

static struct value2text dsl_stats_types[] = {
	{ DSL_STATS_TOTAL, "total" },
	{ DSL_STATS_SHOWTIME, "showtime" },
	{ DSL_STATS_LASTSHOWTIME, "lastshowtime" },
	{ DSL_STATS_CURRENTDAY, "currentday" },
	{ DSL_STATS_QUARTERHOUR, "quarterhour" }
};

static void dsl_stats_to_blob(const struct dsl_line_channel_stats *stats, struct blob_buf *bb)
{
	blobmsg_add_u64(bb, "total_start", stats->total_start);
	blobmsg_add_u64(bb, "showtime_start", stats->showtime_start);
	blobmsg_add_u64(bb, "last_showtime_start", stats->last_showtime_start);
	blobmsg_add_u64(bb, "current_day_start", stats->current_day_start);
	blobmsg_add_u64(bb, "quarter_hour_start", stats->quarter_hour_start);
}

static void dsl_stats_line_interval_to_blob(const struct dsl_line_stats_interval *stats, struct blob_buf *bb)
{
	blobmsg_add_u64(bb, "errored_secs", stats->errored_secs);
	blobmsg_add_u64(bb, "severely_errored_secs", stats->severely_errored_secs);
}

static void dsl_stats_channel_interval_to_blob(const struct dsl_channel_stats_interval *stats, struct blob_buf *bb)
{
	blobmsg_add_u64(bb, "xtur_fec_errors", stats->xtur_fec_errors);
	blobmsg_add_u64(bb, "xtuc_fec_errors", stats->xtuc_fec_errors);
	blobmsg_add_u64(bb, "xtur_hec_errors", stats->xtur_hec_errors);
	blobmsg_add_u64(bb, "xtuc_hec_errors", stats->xtuc_hec_errors);
	blobmsg_add_u64(bb, "xtur_crc_errors", stats->xtur_crc_errors);
	blobmsg_add_u64(bb, "xtuc_crc_errors", stats->xtuc_crc_errors);
}

static int dsl_stats_all(struct ubus_context *ctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method, struct blob_attr *msg)
{
	int retval = UBUS_STATUS_OK;
	static struct blob_buf bb;
	enum dsl_stats_type type;
	struct dsl_line_channel_stats stats;
	struct dsl_line_stats_interval line_stats_interval;
	struct dsl_channel_stats_interval channel_stats_interval;
	int i, j, max_line;
	void *array_line, *array_chan, *table_line, *table_interval, *table_chan;

	// Initialize the buffer
	memset(&bb, 0, sizeof(bb));
	blob_buf_init(&bb, 0);

	array_line = blobmsg_open_array(&bb, DSL_OBJECT_LINE);
	for (i = 0, max_line = dsl_get_line_number(); i < max_line; i++) {
		if (xdsl_ops.get_line_stats == NULL || (*xdsl_ops.get_line_stats)(i, &stats) != 0) {
			retval = UBUS_STATUS_UNKNOWN_ERROR;
			goto __ret;
		}

		// Line table
		table_line = blobmsg_open_table(&bb, "");

		// Line statistics
		blobmsg_add_u32(&bb, "id", (unsigned int)i);
		dsl_stats_to_blob(&stats, &bb);

		// Line interval statistics
		for (j = 0; j < ARRAY_SIZE(dsl_stats_types); j++) {
			table_interval = blobmsg_open_table(&bb, dsl_stats_types[j].text);

			if (xdsl_ops.get_line_stats_interval == NULL || (*xdsl_ops.get_line_stats_interval)
				(i, dsl_stats_types[j].value, &line_stats_interval) != 0) {
				retval = UBUS_STATUS_UNKNOWN_ERROR;
				goto __ret;
			}
			dsl_stats_line_interval_to_blob(&line_stats_interval, &bb);

			blobmsg_close_table(&bb, table_interval);
		}

		// Embed channel(s) inside a line in the format channel: [{},{}...]
		array_chan = blobmsg_open_array(&bb, DSL_OBJECT_CHANNEL);
		table_chan = blobmsg_open_table(&bb, "");

		// Channel statistics
		blobmsg_add_u32(&bb, "id", 0);
		if (xdsl_ops.get_channel_stats == NULL || (*xdsl_ops.get_channel_stats)(i, &stats) != 0) {
			retval = UBUS_STATUS_UNKNOWN_ERROR;
			goto __ret;
		}
		dsl_stats_to_blob(&stats, &bb);

		// Line interval statistics
		for (j = 0; j < ARRAY_SIZE(dsl_stats_types); j++) {
			table_interval = blobmsg_open_table(&bb, dsl_stats_types[j].text);

			if (xdsl_ops.get_channel_stats_interval == NULL || (*xdsl_ops.get_channel_stats_interval)
				(i, dsl_stats_types[j].value, &channel_stats_interval) != 0) {
				retval = UBUS_STATUS_UNKNOWN_ERROR;
				goto __ret;
			}
			dsl_stats_channel_interval_to_blob(&channel_stats_interval, &bb);

			blobmsg_close_table(&bb, table_interval);
		}

		// Close the tables and arrays for the channel
		blobmsg_close_table(&bb, table_chan);
		blobmsg_close_array(&bb, array_chan);

		// Close the table for one line
		blobmsg_close_table(&bb, table_line);
	}
	blobmsg_close_array(&bb, array_line);

	// Send the reply
	ubus_send_reply(ctx, req, bb.head);

__ret:
	blob_buf_free(&bb);

	return retval;
}

static struct ubus_method dsl_main_methods[] = {
	{ .name = "status", .handler = dsl_status_all },
	{ .name = "stats", .handler = dsl_stats_all }
};

static struct ubus_object_type dsl_main_type = UBUS_OBJECT_TYPE("dsl", dsl_main_methods);

static struct ubus_object dsl_main_object = {
	.name = "dsl",
	.type = &dsl_main_type,
	.methods = dsl_main_methods,
	.n_methods = ARRAY_SIZE(dsl_main_methods),
};

static int dsl_line_status(struct ubus_context *ctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method, struct blob_attr *msg)
{
	static struct blob_buf bb;
	struct dsl_line line;
	int retval = UBUS_STATUS_OK;
	int num = -1;

	// Initialize the buffer
	memset(&bb, 0, sizeof(bb));
	blob_buf_init(&bb, 0);

	// Get line status
	sscanf(obj->name, "dsl.line.%d", &num);
	if (xdsl_ops.get_line_info == NULL || (*xdsl_ops.get_line_info)(num, &line) != 0) {
		retval = UBUS_STATUS_UNKNOWN_ERROR;
		goto __ret;
	}
	dsl_status_line_to_blob(&line, &bb);

	// Send the reply
	ubus_send_reply(ctx, req, bb.head);

__ret:
	blob_buf_free(&bb);
	return retval;
}

static int dsl_line_stats(struct ubus_context *ctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method, struct blob_attr *msg)
{
	static struct blob_buf bb;
	struct blob_attr *tb[__DSL_STATS_MAX];
	enum dsl_stats_type type = DSL_STATS_QUARTERHOUR + 1;
	struct dsl_line_channel_stats stats;
	struct dsl_line_stats_interval line_stats_interval;
	int retval = UBUS_STATUS_OK;
	int num = -1;
	int i, j;
	void *table;

	// Parse and validation check the interval type if any
	blobmsg_parse(dsl_stats_policy, __DSL_STATS_MAX, tb, blob_data(msg), blob_len(msg));
	if (tb[DSL_STATS_INTERVAL]) {
		const char *st = blobmsg_data(tb[DSL_STATS_INTERVAL]);

		for (i = 0; i < ARRAY_SIZE(dsl_stats_types); i++) {
			if (strcasecmp(st, dsl_stats_types[i].text) == 0) {
				type = dsl_stats_types[i].value;
				break;
			}
		}

		if (i >= ARRAY_SIZE(dsl_stats_types)) {
			DSLMNGR_LOG(LOG_ERR, "Wrong argument for interval statistics type\n");
			return UBUS_STATUS_INVALID_ARGUMENT;
		}
	}

	// Initialize the buffer
	memset(&bb, 0, sizeof(bb));
	blob_buf_init(&bb, 0);

	// Get line number
	sscanf(obj->name, "dsl.line.%d", &num);

	// Get line interval statistics
	if (type >= DSL_STATS_TOTAL && type <= DSL_STATS_QUARTERHOUR) {
		if (xdsl_ops.get_line_stats_interval == NULL || (*xdsl_ops.get_line_stats_interval)
			(num, type, &line_stats_interval) != 0) {
			retval = UBUS_STATUS_UNKNOWN_ERROR;
			goto __ret;
		}
		dsl_stats_line_interval_to_blob(&line_stats_interval, &bb);
	} else {
		// Get line statistics
		if (xdsl_ops.get_line_stats == NULL || (*xdsl_ops.get_line_stats)(num, &stats) != 0) {
			retval = UBUS_STATUS_UNKNOWN_ERROR;
			goto __ret;
		}
		dsl_stats_to_blob(&stats, &bb);

		// Get all interval statistics
		for (j = 0; j < ARRAY_SIZE(dsl_stats_types); j++) {
			table = blobmsg_open_table(&bb, dsl_stats_types[j].text);

			if (xdsl_ops.get_line_stats_interval == NULL || (*xdsl_ops.get_line_stats_interval)
				(num, dsl_stats_types[j].value, &line_stats_interval) != 0) {
				retval = UBUS_STATUS_UNKNOWN_ERROR;
				goto __ret;
			}
			dsl_stats_line_interval_to_blob(&line_stats_interval, &bb);

			blobmsg_close_table(&bb, table);
		}
	}

	// Send the reply
	ubus_send_reply(ctx, req, bb.head);

__ret:
	blob_buf_free(&bb);
	return retval;
}

static struct ubus_method dsl_line_methods[] = {
	{ .name = "status", .handler = dsl_line_status },
	UBUS_METHOD("stats", dsl_line_stats, dsl_stats_policy )
};

static struct ubus_object_type dsl_line_type = UBUS_OBJECT_TYPE("dsl.line", dsl_line_methods);

static int dsl_channel_status(struct ubus_context *ctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method, struct blob_attr *msg)
{
	static struct blob_buf bb;
	struct dsl_channel channel;
	int retval = UBUS_STATUS_OK;
	int num = -1;

	// Initialize the buffer
	memset(&bb, 0, sizeof(bb));
	blob_buf_init(&bb, 0);

	// Get channel status
	sscanf(obj->name, "dsl.channel.%d", &num);
	if (xdsl_ops.get_channel_info == NULL || (*xdsl_ops.get_channel_info)(num, &channel) != 0) {
		retval = UBUS_STATUS_UNKNOWN_ERROR;
		goto __ret;
	}
	dsl_status_channel_to_blob(&channel, &bb);

	// Send the reply
	ubus_send_reply(ctx, req, bb.head);

__ret:
	blob_buf_free(&bb);
	return retval;
}

static int dsl_channel_stats(struct ubus_context *ctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method, struct blob_attr *msg)
{
	static struct blob_buf bb;
	struct blob_attr *tb[__DSL_STATS_MAX];
	enum dsl_stats_type type = DSL_STATS_QUARTERHOUR + 1;
	struct dsl_line_channel_stats stats;
	struct dsl_channel_stats_interval channel_stats_interval;
	int retval = UBUS_STATUS_OK;
	int num = -1;
	int i, j;
	void *table;

	// Parse and validation check the interval type if any
	blobmsg_parse(dsl_stats_policy, __DSL_STATS_MAX, tb, blob_data(msg), blob_len(msg));
	if (tb[DSL_STATS_INTERVAL]) {
		const char *st = blobmsg_data(tb[DSL_STATS_INTERVAL]);

		for (i = 0; i < ARRAY_SIZE(dsl_stats_types); i++) {
			if (strcasecmp(st, dsl_stats_types[i].text) == 0) {
				type = dsl_stats_types[i].value;
				break;
			}
		}

		if (i >= ARRAY_SIZE(dsl_stats_types)) {
			DSLMNGR_LOG(LOG_ERR, "Wrong argument for interval statistics type\n");
			return UBUS_STATUS_INVALID_ARGUMENT;
		}
	}

	// Initialize the buffer
	memset(&bb, 0, sizeof(bb));
	blob_buf_init(&bb, 0);

	// Get channel number
	sscanf(obj->name, "dsl.channel.%d", &num);

	// Get channel interval statistics
	if (type >= DSL_STATS_TOTAL && type <= DSL_STATS_QUARTERHOUR) {
		if (xdsl_ops.get_channel_stats_interval == NULL || (*xdsl_ops.get_channel_stats_interval)
			(num, type, &channel_stats_interval) != 0) {
			retval = UBUS_STATUS_UNKNOWN_ERROR;
			goto __ret;
		}
		dsl_stats_channel_interval_to_blob(&channel_stats_interval, &bb);
	} else {
		// Get channel statistics
		if (xdsl_ops.get_channel_stats == NULL || (*xdsl_ops.get_channel_stats)(num, &stats) != 0) {
			retval = UBUS_STATUS_UNKNOWN_ERROR;
			goto __ret;
		}
		dsl_stats_to_blob(&stats, &bb);

		// Get all interval statistics
		for (j = 0; j < ARRAY_SIZE(dsl_stats_types); j++) {
			table = blobmsg_open_table(&bb, dsl_stats_types[j].text);

			if (xdsl_ops.get_channel_stats_interval == NULL || (*xdsl_ops.get_channel_stats_interval)
				(num, dsl_stats_types[j].value, &channel_stats_interval) != 0) {
				retval = UBUS_STATUS_UNKNOWN_ERROR;
				goto __ret;
			}
			dsl_stats_channel_interval_to_blob(&channel_stats_interval, &bb);

			blobmsg_close_table(&bb, table);
		}
	}

	// Send the reply
	ubus_send_reply(ctx, req, bb.head);

__ret:
	blob_buf_free(&bb);
	return retval;
}

static struct ubus_method dsl_channel_methods[] = {
	{ .name = "status", .handler = dsl_channel_status },
	UBUS_METHOD("stats", dsl_channel_stats, dsl_stats_policy )
};

static struct ubus_object_type dsl_channel_type = UBUS_OBJECT_TYPE("dsl.channel", dsl_channel_methods);

int dsl_add_ubus_objects(struct ubus_context *ctx)
{
	struct ubus_object *line_objects = NULL;
	struct ubus_object *channel_objects = NULL;
	int ret, max_line, max_channel, i;

	ret = ubus_add_object(ctx, &dsl_main_object);
	if (ret) {
		DSLMNGR_LOG(LOG_ERR, "Failed to add UBUS object '%s', %s\n",
				dsl_main_object.name, ubus_strerror(ret));
		return -1;
	}

	// Add objects dsl.line.x
	max_line = dsl_get_line_number();
	line_objects = calloc(max_line, sizeof(struct ubus_object));
	if (!line_objects) {
		DSLMNGR_LOG(LOG_ERR, "Out of memory\n");
		return -1;
	}
	for (i = 0; i < max_line; i++) {
		char *obj_name;

		if (asprintf(&obj_name, "dsl.line.%d", i) <= 0) {
			DSLMNGR_LOG(LOG_ERR, "Out of memory\n");
			return -1;
		}

		line_objects[i].name = obj_name;
		line_objects[i].type = &dsl_line_type;
		line_objects[i].methods = dsl_line_methods;
		line_objects[i].n_methods = ARRAY_SIZE(dsl_line_methods);

		ret = ubus_add_object(ctx, &line_objects[i]);
		if (ret) {
			DSLMNGR_LOG(LOG_ERR, "Failed to add UBUS object '%s', %s\n",
					obj_name, ubus_strerror(ret));
			return -1;
		}
	}

	// Add objects dsl.channel.x
	max_channel = dsl_get_channel_number();
	channel_objects = calloc(max_channel, sizeof(struct ubus_object));
	if (!channel_objects) {
		DSLMNGR_LOG(LOG_ERR, "Out of memory\n");
		return -1;
	}
	for (i = 0; i < max_channel; i++) {
		char *obj_name;

		if (asprintf(&obj_name, "dsl.channel.%d", i) <= 0) {
			DSLMNGR_LOG(LOG_ERR, "Out of memory\n");
			return -1;
		}

		channel_objects[i].name = obj_name;
		channel_objects[i].type = &dsl_channel_type;
		channel_objects[i].methods = dsl_channel_methods;
		channel_objects[i].n_methods = ARRAY_SIZE(dsl_channel_methods);

		ret = ubus_add_object(ctx, &channel_objects[i]);
		if (ret) {
			DSLMNGR_LOG(LOG_ERR, "Failed to add UBUS object '%s', %s\n",
					obj_name, ubus_strerror(ret));
			return -1;
		}
	}

	// Returns on success
	return 0;

__error_ret:
	if (line_objects)
		free(line_objects);
	if (channel_objects)
		free(channel_objects);
	return -1;
}
