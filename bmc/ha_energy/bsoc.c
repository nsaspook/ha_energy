/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/cFiles/file.c to edit this template
 */
#include "bsoc.h"
#include "mqtt_rec.h"

static double ac_weight = 0.0f, gti_weight = 0.0f;

bool bsoc_init(void) {
    ac_weight = 0.0f;
    gti_weight = 0.0f;
    if (pthread_mutex_init(&ha_lock, NULL) != 0) {
        printf("\n mutex init has failed\n");
        return false;
    }
    return true;
};

bool bsoc_data_collect(void) {
    bool ret = false;
    pthread_mutex_lock(&ha_lock); // lockout MQTT var updates

    ac_weight = mvar[V_FBEKW];
    gti_weight = mvar[V_FBEKW];

    pthread_mutex_unlock(&ha_lock); // resume MQTT var updates
#ifdef BSOC_DEBUG
    printf("\r\nmqtt var bsoc update\r\n");
#endif
    return ret;
}

double bsoc_ac(void) {

    return ac_weight;
};

double bsoc_gti(void) {

    return gti_weight;
};