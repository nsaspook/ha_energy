#define _DEFAULT_SOURCE
#include "mqtt_rec.h"
#include "energy.h"
#include "bsoc.h"

/*
 * send PID power control variables to Home Assistant
 */
void mqtt_ha_pid(MQTTClient client_p, const char * topic_p) {
    cJSON *json;

    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    ha_flag_vars_ss.deliveredtoken = 0;

    json = cJSON_CreateObject();
    cJSON_AddNumberToObject(json, "piderror", E.mode.error);
    cJSON_AddNumberToObject(json, "totalsystem", E.mode.total_system);
    cJSON_AddNumberToObject(json, "gtinet", E.mode.gti_dumpload);
    // convert the cJSON object to a JSON string
    char *json_str = cJSON_Print(json);

    pubmsg.payload = json_str;
    pubmsg.payloadlen = strlen(json_str);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;

    MQTTClient_publishMessage(client_p, topic_p, &pubmsg, &token);
    // a busy, wait loop for the async delivery thread to complete
    {
        uint32_t waiting = 0;
        while (ha_flag_vars_ss.deliveredtoken != token) {
            usleep(TOKEN_DELAY);
            if (waiting++ > MQTT_TIMEOUT) {
                fprintf(fout, "\r\nSW Still Waiting, timeout\r\n");
                break;
            }
        };
    }

    cJSON_free(json_str);
    cJSON_Delete(json);
}

/*
 * turn on and off the Matter switches controlled by Home Assistant
 */
void mqtt_ha_switch(MQTTClient client_p, const char * topic_p, bool sw_state) {
    cJSON *json;
    static bool spam = false;
    static uint32_t less_spam = 0;

    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    ha_flag_vars_ss.deliveredtoken = 0;

    json = cJSON_CreateObject();
    if (sw_state) {
        cJSON_AddStringToObject(json, "state", "ON");
        spam = true;
        less_spam = 0;
    } else {
        cJSON_AddStringToObject(json, "state", "OFF");
    }
    // convert the cJSON object to a JSON string
    char *json_str = cJSON_Print(json);

    pubmsg.payload = json_str;
    pubmsg.payloadlen = strlen(json_str);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;

#ifdef DEBUG_HA_CMD
    if (spam) {
        log_time(true);
        fprintf(fout, "HA switch command %s, %s\r\n", topic_p, json_str);
        if (!sw_state) {
            if (less_spam++ > 3) {
                spam = false;
                less_spam = 0;
            }
        }
    }
#endif    

    MQTTClient_publishMessage(client_p, topic_p, &pubmsg, &token);
    // a busy, wait loop for the async delivery thread to complete
    {
        uint32_t waiting = 0;
        while (ha_flag_vars_ss.deliveredtoken != token) {
            usleep(TOKEN_DELAY);
            if (waiting++ > MQTT_TIMEOUT) {
                fprintf(fout, "\r\nSW Still Waiting, timeout\r\n");
                break;
            }
        };
    }

    cJSON_free(json_str);
    cJSON_Delete(json);
    usleep(HA_SW_DELAY);
}

/*
 * send mqtt messages to the dumpload GTI controller
 */
bool mqtt_gti_power(MQTTClient client_p, const char * topic_p, char * msg) {
    bool ret = true;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    ha_flag_vars_ss.deliveredtoken = 0;
    static bool spam = false;

    pubmsg.payload = msg;
    pubmsg.payloadlen = strlen(msg);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;

#ifdef GTI_NO_POWER
    MQTTClient_publishMessage(client_p, "mateq84/data/gticmd_nopower", &pubmsg, &token);
#else
    if (bsoc_gti() > MIN_BAT_KW) {
#ifdef DEBUG_HA_CMD
        log_time(true);
        fprintf(fout, "HA GTI power command %s, SDEV %5.2f\r\n", msg, get_batc_dev());
        spam = true;
#endif
        MQTTClient_publishMessage(client_p, topic_p, &pubmsg, &token); // run power commands
    } else {
        ret = false;
        pubmsg.payload = "Z#";
        pubmsg.payloadlen = strlen("Z#");
        if (!spam) {
            MQTTClient_publishMessage(client_p, "mateq84/data/gticmd_spam", &pubmsg, &token);
        } else {
            MQTTClient_publishMessage(client_p, topic_p, &pubmsg, &token); // only shutdown GTI power
        }
#ifdef DEBUG_HA_CMD
        if (spam) {
            log_time(true);
            fprintf(fout, "HA GTI power set to zero\r\n");
            spam = false;
        }
#endif
    }
#endif
    // a busy, wait loop for the async delivery thread to complete
    {
        uint32_t waiting = 0;
        while (ha_flag_vars_ss.deliveredtoken != token) {
            usleep(TOKEN_DELAY);
            if (waiting++ > MQTT_TIMEOUT) {
                fprintf(fout, "\r\nGTI Still Waiting, timeout\r\n");
                break;
            }
        };
    }
    usleep(HA_SW_DELAY);
    return ret;
}

