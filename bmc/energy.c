/*
 * HA Energy control using MQTT JSON and HTTP format data from various energy monitor sources
 * asynchronous mode using threads
 * 
 * long life HA token: eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiI2OTczODQyMGZlMDU0MjVmYjk1OWY0YjM3Mjg4NjRkOSIsImlhdCI6MTcwODg3NzA1OSwiZXhwIjoyMDI0MjM3MDU5fQ.5vfW85qQ2DO3vAyM_lcm1YyNqIX2O8JlTqoYKLdxf6M
 *
 * This file may be freely modified, distributed, and combined with
 * other software, as long as proper attribution is given in the
 * source code.
 */
#define _DEFAULT_SOURCE
#include "ha_energy/energy.h"
#include "ha_energy/mqtt_rec.h"
#include "ha_energy/bsoc.h"

#define LOG_VERSION     "V0.39"
#define MQTT_VERSION    "V3.11"
#define ADDRESS         "tcp://10.1.1.172:1883"
#define CLIENTID1       "Energy_Mqtt_HA1"
#define CLIENTID2       "Energy_Mqtt_HA2"
#define TOPIC_P         "mateq84/data/gticmd"
#define TOPIC_PACA      "home-assistant/gtiac/availability"
#define TOPIC_PDCA      "home-assistant/gtidc/availability"
#define TOPIC_PACC      "home-assistant/gtiac/contact"
#define TOPIC_PDCC      "home-assistant/gtidc/contact"
#define TOPIC_PPID      "home-assistant/solar/pid"
#define TOPIC_SS        "mateq84/data/solar"
#define TOPIC_SD        "mateq84/data/dumpload"
#define QOS             1
#define TIMEOUT         10000L
#define SPACING_USEC    500 * 1000

/*
 * V0.25 add Home Assistant Matter controlled utility power control switching
 * V0.26 BSOC weights for system condition for power diversion
 * V0.27 -> V0.28 GTI power ramps stability using battery current STD DEV
 * V0.29 log date-time and spam control
 * V0.30 add iammeter http data reading and processing
 * V0.31 refactor http code and a few vars
 * V0.32 AC and GTI power triggers reworked
 * V0.33 refactor system parms into energy structure energy_type E
 * V0.34 GTI and AC Inverter battery energy run down limits adjustments per energy usage and solar production
 * V0.35 more refactors and global variable consolidation
 * V0.36 more command repeat fixes for ramp up/down dumpload commands
 * V0.37 Power feedback to use PV power to GTI and AC loads
 * V0.38 signal filters to smooth large power swings in control optimization
 * V0.39 fix optimizer bugs and add AC load switching set-points in BSOC control
 */

/*
 * for the publish and subscribe topic pair
 * passed as a context variable
 */
// for local device Comedi device control
struct ha_flag_type ha_flag_vars_pc = {
    .runner = false,
    .receivedtoken = false,
    .deliveredtoken = false,
    .rec_ok = false,
    .ha_id = P8055_ID,
    .var_update = 0,
};

// solar data from mateq84
struct ha_flag_type ha_flag_vars_ss = {
    .runner = false,
    .receivedtoken = false,
    .deliveredtoken = false,
    .rec_ok = false,
    .ha_id = FM80_ID,
    .var_update = 0,
    .energy_mode = NORM_MODE,
};

// dumpload data from mbmc_k42
struct ha_flag_type ha_flag_vars_sd = {
    .runner = false,
    .receivedtoken = false,
    .deliveredtoken = false,
    .rec_ok = false,
    .ha_id = DUMPLOAD_ID,
    .var_update = 0,
};

char *token;
const char *board_name = "NO_BOARD", *driver_name = "NO_DRIVER";
cJSON *json;

FILE* fout;

struct energy_type E = {
    .once_gti = true,
    .once_ac = true,
    .once_gti_zero = true,
    .iammeter = false,
    .fm80 = false,
    .dumpload = false,
    .ac_low_adj = 0.0f,
    .gti_low_adj = 0.0f,
    .ac_sw_on = true,
    .gti_sw_on = true,
    .im_delay = 0,
    .gti_delay = 0,
    .im_display = 0,
    .rc = 0,
    .speed_go = 0,
    .mode.pid.iMax = PV_IMAX,
    .mode.pid.iMin = 0.0f,
    .mode.pid.pGain = PV_PGAIN,
    .mode.pid.iGain = PV_IGAIN,
    .mode.mode_tmr = 0,
    .mode.mode = true,
    .mode.in_control = false,
    .ac_sw_status = false,
    .gti_sw_status = false,
    .solar_mode= false,
    .solar_shutdown = false,
};

