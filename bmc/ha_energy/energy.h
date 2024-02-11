/* 
 * File:   bmc.h
 * Author: root
 *
 * Created on September 21, 2012, 12:54 PM
 */

#ifndef BMC_H
#define BMC_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdlib.h>
#include <stdio.h> /* for printf() */
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include <cjson/cJSON.h>
#include "MQTTClient.h"

#define DAQ_STR 32
#define DAQ_STR_M DAQ_STR-1

#define MQTT_TIMEOUT    400

#define NO_CYLON

#define UNIT_TEST       0
#define NORM_MODE       1

    extern volatile struct bmcdata bmc; /* DIO buffer */
    extern struct didata datain;
    extern struct dodata dataout;

    extern int channels_ai, channels_ao, channels_di, channels_do, channels_counter;
    extern struct ha_flag_type ha_flag_vars_ss;

    void timer_callback(int32_t);

    void connlost(void *, char *);

#ifdef __cplusplus
}
#endif

#endif /* BMC_H */

