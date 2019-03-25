/*
 * xdsl.h - library header file
 * This file provides definition for the libdsl APIs and related
 * structures.
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
#ifndef _XDSL_H
#define _XDSL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

/** Common definitions */
#define XDSL_MAX_LINES	1

typedef struct { long us; long ds; } dsl_long_t;
typedef struct { unsigned long us; unsigned long ds; } dsl_ulong_t;
typedef struct { bool us; bool ds; } dsl_bool_t;
typedef struct { long array[24]; int count; } dsl_long_sequence_t;
typedef struct { unsigned long array[24]; int count; } dsl_ulong_sequence_t;

/** enum dsl_status - operational status of a line or channel */
enum dsl_if_status {
	/* Starts with non-zero in order to distinguish from an uninitialized value which is usually 0 */
	IF_UP = 1,
	IF_DOWN,
	IF_UNKNOWN,
	IF_DORMANT,
	IF_NOTPRESENT,
	IF_LLDOWN,
	IF_ERROR
};

/** enum dsl_link_status - link status */
enum dsl_link_status {
	/* Starts with non-zero in order to distinguish from an uninitialized value which is usually 0 */
	LINK_UP = 1,
	LINK_INITIALIZING,
	LINK_ESTABLISHING,
	LINK_NOSIGNAL,
	LINK_DISABLED,
	LINK_ERROR
};

/** enum dsl_modtype - DSL modes */
enum dsl_modtype {
	MOD_G_922_1_ANNEX_A = 1,
	MOD_G_922_1_ANNEX_B = 1 << 1,
	MOD_G_922_1_ANNEX_C = 1 << 2,
	MOD_T1_413          = 1 << 3,
	MOD_T1_413i2        = 1 << 4,
	MOD_ETSI_101_388    = 1 << 5,
	MOD_G_992_2         = 1 << 6,
	MOD_G_992_3_Annex_A = 1 << 7,
	MOD_G_992_3_Annex_B = 1 << 8,
	MOD_G_992_3_Annex_C = 1 << 9,
	MOD_G_992_3_Annex_I = 1 << 10,
	MOD_G_992_3_Annex_J = 1 << 11,
	MOD_G_992_3_Annex_L = 1 << 12,
	MOD_G_992_3_Annex_M = 1 << 13,
	MOD_G_992_4         = 1 << 14,
	MOD_G_992_5_Annex_A = 1 << 15,
	MOD_G_992_5_Annex_B = 1 << 16,
	MOD_G_992_5_Annex_C = 1 << 17,
	MOD_G_992_5_Annex_I = 1 << 18,
	MOD_G_992_5_Annex_J = 1 << 19,
	MOD_G_992_5_Annex_M = 1 << 20,
	MOD_G_993_1         = 1 << 21,
	MOD_G_993_1_Annex_A = 1 << 22,
	MOD_G_993_2_Annex_A = 1 << 23,
	MOD_G_993_2_Annex_B = 1 << 24,
	MOD_G_993_2_Annex_C = 1 << 25
};

/** enum dsl_xtse_bit - XTSE bit definition. Refer to dsl_line.xtse for details */
enum dsl_xtse_bit {
	/* Octet 1 - ADSL, i.e. g.dmt */
	T1_413							= 1,
	ETSI_101_388					= 2,
	G_992_1_POTS_NON_OVERLAPPED		= 3, /* Annex A */
	G_992_1_POTS_OVERLAPPED			= 4, /* Annex A */
	G_992_1_ISDN_NON_OVERLAPPED		= 5, /* Annex B */
	G_992_1_ISDN_OVERLAPPED			= 6, /* Annex B */
	G_992_1_TCM_ISDN_NON_OVERLAPPED	= 7, /* Annex C */
	G_992_1_TCM_ISDN_OVERLAPPED		= 8, /* Annex C */

	/* Octet 2 - Splitter-less ADSL, i.e. g.lite */
	G_992_2_POTS_NON_OVERLAPPED		= 9, /* Annex A */
	G_992_2_POTS_OVERLAPPED			= 10, /* Annex B */
	G_992_2_TCM_ISDN_NON_OVERLAPPED	= 11, /* Annex C */
	G_992_2_TCM_ISDN_OVERLAPPED		= 12, /* Annex C */
	/* Bits 13 - 16 are reserved */

