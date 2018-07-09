/*
 * xdsl.h - XDSL library header file
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
#ifndef _XDSL_H
#define _XDSL_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct dsl_long_data { long us; long ds; } dsl_long_t;
typedef struct dsl_bool_data { bool us; bool ds; } dsl_bool_t;

enum {
	trellis_off,
	trellis_on,
};

/** enum dsl_modetype - dsl modes */
enum dsl_modtype {
	MOD_GDMT,    /* G.992.1 */
	MOD_T1413,   /* T1.413 */
	MOD_GLITE,   /* G.992.2 */
	MOD_ADSL2,   /* G.992.3 */
	MOD_ADSL2P,  /* G.992.5 */
	MOD_READSL2, /* G.992.3 Annex L */
	MOD_VDSL,    /* G.993.1 */
	MOD_VDSL2,   /* G.993.2 */
	MOD_VDSL2P,  /* G.993.2 Annex Q */
	MOD_GFAST,   /* G.9700, G.9701 */
	MOD_UNDEFINED
};

#define is_adsl(m)	(m >= MOD_GDMT && m < MOD_ADSL2)
#define is_adsl2(m)	(m >= MOD_ADSL2 && m < MOD_VDSL)
#define is_vdsl2(m)	(m > MOD_VDSL && m <= MOD_VDSL2P)
#define is_gfast(m)	(m == MOD_GFAST)

/** enum dsl_linestatus - line status */
enum dsl_linestatus {
	LINE_UP,
	LINE_INITING,
	LINE_ESTABLISHING,
	LINE_NOSIGNAL,
	LINE_ERROR,
};

/** enum dsl_linestate - physical line state */
enum dsl_linestate {
	LINE_DOWN,        /* no sync */
	LINE_HANDSHAKING, /* hello exchange phase */
	LINE_TRAINING,    /* establishing sync */
	LINE_SHOWTIME,    /* successfully synced */
	LINE_UNKNOWN,     /* unknown state */
};

/** enum dsl_vdsl2_profile - vdsl2 profiles */
enum dsl_vdsl2_profile {
	VDSL2_8a    = 1<<0,
	VDSL2_8b    = 1<<1,
	VDSL2_8c    = 1<<2,
	VDSL2_8d    = 1<<3,
	VDSL2_12a   = 1<<4,
	VDSL2_12b   = 1<<5,
	VDSL2_17a   = 1<<6,
	VDSL2_30a   = 1<<7,
	VDSL2_35b   = 1<<8,
};

/** struct dsl_caps - dsl capabiltities */
struct dsl_caps {
	uint32_t stds;            /** bitmap of STD_* */
	uint32_t modes;           /** bitmap of MOD_* */
	uint32_t profiles;        /** bitmap of PROFILE_ * */
	dsl_long_t rate_kbps_max; /** max bitrate in Kbps */
};

/** enum dsl_stattype - type of collected statistics */
enum dsl_stattype {
	STAT_CURR_LINK,     /** since latest linkup */
	STAT_CURR_15MINS,   /** latest 15 mins stats */
	STAT_PREV_15MINS,   /** previous 15 mins stats */
	STAT_CURR_24HRS,    /** current day's stats */
	STAT_PREV_24HRS,    /** previous day's stats */
	STAT_TOTAL,         /** total stats since power up */
};

/** struct dsl_counters - dsl statistics counters */
struct dsl_counters {
	unsigned long tx_bytes;
	unsigned long rx_bytes;
	unsigned long tx_packets;
	unsigned long rx_packets;

	/** packets dropped due to packets */
	unsigned long tx_error_packets;
	unsigned long rx_error_packets;

	/** no-error packets dropped due to other factors */
	unsigned long tx_dropped_packets;
	unsigned long rx_dropped_pcakets;
};

/* Traffic type */
enum dsl_traffictype {
	TC_PTM,
	TC_ATM,
	TC_RAW,
	TC_NOT_CONNECTED,
};

/**enum dsl_powerstate - power states */
enum dsl_powerstate {
	L0,
	L1,
	L2,
	L3,
};

/** struct dsl_line_params - dsl line parameters */
struct dsl_line_params {
	dsl_long_t msgc;    /** # of bytes in OH channel message */
	dsl_long_t k;       /** # of bytes in DMT frame */
	dsl_long_t b;       /** # of bytes in Mux data frame */
	dsl_long_t t;       /** # of Mux data frames in OH subframe */
	dsl_long_t r;       /** # of check bytes in FEC data frames (RFEC) */
	dsl_long_t s;       /** ratio of FEC over PMD data frame */
	dsl_long_t l;       /** # of bits in PMD data frame (LSYMB) */
	dsl_long_t d;       /** interleave depth (INTLVDEPTH)*/
	dsl_long_t m;       /** # of Mux frames per FEC data frame */
	dsl_long_t i;
	dsl_long_t n;       /** length of code word (NFEC) */
	dsl_long_t q;       /** # of RS code words per DTU */
	dsl_long_t v;       /** # of padding octets per DTU */
	dsl_long_t delay;   /** delay in msecs due to interleaving */
	dsl_long_t inp;     /** actual Impulse Noise Protection (DMT symbol) */
	dsl_long_t inprein; /** actual INP against REIN noise (ACTINPREIN) */
};

