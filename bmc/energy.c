/*
 * HA Energy control using MQTT JSON and HTTP format data from various energy monitor sources
 * asynchronous mode using threads
 * 
 * long life HA token: eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJpc3MiOiI2OTczODQyMGZlMDU0MjVmYjk1OWY0YjM3Mjg4NjRkOSIsImlhdCI6MTcwODg3NzA1OSwiZXhwIjoyMDI0MjM3MDU5fQ.5vfW85qQ2DO3vAyM_lcm1YyNqIX2O8JlTqoYKLdxf6M
 *
 * This file may be freely modified, distributed, and combined with
 * other software, as long as proper attribution is given in the
 * source code.
 * Daemon example code:
 * https://github.com/pasce/daemon-skeleton-linux-c
 */
#define _DEFAULT_SOURCE
#include "ha_energy/energy.h"
#include "ha_energy/mqtt_rec.h"
#include "ha_energy/bsoc.h"

/*
 * V0.25 add Home Assistant Matter controlled utility power control switching
 * V0.26 BSOC weights for system condition for power diversion
 * V0.27 -> V0.28 GTI power ramps stability using battery current STD DEV
 * V0.29 log date-time and spam control
 * V0.30 add iammeter http data reading and processing
 * V0.31 refactor http code and a few vars
 * V0.32 AC and GTI power triggers reworked
 * V0.33 refactor system parms into energy structure energy_type E
 * V0.34 GTI and AC Inverter battery energy run down limits adjustments per energy usage and solar production
 * V0.35 more refactors and global variable consolidation
 * V0.36 more command repeat fixes for ramp up/down dumpload commands
 * V0.37 Power feedback to use PV power to GTI and AC loads
 * V0.38 signal filters to smooth large power swings in control optimization
 * V0.39 fix optimizer bugs and add AC load switching set-points in BSOC control
 * V0.40 shutdown and restart fixes
 * V0.41 fix errors and warning per cppcheck
 * V0.42 fake ac charger for dumpload using FAKE_VPV define
 * V0.43 adjust PV_BIAS per float or charging status
 * V0.44 tune for spring/summer solar conditions
 * V0.50 convert main loop code to FSM
 * V0.51 logging time additions
 * V0.52 tune GTI inverter levels for better conversion efficiency
 * V0.53 sync to HA back-end switch status
 * V0.54 data source shutdown functions
 * V0.55 off-grid inverter power tracking for HA
 * V0.56 run as Daemon in background
 * V0.62 adjust battery critical to keep making energy calculations
 * V0.63 add IP address logging
 * V0.64 Dump Load excess load mode programming
 * V.065 DL excess logic tuning and power adjustments
 */

/*
 * for the publish and subscribe topic pair
 * passed as a context variable
 */
// for local device Comedi device control
struct ha_flag_type ha_flag_vars_pc = {
	.runner = false,
	.receivedtoken = false,
	.deliveredtoken = false,
	.rec_ok = false,
	.ha_id = P8055_ID,
	.var_update = 0,
};

// solar data from mateq84
struct ha_flag_type ha_flag_vars_ss = {
	.runner = false,
	.receivedtoken = false,
	.deliveredtoken = false,
	.rec_ok = false,
	.ha_id = FM80_ID,
	.var_update = 0,
	.energy_mode = NORM_MODE,
};

// dumpload data from mbmc_k42
struct ha_flag_type ha_flag_vars_sd = {
	.runner = false,
	.receivedtoken = false,
	.deliveredtoken = false,
	.rec_ok = false,
	.ha_id = DUMPLOAD_ID,
	.var_update = 0,
};

const char *board_name = "NO_BOARD", *driver_name = "NO_DRIVER";

FILE* fout;

