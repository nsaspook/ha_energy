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

#define MAX_IM_VAR  48

    extern CURL *curl;
    extern CURLcode res;
    extern volatile double im_vars[IA_LAST][PHASE_LAST];
    extern volatile bool iammeter, fm80, dumpload;
    extern FILE* fout;

#ifdef __cplusplus
}
#endif

#endif /* HTTP_VARS_H */

