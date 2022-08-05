/*
 * (c) Copyright 2013/2022 -- Raul Fernandez Ortega
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */

#ifndef _SFSTRUCT_H_
#define _SFSTRUCT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <sched.h>

#ifdef __cplusplus
}
#endif

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <string>
#include <vector>

#include "defs.h"
#include "sfclasses.hpp"
#include "sfio.hpp"

using namespace std;

struct sample_format {
  bool isfloat;
  bool swap;
  int bytes;
  int sbytes;
  double scale;
  int format;
};

struct buffer_format {
  struct sample_format sf;
  int sample_spacing; /* in samples */
  int byte_offset;    /* in bytes */
};

struct filter_process {
  int n_unique_channels[2];
  vector<int> unique_channels[2];
  int n_filters;    
  vector<sffilter> filters;
};

/* digital audio interface */

struct dai_channels {
  struct sample_format sf;
    /* how many channels to open (on device level) */
  int open_channels;
  /* how many channels of the opened channels that are used (on device level)
     used_channels <= open_channels */    
  int used_channels;
  /* array (used_channels elements long) which contains the channel indexes
     on device level (0 <= index < open_channels) of the used channels. */
  vector<int> channel_selection;
  /* array (used_channels elements long) which contains the channel indexes
     on a logical level. These indexes are those used in the dai_* function
     calls. */
  vector<int> channel_name;
};

struct _delaybuffer_t_ {
  int fragsize;    /* fragment size */
  int maxdelay;    /* maximum allowable delay, or negative if delay cannot be
		      changed in runtime */
  int curdelay;    /* current delay */
  int curbuf;      /* index of current full-sized buffer */
  int n_fbufs;     /* number of full-sized buffers currently used */
  int n_fbufs_cap; /* number of full-sized buffers allocated */
  void **fbufs;    /* full-sized buffers */
  int n_rest;      /* samples in rest buffer */
  void *rbuf;      /* rest buffer */
  void *shortbuf[2]; /* pointers to buffers which fit the whole delay, only
			used when delay is <= fragment size */
};

typedef struct _delaybuffer_t_ delaybuffer_t;

struct subdev {
  volatile bool finished;
  bool isinterleaved;
  bool bad_alignment;
  int index;
  int fd;
  int buf_size;
  int buf_offset;
  int buf_left;
  int block_size;
  int block_size_frames;
  struct dai_channels channels;
  delaybuffer_t **db;
  struct {
    int iodelay_fill;
    int curbuf;
    int frames_left;
  } cb;
};

struct iodev {
  int virtual_channels;
  vector<int> channel_intname;
  vector<string> channel_name;
  vector<int> virt2phys;
  struct dai_channels ch; /* physical channels */
  xmlNodePtr device_params;
  int maxdelay;
  bool apply_dither;
  bool auto_format;
};

struct dai_subdevice {
  struct dai_channels channels;
  int i_handle;
  bool uses_clock;
  int sched_policy;
  struct sched_param sched_param;
  int module;
};

struct dai_buffer_format {
  int n_bytes;
  int n_samples;
  int n_channels;
  struct buffer_format bf[SF_MAXCHANNELS];
};

struct sfconf {
  double cpu_mhz;
  int n_cpus;
  int sampling_rate;
  int filter_length;
  int n_blocks;
  int flowthrough_blocks;
  int realtime_maxprio;
  int realtime_midprio;
  int realtime_usermaxprio;
  int realtime_minprio;
  int realsize;    
  bool powersave;
  double analog_powersave;
  bool overflow_control;
  bool benchmark;
  bool debug;
  bool quiet;
  bool overflow_warnings;
  bool show_progress;
  bool realtime_priority;
  bool lock_memory;
  bool synched_write;
  bool allow_poll_mode;
  vector<struct dither_state*> dither_state;
  int n_coeffs;
  vector<struct sfcoeff> coeffs;
  void ***coeffs_data;
  int n_channels[2];
  vector<struct sfchannel> channels[2];
  int n_physical_channels[2];
  vector<int> n_virtperphys[2];
  vector< vector<int> > phys2virt[2];
  vector<int> virt2phys[2];
  struct dai_subdevice subdevs[2];
  vector<int> delay[2];
  vector<int> maxdelay[2];
  vector<bool> mute[2];
  int n_filters;
  vector<struct sffilter> filters;
  vector<struct sffilter_control> initfctrl;
  struct filter_process fproc;
  IO *io;
  int n_logicmods;
  vector<string> logicnames;
};

struct apply_subdelay_params {
  int subdelay;
  void *rest;    
};

#endif
