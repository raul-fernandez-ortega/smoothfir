/*
 * (c) Copyright 2013 -- Raul Fernandez Ortega
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#ifndef _DEFS_H_
#define _DEFS_H_

#include "sysarch.h"

#include <stdbool.h>
#include <sys/types.h>

#define SF_VERSION_MAJOR 2
#define SF_VERSION_MINOR 0
    
/* limits */
#define SF_MAXCHANNELS 256
#define SF_MAXFILTERS 256
#define SF_MAXMODULES 256
#define SF_MAXOBJECTNAME 128
#define SF_MAXCOEFFPARTS 128
#define SF_MAXPROCESSES 64

#define SF_IN 0
#define SF_OUT 1

#define SF_SAMPLE_FORMAT_MIN SF_SAMPLE_FORMAT_S8
#define SF_SAMPLE_FORMAT_S8 1
#define SF_SAMPLE_FORMAT_S16_LE 2
#define SF_SAMPLE_FORMAT_S16_BE 3
#define SF_SAMPLE_FORMAT_S16_4LE 4
#define SF_SAMPLE_FORMAT_S16_4BE 5
#define SF_SAMPLE_FORMAT_S24_LE 6
#define SF_SAMPLE_FORMAT_S24_BE 7
#define SF_SAMPLE_FORMAT_S24_4LE 8
#define SF_SAMPLE_FORMAT_S24_4BE 9
#define SF_SAMPLE_FORMAT_S32_LE 10
#define SF_SAMPLE_FORMAT_S32_BE 11
#define SF_SAMPLE_FORMAT_FLOAT_LE 12
#define SF_SAMPLE_FORMAT_FLOAT_BE 13
#define SF_SAMPLE_FORMAT_FLOAT64_LE 14
#define SF_SAMPLE_FORMAT_FLOAT64_BE 15

#define SF_SAMPLE_FORMAT_MAX SF_SAMPLE_FORMAT_FLOAT64_BE

/* macro sample formats */    
#define SF_SAMPLE_FORMAT_S16_NE 16
#define SF_SAMPLE_FORMAT_S16_4NE 17
#define SF_SAMPLE_FORMAT_S24_NE 18
#define SF_SAMPLE_FORMAT_S24_4NE 19
#define SF_SAMPLE_FORMAT_S32_NE 20
#define SF_SAMPLE_FORMAT_FLOAT_NE 21
#define SF_SAMPLE_FORMAT_FLOAT64_NE 22
#define SF_SAMPLE_FORMAT_AUTO 23

#define SF_SAMPLE_FORMAT_MACRO_MAX 23    

/* exit values */
#define SF_EXIT_OK 0
#define SF_EXIT_OTHER 1
#define SF_EXIT_INVALID_CONFIG 2
#define SF_EXIT_NO_MEMORY 3
#define SF_EXIT_INVALID_INPUT 4
#define SF_EXIT_BUFFER_UNDERFLOW 5

/* callback events */
#define SF_CALLBACK_EVENT_NORMAL 0
#define SF_CALLBACK_EVENT_ERROR 1
#define SF_CALLBACK_EVENT_LAST_INPUT 2
#define SF_CALLBACK_EVENT_FINISHED 3    
    
#define SF_SAMPLE_SLOTS 100
#define SF_UNDEFINED_SUBDELAY (-SF_SAMPLE_SLOTS)
    
#define DEBUG_RING_BUFFER_SIZE 1024
#define DEBUG_MAX_DAI_LOOPS 32

#define SF_MAXCHANNELS 256

struct sfoverflow {
  unsigned int n_overflows;
  int32_t intlargest;
  double largest;
  double max;
};

typedef unsigned long ull_t;
typedef long ll_t;
typedef struct _td_conv_t_ td_conv_t;

#endif
