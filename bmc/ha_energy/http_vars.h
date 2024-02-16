/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/cFiles/file.h to edit this template
 */

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

    extern CURL *curl;
    extern CURLcode res;
    extern volatile double im_vars[PHASE_LAST][IA_LAST];
    extern volatile bool iammeter, fm80, dumpload;


#ifdef __cplusplus
}
#endif

#endif /* HTTP_VARS_H */