	/* Octet 3 - ADSL2 */
	/* Bits 17 - 18 are reserved */
	G_992_3_POTS_NON_OVERLAPPED		= 19, /* Annex A */
	G_992_3_POTS_OVERLAPPED			= 20, /* Annex A */
	G_992_3_ISDN_NON_OVERLAPPED		= 21, /* Annex B */
	G_992_3_ISDN_OVERLAPPED			= 22, /* Annex B */
	G_992_3_TCM_ISDN_NON_OVERLAPPED	= 23, /* Annex C */
	G_992_3_TCM_ISDN_OVERLAPPED		= 24, /* Annex C */

	/* Octet 4 - Splitter-less ADSL2 and ADSL2 */
	G_992_4_POTS_NON_OVERLAPPED		= 25, /* Annex A */
	G_992_4_POTS_OVERLAPPED			= 26, /* Annex A */
	/* Bits 27 - 28 are reserved */
	G_992_3_ANNEX_I_NON_OVERLAPPED	= 29, /* All digital mode */
	G_992_3_ANNEX_I_OVERLAPPED		= 30, /* All digital mode */
	G_992_3_ANNEX_J_NON_OVERLAPPED	= 31, /* All digital mode */
	G_992_3_ANNEX_J_OVERLAPPED		= 32, /* All digital mode */


	/* Octet 5 - Splitter-less ADSL2 and ADSL2 */
	G_992_4_ANNEX_I_NON_OVERLAPPED	= 33, /* All digital mode */
	G_992_4_ANNEX_I_OVERLAPPED		= 34, /* All digital mode */
	G_992_3_POTS_MODE_1				= 35, /* Annex L, non-overlapped, wide upstream */
	G_992_3_POTS_MODE_2				= 36, /* Annex L, non-overlapped, narrow upstream */
	G_992_3_POTS_MODE_3				= 37, /* Annex L, overlapped, wide upstream */
	G_992_3_POTS_MODE_4				= 38, /* Annex L, overlapped, narrow upstream */
	G_992_3_EXT_POTS_NON_OVERLAPPED	= 39, /* Annex M */
	G_992_3_EXT_POTS_OVERLAPPED		= 40, /* Annex M */

	/* Octet 6 - ADSL2+ */
	G_992_5_POTS_NON_OVERLAPPED		= 41, /* Annex A */
	G_992_5_POTS_OVERLAPPED			= 42, /* Annex A */
	G_992_5_ISDN_NON_OVERLAPPED		= 43, /* Annex B */
	G_992_5_ISDN_OVERLAPPED			= 44, /* Annex B */
	G_992_5_TCM_ISDN_NON_OVERLAPPED	= 45, /* Annex C */
	G_992_5_TCM_ISDN_OVERLAPPED		= 46, /* Annex C */
	G_992_5_ANNEX_I_NON_OVERLAPPED	= 47, /* All digital mode */
	G_992_5_ANNEX_I_OVERLAPPED		= 48, /* All digital mode */

	/* Octet 7 - ADSL2+ */
	G_992_5_ANNEX_J_NON_OVERLAPPED	= 49, /* All digital mode */
	G_992_5_ANNEX_J_OVERLAPPED		= 50, /* All digital mode */
	G_992_5_EXT_POTS_NON_OVERLAPPED	= 51, /* Annex M */
	G_992_5_EXT_POTS_OVERLAPPED		= 52, /* Annex M */
	/* Bits 53 - 56 are reserved */

	/* Octet 8 - VDSL2 */
	G_993_2_NORTH_AMERICA			= 57, /* Annex A */
	G_993_2_EUROPE					= 58, /* Annex B */
	G_993_2_JAPAN					= 59, /* Annex C */
	/* Bits 60 - 64 are reserved */
};

/**
 * This macro determines whether a XTSE bit is set
 *
 * @param[in] xtse unsigned char[8] as defined in dsl_line.xtse
 * @param[in] bit Bit number as defined in G.997.1 clause 7.3.1.1.1 XTU transmission system enabling (XTSE)
 *
 * @return A non-zero value is the bit is set. Otherwise 0
 */
#define XTSE_BIT_GET(xtse, bit) (xtse[((bit) - 1) / 8] & (1 << (((bit) - 1) % 8)))

/** struct dsl_standard - DSL standards */
struct dsl_standard {
	bool use_xtse; /* true if xtse is used. false if mode is used */
	union {
		/** Bit maps defined in dsl_modtype */
		unsigned long mode;
		/** Transmission system types to be allowed by the xTU on this line instance */
		unsigned char xtse[8];
	};
};

