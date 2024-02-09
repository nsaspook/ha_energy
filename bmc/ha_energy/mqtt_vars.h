/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/cFiles/file.h to edit this template
 */

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

    enum mqtt_vars {
        V_FCCM,
        V_FBEKW,
        V_FRUNT,
        V_FBV,
        V_FLO,
        V_FSO,
        V_FLAST,
        V_DVPV,
        V_DPPV,
        V_DPBAT,
        V_DVBAT,
        V_DCMPPT,
        V_DGTI,
        V_DLAST,
    };

    extern double mvar[V_DLAST];

    extern const char* mqtt_name[V_DLAST];

#ifdef __cplusplus
}
#endif

#endif /* MQTT_VARS_H */