struct energy_type E = {
	.once_gti = true,
	.once_ac = true,
	.once_gti_zero = true,
	.iammeter = false,
	.fm80 = false,
	.dumpload = false,
	.ac_low_adj = 0.0f,
	.gti_low_adj = 0.0f,
	.ac_sw_on = true,
	.gti_sw_on = true,
	.im_delay = 0,
	.gti_delay = 0,
	.im_display = 0,
	.rc = 0,
	.speed_go = 0,
	.mode.pid.iMax = PV_IMAX,
	.mode.pid.iMin = 0.0f,
	.mode.pid.pGain = PV_PGAIN,
	.mode.pid.iGain = PV_IGAIN,
	.mode.mode_tmr = 0,
	.mode.mode = true,
	.mode.in_pid_control = false,
	.mode.dl_mqtt_max = PV_DL_MPTT_MAX,
	.mode.E = E_INIT,
	.mode.R = R_INIT,
	.mode.no_float = true,
	.mode.data_error = false,
	.ac_sw_status = false,
	.gti_sw_status = false,
	.solar_mode = false,
	.solar_shutdown = false,
	.mode.pv_bias = PV_BIAS_LOW,
	.sane = S_DLAST,
	.startup = true,
	.ac_mismatch = false,
	.dc_mismatch = false,
	.mode_mismatch = false,
	.link.shutdown = 0,
	.mode.bat_crit = false,
	.dl_excess = false,
	.dl_excess_adj = 0.0f,
};

static bool solar_shutdown(void);
void showIP(void);

void showIP(void)
{
	struct ifaddrs *ifaddr, *ifa;
	int s;
	char host[NI_MAXHOST];

	if (getifaddrs(&ifaddr) == -1) {
		perror("getifaddrs");
		exit(EXIT_FAILURE);
	}


	for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
		if (ifa->ifa_addr == NULL)
			continue;

		s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);

		if (ifa->ifa_addr->sa_family == AF_INET) {
			if (s != 0) {
				exit(EXIT_FAILURE);
			}
			printf("\tInterface : <%s>\n", ifa->ifa_name);
			printf("\t  Address : <%s>\n", host);
		}
	}

	freeifaddrs(ifaddr);
}

static void skeleton_daemon()
{
	pid_t pid;

	/* Fork off the parent process */
	pid = fork();

	/* An error occurred */
	if (pid < 0) {
		printf("\r\n%sDAEMON failure  LOG Version %s : MQTT Version %s\r\n", log_time(false), LOG_VERSION, MQTT_VERSION);
		exit(EXIT_FAILURE);
	}

	/* Success: Let the parent terminate */
	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	/* On success: The child process becomes session leader */
	if (setsid() < 0) {
		exit(EXIT_FAILURE);
	}

	/* Catch, ignore and handle signals */
	/*TODO: Implement a working signal handler */
	//    signal(SIGCHLD, SIG_IGN);
	//    signal(SIGHUP, SIG_IGN);

	/* Fork off for the second time*/
	pid = fork();

	/* An error occurred */
	if (pid < 0) {
		exit(EXIT_FAILURE);
	}

	/* Success: Let the parent terminate */
	if (pid > 0) {
		exit(EXIT_SUCCESS);
	}

	/* Set new file permissions */
	umask(0);

	/* Change the working directory to the root directory */
	/* or another appropriated directory */
	chdir("/");

	/* Close all open file descriptors */
	int x;
	for (x = sysconf(_SC_OPEN_MAX); x >= 0; x--) {
		close(x);
	}

}

bool sanity_check(void)
{
	if (E.mvar[V_PWA] > PWA_SANE) {
		E.sane = S_PWA;
		return false;
	}
	if (E.mvar[V_PAMPS] > PAMPS_SANE) {
		E.sane = S_PAMPS;
		return false;
	}
	if (E.mvar[V_PVOLTS] > PVOLTS_SANE) {
		E.sane = S_PVOLTS;
		return false;
	}
	if (E.mvar[V_FBAMPS] > BAMPS_SANE) {
		E.sane = S_FBAMPS;
		return false;
	}
	return true;
}

/*
 * Async processing threads
 */

/*
 * data update timer flag
 * and 10 second software time clock
 */
void timer_callback(int32_t signum)
{
	signal(signum, timer_callback);
	ha_flag_vars_ss.runner = true;
	E.ten_sec_clock++;
}

/*
 * Broker errors
 */
void connlost(void *context, char *cause)
{
	struct ha_flag_type *ha_flag = context;
	int32_t id_num;

	// bug-out if no context variables passed to callback
	if (context == NULL) {
		id_num = -1;
	} else {
		id_num = ha_flag->ha_id;
	}
	fprintf(fout, "\n%s Connection lost\n", log_time(false));
	fprintf(fout, "%s     cause: %s, %d\n", log_time(false), cause, id_num);
	fflush(fout);
	exit(EXIT_FAILURE);
}