/** struct dsl_line_errors - dsl line errors */
struct dsl_line_errors {
	dsl_long_t sferr;
	dsl_long_t rs_corr;
	dsl_long_t rs_uncorr;
	dsl_long_t hec;
	dsl_long_t ocd;
	dsl_long_t lcd;
	dsl_long_t fec;
	dsl_long_t crc;
	dsl_long_t ohf;
	dsl_long_t lof;
	dsl_long_t lol;
	dsl_long_t lom;
	dsl_long_t los;
	dsl_long_t lop;
};

/** struct dsl_perfcounters - dsl performance counters */
struct dsl_perfcounters {
	enum dsl_stattype type; /** stats type */
	unsigned long secs;     /** stats collected duration (in seconds) */
	dsl_long_t es;          /** errored seconds */
	dsl_long_t ses;         /** severly errored seconds */
	dsl_long_t uas;         /** unavailable seconds */
	dsl_long_t as;          /** available seconds */
};

/** struct dsl_line_counters - stats counters */
struct dsl_line_counters {
	dsl_long_t sf;         /** super frames */
	dsl_long_t sferr;      /** super frame errors TODO: move */
	dsl_long_t rs;         /** total RS codewords tx/rx */
	dsl_long_t rscorr;     /** RS correctable errors TODO: move */
	dsl_long_t rsuncorr;   /** RS uncorrectable errors TODO: move */
};

/** struct dsl_line_rtxcounters - retransmit counters */
struct dsl_line_rtxcounters {
	dsl_long_t tx;      /** # of retransmitted DTUs */
	dsl_long_t corr;    /** # of corrected retransmissions */
	dsl_long_t uncorr;  /** # of DTU errors which couldnot be corrected */
};

/** struct dsl_link - xdsl link information */
struct dsl_link {
	char name[64];                      /** real name */
	char alias[64];                     /** name alias */
	char fw_version[64];                /** firmware version */
	enum dsl_linestate linestate;
	enum dsl_powerstate pwr_state;      /** L0..L3 */
	enum dsl_traffictype tc_type;       /** Traffic type */
	enum dsl_modtype mode;              /** dsl mode adsl, vdsl2, etc. */
	int annex_info;			    /* TODO: xdsl_mode-> annex_info */
	unsigned int vdsl2_profile;         /** 8a,.. 30a, 35b */
	dsl_long_t rate_kbps_max;           /** max attainable rate in Kbps */
	dsl_bool_t trellis_enabled;         /** whether trellis coding used */
	dsl_long_t snr_margin;              /** snr margin in 0.1 dB */
	dsl_long_t attn;                    /** attenuation in 0.1 dB */
	dsl_long_t power;                   /** power in 0.1 dBmV */
	long status;
	unsigned int training_status;
	unsigned long uptime;               /** seconds since link up */
	struct dsl_perfcounters perf_cnts;  /** perforamce stats */
};

/** struct dsl_line - dsl line information structure */
struct dsl_line {
	dsl_long_t rate_kbps;                 /** rate in Kbps */
	struct dsl_line_counters cnts;        /** line stats */
	struct dsl_line_rtxcounters rtx_cnts; /** line retransmit counters */
	struct dsl_line_params param;         /** line parameters */
	struct dsl_line_errors err;           /** line errors */
};

#define XDSL_MAX_LINES	2

/** struct dsl_info - dsl information structure */
struct dsl_info {
	struct dsl_link link;                 /** dsl link informantion */
	uint32_t num_lines;                   /** # of phy lines */
	struct dsl_line line[XDSL_MAX_LINES]; /** dsl lines */
};

struct dsl_config {
	unsigned int mode;	/** bitmap of modes */
	unsigned int vdsl2_profile;
	int us0;
	int trellis;
	int bitswap;
	int sra;
};

struct dsl_ops {
	/** name to match dsl backend */
	const char *name;

	int (*start)(char *name, struct dsl_config *cfg);
	int (*stop)(char *name);
	int (*set_alias)(char *name, char *alias);
	int (*get_mode)(char *name, enum dsl_modtype *m);
	int (*get_supported_standards)(char *name, uint32_t *std);
	int (*get_active_standards)(char *name, uint32_t *std);
	int (*get_line_status)(char *name, enum dsl_linestatus *s);
	int (*get_supported_profiles)(char *name, uint32_t *p);
	int (*get_active_profiles)(char *name, uint32_t *p);
	int (*get_max_bitrate)(char *name, dsl_long_t *max_rate_kbps);
	int (*get_bitrate)(char *name, dsl_long_t *rate_kbps);
	int (*get_stats)(char *name, enum dsl_stattype type,
				struct dsl_perfcounters *c);
	int (*get_status)(char *name, struct dsl_info *info);
};


/* API list */
int dsl_start(char *name, struct dsl_config *cfg);
int dsl_stop(char *name);
int dsl_set_alias(char *name, char *alias);
int dsl_get_mode(char *name, enum dsl_modtype *m);
int dsl_get_supported_standards(char *name, uint32_t *std);
int dsl_get_active_standards(char *name, uint32_t *std);
int dsl_get_line_status(char *name, enum dsl_linestatus *s);
int dsl_get_supported_profiles(char *name, uint32_t *p);
int dsl_get_active_profiles(char *name, uint32_t *p);
int dsl_get_max_bitrate(char *name, dsl_long_t *max_rate_kbps);
int dsl_get_bitrate(char *name, dsl_long_t *rate_kbps);
int dsl_get_stats(char *name, enum dsl_stattype type,
			struct dsl_perfcounters *c);
int dsl_get_status(char *name, struct dsl_info *info);

#ifdef __cplusplus
}
#endif
#endif /* _XDSL_H */
