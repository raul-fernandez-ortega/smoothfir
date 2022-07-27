/*
 * (c) Copyright 2013/2022 -- Raul Fernandez Ortega
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */

#ifndef _SFLOGIC_RACE_HPP_
#define _SFLOGIC_RACE_HPP_

#include <vector>
#include <complex>

#ifdef __cplusplus
extern "C" {
#endif 

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <fftw3.h>

#include <sndfile.h>
  
#ifdef __cplusplus
}
#endif 


#define IS_SFLOGIC_MODULE

#include "sflogic.hpp"

#include "sfmod.h"
#include "emalloc.h"
#include "shmalloc.h"
#include "log2.h"
#include "fdrw.h"
#include "timermacros.h"

#define MSGSIZE (MAX_BANDS * 20)

typedef enum { PcmInt16Bit = 'I', PcmFloat32Bit = 'F', PcmFloat64Bit = 'D' } IFileType;

struct realtime_RACE {
  void *ifftplan;
  int band_count;
  int taps;
  volatile int direct_coeff;
  volatile int cross_coeff;
  volatile int active_coeff;
  bool not_changed;
  double *freq;
  double *mag;
  double *phase;
};

class SFLOGIC_RACE : public SFLOGIC {

private:

  struct realtime_RACE RACEFilter;
  float scale;
  int delay;
  float lowFreq;
  int lowSlope;
  float highFreq;
  int highSlope;
  int sample_rate;
  bool debug;
  void *d_rbuf;
  void *c_rbuf;
  int maxblocks;
  char *dump_direct_file;
  char *dump_cross_file;
  SNDFILE* sf_direct_file;
  SNDFILE* sf_cross_file;
  SF_INFO sf_info;
  int n_coeffs;
  int n_filters;
  int render_postponed_index;
  
  char * strtrim(char s[]);

  void coeff_final(int filter, int *coeff);

  bool finalise_RACE(void);
  
#define real_t float
#define REALSIZE 4
#define DFT dft_f
#define IDFT idft_f
#define CONVOLVE_INPLACE_FFT convolve_inplace_fft_f
#define MINIMUMPHASE minimumphase_f
#define WRITEFILTER_NAME writefilter_f
#define LOWPASSFIR_NAME lowpassfir_f
#define HIGHPASSFIR_NAME highpassfir_f
#define BANDPASSFIR_NAME bandpassfir_f
#define RENDER_RACE_NAME render_race_f
#define CONVOLVE_INPLACE_NAME convolve_inplace_f
#include "renderaux.hpp"
#include "renderrace.hpp"
#undef DFT
#undef IDFT
#undef CONVOLVE_INPLACE_FFT
#undef MINIMUMPHASE
#undef WRITEFILTER_NAME
#undef LOWPASSFIR_NAME
#undef HIGHPASSFIR_NAME
#undef BANDPASSFIR_NAME
#undef RENDER_RACE_NAME
#undef CONVOLVE_INPLACE_NAME
#undef REALSIZE
#undef complex_t
#undef real_t

#define real_t double
#define REALSIZE 8
#define DFT dft_d
#define IDFT idft_d
#define CONVOLVE_INPLACE_FFT convolve_inplace_fft_d
#define MINIMUMPHASE minimumphase_d
#define WRITEFILTER_NAME writefilter_d
#define LOWPASSFIR_NAME lowpassfir_d
#define HIGHPASSFIR_NAME highpassfir_d
#define BANDPASSFIR_NAME bandpassfir_d
#define RENDER_RACE_NAME render_race_d
#define CONVOLVE_INPLACE_NAME convolve_inplace_d
#include "renderaux.hpp"
#include "renderrace.hpp"
#undef DFT
#undef IDFT
#undef CONVOLVE_INPLACE_FFT
#undef MINIMUMPHASE
#undef WRITEFILTER_NAME
#undef LOWPASSFIR_NAME
#undef HIGHPASSFIR_NAME
#undef BANDPASSFIR_NAME
#undef RENDER_RACE_NAME
#undef CONVOLVE_INPLACE_NAME
#undef REALSIZE
#undef complex_t
#undef real_t

public:

  SFLOGIC_RACE(struct sfconf *_sfconf,
	       struct intercomm_area *_icomm,
	       SfAccess *_sfaccess);

  ~SFLOGIC_RACE(void) {};

  int preinit(xmlNodePtr sfparams, int _debug);

  int init(int event_fd, sem_t *synch_sem);

  bool set_config(string direct_coeff, 
		  string cross_coeff,
		  float nscale, 
		  float ndelay, 
		  float nlowFreq, 
		  float nlowSlope, 
		  float nhighFreq, 
		  float nhighSlope);

  bool change_config(float nscale, float ndelay, float nlowFreq, float nlowSlope, float nhighFreq, float nhighSlope);

};


#endif
