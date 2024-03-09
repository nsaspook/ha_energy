#include "pid.h"

/*
 * converted to PI for solar power control
 */
double UpdatePI(volatile struct SPid * const pid, double const error) {
    double pTerm, iTerm;

    pTerm = pid->pGain * error; // calculate the proportional term
    // calculate the integral state with appropriate limiting
    pid->iState += error;

    if (pid->iState > pid->iMax) {
        pid->iState = pid->iMax;
    } else if (pid->iState < pid->iMin) {
        pid->iState = pid->iMin;
    } else {

    }

    iTerm = (pid->iGain * pid->iState); // calculate the integral term

    if ((pTerm + iTerm) > pid->iMax) {
        iTerm = 0.0f;
        pTerm = pid->iMax;
    }
    return pTerm + iTerm;
}

void ResetPI(volatile struct SPid * const pid) {
    pid->dState = 0.0; // not used but cleared
    pid->iState = 0.0;
}