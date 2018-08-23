/*
 * (c) Copyright 2013 -- Raul Fernandez Ortega
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#ifndef _SFRUN_HPP_
#define _SFRUN_HPP_

#include "bit.h"

#ifdef __cplusplus
extern "C" {
#endif 

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/times.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <inttypes.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <math.h>
#include <sched.h>
#include <sys/mman.h>
#include <pthread.h>
#include <semaphore.h>
#ifdef __OS_SUNOS__
#include <ieeefp.h>
#endif

#include "emalloc.h"
#include "shmalloc.h"
#include "fdrw.h"
#include "log2.h"
#include "timestamp.h"
#include "pinfo.h"
#include "timermacros.h"

#ifdef __cplusplus
}
#endif 

#include <string>
#include <vector>

#include "sfclasses.hpp"
#include "sfdither.h"
#include "sfinout.h"
#include "sfdai.hpp"
#include "sfdelay.hpp"
#include "sfcallback.hpp"
#include "sfaccess.hpp"
#include "convolver.hpp"
#include "sflogic_cli.hpp"
#include "sflogic_eq.hpp"
#include "sflogic_ladspa.hpp"
#include "sflogic_race.hpp"
#include "sflogic_py.hpp"
#include "sflogic_recplay.hpp"

struct filter_process_input {
  void **inbuf;
  void **outbuf;
  void **input_freqcbuf;
  void **output_freqcbuf;
  sem_t *filter_read;
  sem_t *filter_write;
  sem_t *input_read;
  sem_t *cb_input_read;
  sem_t *output_write;
  sem_t *cb_output_write;
  int n_procinputs;
  int *procinputs;
  int n_procoutputs;
  int *procoutputs;
  int n_inputs;
  int *inputs;
  int n_outputs;
  int *outputs;
  int n_filters;
  struct sffilter *filters;
  int process_index;
  bool has_bl_input_devs;
  bool has_bl_output_devs;
  bool has_cb_input_devs;
  bool has_cb_output_devs;
};

struct output_process_input {
  sem_t filter_sem;
  sem_t synch_sem;
  sem_t input_sem;
  sem_t extra_input_sem;
  bool trigger_callback_io;
  bool checkdrift;
};

struct input_process_input {
  void **buf;
  sem_t filter_sem;
  sem_t output_sem;
  sem_t extra_output_sem;
 sem_t synch_sem;
};

class SfRun : public SfAccess, public SfCallback {

private:

  struct sfoverflow *reset_overflow_in;
  struct sfoverflow *reset_overflow_out;

  sem_t bl_output_2_bl_input_sem;
  sem_t bl_output_2_cb_input_sem;
  sem_t cb_output_2_bl_input_sem;
  sem_t cb_input_2_filter_sem;
  sem_t filter_2_cb_output_sem;
  sem_t mutex_sem;

  //int n_callback_devs[2];
  //int n_blocking_devs[2];
  int n_procinputs;
  int n_procoutputs;
  int process_index;
  int channels[2][SF_MAXCHANNELS];

  pthread_t sfrun_thread;
  pthread_t *module_thread;
  pthread_t dai_trigger_thread;

  sem_t synch_sem;
  sem_t *synch_module_sem;
  sem_t bl_input_2_filter_sem;
  sem_t filter_2_bl_output_sem;
  sem_t dai_block;

  void **input_freqcbuf, *input_freqcbuf_base;
  void **output_freqcbuf, *output_freqcbuf_base;

  time_t lastprinttime;
  uint32_t max_period_us;

  bool isRunning;

  void print_debug(void);
  //void init_events(void);
  void sighandler(int sig);
  static void* filter_process_thread(void *args);

public:

  Convolver *sfConv;
  Dai *sfDai;
  Delay *sfDelay;
  vector<SFLOGIC*> sflogic;

  struct sfconf* sfconf;
  struct intercomm_area* icomm;

  bool isinit;
  bool rti_isinit;

  struct module_init_input {
    int n;
    SFLOGIC *sflogic; 
    int event_fd;
    sem_t *synch_sem;
  };

  SfRun(struct sfconf* _sfconf, struct intercomm_area* _icomm);

  ~SfRun(void);

#define D icomm->debug

  bool ismuted(int io, int channel);
  void toggle_mute(int io, int channel);
  
  int set_delay(int io, int channel, int delay);
  int get_delay(int io, int channel);
  
  void print_overflows(void);
  void check_overflows(void);
  void rti_and_overflow(void);

  void icomm_mutex(int lock);
  bool memiszero(void *buf, int size);
  bool test_silent(void *buf, int size, int realsize, double analog_powersave, double scale);

  //void input_process(void *buf[2],sem_t filter_sem, sem_t output_sem, sem_t extra_output_sem, sem_t synch_sem);
  //void* input_process_thread(void *args);
  void* dai_trigger_callback_thread(void *args);

  //void output_process(sem_t filter_sem, sem_t synch_sem, sem_t input_sem, sem_t extra_input_sem, bool trigger_callback_io, bool checkdrift);
  //void* output_process_thread(void *args);

  //void apply_subdelay(void *realbuf, int n_samples, void *arg);

  void filter_process(void);
    
  void sf_callback_ready(int io);

  static void* module_init_thread(void *args);

  void preinit(Convolver *_sfConv, Delay *_sfDelay, vector<SFLOGIC*> _sflogic);

  void sfrun(Convolver *_sfConv, Delay *_sfDelay, vector<SFLOGIC*> _sflogic);

  //void post_start(void);

  double realtime_index(void);

  void reset_peak(void);

  //void
  //sf_register_process(pid_t pid);
  
  /*int sflogic_command(int modindex,
		      const char params[],
		      char **message);*/

  vector<string> sflogic_names(int *n_names);

  void sfio_range(int io,
		  int modindex,
		  int range[2]);

  void sf_make_realtime(pid_t pid,
			int priority,
			const char name[]);

/* Convert from raw sample format to the convolver's own time-domain format. */
  void convolver_raw2cbuf(void *rawbuf, void *cbuf, void *next_cbuf, struct buffer_format *sf, struct sfoverflow *overflow);

  void *convolver_fftplan(int order, int invert, int inplace) { 
    return sfConv->convolver_fftplan(order, invert, inplace); 
  };

  void convolver_coeffs2cbuf(void *src, void *dest) {
    return sfConv->convolver_runtime_coeffs2cbuf(src, dest);
  };

  /* Convert from the convolver's own time-domain format to raw sample format. */
  void convolver_cbuf2raw(void *cbuf, void *outbuf, struct buffer_format *sf, bool apply_dither, void *dither_state, struct sfoverflow *overflow);

  void sfstop(void);
  
};

#endif
