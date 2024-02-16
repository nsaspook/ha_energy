/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/cFiles/file.c to edit this template
 */
#include "http_vars.h"

CURL *curl;
CURLcode res;
volatile double im_vars[PHASE_LAST][IA_LAST];

size_t iammeter_write_callback(char *buffer, size_t size, size_t nitems, void *stream) {
    cJSON *json = cJSON_ParseWithLength(buffer, strlen(buffer));

    if (json == NULL) {
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            fprintf(fout, "Error: %s\n", error_ptr);
        }
        goto iammeter_exit;
    }
#ifdef IM_DEBUG
    fprintf(fout, "\n iammeter_read_callback %s \n", buffer);
#endif

    cJSON *data_result = json;
    data_result = cJSON_GetObjectItemCaseSensitive(json, "Datas");

    if (!data_result) {
        goto iammeter_exit;
    }

    cJSON *jname;
    uint32_t phase = 0;

    cJSON_ArrayForEach(jname, data_result) {
        cJSON *ianame;
#ifdef IM_DEBUG
        fprintf(fout, "\n iammeter variables ");
#endif

        cJSON_ArrayForEach(ianame, jname) {
            uint32_t phase_var = 0;
            im_vars[phase][phase_var] = ianame->valuedouble;
#ifdef IM_DEBUG
            fprintf(fout, "%10.2f ", im_vars[phase][phase_var]);
#endif
            phase_var++;
        }
        phase++;
    }
#ifdef IM_DEBUG
    fprintf(fout, "\n");
#endif

iammeter_exit:
    cJSON_Delete(json);
    return size * nitems;
}

void iammeter_read(void) {

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "http://10.1.1.101/monitorjson");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, iammeter_write_callback);

        res = curl_easy_perform(curl);
        /* Check for errors */
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));
            iammeter = false;
        } else {
            iammeter = true;
        }
        curl_easy_cleanup(curl);
    }
}