/** enum dsl_line_encoding - Line encoding */
enum dsl_line_encoding {
	/* Starts with non-zero in order to distinguish from an uninitialized value which is usually 0 */
	LE_DMT = 1,
	LE_CAP,
	LE_2B1Q,
	LE_43BT,
	LE_PAM,
	LE_QAM
};

/** enum dsl_profile - DSL profiles */
enum dsl_profile {
	VDSL2_8a	= 1,
	VDSL2_8b	= 1 << 1,
	VDSL2_8c	= 1 << 2,
	VDSL2_8d	= 1 << 3,
	VDSL2_12a	= 1 << 4,
	VDSL2_12b	= 1 << 5,
	VDSL2_17a	= 1 << 6,
	VDSL2_30a	= 1 << 7,
	VDSL2_35b	= 1 << 8,
};

/** enum dsl_power_state - power states */
enum dsl_power_state {
	/* Starts with non-zero in order to distinguish from an uninitialized value which is usually 0 */
	DSL_L0 = 1,
	DSL_L1,
	DSL_L2,
	DSL_L3,
	DSL_L4
};

/** struct dsl_line - DSL line parameters */
struct dsl_line {
	/** The current operational state of the DSL line */
	enum dsl_if_status status;
	/** Whether the interface points towards the Internet (true) or towards End Devices (false) */
	bool upstream;
	/** The version of the modem firmware currently installed for this interface */
	char firmware_version[64];
	/** Status of the DSL physical link */
	enum dsl_link_status link_status;
	/** The transmission system types or standards supported */
	struct dsl_standard standard_supported;
	/** The currently used transmission system type, or the standard on this line instance */
	struct dsl_standard standard_used;
	/** The line encoding method used in establishing the Layer 1 DSL connection between the CPE and the DSLAM */
	enum dsl_line_encoding line_encoding;
	/** VDSL2 profiles are allowed on the line. The bitmap is defined in enum dsl_profile */
	unsigned long allowed_profiles;
	/** VDSL2 profile is currently in use on the line */
	enum dsl_profile current_profile;
	/** The power management state of the line */
	enum dsl_power_state power_management_state;
	/** The success failure cause of the initialization */
	unsigned int success_failure_cause;
	/** VTU-R estimated upstream power back-off electrical length per band */
	dsl_ulong_sequence_t upbokler_pb;
	/** Downstream receiver signal level threshold */
	dsl_ulong_sequence_t rxthrsh_ds;
	/** The actual active rate adaptation mode in both directions */
	dsl_ulong_t act_ra_mode;
	/** The actual signal-to-noise margin of the robust overhead channel (ROC) */
	unsigned int snr_mroc_us;
	/** The last successful transmitted initialization state in both directions */
	dsl_ulong_t last_state_transmitted;
	/** The allowed VDSL2 US0 PSD masks for Annex A operation */
	unsigned int us0_mask;
	/** Whether trellis coding is enabled in the downstream and upstream directions */
	dsl_long_t trellis;
	/** Whether the OPTIONAL virtual noise mechanism is in use in both directions */
	dsl_ulong_t act_snr_mode;
	/** The line pair that the modem is using to connection */
	int line_number;
	/** The current maximum attainable data rate in both directions (expressed in Kbps) */
	dsl_ulong_t max_bit_rate;
	/** The current signal-to-noise ratio margin (expressed in 0.1dB) in both directions */
	dsl_long_t noise_margin;
	/** The current signal-to-noise ratio margin of each upstream band */
	dsl_long_sequence_t snr_mpb_us;
	/** The current signal-to-noise ratio margin of each downstream band */
	dsl_long_sequence_t snr_mpb_ds;
	/** The current upstream and downstream signal loss (expressed in 0.1dB). */
	dsl_long_t attenuation;
	/** The current output and received power at the CPE's DSL line (expressed in 0.1dBmV) */
	dsl_long_t power;
	/** xTU-R vendor identifier as defined in G.994.1 and T1.413 in hex binary format */
	char xtur_vendor[16 + 1]; // Adding '\0' in the end for convenient manipulation
	/** T.35 country code of the xTU-R vendor as defined in G.994.1 in hex binary format */
	char xtur_country[4 + 1]; // Adding '\0' in the end for convenient manipulation
	/** xTU-R T1.413 Revision Number as defined in T1.413 Issue 2 */
	unsigned int xtur_ansi_std;
	/** xTU-R Vendor Revision Number as defined in T1.413 Issue 2 */
	unsigned int xtur_ansi_rev;
	/** xTU-C vendor identifier as defined in G.994.1 and T1.413 in hex binary format */
	char xtuc_vendor[16 + 1]; // Adding '\0' in the end for convenient manipulation
	/** T.35 country code of the xTU-C vendor as defined in G.994.1 in hex binary format */
	char xtuc_country[4 + 1]; // Adding '\0' in the end for convenient manipulation
	/** xTU-C T1.413 Revision Number as defined in T1.413 Issue 2 */
	unsigned int xtuc_ansi_std;
	/** xTU-C Vendor Revision Number as defined in T1.413 Issue 2 */
	unsigned int xtuc_ansi_rev;
};