/*
 * Use MQTT/HTTP to send/receive updates to a Solar hardware device
 */
int main(int argc, char *argv[])
{
	struct itimerval new_timer = {
		.it_value.tv_sec = CMD_SEC,
		.it_value.tv_usec = 0,
		.it_interval.tv_sec = CMD_SEC,
		.it_interval.tv_usec = 0,
	};
	struct itimerval old_timer;
	time_t rawtime;
	MQTTClient_connectOptions conn_opts_p = MQTTClient_connectOptions_initializer,
		conn_opts_sd = MQTTClient_connectOptions_initializer;
	MQTTClient_message pubmsg = MQTTClient_message_initializer;
	MQTTClient_deliveryToken token;
	char hname[256];
	size_t hname_len = 12;

	gethostname(hname, hname_len);
	hname[12] = 0;
	printf("\r\n%s  LOG Version %s : MQTT Version %s : Host Name %s\r\n", log_time(false), LOG_VERSION, MQTT_VERSION, hname);
	showIP();
	skeleton_daemon();

	while (true) {
		switch (E.mode.E) {
		case E_INIT:

#ifdef LOG_TO_FILE
			fout = fopen(LOG_TO_FILE, "a");
			if (fout == NULL) {
				fout = fopen(LOG_TO_FILE_ALT, "a");
				if (fout == NULL) {
					fout = stdout;
					printf("\r\n%s Unable to open LOG file %s \r\n", log_time(false), LOG_TO_FILE_ALT);
				}
			}
#else
			fout = stdout;
#endif
			fprintf(fout, "\r\n%s LOG Version %s : MQTT Version %s\r\n", log_time(false), LOG_VERSION, MQTT_VERSION);
			fflush(fout);

			if (!bsoc_init()) {
				fprintf(fout, "\r\n%s bsoc_init failure \r\n", log_time(false));
				fflush(fout);
				exit(EXIT_FAILURE);
			}
			/*
			 * set the timer for MQTT publishing sample speed
			 */
			setitimer(ITIMER_REAL, &new_timer, &old_timer);
			signal(SIGALRM, timer_callback);

			if (strncmp(hname, TNAME, 5) == 0) {
				MQTTClient_create(&E.client_p, LADDRESS, CLIENTID1,
					MQTTCLIENT_PERSISTENCE_NONE, NULL);
				conn_opts_p.keepAliveInterval = 20;
				conn_opts_p.cleansession = 1;
			} else {
				MQTTClient_create(&E.client_p, ADDRESS, CLIENTID1,
					MQTTCLIENT_PERSISTENCE_NONE, NULL);
				conn_opts_p.keepAliveInterval = 20;
				conn_opts_p.cleansession = 1;
			}

			fprintf(fout, "%s Connect MQTT server %s, %s\n", log_time(false), ADDRESS, CLIENTID1);
			fflush(fout);
			MQTTClient_setCallbacks(E.client_p, &ha_flag_vars_ss, connlost, msgarrvd, delivered);
			if ((E.rc = MQTTClient_connect(E.client_p, &conn_opts_p)) != MQTTCLIENT_SUCCESS) {
				fprintf(fout, "%s Failed to connect MQTT server, return code %d %s, %s\n", log_time(false), E.rc, ADDRESS, CLIENTID1);
				fflush(fout);
				pthread_mutex_destroy(&E.ha_lock);
				exit(EXIT_FAILURE);
			}

			if (strncmp(hname, TNAME, 5) == 0) {
				MQTTClient_create(&E.client_sd, LADDRESS, CLIENTID2,
					MQTTCLIENT_PERSISTENCE_NONE, NULL);
				conn_opts_p.keepAliveInterval = 20;
				conn_opts_p.cleansession = 1;
			} else {
				MQTTClient_create(&E.client_sd, ADDRESS, CLIENTID2,
					MQTTCLIENT_PERSISTENCE_NONE, NULL);
				conn_opts_sd.keepAliveInterval = 20;
				conn_opts_sd.cleansession = 1;
			}

			fprintf(fout, "%s Connect MQTT server %s, %s\n", log_time(false), ADDRESS, CLIENTID2);
			fflush(fout);
			MQTTClient_setCallbacks(E.client_sd, &ha_flag_vars_sd, connlost, msgarrvd, delivered);
			if ((E.rc = MQTTClient_connect(E.client_sd, &conn_opts_sd)) != MQTTCLIENT_SUCCESS) {
				fprintf(fout, "%s Failed to connect MQTT server, return code %d %s, %s\n", log_time(false), E.rc, ADDRESS, CLIENTID2);
				fflush(fout);
				pthread_mutex_destroy(&E.ha_lock);
				exit(EXIT_FAILURE);
			}

			/*
			 * on topic received data will trigger the msgarrvd function
			 */
			MQTTClient_subscribe(E.client_p, TOPIC_SS, QOS);
			MQTTClient_subscribe(E.client_sd, TOPIC_SD, QOS);

			pubmsg.payload = "online";
			pubmsg.payloadlen = strlen("online");
			pubmsg.qos = QOS;
			pubmsg.retained = 0;
			ha_flag_vars_ss.deliveredtoken = 0;
			// notify HA we are running and controlling AC power plugs
			MQTTClient_publishMessage(E.client_p, TOPIC_PACA, &pubmsg, &token);
			MQTTClient_publishMessage(E.client_p, TOPIC_PDCA, &pubmsg, &token);

			// sync HA power switches
			mqtt_ha_switch(E.client_p, TOPIC_PDCC, false);
			mqtt_ha_switch(E.client_p, TOPIC_PACC, false);
			mqtt_ha_switch(E.client_p, TOPIC_PDCC, true);
			mqtt_ha_switch(E.client_p, TOPIC_PACC, true);
			mqtt_ha_switch(E.client_p, TOPIC_PDCC, false);
			mqtt_ha_switch(E.client_p, TOPIC_PACC, false);
			//                E.mode.in_pid_control = false;
			E.ac_sw_on = true; // can be switched on once
			E.gti_sw_on = true; // can be switched on once

			/*
			 * use libcurl to read AC power meter HTTP data
			 */
			iammeter_read();

			fprintf(fout, "\r\n%s Solar Energy AC power controller\r\n", log_time(false));

#ifdef FAKE_VPV
			fprintf(fout, "\r\n Faking dumpload PV voltage\r\n");
#endif
			ha_flag_vars_ss.energy_mode = NORM_MODE;
			E.mode.E = E_WAIT;
			break;
		case E_WAIT:
			if (ha_flag_vars_ss.runner || E.speed_go++ > 1500000) {
				E.speed_go = 0;
				ha_flag_vars_ss.runner = false;
				E.mode.E = E_RUN;
			}

			usleep(100);
			/*
			 * main state-machine update sequence
			 */
			bsoc_data_collect();
			if (!sanity_check()) {
				fprintf(fout, "\r\n%s Sanity Check error %d %s \r\n", log_time(false), E.sane, mqtt_name[E.sane]);
				fflush(fout);
			}

			if (solar_shutdown()) {
				if (!E.startup) {
					fprintf(fout, "%s SHUTDOWN Solar Energy Control ---> \r\n", log_time(false));
				}
				fflush(fout);
				ramp_down_gti(E.client_p, true);
				usleep(100000); // wait
				ramp_down_ac(E.client_p, true);
				usleep(100000); // wait
				ramp_down_gti(E.client_p, true);
				usleep(100000); // wait
				ramp_down_ac(E.client_p, true);
				usleep(100000); // wait
				if (!E.startup) {
					fprintf(fout, "%s Completed SHUTDOWN, Press again to RESTART.\r\n", log_time(false));
					fflush(fout);
				}
				fflush(fout);

				uint8_t iam_delay = 0;
				while (solar_shutdown()) {
					usleep(USEC_SEC); // wait
					if ((int32_t) E.mvar[V_HACSW]) {
						ha_ac_off();
					}
					if ((int32_t) E.mvar[V_HDCSW]) {
						ha_dc_off();
					}
					if ((iam_delay++ > IAM_DELAY) && E.link.shutdown) {
						E.fm80 = true;
						E.dumpload = true;
						E.iammeter = true;
					}
				}
				E.link.shutdown = 0;
				fprintf(fout, "%s RESTART Solar Energy Control\r\n", log_time(false));
				fflush(fout);
				bsoc_set_mode(E.mode.pv_bias, true, true);
#ifdef AUTO_CHARGE
				mqtt_ha_switch(E.client_p, TOPIC_PDCC, true);
#endif
				usleep(100000); // wait
				E.gti_sw_status = true;
				ResetPI(&E.mode.pid);
				ha_flag_vars_ss.runner = true;
				E.fm80 = true;
				E.dumpload = true;
				E.iammeter = true;
				E.mode.in_pid_control = false;
				E.mode.R = R_INIT;
			}
			if (ha_flag_vars_ss.receivedtoken) {
				ha_flag_vars_ss.receivedtoken = false;
			}
			if (ha_flag_vars_sd.receivedtoken) {
				ha_flag_vars_sd.receivedtoken = false;
			}
			break;
		case E_RUN:
			usleep(100);
			switch (E.mode.R) {
			case R_INIT:
				E.once_ac = true;
				E.once_gti = true;
				E.ac_sw_on = true;
				E.gti_sw_on = true;
				E.mode.R = R_RUN;
				E.mode.no_float = true;
				break;
			case R_FLOAT:
				if (E.mode.no_float) {
					E.once_ac = true;
					E.once_gti = true;
					E.ac_sw_on = true;
					E.gti_sw_on = true;
					E.gti_sw_status = false;
					E.ac_sw_status = false;
					E.mode.no_float = false;
				}
				if (!E.gti_sw_status) {
					if (gti_test() > MIN_BAT_KW_GTI_HI) {
						mqtt_ha_switch(E.client_p, TOPIC_PDCC, true);
						E.gti_sw_status = true;
						fprintf(fout, "%s R_FLOAT DC switch true \r\n", log_time(false));
					}
				}
				usleep(100000); // wait
				if (!E.ac_sw_status) {
					if (ac_test() > MIN_BAT_KW_AC_HI) {
						mqtt_ha_switch(E.client_p, TOPIC_PACC, true);
						E.ac_sw_status = true;
						fprintf(fout, "%s R_FLOAT AC switch true \r\n", log_time(false));
					}
				}
				E.mode.pv_bias = PV_BIAS;
				fm80_float(true);
				break;
			case R_RUN:
			default:
				E.mode.R = R_RUN;
				E.mode.no_float = true;
				break;
			}
			/*
			 * main state-machine update sequence
			 */
			/*
			 * check for idle/data errors flags from sensors and HA
			 */
			if (!E.mode.data_error) {
				bsoc_set_mode(E.mode.pv_bias, true, false);
				if (E.gti_delay++ >= GTI_DELAY) {
					char gti_str[SBUF_SIZ];
					int32_t error_drive;

					/*
					 * reset the control mode from simple switched power to PID control
					 */
					if (!E.mode.in_pid_control) {
						mqtt_ha_switch(E.client_p, TOPIC_PDCC, true);
						E.gti_sw_status = true;
						usleep(100000); // wait
						mqtt_ha_switch(E.client_p, TOPIC_PACC, true);
						E.ac_sw_status = true;
						E.mode.pv_bias = PV_BIAS;
						fprintf(fout, "%s in_pid_mode AC/DC switch true \r\n", log_time(false));
						fm80_float(true);
					} else {
						if (!fm80_float(true)) {
							E.mode.pv_bias = (int32_t) E.mode.error - PV_BIAS;
						}
					}
					E.mode.in_pid_control = true;
					E.gti_delay = 0;
					/*
					 * adjust power balance if battery charging energy is low
					 */
					if (E.mvar[V_DPBAT] > PV_DL_BIAS_RATE) {
						error_drive = (int32_t) E.mode.error - E.mode.pv_bias; // PI feedback control signal
					} else {
						error_drive = (int32_t) E.mode.error - PV_BIAS_RATE;
					}
					/*
					 * when main battery is in float, crank-up the power draw from the solar panels
					 */
					if (fm80_float(true)) {
						error_drive = (int32_t) (E.mode.error + PV_BIAS);
					}
					/*
					 * don't drive to zero power
					 */
					if (error_drive < 0) {
						error_drive = PV_BIAS_LOW; // control wide power swings
						if (!fm80_sleep()) { // check for using sleep bias
							if ((E.mvar[V_FBEKW] > MIN_BAT_KW_BSOC_SLP) && (E.mvar[V_PWA] > PWA_SLEEP)) {
								error_drive = PV_BIAS_SLEEP; // use higher power when we still have sun for better inverter efficiency
							}
						}
					}

					/*
					 * reduce charging/diversion power to safe PS limits
					 */
					if (E.mode.dl_mqtt_max > PV_DL_MPTT_MAX) {
						error_drive = PV_DL_MPTT_IDLE;
					} else {
						if (E.dl_excess) {
							error_drive = PV_DL_EXCESS + E.dl_excess_adj;
						}
					}

					snprintf(gti_str, SBUF_SIZ - 1, "V%04dX", error_drive); // format for dumpload controller gti power commands
					mqtt_gti_power(E.client_p, TOPIC_P, gti_str);
				}

#ifndef  FAKE_VPV
				if (fm80_float(true) || ((ac1_filter(E.mvar[V_BEN]) > BAL_MAX_ENERGY_AC) && (ac_test() > MIN_BAT_KW_AC_HI))) {
					ramp_up_ac(E.client_p, E.ac_sw_on); // use once control
#ifdef PSW_DEBUG
					fprintf(fout, "%s MIN_BAT_KW_AC_HI AC switch %d \r\n", log_time(false), E.ac_sw_on);
#endif
					E.ac_sw_on = false; // once flag
				}
#endif
				if (((ac2_filter(E.mvar[V_BEN]) < BAL_MIN_ENERGY_AC) || ((ac_test() < (MIN_BAT_KW_AC_LO + E.ac_low_adj))))) {
					if (!fm80_float(true)) {
						ramp_down_ac(E.client_p, E.ac_sw_on); // use once control
#ifdef PSW_DEBUG
						fprintf(fout, "%s MIN_BAT_KW_AC_LO AC switch %d \r\n", log_time(false), E.ac_sw_on);
#endif
					}
					E.ac_sw_on = true;
				}


				/*
				 * Dump Load Excess testing
				 */
				if (((dc1_filter(E.mvar[V_BEN]) > BAL_MAX_ENERGY_GTI) && (gti_test() > MIN_BAT_KW_GTI_HI)) || E.dl_excess) {
#ifndef  FAKE_VPV                            
#ifdef B_DLE_DEBUG
					if (E.dl_excess) {
						fprintf(fout, "%s DL excess ramp_up_gti, DC switch %d\r\n", log_time(false), E.gti_sw_on);
					}
#endif
					ramp_up_gti(E.client_p, E.gti_sw_on, E.dl_excess); // fixme on the ONCE code
#ifdef PSW_DEBUG
					fprintf(fout, "%s MIN_BAT_KW_GTI_HI DC switch %d \r\n", log_time(false), E.gti_sw_on);
#endif
					E.gti_sw_on = false; // once flag
#endif                            
				} else {
					if ((dc2_filter(E.mvar[V_BEN]) < BAL_MIN_ENERGY_GTI) || (gti_test() < (MIN_BAT_KW_GTI_LO + E.gti_low_adj))) {
						ramp_down_gti(E.client_p, true);
#ifdef PSW_DEBUG
						fprintf(fout, "%s MIN_BAT_KW_GTI_LO DC switch %d \r\n", log_time(false), E.gti_sw_on);
#endif
						E.gti_sw_on = true;
					}
				}
			};

#ifdef B_ADJ_DEBUG
			fprintf(fout, "\r\n LO ADJ: AC %8.2fWh, GTI %8.2fWh\r\n", MIN_BAT_KW_AC_LO + E.ac_low_adj, MIN_BAT_KW_GTI_LO + E.gti_low_adj);
#endif
#ifdef B_DLE_DEBUG
			if (E.dl_excess) {
				fprintf(fout, "%s DL excess vars from ha_energy %d %d : Flag %d\r\n", log_time(false), E.mode.con4, E.mode.con5, E.dl_excess);
			}
#endif

			time(&rawtime);

			if (E.im_delay++ >= IM_DELAY) {
				E.im_delay = 0;
				iammeter_read();
			}
			if (E.im_display++ >= IM_DISPLAY) {
				char buffer[SYSLOG_SIZ];
				uint32_t len;

				E.im_display = 0;
				mqtt_ha_pid(E.client_p, TOPIC_PPID);
				if (!(E.fm80 && E.dumpload && E.iammeter)) {
					if (!E.iammeter) {
						E.link.iammeter_error++;
					} else {
						E.link.mqtt_error++;
					}
					E.link.shutdown++;
					fprintf(fout, "\r\n%s !!!! Source data update error !!!! , check FM80 %i, DUMPLOAD %i, IAMMETER %i channels M %u,%u I %u,%u\r\n", log_time(false), E.fm80, E.dumpload, E.fm80,
						E.link.mqtt_count, E.link.mqtt_error, E.link.iammeter_count, E.link.iammeter_error);
					fflush(fout);
					snprintf(buffer, SYSLOG_SIZ - 1, "\r\n%s !!!! Source data update error !!!! , check FM80 %i, DUMPLOAD %i, IAMMETER %i channels M %u,%u I %u,%u\r\n", log_time(false), E.fm80, E.dumpload, E.fm80,
						E.link.mqtt_count, E.link.mqtt_error, E.link.iammeter_count, E.link.iammeter_error);
					syslog(LOG_NOTICE, buffer);
					E.mode.data_error = true;
				} else {
					E.mode.data_error = false;
					E.link.shutdown = 0;
				}
				snprintf(buffer, RBUF_SIZ - 1, "%s", ctime(&rawtime));
				len = strlen(buffer);
				buffer[len - 1] = 0; // munge out the return character
				fprintf(fout, "%s ", buffer);
				fflush(fout);
				E.fm80 = false;
				E.dumpload = false;
				E.iammeter = false;
				sync_ha();
				print_im_vars();
				print_mvar_vars();
				fprintf(fout, "%s\r", ctime(&rawtime));
			}
			E.mode.E = E_WAIT;
			fflush(fout);
			if (E.mode.con6) {
				E.mode.R = R_IDLE;
			}
			if (E.mode.con7) {
				E.mode.E = E_STOP;
			}
			break;
		case E_STOP:
		default:
			fflush(fout);
			fprintf(fout, "\r\n%s HA Energy stopped and exited.\r\n", log_time(false));
			fflush(fout);
			return 0;
			break;
		}
	}
}

