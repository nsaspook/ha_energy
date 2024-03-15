/* 
 * File:   bmc.h
 * Author: root
 *
 * Created on September 21, 2012, 12:54 PM
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
#include "MQTTClient.h"
#include "pid.h"

#define DAQ_STR 32
#define DAQ_STR_M DAQ_STR-1

#define MQTT_TIMEOUT    400
#define SW_QOS          1

#define NO_CYLON

#define UNIT_TEST       1
#define NORM_MODE       0

#define CMD_SEC         10

#define MIN_BAT_KW_GTI_HI   5100.0f
#define MIN_BAT_KW_GTI_LO   4650.0f

#define MIN_BAT_KW_AC_HI    5000.0f
#define MIN_BAT_KW_AC_LO    4500.0f

#define MIN_BAT_KW_BSOC_HI  4000.0f
#define MIN_BAT_KW_BSOC_LO  3500.0f

#define PV_PGAIN            0.85f
#define PV_IGAIN            0.12f
#define PV_IMAX             1400.0f
#define PV_BIAS             120.0f

#define LOG_TO_FILE         "/store/logs/energy.log"

    //#define IM_DEBUG            // WEM3080T LOGGING
    //#define B_ADJ_DEBUG
#define IM_DELAY            1   // tens of second updates    
#define IM_DISPLAY          1
#define GTI_DELAY           1

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
        V_FBV,
        V_FLO,
        V_FSO,
        V_FACE,
        V_BEN,
        V_PWA,
        V_FLAST,
        V_HDCSW,
        V_HACSW,
        V_HSHUT,
        V_HMODE,
        // add other data ranges here
        V_DVPV,
        V_DPPV,
        V_DPBAT,
        V_DVBAT,
        V_DCMPPT,
        V_DGTI,
        V_DLAST,
    };

#define MAX_IM_VAR  IA_LAST*PHASE_LAST

#define L1_P    IA_POWER
#define L2_P    L1_P+IA_LAST
#define L3_P    L2_P+IA_LAST

    struct mode_type {
        volatile double error, target, total_system, gti_dumpload;
        volatile bool mode, in_control;
        volatile uint32_t mode_tmr;
        volatile struct SPid pid;
    };

    struct energy_type {
        volatile double print_vars[MAX_IM_VAR];
        volatile double im_vars[IA_LAST][PHASE_LAST];
        volatile double mvar[V_DLAST];
        volatile bool once_gti, once_ac, iammeter, fm80, dumpload, once_gti_zero;
        volatile double gti_low_adj, ac_low_adj;
        volatile bool ac_sw_on, gti_sw_on, ac_sw_status, gti_sw_status, solar_shutdown, solar_mode;
        volatile uint32_t speed_go, rc, im_delay, im_display, gti_delay;
        pthread_mutex_t ha_lock;
        struct mode_type mode;
        MQTTClient client_p, client_sd;
    };

    extern struct energy_type E;
    extern struct ha_flag_type ha_flag_vars_ss;
    extern FILE* fout;

    void timer_callback(int32_t);
    void connlost(void *, char *);

    void ramp_up_gti(MQTTClient, bool);
    void ramp_up_ac(MQTTClient, bool);
    void ramp_down_gti(MQTTClient, bool);
    void ramp_down_ac(MQTTClient, bool);
    void ha_ac_off(void);
    void ha_ac_on(void);

    size_t iammeter_write_callback(char *, size_t, size_t, void *);
    void iammeter_read(void);
    void print_im_vars(void);
    void print_mvar_vars(void);


#ifdef __cplusplus
}
#endif

#endif /* BMC_H */

