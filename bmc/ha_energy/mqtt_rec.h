/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/cFiles/file.h to edit this template
 */

/* 
 * File:   mqtt_rec.h
 * Author: root
 *
 * Created on February 5, 2024, 2:54 PM
 */

#ifndef MQTT_REC_H
#define MQTT_REC_H

#ifdef __cplusplus
extern "C" {
#endif
#include "energy.h"
#include "mqtt_vars.h"

    enum mqtt_id {
        P8055_ID,
        FM80_ID,
        DUMPLOAD_ID,
        LAST_MQTT_ID,
    };

    struct ha_flag_type {
        volatile MQTTClient_deliveryToken deliveredtoken, receivedtoken;
        volatile bool runner, rec_ok;
        int32_t ha_id; 
        volatile int32_t var_update, energy_mode;
    };

    int32_t msgarrvd(void *, char *, int, MQTTClient_message *);
    void delivered(void *, MQTTClient_deliveryToken);
    
    bool json_get_data(cJSON *, const char *, cJSON *);

#ifdef __cplusplus
}
#endif

#endif /* MQTT_REC_H */