void ramp_up_gti(MQTTClient client_p, bool start, bool excess)
{
	static uint32_t sequence = 0;

	if (start) {
		E.once_gti = true;
	}

	if (E.once_gti) {
		E.once_gti = false;
		sequence = 0;
		if (!excess) {
			mqtt_ha_switch(client_p, TOPIC_PDCC, true);
			E.gti_sw_status = true;
			usleep(500000); // wait for voltage to ramp
		} else {
			sequence = 1;
		}
	}

	switch (sequence) {
	case 4:
		E.once_gti_zero = true;
		break;
	case 3:
	case 2:
	case 1:
		E.once_gti_zero = true;
		if (bat_current_stable() || E.dl_excess) { // check battery current std dev, stop 'motorboating'
			sequence++;
			if (!mqtt_gti_power(client_p, TOPIC_P, "+#")) {
				sequence = 0;
			}; // +100W power
		} else {
			usleep(500000); // wait a bit more for power to be stable
			sequence = 1; // do power ramps when ready
			if (!mqtt_gti_power(client_p, TOPIC_P, "-#")) {
				sequence = 0;
			}; // - 100W power
		}
		break;
	case 0:
		sequence++;
		if (E.once_gti_zero) {
			mqtt_gti_power(client_p, TOPIC_P, "Z#"); // zero power
			E.once_gti_zero = false;
		}
		break;
	default:
		if (E.once_gti_zero) {
			mqtt_gti_power(client_p, TOPIC_P, "Z#"); // zero power
			E.once_gti_zero = false;
		}
		sequence = 0;
		break;
	}
}

