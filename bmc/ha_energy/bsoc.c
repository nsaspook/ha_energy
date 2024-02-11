#include "bsoc.h"
#include "mqtt_rec.h"

static volatile double ac_weight = 0.0f, gti_weight = 0.0f, pv_voltage=0.0f;

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
    pv_voltage = mvar[V_DVPV];

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

    printf("pvp %f, gweight %f, aweight %f\r\n",pv_voltage,gti_weight,ac_weight);
    if (pv_voltage <45.0f) {
        gti_weight=0.0f;
    }
    return gti_weight;
};