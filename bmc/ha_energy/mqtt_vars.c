#define _DEFAULT_SOURCE
#include "mqtt_rec.h"
#include "energy.h"
#include "bsoc.h"

/*
 * turn on and off the Matter switches controlled by Home Assistant
 */
void mqtt_ha_switch(MQTTClient client_p, const char * topic_p, bool sw_state) {
    cJSON *json;

    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    ha_flag_vars_ss.deliveredtoken = 0;

    json = cJSON_CreateObject();
    if (sw_state) {
        cJSON_AddStringToObject(json, "state", "ON");
    } else {
        cJSON_AddStringToObject(json, "state", "OFF");
    }
    // convert the cJSON object to a JSON string
    char *json_str = cJSON_Print(json);

    pubmsg.payload = json_str;
    pubmsg.payloadlen = strlen(json_str);
    pubmsg.qos = 1;
    pubmsg.retained = 0;

#ifdef DEBUG_HA_CMD
    printf("HA switch command\r\n");
#endif    

    MQTTClient_publishMessage(client_p, topic_p, &pubmsg, &token);
    // a busy, wait loop for the async delivery thread to complete
    {
        uint32_t waiting = 0;
        while (ha_flag_vars_ss.deliveredtoken != token) {
            usleep(TOKEN_DELAY);
            if (waiting++ > MQTT_TIMEOUT) {
                printf("\r\nSW Still Waiting, timeout\r\n");
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
void mqtt_gti_power(MQTTClient client_p, const char * topic_p, char * msg) {

    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    ha_flag_vars_ss.deliveredtoken = 0;

    pubmsg.payload = msg;
    pubmsg.payloadlen = strlen(msg);
    pubmsg.qos = 1;
    pubmsg.retained = 0;

#ifdef GTI_NO_POWER
    MQTTClient_publishMessage(client_p, "mateq84/data/gticmd_testing", &pubmsg, &token);
#else
    if (bsoc_gti() > 4500.0f) {
#ifdef DEBUG_HA_CMD
        printf("HA GTI power command %s\r\n", msg);
#endif
        MQTTClient_publishMessage(client_p, topic_p, &pubmsg, &token); // run power commands
    } else {
#ifdef DEBUG_HA_CMD
        printf("HA GTI power set to zero\r\n");
#endif
        pubmsg.payload = "Z#";
        pubmsg.payloadlen = strlen("Z#");
        MQTTClient_publishMessage(client_p, topic_p, &pubmsg, &token); // only shutdown GTI power
    }
#endif
    // a busy, wait loop for the async delivery thread to complete
    {
        uint32_t waiting = 0;
        while (ha_flag_vars_ss.deliveredtoken != token) {
            usleep(TOKEN_DELAY);
            if (waiting++ > MQTT_TIMEOUT) {
                printf("\r\nGTI Still Waiting, timeout\r\n");
                break;
            }
        };
    }
    usleep(HA_SW_DELAY);
}