void ramp_down_gti(MQTTClient client_p, bool sw_off)
{
	if (sw_off) {
		mqtt_ha_switch(client_p, TOPIC_PDCC, false);
		E.once_gti_zero = true;
		E.gti_sw_status = false;
	}
	E.once_gti = true;

	if (E.once_gti_zero) {
		mqtt_gti_power(client_p, TOPIC_P, "Z#"); // zero power
		E.once_gti_zero = false;
	}
}

void ramp_up_ac(MQTTClient client_p, bool start)
{

	if (start) {
		E.once_ac = true;
	}

	if (E.once_ac) {
		E.once_ac = false;
		mqtt_ha_switch(client_p, TOPIC_PACC, true);
		E.ac_sw_status = true;
		usleep(500000); // wait for voltage to ramp
	}
}

void ramp_down_ac(MQTTClient client_p, bool sw_off)
{
	if (sw_off) {
		mqtt_ha_switch(client_p, TOPIC_PACC, false);
		E.ac_sw_status = false;
		usleep(500000);
	}
	E.once_ac = true;
}

void ha_ac_off(void)
{
	mqtt_ha_switch(E.client_p, TOPIC_PACC, false);
	E.ac_sw_status = false;
}

void ha_ac_on(void)
{
	mqtt_ha_switch(E.client_p, TOPIC_PACC, true);
	E.ac_sw_status = true;
}

