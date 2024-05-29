#include "mqtt_rec.h"
#include "bsoc.h"

const char* mqtt_name[V_DLAST] = {
    "pccmode",
    "batenergykw",
    "runtime",
    "bamps",
    "bvolts",
    "load",
    "solar",
    "acenergy",
    "benergy",
    "pwatts",
    "pamps",
    "pvolts",
    "flast",
    "HAdcsw",
    "HAacsw",
    "HAshut",
    "HAmode",
    "HAcon0",
    "HAcon1",
    "HAcon2",
    "HAcon3",
    "HAcon4",
    "HAcon5",
    "HAcon6",
    "HAcon7",
    "DLv_pv",
    "DLp_pv",
    "DLp_bat",
    "DLv_bat",
    "DLc_mppt",
    "DLp_mppt",
    "DLgti",
};

/*
 * data received on topic from the broker, run processing thread
 */
int32_t msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    int32_t i, ret = 1;
    char* payloadptr;
    char buffer[1024];
    struct ha_flag_type *ha_flag = context;

    E.link.mqtt_count++;
    // bug-out if no context variables passed to callback
    if (context == NULL) {
        ret = -1;
        goto null_exit;
    }

#ifdef DEBUG_REC
    fprintf(fout, "Message arrived\n");
#endif
    payloadptr = message->payload;
    for (i = 0; i < message->payloadlen; i++) {
        buffer[i] = *payloadptr++;
    }
    buffer[i] = 0; // make C string

    // parse the JSON data
    cJSON *json = cJSON_ParseWithLength(buffer, message->payloadlen);
    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(fout, "Error: %s\n", error_ptr);
        }
        ret = -1;
        ha_flag->rec_ok = false;
        E.fm80 = false;
        E.dumpload = false;
        E.link.mqtt_error++;
        goto error_exit;
    }

    if (ha_flag->ha_id == FM80_ID) {
#ifdef DEBUG_REC
        fprintf(fout, "FM80 MQTT data\r\n");
#endif
        cJSON *data_result = json;

        for (uint32_t i = V_FCCM; i < V_FLAST; i++) {
            if (json_get_data(json, mqtt_name[i], data_result, i)) {
                ha_flag->var_update++;
            }
        }
        E.fm80 = true;
    }

    if (ha_flag->ha_id == DUMPLOAD_ID) {
#ifdef DEBUG_REC
        fprintf(fout, "DUMPLOAD MQTT data\r\n");
#endif
        cJSON *data_result = json;

        for (uint32_t i = V_HDCSW; i < V_DLAST; i++) {
            if (json_get_data(json, mqtt_name[i], data_result, i)) {
                ha_flag->var_update++;
            }
        }
        E.dumpload = true;
    }

    ha_flag->receivedtoken = true;
    ha_flag->rec_ok = true;
error_exit:
    // delete the JSON object
    cJSON_Delete(json);
null_exit:
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return ret;
}

/*
 * set the broker has message token
 */
void delivered(void *context, MQTTClient_deliveryToken dt) {
    struct ha_flag_type *ha_flag = context;

    // bug-out if no context variables passed to callback
    if (context == NULL) {
        return;
    }
    ha_flag->deliveredtoken = dt;
}

/*
 * lock and load mqtt thread received data into buffered program data array
 */
