#include "bsoc.h"
#include "mqtt_rec.h"

static volatile double ac_weight = 0.0f, gti_weight = 0.0f, pv_voltage = 0.0f;

bool bsoc_init(void) {
    ac_weight = 0.0f;
    gti_weight = 0.0f;
    if (pthread_mutex_init(&ha_lock, NULL) != 0) {
        printf("\n mutex init has failed\n");
        return false;
    }
    return true;
};

/*
 * move the data into work variables from the receive threads array
 */
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

/*
 * energy logic for AC inverter diversion logic
 */
double bsoc_ac(void) {

    return ac_weight;
};

/*
 * energy logic for GTI power diversion
 */
double bsoc_gti(void) {
    printf("pvp %f, gweight %f, aweight %f\r\n", pv_voltage, gti_weight, ac_weight);
    // check for 48VDC AC charger powered from the Solar battery bank AC inverter
    if (pv_voltage < MIN_PV_VOLTS) {
        gti_weight = 0.0f; // reduce power to zero
    }
    return gti_weight;
};

double gti_test(void) {
    return gti_weight;
}