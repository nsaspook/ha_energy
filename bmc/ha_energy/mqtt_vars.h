/*
 * File:   mqtt_vars.h
 * Author: root
 *
 * Created on February 9, 2024, 6:50 AM
 */

#ifndef MQTT_VARS_H
#define MQTT_VARS_H

#ifdef __cplusplus
extern "C" {
#endif

	//#define GTI_NO_POWER      // do we actually run power commands

	//#define DEBUG_HA_CMD    // show debug text
#define HA_SW_DELAY     400000  // usecs
#define TOKEN_DELAY     500
#define GTI_TOKEN_DELAY 600

//#define QOS             2

	extern const char* mqtt_name[V_DLAST];

	void mqtt_ha_switch(MQTTClient, const char *, const bool);
	void mqtt_ha_pid(MQTTClient, const char *);
	void mqtt_ha_shutdown(MQTTClient, const char *);
	bool mqtt_gti_power(MQTTClient, const char *, char *, uint32_t);
	bool mqtt_gti_time(MQTTClient, const char *, char *);

#ifdef __cplusplus
}
#endif

#endif /* MQTT_VARS_H */