bool json_get_data(cJSON *json_src, const char * data_id, cJSON *name, uint32_t i) {
    bool ret = false;
    static uint32_t j = 0;

    // access the JSON data
    name = cJSON_GetObjectItemCaseSensitive(json_src, data_id);
    if (cJSON_IsString(name) && (name->valuestring != NULL)) {
#ifdef GET_DEBUG
        fprintf(fout, "%s Name: %s\n", data_id, name->valuestring);
#endif
        ret = true;
    }
    if (cJSON_IsNumber(name)) {
#ifdef GET_DEBUG
        fprintf(fout, "%s Value: %f\n", data_id, name->valuedouble);
#endif
        if (i > V_DLAST) {
            i = V_DLAST;
        }
        pthread_mutex_lock(&E.ha_lock);
        E.mvar[i] = name->valuedouble;
        pthread_mutex_unlock(&E.ha_lock);

        /*
         * special processing for variables
         */
        if (i == V_DCMPPT) {
            /*
             * load battery current standard deviation array bat_c_std_dev with data
             */
            bsoc_set_std_dev(E.mvar[i], j++);
            if (j >= RDEV_SIZE) {
                j = 0;
            }
        }
        /*
         * update local sw status from HA
         */
        if (i == V_HDCSW) {
            if (E.gti_sw_status != (bool) ((int32_t) E.mvar[i])) {
                E.gti_sw_status = (bool) ((int32_t) E.mvar[i]);
            }
            E.dc_mismatch = false;
        }

        if (i == V_HACSW) {
            if (E.ac_sw_status != (bool) ((int32_t) E.mvar[i])) {
                E.ac_sw_status = (bool) ((int32_t) E.mvar[i]);
            }
            E.ac_mismatch = false;
        }

        if (i == V_HSHUT) {
            E.solar_shutdown = (bool) ((int32_t) E.mvar[i]);
        }
        if (i == V_HMODE) {
            ha_flag_vars_ss.energy_mode = (bool) ((int32_t) E.mvar[i]);
        }
        if (i == V_HCON0) {
            E.mode.con0 = (bool) ((int32_t) E.mvar[i]);
        }
        if (i == V_HCON1) {
            E.mode.con1 = (bool) ((int32_t) E.mvar[i]);
        }
        if (i == V_HCON2) {
            E.mode.con2 = (bool) ((int32_t) E.mvar[i]);
        }
        if (i == V_HCON3) {
            E.mode.con3 = (bool) ((int32_t) E.mvar[i]);
        }
        if (i == V_HCON4) {
            E.mode.con4 = (bool) ((int32_t) E.mvar[i]);
        }
        if (i == V_HCON5) {
            E.mode.con5 = (bool) ((int32_t) E.mvar[i]);
        }
        if (i == V_HCON6) { // HA Energy program idle
            E.mode.con6 = (bool) ((int32_t) E.mvar[i]);
        }
        if (i == V_HCON7) { // HA Energy program exit
            E.mode.con7 = (bool) ((int32_t) E.mvar[i]);
        }
        ret = true;
    }
    return ret;
}

/*
 * logging
 */
void print_mvar_vars(void) {
    fprintf(fout, ", AC Inverter %7.2fW, BAT Energy %7.2fWh, Solar E %7.2fWh, AC E %7.2fWh, PERR %7.2fW, PBAL %7.2fW, ST %7.2f, GDL %7.2f, %d,%d,%d %d,%d,%d\r",
            E.mvar[V_FLO], E.mvar[V_FBEKW], E.mvar[V_FSO], E.mvar[V_FACE], E.mode.error, E.mvar[V_BEN], E.mode.total_system, E.mode.gti_dumpload, (int32_t) E.mvar[V_HDCSW], (int32_t) E.mvar[V_HACSW], (int32_t) E.mvar[V_HMODE],
            (int32_t) E.dc_mismatch, (int32_t) E.ac_mismatch, (int32_t) E.mode_mismatch);
}

/*
 * return float status of FM80 to trigger dumploads
 */
bool fm80_float(const bool set_bias) {
    if ((uint32_t) E.mvar[V_FCCM] == FLOAT_CODE) {
        if (set_bias) {
            E.mode.pv_bias = PV_BIAS_FLOAT;
        }
        if (E.mode.R != R_IDLE) {
            E.mode.R = R_FLOAT;
        }
        return true;
    } else {
        if (E.mode.R == R_FLOAT) {
            E.mode.R = R_RUN;
        }
    }
    return false;
}

/*
 * return sleep status of FM80 to trigger dumploads
 */
bool fm80_sleep(void) {
    if ((uint32_t) E.mvar[V_FCCM] == SLEEP_CODE) {
        return true;
    }
    return false;
}