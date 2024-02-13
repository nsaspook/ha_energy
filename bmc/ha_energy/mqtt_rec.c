#include "mqtt_rec.h"
#include "bsoc.h"

volatile double mvar[V_DLAST];
const char* mqtt_name[V_DLAST] = {
    "pccmode",
    "batenergykw",
    "runtime",
    "bvolts",
    "load",
    "solar",
    "flast",
    "DLv_pv",
    "DLp_pv",
    "DLp_bat",
    "DLv_bat",
    "DLc_mppt",
    "DLgti",
};

pthread_mutex_t ha_lock;

/*
 * data received on topic from the broker
 */
int32_t msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message) {
    int32_t i, ret = 1;
    char* payloadptr;
    char buffer[1024];
    struct ha_flag_type *ha_flag = context;

    // bug-out if no context variables passed to callback
    if (context == NULL) {
        ret = -1;
        ha_flag->rec_ok = false;
        goto null_exit;
    }

#ifdef DEBUG_REC
    printf("Message arrived\n");
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
            printf("Error: %s\n", error_ptr);
        }
        ret = -1;
        ha_flag->rec_ok = false;
        goto error_exit;
        return ret;
    }

    if (ha_flag->ha_id == FM80_ID) {
#ifdef DEBUG_REC
        printf("FM80 MQTT data\r\n");
#endif
        cJSON *data_result = json;

        for (uint32_t i = V_FCCM; i < V_FLAST; i++) {
            if (json_get_data(json, mqtt_name[i], data_result, i)) {
                ha_flag->var_update++;
            }
        }
    }

    if (ha_flag->ha_id == DUMPLOAD_ID) {
#ifdef DEBUG_REC
        printf("DUMPLOAD MQTT data\r\n");
#endif
        cJSON *data_result = json;

        for (uint32_t i = V_DVPV; i < V_DLAST; i++) {
            if (json_get_data(json, mqtt_name[i], data_result, i)) {
                ha_flag->var_update++;
            }
        }
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

bool json_get_data(cJSON *json_src, const char * data_id, cJSON *name, uint32_t i) {
    bool ret = false;
    static uint32_t j = 0;

    // access the JSON data
    name = cJSON_GetObjectItemCaseSensitive(json_src, data_id);
    if (cJSON_IsString(name) && (name->valuestring != NULL)) {
#ifdef GET_DEBUG
        printf("%s Name: %s\n", data_id, name->valuestring);
#endif
        ret = true;
    }
    if (cJSON_IsNumber(name)) {
#ifdef GET_DEBUG
        printf("%s Value: %f\n", data_id, name->valuedouble);
#endif
        if (i > V_DLAST) {
            i = V_DLAST;
        }
        pthread_mutex_lock(&ha_lock);
        mvar[i] = name->valuedouble;
        pthread_mutex_unlock(&ha_lock);

        if (i == V_DCMPPT) {
            bat_c[j++] = mvar[i];
            if (j >= RDEV_SIZE) {
                j = 0;
            }
        }
        ret = true;
    }
    return ret;
}