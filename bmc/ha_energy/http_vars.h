/*
 * File:   http_vars.h
 * Author: root
 *
 * Created on February 16, 2024, 8:37 AM
 */

#ifndef HTTP_VARS_H
#define HTTP_VARS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "energy.h"

#define L1_P    IA_POWER
#define L2_P    L1_P+IA_LAST
#define L3_P    L2_P+IA_LAST

    extern FILE* fout;

#ifdef __cplusplus
}
#endif

#endif /* HTTP_VARS_H */

