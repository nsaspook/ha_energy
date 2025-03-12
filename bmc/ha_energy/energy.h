/** \file energy.h
 *
 */

#ifndef BMC_H
#define BMC_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdlib.h>
#include <stdio.h> /* for printf() */
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include <cjson/cJSON.h>
#include <curl/curl.h>
#include <pthread.h>
#include <sys/stat.h>
#include <syslog.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ifaddrs.h>
#include "MQTTClient.h"
#include "pid.h"


#define LOG_VERSION     "V0.74"
#define MQTT_VERSION    "V3.11"
#define TNAME  "maint9"
#define LADDRESS        "tcp://127.0.0.1:1883"
#ifdef __amd64
#define ADDRESS         "tcp://10.1.1.172:1883"
#else
#define ADDRESS         "tcp://10.1.1.30:1883"
#endif
#define CLIENTID1       "Energy_Mqtt_HA1"
#define CLIENTID2       "Energy_Mqtt_HA2"
#define CLIENTID3       "Energy_Mqtt_HA3" 
#define TOPIC_P         "mateq84/data/gticmd"
#define TOPIC_SPAM      "mateq84/data/spam"
#define TOPIC_PACA      "home-assistant/gtiac/availability"
#define TOPIC_PDCA      "home-assistant/gtidc/availability"
#define TOPIC_PACC      "home-assistant/gtiac/contact"
#define TOPIC_PDCC      "home-assistant/gtidc/contact"
#define TOPIC_PPID      "home-assistant/solar/pid"
#define TOPIC_SHUTDOWN  "home-assistant/solar/shutdown"
#define TOPIC_SS        "mateq84/data/solar"
#define TOPIC_SD        "mateq84/data/dumpload"
#define TOPIC_HA        "home-assistant/status/switch"
#define QOS             1
#define TIMEOUT         10000L
#define SPACING_USEC    500 * 1000
#define USEC_SEC        1000000L

#define DAQ_STR 32
#define DAQ_STR_M DAQ_STR-1

#define SBUF_SIZ        16  // short buffer string size
#define RBUF_SIZ        82
#define SYSLOG_SIZ      512

#define MQTT_TIMEOUT    900
#define SW_QOS          1

#define NO_CYLON
#define CRITIAL_SHUTDOWN_LOG

#define UNIT_TEST       2
#define NORM_MODE       0
#define PID_MODE        1
#define MAX_ERROR       5
#define IAM_DELAY       120

#define CMD_SEC         10
#define TIME_SYNC_SEC   30

	/*
	 * Battery SoC cycle limits parameters
	 */
#define BAT_M_KW            5120.0f
#define BAT_SOC_TOP         0.98f
#define BAT_SOC_HIGH        0.95f
#define BAT_SOC_LOW         0.68f
#define BAT_SOC_LOW_AC      0.72f	
#define BAT_CRITICAL        746.0f /// one electrical HP, for one hour
#define MIN_BAT_KW_BSOC_SLP 4000.0f
#define MIN_BAT_KW_BSOC_HI  4550.0f

#define MIN_BAT_KW_GTI_HI   BAT_M_KW*BAT_SOC_TOP
#define MIN_BAT_KW_GTI_LO   BAT_M_KW*BAT_SOC_LOW

#define MIN_BAT_KW_AC_HI    BAT_M_KW*BAT_SOC_HIGH
#define MIN_BAT_KW_AC_LO    BAT_M_KW*BAT_SOC_LOW_AC

	/*
	 * PV panel cycle limits parameters
	 */
#define PV_PGAIN            0.85f
#define PV_IGAIN            0.12f
#define PV_IMAX             1400.0f
#define PV_BIAS             288.0f
#define PV_BIAS_ZERO          0.0f
#define PV_BIAS_LOW         222.0f
#define PV_BIAS_FLOAT       399.0f
#define PV_BIAS_SLEEP       480.0f
#define PV_BIAS_RATE        320.0f
#define PV_DL_MPTT_MAX     1200.0f
#define PV_DL_MPTT_EXCESS  1300.0f
#define PV_DL_MPTT_IDLE     57.0f
#define PV_DL_BIAS_RATE     75.0f
#define PV_DL_EXCESS       500.0f
#define PV_DL_B_AH_LOW     100.0f
#define PV_DL_B_AH_MIN     150.0f // DL battery should be at least 175Ah
#define PV_DL_B_V_LOW       23.8f // Battery low-voltqage cutoff
#define PWA_SLEEP          200.0f
#define DL_AC_DC_EFF        1.24f

	/*
	 * Energy control loop parameters
	 */
