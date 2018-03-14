/*
 * dslmgr.c - provides "xdsl" UBUS object
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
#include <libubox/blobmsg.h>
#include <libubox/blobmsg_json.h>
#include <libubox/uloop.h>
#include <libubox/ustream.h>
#include <libubox/utils.h>
#include <libubus.h>
#include <uci.h>

#include <xdsl.h>

#include "dslmgr.h"

enum {
	DSL_STATS_TYPE,
	__DSL_STATS_MAX,
};

static const struct blobmsg_policy dsl_stats_policy[__DSL_STATS_MAX] = {
	[DSL_STATS_TYPE] = { .name = "type", .type = BLOBMSG_TYPE_STRING },
};

enum {
	DSL_STATUS_LINE_ID,
	__DSL_STATUS_MAX,
};

static const struct blobmsg_policy dsl_status_policy[__DSL_STATUS_MAX] = {
	[DSL_STATUS_LINE_ID] = { .name = "line", .type = BLOBMSG_TYPE_INT32 },
};



const char *dsl_mode_str(enum dsl_modtype v)
{
#define E2S(v)	case v: return #v
	switch(v) {
	E2S(MOD_GDMT);
	E2S(MOD_T1413);
	E2S(MOD_GLITE);
	E2S(MOD_ADSL2);
	E2S(MOD_ADSL2P);
	E2S(MOD_READSL2);
	E2S(MOD_VDSL);
	E2S(MOD_VDSL2);
	E2S(MOD_VDSL2P);
	E2S(MOD_GFAST);
	E2S(MOD_UNDEFINED);
	default:
		return "MOD_Undefined";
	}
};

const char *dsl_tc_str(enum dsl_traffictype v)
{
#define E2S(v)	case v: return #v
	switch(v) {
	E2S(TC_PTM);
	E2S(TC_ATM);
	E2S(TC_RAW);
	E2S(TC_NOT_CONNECTED);
	default:
		return "TC_Undefined";
	}
};

const char *dsl_pwr_state_str(enum dsl_powerstate v)
{
#define E2S(v)	case v: return #v
	switch(v) {
	E2S(L0);
	E2S(L1);
	E2S(L2);
	E2S(L3);
	default:
		return "Unknown";
	}
};

const char *dsl_line_status_str(enum dsl_linestate v)
{
#define E2S(v)	case v: return #v
	switch(v) {
	E2S(LINE_DOWN);
	E2S(LINE_HANDSHAKING);
	E2S(LINE_TRAINING);
	E2S(LINE_SHOWTIME);
	default:
		return "LINE_Unknown";
	}
};

const char *dsl_stattype_str(enum dsl_stattype t)
{
	switch (t) {
	case STAT_CURR_LINK:
		return "since linkup";
	case STAT_CURR_15MINS:
		return "current 15 mins";
	case STAT_PREV_15MINS:
		return "previous 15 mins";
	case STAT_CURR_24HRS:
		return "current day";
	case STAT_PREV_24HRS:
		return "previous day";
	case STAT_TOTAL:
		return "total";
	default:
		return "";
	}
}

void dslstats_to_blob_buffer(struct dsl_perfcounters *perf, struct blob_buf *b)
{
	void *obj;

	obj = blobmsg_open_table(b, dsl_stattype_str(perf->type));
		blobmsg_add_u64(b, "es_down", perf->es.ds);
		blobmsg_add_u64(b, "es_up", perf->es.us);
		blobmsg_add_u64(b, "ses_down", perf->ses.ds);
		blobmsg_add_u64(b, "ses_up", perf->ses.us);
		blobmsg_add_u64(b, "uas_down", perf->uas.ds);
		blobmsg_add_u64(b, "uas_up", perf->uas.us);
	blobmsg_close_table(b, obj);
}

void dslinfo_to_blob_buffer(struct dsl_info *info, int lineid, struct blob_buf *b)
{
	void *t, *array, *obj;
	struct dsl_link *link = &info->link;
	int i;

	t = blobmsg_open_table(b, "dslstatus");
	blobmsg_add_string(b, "mode", dsl_mode_str(link->mode) + 4);
	blobmsg_add_string(b, "traffic", dsl_tc_str(link->tc_type) + 3);
	//blobmsg_add_string(b, "status", link->status);
	blobmsg_add_string(b, "link_power_state",
				dsl_pwr_state_str(link->pwr_state));
	blobmsg_add_string(b, "line_status",
			dsl_line_status_str(link->training_status) + 5);
	blobmsg_add_u8(b, "trellis_up", link->trellis_enabled.us);
	blobmsg_add_u8(b, "trellis_down", link->trellis_enabled.ds);
	blobmsg_add_u32(b, "snr_up_x10", link->snr_margin.us);
	blobmsg_add_u32(b, "snr_down_x10", link->snr_margin.ds);
	blobmsg_add_u32(b, "pwr_up_x10", link->power.us);
	blobmsg_add_u32(b, "pwr_down_x10", link->power.ds);
	blobmsg_add_u32(b, "attn_up_x10", link->attn.us);
	blobmsg_add_u32(b, "attn_down_x10", link->attn.ds);
	blobmsg_add_u32(b, "max_rate_up", link->rate_kbps_max.us);
	blobmsg_add_u32(b, "max_rate_down", link->rate_kbps_max.ds);

	if (lineid == -1)
		array = blobmsg_open_array(b, "line");

	for (i = 0; i < info->num_lines; i++) {
		struct dsl_line *line = &info->line[i];
		struct dsl_line_params *p = &line->param;

		if (lineid != -1 && lineid != i)
			continue;

			obj = blobmsg_open_table(b, NULL);
				blobmsg_add_u32(b, "id", i);
				blobmsg_add_u32(b, "rate_up", line->rate_kbps.us);
				blobmsg_add_u32(b, "rate_down", line->rate_kbps.ds);
				blobmsg_add_u32(b, "msgc_up", p->msgc.us);
				blobmsg_add_u32(b, "msgc_down", p->msgc.ds);
				blobmsg_add_u32(b, "b_down", p->b.ds);
				blobmsg_add_u32(b, "b_up", p->b.us);
				blobmsg_add_u32(b, "m_down", p->m.ds);
				blobmsg_add_u32(b, "m_up", p->m.us);
				blobmsg_add_u32(b, "t_down", p->t.ds);
				blobmsg_add_u32(b, "t_up", p->t.us);
				blobmsg_add_u32(b, "r_down", p->r.ds);
				blobmsg_add_u32(b, "r_up", p->r.us);
				blobmsg_add_u32(b, "s_down_x10000", p->s.ds * 10000);
				blobmsg_add_u32(b, "s_up_x10000", p->s.us * 10000);
				blobmsg_add_u32(b, "l_down", p->l.ds);
				blobmsg_add_u32(b, "l_up", p->l.us);
				blobmsg_add_u32(b, "d_down", p->d.ds);
				blobmsg_add_u32(b, "d_up", p->d.us);
				blobmsg_add_u32(b, "delay_down", p->delay.ds);
				blobmsg_add_u32(b, "delay_up", p->delay.us);
				blobmsg_add_u32(b, "inp_down_x100", p->inp.ds * 100);
				blobmsg_add_u32(b, "inp_up_x100", p->inp.us * 100);
				blobmsg_add_u64(b, "sf_down", line->cnts.sf.ds);
				blobmsg_add_u64(b, "sf_up", line->cnts.sf.us);
				blobmsg_add_u64(b, "sf_err_down", line->cnts.sferr.ds);
				blobmsg_add_u64(b, "sf_err_up", line->cnts.sferr.us);
				blobmsg_add_u64(b, "rs_down", line->cnts.rs.ds);
				blobmsg_add_u64(b, "rs_up", line->cnts.rs.us);
				blobmsg_add_u64(b, "rs_corr_down", line->cnts.rscorr.ds);
				blobmsg_add_u64(b, "rs_corr_up", line->cnts.rscorr.us);
				blobmsg_add_u64(b, "rs_uncorr_down", line->cnts.rsuncorr.ds);
				blobmsg_add_u64(b, "rs_uncorr_up", line->cnts.rsuncorr.us);
				blobmsg_add_u64(b, "hec_down", line->err.hec.ds);
				blobmsg_add_u64(b, "hec_up", line->err.hec.us);
				blobmsg_add_u64(b, "ocd_down", line->err.ocd.ds);
				blobmsg_add_u64(b, "ocd_up", line->err.ocd.us);
				blobmsg_add_u64(b, "lcd_down", line->err.lcd.ds);
				blobmsg_add_u64(b, "lcd_up", line->err.lcd.us);
				blobmsg_add_u64(b, "fec_down", line->err.fec.ds);
				blobmsg_add_u64(b, "fec_up", line->err.fec.us);
				blobmsg_add_u64(b, "crc_down", line->err.crc.ds);
				blobmsg_add_u64(b, "crc_up", line->err.crc.us);
			blobmsg_close_table(b, obj);
	}
	if (lineid == -1)
		blobmsg_close_array(b, array);

	array = blobmsg_open_table(b, "counters");
		obj = blobmsg_open_table(b, "total");
			blobmsg_add_u64(b, "es_down", link->perf_cnts.es.ds);
			blobmsg_add_u64(b, "es_up", link->perf_cnts.es.us);
			blobmsg_add_u64(b, "ses_down", link->perf_cnts.ses.ds);
			blobmsg_add_u64(b, "ses_up", link->perf_cnts.ses.us);
			blobmsg_add_u64(b, "uas_down", link->perf_cnts.uas.ds);
			blobmsg_add_u64(b, "uas_up", link->perf_cnts.uas.us);
		blobmsg_close_table(b, obj);
	blobmsg_close_array(b, array);
	blobmsg_close_table(b, t);
}

static int uci_get_dsl_type(char *type)
{
	static struct uci_context *ctx = NULL;
	static struct uci_package *pkg = NULL;
	struct uci_element *e;
	int ret = 0;

	ctx = uci_alloc_context();
	if (uci_load(ctx, "dsl", &pkg))
		return -1;

	uci_foreach_element(&pkg->sections, e) {
		struct uci_section *s = uci_to_section(e);
		const char *dsl_type;

		if (strcmp(s->type, "dsl-line"))
			continue;

		dsl_type = uci_lookup_option_string(ctx, s, "type");
		if (dsl_type)
			sprintf(type, "%s", dsl_type);
		else
			ret = -1;
	}
	uci_free_context(ctx);
	return ret;
}

static int uci_get_dsl_config(struct dsl_config *cfg)
{
	static struct uci_context *ctx = NULL;
	static struct uci_package *pkg = NULL;
	struct uci_element *e;

	memset(cfg, 0, sizeof(struct dsl_config));

	ctx = uci_alloc_context();
	if (uci_load(ctx, "dsl", &pkg))
		return -1;

	uci_foreach_element(&pkg->sections, e) {
		struct uci_section *s = uci_to_section(e);
		struct uci_option *o;
		struct uci_ptr ptr;
		char mode_path[64] = {0};
		char profile_path[64] = {0};
		const char *bitswap, *sra, *us0, *trellis;

		if (strcmp(s->type, "dsl-line"))
			continue;

		sprintf(mode_path, "dsl.%s.mode", s->e.name);
		if (uci_lookup_ptr(ctx, &ptr, mode_path, true) == UCI_OK) {
			struct uci_element *ent;
			o = ptr.o;
			if (o->type == UCI_TYPE_LIST) {
				uci_foreach_element(&o->v.list, ent) {
					if (!strcmp(ent->name, "gdmt"))
						cfg->mode |= (1 << MOD_GDMT);
					if (!strcmp(ent->name, "glite"))
						cfg->mode |= (1 << MOD_GLITE);
					if (!strcmp(ent->name, "t1413"))
						cfg->mode |= (1 << MOD_T1413);
					if (!strcmp(ent->name, "adsl2"))
						cfg->mode |= (1 << MOD_ADSL2);
					if (!strcmp(ent->name, "adsl2p"))
						cfg->mode |= (1 << MOD_ADSL2P);
					if (!strcmp(ent->name, "vdsl2"))
						cfg->mode |= (1 << MOD_VDSL2);
					if (!strcmp(ent->name, "gfast"))
						cfg->mode |= (1 << MOD_GFAST);
				}
			}
		}

		sprintf(profile_path, "dsl.%s.profile", s->e.name);
		if (uci_lookup_ptr(ctx, &ptr, profile_path, true) == UCI_OK) {
			struct uci_element *ent;
			o = ptr.o;
			if (o->type == UCI_TYPE_LIST) {
				uci_foreach_element(&o->v.list, ent) {
					if (!strcmp(ent->name, "8a"))
						cfg->vdsl2_profile |= VDSL2_8a;
					if (!strcmp(ent->name, "8b"))
						cfg->vdsl2_profile |= VDSL2_8b;
					if (!strcmp(ent->name, "8c"))
						cfg->vdsl2_profile |= VDSL2_8c;
					if (!strcmp(ent->name, "8d"))
						cfg->vdsl2_profile |= VDSL2_8d;
					if (!strcmp(ent->name, "12a"))
						cfg->vdsl2_profile |= VDSL2_12a;
					if (!strcmp(ent->name, "12b"))
						cfg->vdsl2_profile |= VDSL2_12b;
					if (!strcmp(ent->name, "17a"))
						cfg->vdsl2_profile |= VDSL2_17a;
					if (!strcmp(ent->name, "30a"))
						cfg->vdsl2_profile |= VDSL2_30a;
					if (!strcmp(ent->name, "35b"))
						cfg->vdsl2_profile |= VDSL2_35b;
				}
			}
		}

		trellis = uci_lookup_option_string(ctx, s, "trellis");
		bitswap = uci_lookup_option_string(ctx, s, "bitswap");
		sra = uci_lookup_option_string(ctx, s, "sra");
		us0 = uci_lookup_option_string(ctx, s, "us0");

		if (trellis && atoi(trellis) == 1)
			cfg->trellis = 1;

		if (bitswap && atoi(bitswap) == 1)
			cfg->bitswap = 1;

		if (sra && atoi(sra) == 1)
			cfg->sra = 1;

		if (us0 && atoi(us0) == 1)
			cfg->us0 = 1;
	}
	uci_free_context(ctx);
	return 0;
}

int dslmgr_dsl_start(struct ubus_context *ctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method,
		struct blob_attr *msg)
{
	struct dsl_config cfg, *cfgptr = NULL;
	char type[128] = {0};

	if (uci_get_dsl_type(type) == -1)
		return UBUS_STATUS_UNKNOWN_ERROR;

	if (uci_get_dsl_config(&cfg) == 0)
		cfgptr = &cfg;

	if (dsl_start(type, cfgptr) < 0)
		return UBUS_STATUS_UNKNOWN_ERROR;

	return UBUS_STATUS_OK;
}

int dslmgr_dsl_stop(struct ubus_context *ctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method,
		struct blob_attr *msg)
{
	char type[128] = {0};

	if (uci_get_dsl_type(type) == -1)
		return UBUS_STATUS_UNKNOWN_ERROR;

	if (dsl_stop(type) < 0)
		return UBUS_STATUS_UNKNOWN_ERROR;

	return UBUS_STATUS_OK;
}

int dslmgr_dump_dslstats(struct ubus_context *ctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method,
		struct blob_attr *msg)
{
	static struct blob_buf bb;
	struct blob_attr *tb[__DSL_STATS_MAX];
	struct dsl_perfcounters stats;
	enum dsl_stattype type = STAT_CURR_LINK;
	char dsltype[128] = {0};

	blobmsg_parse(dsl_stats_policy, __DSL_STATS_MAX, tb,
					blob_data(msg), blob_len(msg));

	memset(&stats, 0, sizeof(struct dsl_perfcounters));
        if ((tb[DSL_STATS_TYPE])) {
		char st[64] = {0};

		strncpy(st, blobmsg_data(tb[DSL_STATS_TYPE]), sizeof(st)-1);
		if (!strncasecmp(st, "now15", 5))
			type = STAT_CURR_15MINS;
		else if (!strncasecmp(st, "prev15", 6))
			type = STAT_PREV_15MINS;
		else if (!strncasecmp(st, "now24", 6))
			type = STAT_CURR_24HRS;
		else if (!strncasecmp(st, "prev24", 6))
			type = STAT_PREV_24HRS;
		else if (!strncasecmp(st, "total", 6))
			type = STAT_TOTAL;
		else if (!strncasecmp(st, "link", 6))
			type = STAT_CURR_LINK;
		else
			return UBUS_STATUS_INVALID_ARGUMENT;
	}

	if (uci_get_dsl_type(dsltype) == -1)
		return UBUS_STATUS_UNKNOWN_ERROR;

	if (dsl_get_stats(dsltype, type, &stats) < 0)
		return UBUS_STATUS_UNKNOWN_ERROR;

	blob_buf_init(&bb, 0);
	dslstats_to_blob_buffer(&stats, &bb);

	ubus_send_reply(ctx, req, bb.head);

	return UBUS_STATUS_OK;
}

int dslmgr_dump_dslstatus(struct ubus_context *ctx, struct ubus_object *obj,
		struct ubus_request_data *req, const char *method,
		struct blob_attr *msg)
{
	static struct blob_buf bb;
	struct dsl_info dslinfo;
	struct blob_attr *tb[__DSL_STATUS_MAX];
	int ret;
	int lineid = -1;
	char type[128] = {0};

	blobmsg_parse(dsl_status_policy, __DSL_STATUS_MAX, tb,
					blob_data(msg), blob_len(msg));

        if (tb[DSL_STATUS_LINE_ID]) {
		lineid = blobmsg_get_u32(tb[DSL_STATUS_LINE_ID]);
		if (lineid < 0 || lineid > XDSL_MAX_LINES)
			return UBUS_STATUS_INVALID_ARGUMENT;
	}

	if (uci_get_dsl_type(type) == -1)
		return UBUS_STATUS_UNKNOWN_ERROR;

	memset(&dslinfo, 0, sizeof(dslinfo));
	ret = dsl_get_status(type, &dslinfo);
	if (ret != 0)
		return UBUS_STATUS_UNKNOWN_ERROR;

	blob_buf_init(&bb, 0);
	dslinfo_to_blob_buffer(&dslinfo, lineid, &bb);

	ubus_send_reply(ctx, req, bb.head);
	return UBUS_STATUS_OK;
}

struct ubus_method dsl_methods[] = {
	UBUS_METHOD("status", dslmgr_dump_dslstatus, dsl_status_policy),
	UBUS_METHOD_NOARG("start", dslmgr_dsl_start),
	UBUS_METHOD_NOARG("stop", dslmgr_dsl_stop),
	UBUS_METHOD("stats", dslmgr_dump_dslstats, dsl_stats_policy),
};

struct ubus_object_type dsl_type = UBUS_OBJECT_TYPE("xdsl", dsl_methods);

struct ubus_object dsl_object = {
	.name = "xdsl",
	.type = &dsl_type,
	.methods = dsl_methods,
	.n_methods = ARRAY_SIZE(dsl_methods),
};
