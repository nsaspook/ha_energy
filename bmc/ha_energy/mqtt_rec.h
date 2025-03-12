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

#define RDEV_SIZE        10

#define SLEEP_CODE      0
#define FLOAT_CODE      1
	//#define DEBUG_REC
	//#define GET_DEBUG

#define MBMQTT  1024

	enum mqtt_id {
		P8055_ID,
		FM80_ID,
		DUMPLOAD_ID,
		HA_ID,
		LAST_MQTT_ID,
	};

	struct ha_flag_type {
		volatile MQTTClient_deliveryToken deliveredtoken, receivedtoken;
		volatile bool runner, rec_ok;
		int32_t ha_id;
		volatile int32_t var_update, energy_mode;
		volatile enum client_id cid;
	};

	extern FILE* fout;

	int32_t msgarrvd(void *, char *, int, MQTTClient_message *);
	void delivered(void *, MQTTClient_deliveryToken);

	bool json_get_data(cJSON *, const char *, cJSON *, uint32_t);
	bool fm80_float(const bool set_bias);
	bool fm80_sleep(void);

#ifdef __cplusplus
}
#endif

#endif /* MQTT_REC_H */