void ha_dc_off(void)
{
	mqtt_ha_switch(E.client_p, TOPIC_PDCC, false);
	E.gti_sw_status = false;
}

void ha_dc_on(void)
{
	mqtt_ha_switch(E.client_p, TOPIC_PDCC, true);
	E.gti_sw_status = true;
}

//#define DEBUG_SHUTDOWN

static bool solar_shutdown(void)
{
	static bool ret = false;

	if (E.startup) {
		ret = true;
		E.startup = false;
		return ret;
	} else {
		ret = false;

		/*
		 * FIXME
		 * 
		 */

		//		return ret;
	}

	if (E.solar_shutdown) {
		ret = true;
	} else {
		ret = false;
	}

	if ((E.mvar[V_FBEKW] < BAT_CRITICAL) && !E.startup) { // special case for low battery
		if (!E.mode.bat_crit) {
			ret = true;
#ifdef CRITIAL_SHUTDOWN_LOG
			fprintf(fout, "%s Solar BATTERY CRITICAL shutdown comms check ret = %d \r\n", log_time(false), ret);
			fflush(fout);
#endif
			E.mode.bat_crit = true;
			return ret;
		}
	} else {
		E.mode.bat_crit = false;
	}

	if (E.link.shutdown >= MAX_ERROR) {
		ret = true;
		if (E.fm80 && E.dumpload && E.iammeter) {
			ret = false;
			E.link.shutdown = 0;
		}

#ifdef DEBUG_SHUTDOWN
		fprintf(fout, "%s Solar shutdown comms check ret = %d \r\n", log_time(false), ret);
		fflush(fout);
#endif
	}
	return ret;
}

