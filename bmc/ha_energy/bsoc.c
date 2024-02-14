#include "bsoc.h"
#include "mqtt_rec.h"

static volatile double ac_weight = 0.0f, gti_weight = 0.0f, pv_voltage = 0.0f, bat_current = 0.0f, batc_std_dev = 0.0f;
double bat_c[DEV_SIZE];

static void calculateStandardDeviation(uint32_t, double *);

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
    static uint32_t i = 0;
    pthread_mutex_lock(&ha_lock); // lockout MQTT var updates

    ac_weight = mvar[V_FBEKW];
    gti_weight = mvar[V_FBEKW];
    pv_voltage = mvar[V_DVPV];
    bat_current = mvar[V_DCMPPT];

    pthread_mutex_unlock(&ha_lock); // resume MQTT var updates

    bat_c[i++] = bat_current;
    if (i >= DEV_SIZE) {
        i = 0;
    }

    calculateStandardDeviation(DEV_SIZE, bat_c);

#ifdef BSOC_DEBUG
    fprintf(fout,"\r\nmqtt var bsoc update\r\n");
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
#ifdef BSOC_DEBUG
    fprintf(fout,"pvp %f, gweight %f, aweight %f, batc %f\r\n", pv_voltage, gti_weight, ac_weight, bat_current);
#endif
    // check for 48VDC AC charger powered from the Solar battery bank AC inverter
    if (pv_voltage < MIN_PV_VOLTS) {
        gti_weight = 0.0f; // reduce power to zero
    }
    return gti_weight;
};

/*
 * this is a dupe function of bsoc_gti for testing
 */
double gti_test(void) {
    // check for 48VDC AC charger powered from the Solar battery bank AC inverter
    if (pv_voltage < MIN_PV_VOLTS) {
        gti_weight = 0.0f; // reduce power to zero
#ifdef BSOC_DEBUG
        fprintf(fout,"pvp %f, gweight %f, aweight %f, batc %f\r\n", pv_voltage, gti_weight, ac_weight, bat_current);
#endif
    }
    return gti_weight;
}

double ac_test(void) {
    return ac_weight;
}

/*
 * used to wait until GTI power ramps are stable
 * batc_std_dev needs to be under some standard value to continue power ramps
 */
void calculateStandardDeviation(uint32_t N, double data[]) {
    // variable to store sum of the given data
    double sum = 0;

    for (int i = 0; i < N; i++) {
        sum += data[i];
    }

    // calculating mean
    double mean = sum / N;

    // temporary variable to store the summation of square
    // of difference between individual data items and mean
    double values = 0;

    for (int i = 0; i < N; i++) {
        values += pow(data[i] - mean, 2);
    }

    // variance is the square of standard deviation
    double variance = values / N;

    // calculating standard deviation by finding square root
    // of variance
    double standardDeviation = sqrt(variance);
    batc_std_dev = standardDeviation;

#ifdef BSOC_DEBUG
    // printing standard deviation
    fprintf(fout,"STD DEV of Current %.2f\r\n", standardDeviation);
#endif
}

bool bat_current_stable(void) {
    static double gap = 0.0f;

    if (batc_std_dev <= (MAX_BATC_DEV + gap)) {
        gap = MAX_BATC_DEV;
        return true;
    } else {
        gap = 0.0f;
        return false;
    }
}