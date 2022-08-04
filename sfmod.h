/*
 * (c) Copyright 2013 -- Raul Fernandez Ortega 
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#ifndef _SFMOD_H_
#define _SFMOD_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <sys/time.h>
#include <sched.h>
#include <semaphore.h>
#include <inttypes.h>

#include "defs.h"

#ifdef __cplusplus
}
#endif
#include <libxml/parser.h>
#include <libxml/tree.h>
#include <string>
#include <vector>

using namespace std;

struct debug_input {
  struct {
    uint64_t ts_start_call;
    uint64_t ts_start_ret;
  } init;
  struct {
    int fdmax;
    int retval;
    uint64_t ts_call;
    uint64_t ts_ret;
  } select;
  struct {
    int fd;
    void *buf;
    int offset;
    int count;
    int retval;
    uint64_t ts_call;
    uint64_t ts_ret;
  } read;
};

struct debug_output {
  struct {
    uint64_t ts_synchfd_call;
    uint64_t ts_synchfd_ret;
    uint64_t ts_start_call;
    uint64_t ts_start_ret;
  } init;
  struct {
    int fdmax;
    int retval;
    uint64_t ts_call;
    uint64_t ts_ret;
  } select;
  struct {
    int fd;
    void *buf;
    int offset;
    int count;
    int retval;
    uint64_t ts_call;
    uint64_t ts_ret;
  } write;
};

/* debug structs */
struct debug_input_process {
  struct debug_input d[DEBUG_MAX_DAI_LOOPS];
  int dai_loops;
  struct {
    uint64_t ts_call;
    uint64_t ts_ret;
  } r_output;
  struct {
    uint64_t ts_call;
    uint64_t ts_ret;
  } w_filter;
};

struct debug_output_process {
  struct {
    uint64_t ts_call;
    uint64_t ts_ret;
  } r_filter;
  struct {
    uint64_t ts_call;
    uint64_t ts_ret;
  } w_input;
  struct debug_output d[DEBUG_MAX_DAI_LOOPS];
  int dai_loops;
};

struct debug_filter_process {
  struct {
    uint64_t ts_call;
    uint64_t ts_ret;
  } r_input;
  struct {
    uint64_t ts_call;
    uint64_t ts_ret;
  } mutex;
  struct {
    uint64_t ts_call;
    uint64_t ts_ret;
  } fsynch_fd;
  struct {
    uint64_t ts_call;
    uint64_t ts_ret;
  } fsynch_td;
  struct {
    uint64_t ts_call;
    uint64_t ts_ret;
  } w_output;    
};

#define SF_FDEVENT_PEAK 0x1
#define SF_FDEVENT_INITIALISED 0x2   

inline int sf_sampleformat_size(int format)
{

  switch (format) {
  case SF_SAMPLE_FORMAT_S8:
    return 1;
  case SF_SAMPLE_FORMAT_S16_LE:
    return 2;
  case SF_SAMPLE_FORMAT_S16_BE:
    return 2;
  case SF_SAMPLE_FORMAT_S16_4LE:
    return 4;
  case SF_SAMPLE_FORMAT_S16_4BE:
    return 4;
  case SF_SAMPLE_FORMAT_S24_LE:
    return 3;
  case SF_SAMPLE_FORMAT_S24_BE:
	return 3;
  case SF_SAMPLE_FORMAT_S24_4LE:
    return 4;
  case SF_SAMPLE_FORMAT_S24_4BE:
    return 4;
  case SF_SAMPLE_FORMAT_S32_LE:
    return 4;
  case SF_SAMPLE_FORMAT_S32_BE:
    return 4;
  case SF_SAMPLE_FORMAT_FLOAT_LE:
    return 4;
  case SF_SAMPLE_FORMAT_FLOAT_BE:
    return 4;
  case SF_SAMPLE_FORMAT_FLOAT64_LE:
    return 8;
  case SF_SAMPLE_FORMAT_FLOAT64_BE:
    return 8;
  default:
    return 0;
  }
}

inline const char *sf_strsampleformat(int format)
{
  switch (format) {
  case SF_SAMPLE_FORMAT_S8:
    return "S8";
  case SF_SAMPLE_FORMAT_S16_LE:
    return "S16_LE";
  case SF_SAMPLE_FORMAT_S16_BE:
    return "S16_BE";
  case SF_SAMPLE_FORMAT_S16_NE:
    return "S16_NE";
  case SF_SAMPLE_FORMAT_S16_4LE:
    return "S16_4LE";
  case SF_SAMPLE_FORMAT_S16_4BE:
    return "S16_4BE";
  case SF_SAMPLE_FORMAT_S16_4NE:
    return "S16_4NE";
  case SF_SAMPLE_FORMAT_S24_LE:
    return "S24_LE";
  case SF_SAMPLE_FORMAT_S24_BE:
    return "S24_BE";
  case SF_SAMPLE_FORMAT_S24_NE:
    return "S24_NE";
  case SF_SAMPLE_FORMAT_S24_4LE:
    return "S24_4LE";
  case SF_SAMPLE_FORMAT_S24_4BE:
    return "S24_4BE";
  case SF_SAMPLE_FORMAT_S24_4NE:
    return "S24_4NE";
  case SF_SAMPLE_FORMAT_S32_LE:
    return "S32_LE";
  case SF_SAMPLE_FORMAT_S32_BE:
    return "S32_BE";
  case SF_SAMPLE_FORMAT_S32_NE:
    return "S32_NE";
  case SF_SAMPLE_FORMAT_FLOAT_LE:
    return "FLOAT_LE";
  case SF_SAMPLE_FORMAT_FLOAT_BE:
    return "FLOAT_BE";
  case SF_SAMPLE_FORMAT_FLOAT_NE:
    return "FLOAT_NE";
  case SF_SAMPLE_FORMAT_FLOAT64_LE:
    return "FLOAT64_LE";
  case SF_SAMPLE_FORMAT_FLOAT64_BE:
    return "FLOAT64_BE";
  case SF_SAMPLE_FORMAT_FLOAT64_NE:
    return "FLOAT64_NE";
  case SF_SAMPLE_FORMAT_AUTO:
    return "AUTO";
  default:
    return "##unknown sample format##";
  }
}

#endif