/** struct dsl_line_channel_stats - Statistics counters for DSL line and channel */
struct dsl_line_channel_stats {
	/** The number of seconds since the beginning of the period used for collection of Total statistics */
	unsigned int total_start;
	/** The number of seconds since the most recent DSL Showtime */
	unsigned int showtime_start;
	/** The number of seconds since the second most recent DSL Showtime-the beginning of the period used for
	 *  collection of LastShowtime statistics */
	unsigned int last_showtime_start;
	/** The number of seconds since the beginning of the period used for collection of CurrentDay statistics */
	unsigned int current_day_start;
	/** The number of seconds since the beginning of the period used for collection of QuarterHour statistics */
	unsigned int quarter_hour_start;
};

/** struct dsl_line_stats_interval - This is a common structure for all interval statistics */
struct dsl_line_stats_interval {
	/** Number of errored seconds */
	unsigned int errored_secs;
	/** Number of severely errored seconds */
	unsigned int severely_errored_secs;
};

/**
 *	enum dsl_stats_type - Type of DSL interval statistics
 */
enum dsl_stats_type {
	/* Starts with non-zero in order to distinguish from an uninitialized value which is usually 0 */
	DSL_STATS_TOTAL = 1,
	DSL_STATS_SHOWTIME,
	DSL_STATS_LASTSHOWTIME,
	DSL_STATS_CURRENTDAY,
	DSL_STATS_QUARTERHOUR
};

/** enum dsl_link_encapsulation - Type of link encapsulation method defineds as bit maps */
enum dsl_link_encapsulation {
	G_992_3_ANNEK_K_ATM	= 1,
	G_992_3_ANNEK_K_PTM	= 1 << 1,
	G_993_2_ANNEK_K_ATM	= 1 << 2,
	G_993_2_ANNEK_K_PTM	= 1 << 3,
	G_994_1_AUTO		= 1 << 4
};

/** struct dsl_channel - DSL channel parameters */
struct dsl_channel {
	/** The current operational state of the DSL channel */
	enum dsl_if_status status;
	/** Which link encapsulation standards and recommendations are supported by the channel */
	unsigned long link_encapsulation_supported;
	/** The link encapsulation standard that the channel instance is using for the connection. */
	enum dsl_link_encapsulation link_encapsulation_used;
	/** The index of the latency path supporting the bearer channel */
	unsigned int lpath;
	/** The interleaver depth D for the latency path indicated in lpath */
	unsigned int intlvdepth;
	/** The interleaver block length in use on the latency path indicated in lpath */
	int intlvblock;
	/** The actual delay, in milliseconds, of the latency path due to interleaving */
	unsigned int actual_interleaving_delay;
	/** The actual impulse noise protection (INP) provided by the latency path indicated in lpath */
	int actinp;
	/** Whether the value reported in actinp was computed assuming the receiver does not use erasure decoding */
	bool inpreport;
	/** Reports the size, in octets, of the Reed-Solomon codeword in use on the latency path indicated in lpath */
	int nfec;
	/** The number of redundancy bytes per Reed-Solomon codeword on the latency path indicated in lpath */
	int rfec;
	/** The number of bits per symbol assigned to the latency path indicated in lpath */
	int lsymb;
	/** The current physical layer aggregate data rate (expressed in Kbps) in both directions. */
	dsl_ulong_t curr_rate;
	/** Actual net data rate expressed in Kbps in both directions */
	dsl_ulong_t actndr;
	/** Actual impulse noise protection in both directions against REIN, expressed in 0.1 DMT symbols */
	dsl_ulong_t actinprein;
};

