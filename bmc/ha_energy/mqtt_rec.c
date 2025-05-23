#include "bsoc.h"

/** \file mqtt_rec.c
 * data received on topic from the broker, run processing thread
 * max message length set by MBMQTT
 */
int32_t msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
	int32_t i, ret = 1;
	const char* payloadptr;
	char buffer[MBMQTT];
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
	/*
	 * move the received message into a processing holding buffer
	 */
	payloadptr = message->payload;
	for (i = 0; i < message->payloadlen; i++) {
		buffer[i] = *payloadptr++;
	}
	buffer[i] = 0; // make a null terminated C string

	// parse the JSON data in the holding buffer
	cJSON *json = cJSON_ParseWithLength(buffer, message->payloadlen);
	if (json == NULL) {
		const char *error_ptr = cJSON_GetErrorPtr();
		if (error_ptr != NULL) {
			fprintf(fout, "%s Error: %s NULL cJSON pointer %s\n", log_time(false), error_ptr, topicName);
		}
		ret = -1;
		ha_flag->rec_ok = false;
		E.fm80 = false;
		E.dumpload = false;
		E.homeassistant = false;
		E.link.mqtt_error++;
		goto error_exit;
	}

	/*
	 * MQTT messages from the FM80 Q84 interface
	 */
	if (ha_flag->ha_id == FM80_ID) {
#ifdef DEBUG_REC
		fprintf(fout, "FM80 MQTT data\r\n");
#endif
		cJSON *data_result = json;

		for (uint32_t ii = V_FCCM; ii < V_FLAST; ii++) {
			if (json_get_data(json, mqtt_name[ii], data_result, ii)) {
				ha_flag->var_update++;
			}
		}
		E.fm80 = true;
	}

	/*
	 * MQTT messages from the K42 dumpload/gti interface
	 */
	if (ha_flag->ha_id == DUMPLOAD_ID) {
#ifdef DEBUG_REC
		fprintf(fout, "DUMPLOAD MQTT data\r\n");
#endif
		cJSON *data_result = json;

		for (uint32_t ii = V_HDCSW; ii < V_DLAST; ii++) {
			if (json_get_data(json, mqtt_name[ii], data_result, ii)) {
				ha_flag->var_update++;
			}
		}
		E.dumpload = true;
	}

	/*
	 * MQTT messages from the Linux HA_ENERGY interface
	 */
	if (ha_flag->ha_id == HA_ID) {
#ifdef DEBUG_REC
		fprintf(fout, "Home Assistant MQTT data\r\n");
#endif
		cJSON *data_result = json;

		if (json_get_data(json, mqtt_name[V_HACSW], data_result, V_HACSW)) {
			ha_flag->var_update++;
		}
		data_result = json;
		if (json_get_data(json, mqtt_name[V_HDCSW], data_result, V_HDCSW)) {
			ha_flag->var_update++;
		}

		E.homeassistant = true;
	}

	// done with processing MQTT async message, set state flags
	ha_flag->receivedtoken = true;
	ha_flag->rec_ok = true;
	/*
	 * exit and delete/free resources. In steps depending of possible error conditions
	 */
error_exit:
	// delete the JSON object
	cJSON_Delete(json);
null_exit:
	// free the MQTT objects
	MQTTClient_freeMessage(&message);
	MQTTClient_free(topicName);
	return ret;
}

/*
 * set the broker has message token
 */
void delivered(void *context, MQTTClient_deliveryToken dt)
{
	struct ha_flag_type *ha_flag = context;

	// bug-out if no context variables passed to callback
	if (context == NULL) {
		return;
	}
	ha_flag->deliveredtoken = dt;
}

/*
 * lock and load MQTT thread received data into buffered program data array
 * for main program thread processing using a mutex_lock
 */
bool json_get_data(cJSON *json_src, const char * data_id, cJSON *name, uint32_t i)
{
	bool ret = false;
	static uint32_t j = 0;

	// access the JSON data using the lookup string passed in data_id
	name = cJSON_GetObjectItemCaseSensitive(json_src, data_id);

	/*
	 * process string values
	 */
	if (cJSON_IsString(name) && (name->valuestring != NULL)) {
#ifdef GET_DEBUG
		fprintf(fout, "%s Name: %s\n", data_id, name->valuestring);
#endif
		ret = true;
	}

	/*
	 * process numeric values
	 */
	if (cJSON_IsNumber(name)) {
#ifdef GET_DEBUG
		fprintf(fout, "%s Value: %f\n", data_id, name->valuedouble);
#endif
		if (i > V_DLAST) { // check for out-of-range index
			i = V_DLAST;
		}

		// lock the main value array during updates
		pthread_mutex_lock(&E.ha_lock);
		E.mvar[i] = name->valuedouble;
		pthread_mutex_unlock(&E.ha_lock);

		/*
		 * special processing for variable data received
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
		 * update local MATTER switch status from HA
		 */
		if (i == V_HDCSW) {
			E.gti_sw_status = (bool) ((int32_t) E.mvar[i]);
			E.dc_mismatch = false;
		}

		if (i == V_HACSW) {
			E.ac_sw_status = (bool) ((int32_t) E.mvar[i]);
			E.ac_mismatch = false;
		}

		// command HA_ENERGY to shutdown mode
		if (i == V_HSHUT) {
			E.solar_shutdown = (bool) ((int32_t) E.mvar[i]);
		}
		// set HA_ENERGY energy processing mode
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
		if (i == V_HCON4) { // set DL GTI excess load MODE
			E.mode.con4 = (bool) ((int32_t) E.mvar[i]);
		}
		if (i == V_HCON5) { // clear DL GTI excess load MODE
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
 * logging to system log file
 */
void print_mvar_vars(void)
{
	fprintf(fout, ", Inv P %6.2fW, BAT E %6.2fWh, Solar E %7.2fWh, AC E %7.2fWh, PERR %6.2fW, PBAL %6.2fW, ST %6.2f, GDL %6.2f, %d,%d,%d %d\r",
		E.mvar[V_FLO], E.mvar[V_FBEKW], E.mvar[V_FSO], E.mvar[V_FACE], E.mode.error, E.mvar[V_BEN], E.mode.total_system, E.mode.gti_dumpload, (int32_t) E.mvar[V_HDCSW], (int32_t) E.mvar[V_HACSW], (int32_t) E.mvar[V_HMODE],
		(int32_t) C.dl_bat_charge_zero);
}

/*
 * return float status of FM80 to trigger excess energy modes
 */
bool fm80_float(const bool set_bias)
{
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
 * return sleep status of FM80 to select solar production or 'night' modes
 * by adjusting load drive bias
 */
bool fm80_sleep(void)
{
	if ((uint32_t) E.mvar[V_FCCM] == SLEEP_CODE) {
		return true;
	}
	return false;
}