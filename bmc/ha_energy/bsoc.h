/* 
 * File:   bsoc.h
 * Author: root
 *
 * Created on February 10, 2024, 6:24 PM
 */

#ifndef BSOC_H
#define BSOC_H

#ifdef __cplusplus
extern "C" {
#endif
#include <math.h>
	//#define BSOC_DEBUG

#define MIN_PV_VOLTS    5.0f
#define MIN_BAT_VOLTS   23.0f
#define MIN_BAT_KW      4100.0f

#define DEV_SIZE        10
#define MAX_BATC_DEV    1.5f
#define BAT_C_DRAW      3.0f

#define PBAL_OFFSET     -50.0f // postive bias for control point
#define PV_FULL_PWR     300.0f
#define PV_MIN_PWR      160.0f
#define PV_V_NOM        60.0f
#define PV_V_FAKE       0.336699f

#define COEF            8.0f
#define COEFN           4.0f
#define COEFF           2.0f

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
#include <math.h>
#include "pid.h"
#include "mqtt_rec.h"

	bool bsoc_init(void);
	bool bsoc_data_collect(void);
	double bsoc_ac(void);
	double bsoc_gti(void);
	double gti_test(void);
	double ac_test(void);
	double get_batc_dev(void);
	bool bat_current_stable(void);
	void bsoc_set_std_dev(const double, const uint32_t);

	double calculateStandardDeviation(const uint32_t, const double *);

	bool bsoc_set_mode(const double, const bool, const bool);

	double ac0_filter(const double);
	double ac1_filter(const double);
	double ac2_filter(const double);
	double dc0_filter(const double);
	double dc1_filter(const double);
	double dc2_filter(const double);
	double drive0_filter(const double);
	double drive1_filter(const double);

#ifdef __cplusplus
}
#endif

#endif /* BSOC_H */

