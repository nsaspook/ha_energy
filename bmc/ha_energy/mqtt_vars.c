/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/cFiles/file.c to edit this template
 */
#define _DEFAULT_SOURCE
#include "mqtt_rec.h"
#include "energy.h"

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

    MQTTClient_publishMessage(client_p, topic_p, &pubmsg, &token);
    // a busy, wait loop for the async delivery thread to complete
    {
        uint32_t waiting = 0;
        while (ha_flag_vars_ss.deliveredtoken != token) {
            usleep(250);
            if (waiting++ > MQTT_TIMEOUT) {
                printf("\r\nSW Still Waiting, timeout\r\n");
                break;
            }
        };
    }

    cJSON_free(json_str);
    cJSON_Delete(json);
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

    MQTTClient_publishMessage(client_p, topic_p, &pubmsg, &token);
    // a busy, wait loop for the async delivery thread to complete
    {
        uint32_t waiting = 0;
        while (ha_flag_vars_ss.deliveredtoken != token) {
            usleep(250);
            if (waiting++ > MQTT_TIMEOUT) {
                printf("\r\nGTI Still Waiting, timeout\r\n");
                break;
            }
        };
    }
}