char * log_time(bool log)
{
	static char time_log[RBUF_SIZ] = {0};
	static uint32_t len = 0, sync_time = TIME_SYNC_SEC - 1;
	time_t rawtime_log;

	tzset();
	time(&rawtime_log);
	if (sync_time++ > TIME_SYNC_SEC) {
		sync_time = 0;
		snprintf(time_log, RBUF_SIZ - 1, "VT%lut", rawtime_log); // format for dumpload controller gti time commands
		mqtt_gti_time(E.client_p, TOPIC_P, time_log);
	}

	sprintf(time_log, "%s", ctime(&rawtime_log));
	len = strlen(time_log);
	time_log[len - 1] = 0; // munge out the return character
	if (log) {
		fprintf(fout, "%s ", time_log);
		fflush(fout);
	}

	return time_log;
}

bool sync_ha(void)
{
	bool sync = false;
	if (E.gti_sw_status != (bool) ((int32_t) E.mvar[V_HDCSW])) {
		mqtt_ha_switch(E.client_p, TOPIC_PDCC, !E.ac_sw_status);
		E.dc_mismatch = true;
		sync = true;
		fprintf(fout, "DC_MM ");
		fflush(fout);
	} else {
		E.dc_mismatch = false;
	}
	if (E.ac_sw_status != (bool) ((int32_t) E.mvar[V_HACSW])) {
		mqtt_ha_switch(E.client_p, TOPIC_PACC, !E.ac_sw_status);
		E.ac_mismatch = true;
		fprintf(fout, "AC_MM ");
		fflush(fout);
		sync = true;
	} else {
		E.ac_mismatch = false;
	}
	return sync;
}