/** This value shall be used if any of elements defined in "struct dsl_channel_stats_interval" is unavailable */
#define DSL_INVALID_STATS_COUNTER 4294967295
/**
 *  struct dsl_channel_stats_interval - This is a common structure for all interval statistics
 */
struct dsl_channel_stats_interval {
	/** Number of FEC errors */
	unsigned int xtur_fec_errors;
	/** Number of FEC errors detected by the ATU-C */
	unsigned int xtuc_fec_errors;
	/** Number of HEC errors */
	unsigned int xtur_hec_errors;
	/** Number of HEC errors detected by the ATU-C */
	unsigned int xtuc_hec_errors;
	/** Number of CRC errors */
	unsigned int xtur_crc_errors;
	/** Number of CRC errors detected by the ATU-C */
	unsigned int xtuc_crc_errors;
};

/**
 * This function gets the number of DSL lines
 *
 * @return the number of DSL lines on success. Otherwise a negative value is returned.
 *
 * Note that this API must be implemented on all platforms on which libdsl is enabled.
 */
int dsl_get_line_number(void);

/**
 * This function gets the number of DSL channels
 *
 * @return the number of DSL channels on success. Otherwise a negative value is returned.
 *
 * Note that this API must be implemented on all platforms on which libdsl is enabled.
 */
int dsl_get_channel_number(void);

/**
 * This function gets the DSL line information
 *
 * @param[in] line_num - The line number which starts with 0
 * @param[out] line - The output parameter to receive the data
 *
 * @return 0 on success. Otherwise a negative value is returned
 */
int dsl_get_line_info(int line_num, struct dsl_line *line);

/**
 * This function gets the statistics counters of a DSL line
 *
 * @param[in] line_num - The line number which starts with 0
 * @param[out] stats The output parameter to receive the data
 *
 * @return 0 on success. Otherwise a negative value is returned
 */
int dsl_get_line_stats(int line_num, struct dsl_line_channel_stats *stats);

/**
 * This function gets the interval statistics counters of a DSL line
 *
 * @param[in] line_num - The line number which starts with 0
 * @param[in] type The type of interval statistics
 * @param[out] stats The output parameter to receive the data
 *
 * @return 0 on success. Otherwise a negative value is returned
 */
int dsl_get_line_stats_interval(int line_num, enum dsl_stats_type type, struct dsl_line_stats_interval *stats);

/**
 * This function gets the DSL channel information
 *
 * @param[in] chan_num - The channel number which starts with 0
 * @param[out] channel The output parameter to receive the data
 *
 * @return 0 on success. Otherwise a negative value is returned
 */
int dsl_get_channel_info(int chan_num, struct dsl_channel *channel);

/**
 * This function gets the statistics counters of a DSL channel
 *
 * @param[in] chan_num - The channel number which starts with 0
 * @param[out] stats The output parameter to receive the data
 *
 * @return 0 on success. Otherwise a negative value is returned
 */
int dsl_get_channel_stats(int chan_num, struct dsl_line_channel_stats *stats);

/**
 * This function gets the interval statistics counters of a DSL channel
 *
 * @param[in] chan_num - The channel number which starts with 0
 * @param[in] type The type of interval statistics
 * @param[out] stats The output parameter to receive the data
 *
 * @return 0 on success. Otherwise a negative value is returned
 */
int dsl_get_channel_stats_interval(int chan_num, enum dsl_stats_type type, struct dsl_channel_stats_interval *stats);

/**
 *  struct dsl_ops - This structure defines the DSL operations.
 *  A function pointer shall be NULL if the operation
 */
struct dsl_ops {
	int (*get_line_info)(int line_num, struct dsl_line *line);
	int (*get_line_stats)(int line_num, struct dsl_line_channel_stats *stats);
	int (*get_line_stats_interval)(int line_num, enum dsl_stats_type type,
			struct dsl_line_stats_interval *stats);
	int (*get_channel_info)(int chan_num, struct dsl_channel *channel);
	int (*get_channel_stats)(int chan_num, struct dsl_line_channel_stats *stats);
	int (*get_channel_stats_interval)(int chan_num, enum dsl_stats_type type,
			struct dsl_channel_stats_interval *stats);
};

/** This global variable must be defined for each platform specific implementation */
extern const struct dsl_ops xdsl_ops;

#ifdef __cplusplus
}
#endif
#endif /* _XDSL_H */