static bool solar_shutdown(void);

/*
 * Async processing threads
 */

/*
 * Comedi data update timer flag
 */
void timer_callback(int32_t signum) {
    signal(signum, timer_callback);
    ha_flag_vars_ss.runner = true;
}

/*
 * Broker errors
 */
void connlost(void *context, char *cause) {
    struct ha_flag_type *ha_flag = context;
    int32_t id_num;

    // bug-out if no context variables passed to callback
    if (context == NULL) {
        id_num = -1;
    } else {
        id_num = ha_flag->ha_id;
    }
    printf("\nConnection lost\n");
    printf("     cause: %s, %d\n", cause, id_num);
    exit(EXIT_FAILURE);
}

/*
 * Use MQTT to send/receive DAQ updates to a Comedi hardware device
 */
int main(int argc, char *argv[]) {
    struct itimerval new_timer = {
        .it_value.tv_sec = CMD_SEC,
        .it_value.tv_usec = 0,
        .it_interval.tv_sec = CMD_SEC,
        .it_interval.tv_usec = 0,
    };
    struct itimerval old_timer;

    time_t rawtime;


    MQTTClient_connectOptions conn_opts_p = MQTTClient_connectOptions_initializer,
            conn_opts_sd = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;

    printf("\r\n LOG Version %s : MQTT Version %s\r\n", LOG_VERSION, MQTT_VERSION);

#ifdef LOG_TO_FILE
    fout = fopen(LOG_TO_FILE, "a");
    if (fout == NULL) {
        fout = stdout;
        printf("\r\nUnable to open LOG file %s \r\n", LOG_TO_FILE);
    }
#else
    fout = stdout;
#endif
    fprintf(fout, "\r\n LOG Version %s : MQTT Version %s\r\n", LOG_VERSION, MQTT_VERSION);

    if (!bsoc_init()) {
        exit(EXIT_FAILURE);
    }
    /*
     * set the timer for MQTT publishing sample speed
     */
    setitimer(ITIMER_REAL, &new_timer, &old_timer);
    signal(SIGALRM, timer_callback);

    MQTTClient_create(&E.client_p, ADDRESS, CLIENTID1,
            MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts_p.keepAliveInterval = 20;
    conn_opts_p.cleansession = 1;

    MQTTClient_setCallbacks(E.client_p, &ha_flag_vars_ss, connlost, msgarrvd, delivered);
    if ((E.rc = MQTTClient_connect(E.client_p, &conn_opts_p)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", E.rc);
        pthread_mutex_destroy(&E.ha_lock);
        exit(EXIT_FAILURE);
    }

    MQTTClient_create(&E.client_sd, ADDRESS, CLIENTID2,
            MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts_sd.keepAliveInterval = 20;
    conn_opts_sd.cleansession = 1;

    MQTTClient_setCallbacks(E.client_sd, &ha_flag_vars_sd, connlost, msgarrvd, delivered);
    if ((E.rc = MQTTClient_connect(E.client_sd, &conn_opts_sd)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", E.rc);
        pthread_mutex_destroy(&E.ha_lock);
        exit(EXIT_FAILURE);
    }

    /*
     * on topic received data will trigger the msgarrvd function
     */
    MQTTClient_subscribe(E.client_p, TOPIC_SS, QOS);
    MQTTClient_subscribe(E.client_sd, TOPIC_SD, QOS);

    pubmsg.payload = "online";
    pubmsg.payloadlen = strlen("online");
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
    ha_flag_vars_ss.deliveredtoken = 0;
    // notify HA we are running and controlling AC power plugs
    MQTTClient_publishMessage(E.client_p, TOPIC_PACA, &pubmsg, &token);
    MQTTClient_publishMessage(E.client_p, TOPIC_PDCA, &pubmsg, &token);

    // sync HA power switches
    mqtt_ha_switch(E.client_p, TOPIC_PDCC, false);
    mqtt_ha_switch(E.client_p, TOPIC_PACC, false);
    mqtt_ha_switch(E.client_p, TOPIC_PDCC, true);
    mqtt_ha_switch(E.client_p, TOPIC_PACC, true);
    mqtt_ha_switch(E.client_p, TOPIC_PDCC, false);
    mqtt_ha_switch(E.client_p, TOPIC_PACC, false);

    /*
     * use libcurl to read AC power meter HTTP data
     */
    iammeter_read();

    {
        printf("\r\n Solar Energy AC power controller\r\n");
        fprintf(fout, "\r\n Solar Energy AC power controller\r\n");

        while (true) {

            usleep(100);

            if (ha_flag_vars_ss.runner || E.speed_go++ > 1500000) {
                E.speed_go = 0;
                ha_flag_vars_ss.runner = false;
                bsoc_data_collect();
                if (solar_shutdown()) {
                    time(&rawtime);
                    fprintf(fout, "%s\r\n", ctime(&rawtime));
                    fprintf(fout, " SHUTDOWN Solar Energy ---> \r\n");
                    ramp_down_gti(E.client_p, true);
                    ramp_down_ac(E.client_p, true);
                    fprintf(fout, " Completed.\r\n");
                    fflush(fout);
                    while (solar_shutdown()) {
                        usleep(100000); // wait
                    }
                    time(&rawtime);
                    fprintf(fout, "%s\r\n", ctime(&rawtime));
                    fprintf(fout, " RESTART Solar Energy\r\n");
                    fflush(fout);
                }
                if (bsoc_set_mode(PV_BIAS, true)) {
                    char gti_str[16];
                    int32_t error_drive;

                    ha_flag_vars_ss.energy_mode = PID_MODE;
                    if (E.gti_delay++ >= GTI_DELAY) {
                        if (!E.mode.in_control) {
                            mqtt_ha_switch(E.client_p, TOPIC_PDCC, true);
                            E.gti_sw_status = true;
                            usleep(100000); // wait
                            mqtt_ha_switch(E.client_p, TOPIC_PACC, true);
                            E.ac_sw_status = true;
                        }
                        E.mode.in_control = true;
                        E.gti_delay = 0;
                        error_drive = (int32_t) E.mode.error; // PI feedback control signal
                        if (error_drive < 0) {
                            error_drive = PV_BIAS; // control wide power swings
                        }
                        snprintf(gti_str, 15, "V%04dX", error_drive); // format for dumpload controller gti power commands
                        mqtt_gti_power(E.client_p, TOPIC_P, gti_str);
                    }
                } else {
                    ha_flag_vars_ss.energy_mode = ((int32_t) E.mvar[V_HMODE]);
                    if (E.mode.in_control) {
                        E.mode.in_control = false;
                        ramp_down_gti(E.client_p, true);
                        ramp_down_ac(E.client_p, true);
                    }

                    if (ha_flag_vars_ss.energy_mode == NORM_MODE) {
                        if (fm80_float() || (ac_test() > MIN_BAT_KW_AC_HI)) {
                            ramp_up_ac(E.client_p, E.ac_sw_on);
                            E.ac_sw_on = false;
                        }
                        if (ac_test() < (MIN_BAT_KW_AC_LO + E.ac_low_adj)) {
                            ramp_down_ac(E.client_p, true);
                            E.ac_sw_on = true;
                        }
                        if ((gti_test() > MIN_BAT_KW_GTI_HI)) {
                            ramp_up_gti(E.client_p, E.gti_sw_on); // fixme on the ONCE code
                            E.gti_sw_on = false;
                        } else {
                            if (gti_test() < (MIN_BAT_KW_GTI_LO + E.gti_low_adj)) {
                                ramp_down_gti(E.client_p, true);
                                E.gti_sw_on = true;
                            }
                        }
                    };
                }
#ifdef B_ADJ_DEBUG
                fprintf(fout, "\r\n LO ADJ: AC %8.2fWh, GTI %8.2fWh\r\n", MIN_BAT_KW_AC_LO + E.ac_low_adj, MIN_BAT_KW_GTI_LO + E.gti_low_adj);
#endif

                time(&rawtime);

                if (E.im_delay++ >= IM_DELAY) {
                    E.im_delay = 0;
                    iammeter_read();
                }
                if (E.im_display++ >= IM_DISPLAY) {
                    char buffer[80];
                    uint32_t len;

                    E.im_display = 0;
                    mqtt_ha_pid(E.client_p, TOPIC_PPID);
                    if (!(E.fm80 && E.dumpload && E.iammeter)) {
                        fprintf(fout, "\r\n !!!! Source data update error !!!! , check FM80 %i, DUMPLOAD %i, IAMMETER %i channels\r\n", E.fm80, E.dumpload, E.fm80);
                        fprintf(stderr, "\r\n !!!! Source data update error !!!! , check FM80 %i, DUMPLOAD %i, IAMMETER %i channels\r\n", E.fm80, E.dumpload, E.fm80);
                    }
                    sprintf(buffer, "%s", ctime(&rawtime));
                    len = strlen(buffer);
                    buffer[len - 1] = 0; // munge out the return character
                    fprintf(fout, "%s ", buffer);
                    E.fm80 = false;
                    E.dumpload = false;
                    E.iammeter = false;
                    print_im_vars();
                    print_mvar_vars();
                    fprintf(fout, "%s\r", ctime(&rawtime));
                }
            } else {
                if (ha_flag_vars_ss.receivedtoken) {
                    ha_flag_vars_ss.receivedtoken = false;
                }
                if (ha_flag_vars_sd.receivedtoken) {
                    ha_flag_vars_sd.receivedtoken = false;
                }
            }
            fflush(fout);
        }
    }
    return 0;
}

void ramp_up_gti(MQTTClient client_p, bool start) {
    static uint32_t sequence = 0;

    if (start) {
        E.once_gti = true;
    }

    if (E.once_gti) {
        E.once_gti = false;
        sequence = 0;
        mqtt_ha_switch(client_p, TOPIC_PDCC, true);
        E.gti_sw_status = true;
        usleep(500000); // wait for voltage to ramp
    }

    switch (sequence) {
        case 4:
            E.once_gti_zero = true;
            break;
        case 3:
        case 2:
        case 1:
            E.once_gti_zero = true;
            if (bat_current_stable()) { // check battery current std dev, stop 'motorboating'
                sequence++;
                if (!mqtt_gti_power(client_p, TOPIC_P, "+#")) {
                    sequence = 0;
                }; // +100W power
            } else {
                usleep(500000); // wait a bit more for power to be stable
                sequence = 1; // do power ramps when ready
                if (!mqtt_gti_power(client_p, TOPIC_P, "-#")) {
                    sequence = 0;
                }; // - 100W power
            }
            break;
        case 0:
            sequence++;
            if (E.once_gti_zero) {
                mqtt_gti_power(client_p, TOPIC_P, "Z#"); // zero power
                E.once_gti_zero = false;
            }
            break;
        default:
            if (E.once_gti_zero) {
                mqtt_gti_power(client_p, TOPIC_P, "Z#"); // zero power
                E.once_gti_zero = false;
            }
            sequence = 0;
            break;
    }
}

void ramp_down_gti(MQTTClient client_p, bool sw_off) {
    if (sw_off) {
        mqtt_ha_switch(client_p, TOPIC_PDCC, false);
        E.once_gti_zero = true;
        E.gti_sw_status = false;
    }
    E.once_gti = true;

    if (E.once_gti_zero) {
        mqtt_gti_power(client_p, TOPIC_P, "Z#"); // zero power
        E.once_gti_zero = false;
    }
}

void ramp_up_ac(MQTTClient client_p, bool start) {

    if (start) {
        E.once_ac = true;
    }

    if (E.once_ac) {
        E.once_ac = false;
        mqtt_ha_switch(client_p, TOPIC_PACC, true);
        E.ac_sw_status = true;
        usleep(500000); // wait for voltage to ramp
    }
}

void ramp_down_ac(MQTTClient client_p, bool sw_off) {
    if (sw_off) {
        mqtt_ha_switch(client_p, TOPIC_PACC, false);
        E.ac_sw_status = false;
    }
    E.once_ac = true;
}

void ha_ac_off(void) {
    mqtt_ha_switch(E.client_p, TOPIC_PACC, false);
    E.ac_sw_status = false;
}

void ha_ac_on(void) {
    mqtt_ha_switch(E.client_p, TOPIC_PACC, true);
    E.ac_sw_status = true;
}

static bool solar_shutdown(void) {
    static bool ret = false;
    if (E.solar_shutdown) {
        ret = true;
    } else {
        ret = false;
    }
    return ret;
}