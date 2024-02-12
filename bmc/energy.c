/*
 * HA Energy control using MQTT JSON format data from various energy monitor sources
 * asynchronous mode using threads
 *
 * This file may be freely modified, distributed, and combined with
 * other software, as long as proper attribution is given in the
 * source code.
 */
#define _DEFAULT_SOURCE
#include "ha_energy/energy.h"
#include "ha_energy/mqtt_rec.h"
#include "ha_energy/bsoc.h"

#define LOG_VERSION     "V0.27"
#define MQTT_VERSION    "V3.11"
#define ADDRESS         "tcp://10.1.1.172:1883"
#define CLIENTID1       "Energy_Mqtt_HA1"
#define CLIENTID2       "Energy_Mqtt_HA2"
#define TOPIC_P         "mateq84/data/gticmd"
#define TOPIC_PACA      "home-assistant/gtiac/availability"
#define TOPIC_PDCA      "home-assistant/gtidc/availability"
#define TOPIC_PACC      "home-assistant/gtiac/contact"
#define TOPIC_PDCC      "home-assistant/gtidc/contact"
#define TOPIC_SS        "mateq84/data/solar"
#define TOPIC_SD        "mateq84/data/dumpload"
#define QOS             1
#define TIMEOUT         10000L
#define SPACING_USEC    500 * 1000

/*
 * V0.25 add Home Assistant Matter controlled utility power control switching
 * V0.26 BSOC weights for system condition for power diversion
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
    .energy_mode = UNIT_TEST,
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
    uint32_t speed_go = 0, sequence = 0, rc;
    struct itimerval new_timer = {
        .it_value.tv_sec = CMD_SEC,
        .it_value.tv_usec = 0,
        .it_interval.tv_sec = CMD_SEC,
        .it_interval.tv_usec = 0,
    };
    struct itimerval old_timer;

    MQTTClient client_p, client_sd;
    MQTTClient_connectOptions conn_opts_p = MQTTClient_connectOptions_initializer,
            conn_opts_sd = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;

    printf("\r\n LOG Version %s : MQTT Version %s\r\n", LOG_VERSION, MQTT_VERSION);

    if (!bsoc_init()) {
        exit(EXIT_FAILURE);
    }
    /*
     * set the timer for MQTT publishing sample speed
     */
    setitimer(ITIMER_REAL, &new_timer, &old_timer);
    signal(SIGALRM, timer_callback);

    MQTTClient_create(&client_p, ADDRESS, CLIENTID1,
            MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts_p.keepAliveInterval = 20;
    conn_opts_p.cleansession = 1;

    MQTTClient_setCallbacks(client_p, &ha_flag_vars_ss, connlost, msgarrvd, delivered);
    if ((rc = MQTTClient_connect(client_p, &conn_opts_p)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", rc);
        pthread_mutex_destroy(&ha_lock);
        exit(EXIT_FAILURE);
    }

    MQTTClient_create(&client_sd, ADDRESS, CLIENTID2,
            MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts_sd.keepAliveInterval = 20;
    conn_opts_sd.cleansession = 1;

    MQTTClient_setCallbacks(client_sd, &ha_flag_vars_sd, connlost, msgarrvd, delivered);
    if ((rc = MQTTClient_connect(client_sd, &conn_opts_sd)) != MQTTCLIENT_SUCCESS) {
        printf("Failed to connect, return code %d\n", rc);
        pthread_mutex_destroy(&ha_lock);
        exit(EXIT_FAILURE);
    }

    /*
     * on topic received data will trigger the msgarrvd function
     */
    MQTTClient_subscribe(client_p, TOPIC_SS, QOS);
    MQTTClient_subscribe(client_sd, TOPIC_SD, QOS);

    pubmsg.payload = "online";
    pubmsg.payloadlen = strlen("online");
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
    ha_flag_vars_ss.deliveredtoken = 0;
    // notify HA we are running and controlling AC power plugs
    MQTTClient_publishMessage(client_p, TOPIC_PACA, &pubmsg, &token);
    MQTTClient_publishMessage(client_p, TOPIC_PDCA, &pubmsg, &token);

    // sync HA power switches
    mqtt_ha_switch(client_p, TOPIC_PDCC, false);
    mqtt_ha_switch(client_p, TOPIC_PACC, false);
    mqtt_ha_switch(client_p, TOPIC_PDCC, true);
    mqtt_ha_switch(client_p, TOPIC_PACC, true);
    mqtt_ha_switch(client_p, TOPIC_PDCC, false);
    mqtt_ha_switch(client_p, TOPIC_PACC, false);

    {
        printf("\r\n Solar Energy AC power controller\r\n");

        if (ha_flag_vars_ss.energy_mode == UNIT_TEST) {
            mqtt_ha_switch(client_p, TOPIC_PDCC, true);
            usleep(500000); // wait for voltage to ramp
        }

        while (true) {

            usleep(100);

            if (ha_flag_vars_ss.runner || speed_go++ > 1500000) {
                speed_go = 0;
                ha_flag_vars_ss.runner = false;
                bsoc_data_collect();

                if (ha_flag_vars_ss.energy_mode == UNIT_TEST) {
                    switch (sequence) {
                        case 5:
                        case 4:
                            sequence++;
                            mqtt_gti_power(client_p, TOPIC_P, "-#"); // - 100W power
                            break;
                        case 3:
                        case 2:
                        case 1:
                            sequence++;
                            mqtt_gti_power(client_p, TOPIC_P, "+#"); // +100W power
                            break;
                        case 0:
                            sequence++;
                            mqtt_gti_power(client_p, TOPIC_P, "Z#"); // zero power
                            break;
                        default:
                            mqtt_gti_power(client_p, TOPIC_P, "Z#"); // zero power
                            sequence = 0;
                            break;
                    }
                }
            } else {
                if (ha_flag_vars_ss.receivedtoken) {
                    ha_flag_vars_ss.receivedtoken = false;
                }
                if (ha_flag_vars_sd.receivedtoken) {
                    ha_flag_vars_sd.receivedtoken = false;
                }
            }
        }
    }
    return 0;
}

