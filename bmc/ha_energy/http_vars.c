#include "http_vars.h"

static CURL *curl;
static CURLcode res;
static void iammeter_get_data(double, uint32_t, uint32_t);

/*
 * read and format data returned from libcurl http WRITEDATA function call
 */
size_t iammeter_write_callback(char *buffer, size_t size, size_t nitems, void *stream) {
    cJSON *json = cJSON_ParseWithLength(buffer, strlen(buffer));
    struct energy_type * e = stream;
    uint32_t next_var = 0;

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

    cJSON *data_result = cJSON_GetObjectItemCaseSensitive(json, "Datas");

    if (!data_result) {
        size = 0;
        nitems = 0;
        goto iammeter_exit;
    }

    cJSON *jname;
    uint32_t phase = PHASE_A;

    cJSON_ArrayForEach(jname, data_result) {
        cJSON *ianame;
#ifdef IM_DEBUG
        fprintf(fout, "\n iammeter variables ");
#endif

        cJSON_ArrayForEach(ianame, jname) {
            uint32_t phase_var = IA_VOLTAGE;
            iammeter_get_data(ianame->valuedouble, phase_var, phase);
            e->print_vars[next_var++] = ianame->valuedouble;
#ifdef IM_DEBUG
            fprintf(fout, "%8.2f ", im_vars[phase_var][phase]);
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

/*
 * use the standard IAMMETER HTTP API for AC line power status
 */
void iammeter_read(void) {

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "http://10.1.1.101/monitorjson");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, iammeter_write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, E.print_vars); // external data array for iammeter values

        res = curl_easy_perform(curl);
        /* Check for errors */
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n",
                    curl_easy_strerror(res));
            E.iammeter = false;
        } else {
            E.iammeter = true;
        }
        curl_easy_cleanup(curl);
    }
}

/*
 * move data into global state structure
 */
static void iammeter_get_data(double valuedouble, uint32_t i, uint32_t j) {
    E.im_vars[i][j] = valuedouble;
}

/*
 * logging
 */
void print_im_vars(void) {
    fprintf(fout, "House L1 %7.2fW, House L2 %7.2fW, GTI L1 %7.2fW", E.print_vars[L1_P], E.print_vars[L2_P], E.print_vars[L3_P]);
}