#include "bsoc.h"
#include "mqtt_rec.h"

static volatile double ac_weight = 0.0f, gti_weight = 0.0f, pv_voltage = 0.0f, bat_current = 0.0f, batc_std_dev = 0.0f, bat_voltage = 0.0f;
static double bat_c_std_dev[DEV_SIZE], coef = COEF;

static double error_filter(double);

bool bsoc_init(void)
{
	ac_weight = 0.0f;
	gti_weight = 0.0f;
	if (pthread_mutex_init(&E.ha_lock, NULL) != 0) {
		fprintf(fout, "\n%s mutex init has failed\n", log_time(false));
		return false;
	}
	return true;
};

/*
 * set battery std dev array value
 */
void bsoc_set_std_dev(double value, uint32_t i)
{
	bat_c_std_dev[i] = value;
}

/*
 * move the data into work variables from the receive threads array
 */
bool bsoc_data_collect(void)
{
	bool ret = false;
	static uint32_t i = 0;
	pthread_mutex_lock(&E.ha_lock); // lockout MQTT var updates

	ac_weight = E.mvar[V_FBEKW];
	gti_weight = E.mvar[V_FBEKW];
#ifdef FAKE_VPV // no DUMPLOAD AC charger
	if (E.gti_sw_on) {
		pv_voltage = PV_V_NOM;
	} else {
		pv_voltage = PV_V_FAKE;
	}
	E.mvar[V_DVPV] = pv_voltage;
#else
	pv_voltage = E.mvar[V_DVPV];
#endif
	bat_voltage = E.mvar[V_DVBAT];
	bat_current = E.mvar[V_DCMPPT];
	E.ac_low_adj = E.mvar[V_FSO]* -0.5f;
	E.gti_low_adj = E.mvar[V_FACE] * -0.5f;
	E.mode.dl_mqtt_max = E.mvar[V_DPMPPT];

	pthread_mutex_unlock(&E.ha_lock); // resume MQTT var updates

	if (E.ac_low_adj < -2000.0f) {
		E.ac_low_adj = -2000.0f;
	}
	if (E.gti_low_adj < -2000.0f) {
		E.gti_low_adj = -2000.0f;
	}

	bat_c_std_dev[i++] = bat_current;
	if (i >= DEV_SIZE) {
		i = 0;
	}

	calculateStandardDeviation(DEV_SIZE, bat_c_std_dev);

#ifdef BSOC_DEBUG
	fprintf(fout, "\r\nmqtt var bsoc update\r\n");
#endif
	return ret;
}

/*
 * energy logic for AC inverter diversion logic
 */
double bsoc_ac(void)
{

	return ac0_filter(ac_weight);
};

/*
 * energy logic for GTI power diversion
 * add E.dl_excess to send power to GTI without the charger being on
 */
double bsoc_gti(void)
{
#ifdef BSOC_DEBUG
	fprintf(fout, "pvp %f, gweight %f, aweight %f, batv %f, batc %f\r\n", pv_voltage, gti_weight, ac_weight, bat_voltage, bat_current);
#endif
	// check for 48VDC AC charger powered from the Solar battery bank AC inverter unless E.dl_excess is TRUE
	if (((pv_voltage < MIN_PV_VOLTS) && (!E.dl_excess)) || (bat_voltage < MIN_BAT_VOLTS)) {
		gti_weight = 0.0f; // reduce power to zero
	} else {
		if (E.dl_excess) {
			if (E.mvar[V_DAHBAT] > PV_DL_B_AH_LOW) {
				gti_weight = PV_DL_EXCESS;
			} else {
				gti_weight = 0.0f; // reduce power to zero
			}
		}
	}


	return dc0_filter(gti_weight);
};

/*
 * this is a dupe function of bsoc_gti for testing
 */
double gti_test(void)
{
	// check for 48VDC AC charger powered from the Solar battery bank AC inverter
	if (((pv_voltage < MIN_PV_VOLTS) && (!E.dl_excess)) || (bat_voltage < MIN_BAT_VOLTS)) {
		gti_weight = 0.0f; // reduce power to zero
#ifdef BSOC_DEBUG
		fprintf(fout, "pvp %8.2f, gweight %8.2f, aweight %8.2f, batv %8.2f, batc %8.2f\r\n", pv_voltage, gti_weight, ac_weight, bat_voltage, bat_current);
#endif
	} else {
		if (E.dl_excess) {
			if (E.mvar[V_DAHBAT] > PV_DL_B_AH_LOW) {
				gti_weight = PV_DL_EXCESS;
			} else {
				gti_weight = 0.0f; // reduce power to zero
			}
		}
	}
	return dc0_filter(gti_weight);
}

double ac_test(void)
{
	return ac0_filter(ac_weight);
}

double get_batc_dev(void)
{
	return batc_std_dev;
}

/*
 * used to wait until GTI power ramps are stable
 * batc_std_dev needs to be under some standard value to continue power ramps
 */
