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

#define BSOC_DEBUG
    
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

    bool bsoc_init(void);
    bool bsoc_data_collect(void);
    double bsoc_ac(void);
    double bsoc_gti(void);

#ifdef __cplusplus
}
#endif

#endif /* BSOC_H */

