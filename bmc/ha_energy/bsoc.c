/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/cFiles/file.c to edit this template
 */
#include "bsoc.h"

static double ac_weight = 0.0f, gti_weight = 0.0f;

void bsoc_init(void) {
    ac_weight = 0.0f;
    gti_weight = 0.0f;
};

bool bsoc_data_collect(void)
{
    bool ret=false;
    
    return ret;
}

double bsoc_ac(void) {
    
    return ac_weight;
};

double bsoc_gti(void) {

    return gti_weight;
};