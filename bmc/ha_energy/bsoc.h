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

    //#define BSOC_DEBUG

#define MIN_PV_VOLTS    5.0f
#define MIN_BAT_KW      4100.0f

#define DEV_SIZE        10
#define MAX_BATC_DEV    0.6f
#define BAT_C_DRAW      2.0f

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

    extern double bat_c_std_dev[DEV_SIZE];

    bool bsoc_init(void);
    bool bsoc_data_collect(void);
    double bsoc_ac(void);
    double bsoc_gti(void);
    double gti_test(void);
    double ac_test(void);
    double get_batc_dev(void);
    bool bat_current_stable(void);

    double calculateStandardDeviation(uint32_t, double *);

#ifdef __cplusplus
}
#endif

#endif /* BSOC_H */

