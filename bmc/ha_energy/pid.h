/* 
 * File:   pid.h
 * Author: root
 *
 * Created on March 6, 2024, 7:03 AM
 */

#ifndef PID_H
#define PID_H

#ifdef __cplusplus
extern "C" {
#endif

    struct SPid {
        double dState; // Last position input
        double iState; // Integrator state
        double iMax, iMin; // Maximum and minimum allowable integrator state
        double iGain, // integral gain
        pGain, // proportional gain
        dGain; // derivative gain
    };

    double UpdatePI(volatile struct SPid * const, const double);
    void ResetPI(volatile struct SPid * const);


#ifdef __cplusplus
}
#endif

#endif /* PID_H */