#define BAL_MIN_ENERGY_AC   -200.0f
#define BAL_MAX_ENERGY_AC   200.0f
#define BAL_MIN_ENERGY_GTI  -1400.0f
#define BAL_MAX_ENERGY_GTI  200.0f

#define LOG_TO_FILE         "/store/logs/energy.log"
#define LOG_TO_FILE_ALT     "/tmp/energy.log"

#define MAX_LOG_SPAM  60
#define LOW_LOG_SPAM  2
#define RESET_LOG_SPAM  120

	//#define IM_DEBUG                      // WEM3080T LOGGING
	//#define B_ADJ_DEBUG                   // debug printing
	//#define FAKE_VPV                      // NO AC CHARGER for DUMPLOAD, batteries are cross-connected to a parallel bank
	//#define PSW_DEBUG
	//#define DEBUG_SHUTDOWN

	//#define AUTO_CHARGE                   // turn on dumpload charger during restarts
	//#define B_DLE_DEBUG    // Dump Load debugging
	//#define BSOC_DEGUB
	//#define DEBUG_HA_CMD

#define IM_DELAY            1   // tens of second updates
#define IM_DISPLAY          1
#define GTI_DELAY           1

	/*
	 * sane limits for system data elements
	 */
#define PWA_SANE            1700.0f
#define PAMPS_SANE          16.0f
#define PVOLTS_SANE         150.0f
#define BAMPS_SANE          70.0f

	/*
	    Three Phase WiFi Energy Meter (WEM3080T)
	name	Unit	Description
	wem3080t_voltage_a	V	A phase voltage
	wem3080t_current_a	A	A phase current
	wem3080t_power_a	W	A phase active power
	wem3080t_importenergy_a	kWh	A phase import energy
	wem3080t_exportgrid_a	kWh	A phase export energy
	wem3080t_frequency_a	kWh	A phase frequency
	wem3080t_pf_a	kWh	A phase power factor
	wem3080t_voltage_b	V	B phase voltage
	wem3080t_current_b	A	B phase current
	wem3080t_power_b	W	B phase active power
	wem3080t_importenergy_b	kWh	B phase import energy
	wem3080t_exportgrid_b	kWh	B phase export energy
	wem3080t_frequency_b	kWh	B phase frequency
	wem3080t_pf_b	kWh	B phase power factor
	wem3080t_voltage_c	V	C phase voltage
	wem3080t_current_c	A	C phase current
	wem3080t_power_c	W	C phase active power
	wem3080t_importenergy_c	kWh	C phase import energy
	wem3080t_exportgrid_c	kWh	C phase export energy
	wem3080t_frequency_c	kWh	C phase frequency
	wem3080t_pf_c	kWh	C phase power factor
	 */

	enum client_id {
		ID_C1,
		ID_C2,
		ID_C3,
	};
	
	enum energy_state {
		E_INIT,
		E_RUN,
		E_WAIT,
		E_IDLE,
		E_STOP,
		E_LAST,
	};

	enum running_state {
		R_INIT,
		R_FLOAT,
		R_SLEEP,
		R_RUN,
		R_IDLE,
		R_LAST,
	};

	enum iammeter_phase {
		PHASE_A,
		PHASE_B,
		PHASE_C,
		PHASE_LAST,
	};

	enum iammeter_id {
		IA_VOLTAGE,
		IA_CURRENT,
		IA_POWER,
		IA_IMPORT,
		IA_EXPORT,
		IA_FREQ,
		IA_PF,
		IA_LAST,
	};

	enum mqtt_vars {
		V_FCCM,
		V_FBEKW,
		V_FRUNT,
		V_FBAMPS,
		V_FBV,
		V_FLO,
		V_FSO,
		V_FACE,
		V_BEN,
		V_PWA,
		V_PAMPS,
		V_PVOLTS,
		V_FLAST,
		V_HDCSW,
		V_HACSW,
		V_HSHUT,
		V_HMODE,
		V_HCON0,
		V_HCON1,
		V_HCON2,
		V_HCON3,
		V_HCON4,
		V_HCON5,
		V_HCON6,
		V_HCON7,
		// add other data ranges here
		V_DVPV,
		V_DPPV,
		V_DPBAT,
		V_DVBAT,
		V_DCMPPT,
		V_DPMPPT,
		V_DAHBAT,
		V_DCCMODE,
		V_DGTI,
		V_DLAST,
	};

	enum sane_vars {
		S_FCCM,
		S_FBEKW,
		S_FRUNT,
		S_FBAMPS,
		S_FBV,
		S_FLO,
		S_FSO,
		S_FACE,
		S_BEN,
		S_PWA,
		S_PAMPS,
		S_PVOLTS,
		S_FLAST,
		S_HDCSW,
		S_HACSW,
		S_HSHUT,
		S_HMODE,
		// add other data ranges here
		S_DVPV,
		S_DPPV,
		S_DPBAT,
		S_DVBAT,
		S_DCMPPT,
		S_DPMPPT,
		S_DAHBAT,
		S_DCCMODE,
		S_DGTI,
		S_DLAST,
	};

