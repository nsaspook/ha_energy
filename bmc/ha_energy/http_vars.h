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

#define IAMM1 "http://10.1.1.101/monitorjson"
#define IAMM2 "http://10.1.1.102/monitorjson"
#define IAMM3 "http://10.1.1.103/monitorjson"

//#define IM_DEBUG2
	extern FILE* fout;

	size_t iammeter_write_callback1(char *, size_t, size_t, void *);
	size_t iammeter_write_callback2(char *, size_t, size_t, void *);
	void iammeter_read1(const char *);
	void iammeter_read2(const char *);
	void iammeter_read3(const char *);
	void print_im_vars(void);
	void print_mvar_vars(void);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_VARS_H */

