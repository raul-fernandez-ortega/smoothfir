/*
 * (c) Copyright 2013 -- Raul Fernandez Ortega
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */

#ifndef _SFCLASSES_HPP_
#define _SFCLASSES_HPP_

extern "C" {

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
}

#include <string>
#include <vector>
#include "sfmod.h"

using namespace std;

struct sfcoeff {
  int is_shared;
  string name;
  int intname;
  int n_blocks;
};

struct sfchannel {
  string name;
  int intname;
};

struct sffilter {
  string name;
  int intname;
  int crossfade;
  int n_channels[2];
  vector<int> channels[2];
  int n_filters[2];
  vector<int> filters[2];
};

struct sffilter_control {
  int coeff;
  int delayblocks;
  vector<double> scale[2];
  vector<double> fscale;
};

struct intercomm_area {
  bool doreset_overflow;
  int sync;
  uint32_t period_us;
  double realtime_index;
  vector<struct sffilter_control> fctrl;
  vector<struct sfoverflow> in_overflow;
  vector<struct sfoverflow> out_overflow;
  uint32_t *ismuted[2];
  vector<int> delay[2];
  bool full_proc;
  bool ignore_rtprio;

  struct {
    uint64_t ts_start;
    struct debug_input_process i[DEBUG_RING_BUFFER_SIZE];
    struct debug_output_process o[DEBUG_RING_BUFFER_SIZE];
    struct debug_filter_process f[DEBUG_RING_BUFFER_SIZE];
    uint32_t periods;
  } debug;
};

#endif