#define MAX_IM_VAR  IA_LAST*PHASE_LAST

#define L1_P    IA_POWER
#define L2_P    L1_P+IA_LAST
#define L3_P    L2_P+IA_LAST

	struct link_type {
		volatile uint32_t iammeter_error, iammeter_count;
		volatile uint32_t mqtt_error, mqtt_count;
		volatile uint32_t shutdown;
	};

	struct mode_type {
		volatile double error, target, total_system, gti_dumpload, pv_bias, dl_mqtt_max, off_grid, sequence;
		volatile bool mode, in_pid_control, con0, con1, con2, con3, con4, con5, con6, con7, no_float, data_error, bat_crit;
		volatile uint32_t mode_tmr;
		volatile struct SPid pid;
		volatile enum energy_state E;
		volatile enum running_state R;
	};

	struct energy_type {
		volatile double print_vars[MAX_IM_VAR];
		volatile double im_vars[IA_LAST][PHASE_LAST];
		volatile double mvar[V_DLAST + 1];
		volatile bool once_gti, once_ac, iammeter, fm80, dumpload, homeassistant, once_gti_zero;
		volatile double gti_low_adj, ac_low_adj, dl_excess_adj;
		volatile bool ac_sw_on, gti_sw_on, ac_sw_status, gti_sw_status, solar_shutdown, solar_mode, startup, ac_mismatch, dc_mismatch, mode_mismatch, dl_excess;
		volatile uint32_t speed_go, im_delay, im_display, gti_delay;
		volatile int32_t rc, sane;
		volatile uint32_t ten_sec_clock, log_spam, log_time_reset;
		pthread_mutex_t ha_lock;
		struct mode_type mode;
		struct link_type link;
		MQTTClient client_p, client_sd, client_ha;
	};

	extern struct energy_type E;
	extern struct ha_flag_type ha_flag_vars_ss;
	extern FILE* fout;

	void timer_callback(int32_t);
	void connlost(void *, char *);

	void ramp_up_gti(MQTTClient, bool, bool);
	void ramp_up_ac(MQTTClient, bool);
	void ramp_down_gti(MQTTClient, bool);
	void ramp_down_ac(MQTTClient, bool);
	void ha_ac_off(void);
	void ha_ac_on(void);
	void ha_dc_off(void);
	void ha_dc_on(void);

	size_t iammeter_write_callback(char *, size_t, size_t, void *);
	void iammeter_read(void);
	void print_im_vars(void);
	void print_mvar_vars(void);

	bool sanity_check(void);
	char * log_time(bool);
	bool sync_ha(void);
	bool log_timer(void);

#ifdef __cplusplus
}
#endif

#endif /* BMC_H */