double calculateStandardDeviation(uint32_t N, const double data[])
{
	// variable to store sum of the given data
	double sum = 0;

	for (int i = 0; i < N; i++) {
		sum += data[i];
	}

	// calculating mean
	double mean = sum / N;

	// temporary variable to store the summation of square
	// of difference between individual data items and mean
	double values = 0;

	for (int i = 0; i < N; i++) {
		values += pow(data[i] - mean, 2);
	}

	// variance is the square of standard deviation
	double variance = values / N;

	// calculating standard deviation by finding square root
	// of variance
	double standardDeviation = sqrt(variance);
	batc_std_dev = standardDeviation;

#ifdef BSOC_DEBUG
	// printing standard deviation
	fprintf(fout, "STD DEV of Current %.2f\r\n", standardDeviation);
#endif
	return standardDeviation;
}

bool bat_current_stable(void)
{
	static double gap = 0.0f;

	if (batc_std_dev <= (MAX_BATC_DEV + gap)) {
		gap = MAX_BATC_DEV;
		if (bat_c_std_dev[0] < BAT_C_DRAW) {
			return true;
		} else {
			gap = 0.0f;
			return false;
		}
	} else {
		gap = 0.0f;
		return false;
	}
}

/*
 * set mode to true to add GTI power to AC power for control power feedback
 * target is the positive power bias to keep the battery(s) charged
 */
bool bsoc_set_mode(double target, bool mode, bool init)
{
	static bool bsoc_mode = false;
	static bool bsoc_high = false, ha_ac_mode = true;
	static double accum = 0.0f, vpwa = 0.0f;

	if (init) {
		bsoc_mode = false;
		bsoc_high = false;
		ha_ac_mode = true;
		accum = 0.0f;
		vpwa = 0.0f;
		return true;
	}
	/*
	 * running avg filter
	 */
	accum = accum - accum / COEFN + E.mvar[V_PWA];
	vpwa = accum / COEFN;

	if ((vpwa >= PV_FULL_PWR) && (E.mvar[V_FBEKW] >= MIN_BAT_KW_BSOC_HI)) {
		if (!bsoc_mode) {
			ResetPI(&E.mode.pid);
		}
		bsoc_mode = true;
		bsoc_high = true;
		if (!ha_ac_mode) {
			ha_ac_on();
			ha_ac_mode = true;
		}

	} else {
		if (bsoc_high) { // turn off at min limit power
			if ((vpwa >= PV_MIN_PWR) && (E.mvar[V_FBEKW] >= MIN_BAT_KW_BSOC_HI)) {
				bsoc_mode = true;
				if (ha_ac_mode) {
					ha_ac_off();
					ha_ac_mode = false;
				}
			} else {
				bsoc_high = false;
				ha_ac_mode = false;
			}
		}
	}

	E.mode.gti_dumpload = (E.print_vars[L3_P]* -1.0f) + E.mvar[V_DPPV]; // use as a temp variable
	E.mode.total_system = (E.mvar[V_FLO] - E.mode.gti_dumpload) + E.mvar[V_DPPV] +(E.print_vars[L3_P]* -1.0f);
	E.mode.gti_dumpload = (E.print_vars[L3_P]* -1.0f) - E.mvar[V_DPPV]; // use this value

	/*
	 * look at system energy balance for power control drive
	 */
	if (mode) { // add GTI power from dumpload
		E.mode.error = (int32_t) UpdatePI(&E.mode.pid, E.mvar[V_BEN] + E.mode.gti_dumpload + PBAL_OFFSET);
	} else {
		E.mode.error = (int32_t) UpdatePI(&E.mode.pid, E.mvar[V_BEN] + PBAL_OFFSET);
	}

	if (E.mode.error > 0.0f) {
		coef = COEF;
	} else {
		coef = COEFN;
	}
	E.mode.target = target;
	E.mode.error = round(error_filter(E.mode.error));
	/*
	 * check for idle flag from HA
	 */
	if (E.mode.con6) {
		ha_ac_mode = true;
		bsoc_mode = false;
	}

	if (E.mode.con4) {
		E.dl_excess = true;
	}

	if (E.mode.con5) {
		E.dl_excess = false;
	}

	return bsoc_mode;
}

/*
 * power drive error signal smoothing
 */
static double error_filter(double raw)
{
	static double accum = 0.0f;
	accum = accum - accum / coef + raw;
	return accum / coef;
}

double ac0_filter(double raw)
{
	static double accum = 0.0f;
	static double coef = COEFF;
	accum = accum - accum / coef + raw;
	return accum / coef;
}

double ac1_filter(double raw)
{
	static double accum = 0.0f;
	static double coef = COEF;
	accum = accum - accum / coef + raw;
	return accum / coef;
}

double ac2_filter(double raw)
{
	static double accum = 0.0f;
	static double coef = COEF;
	accum = accum - accum / coef + raw;
	return accum / coef;
}

double dc0_filter(double raw)
{
	static double accum = 0.0f;
	static double coef = COEFF;
	accum = accum - accum / coef + raw;
	return accum / coef;
}

double dc1_filter(double raw)
{
	static double accum = 0.0f;
	static double coef = COEF;
	accum = accum - accum / coef + raw;
	return accum / coef;
}

double dc2_filter(double raw)
{
	static double accum = 0.0f;
	static double coef = COEF;
	accum = accum - accum / coef + raw;
	return accum / coef;
}

double drive0_filter(double raw)
{
	static double accum = 0.0f;
	static double coef = COEF;
	accum = accum - accum / coef + raw;
	return accum / coef;
}

double drive1_filter(double raw)
{
	static double accum = 0.0f;
	static double coef = COEFF;
	accum = accum - accum / coef + raw;
	return accum / coef;
}
