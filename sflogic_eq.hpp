/*
 * (c) Copyright 2013 -- Raul Fernandez Ortega
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */

#ifndef _SFLOGIC_EQ_HPP_
#define _SFLOGIC_EQ_HPP_

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

#define MAX_EQUALISERS 64
#define MAX_BANDS 128
#define MSGSIZE (MAX_BANDS * 20)

#define CMD_CHANGE_MAGNITUDE 1
#define CMD_CHANGE_PHASE 2
#define CMD_GET_INFO 3

inline void trim(string& str)
{
  size_t pos = str.find_last_not_of(' ');
  if(pos != string::npos) {
    str.erase(pos + 1);
    pos = str.find_first_not_of(' ');
    if(pos != string::npos) str.erase(0, pos);
  }
  else str.erase(str.begin(), str.end());
};

struct realtime_eq {
  void *ifftplan;
  int band_count;
  int taps;
  bool minphase;
  volatile int coeff[2];
  volatile int active_coeff;
  volatile bool not_changed;
  double *freq;
  double *mag;
  double *phase;
};

class SFLOGIC_EQ : public SFLOGIC {

private:

  struct realtime_eq *equalisers;
  int n_equalisers;
  int sample_rate;
  //int block_length;
  int n_maxblocks;
  int n_coeffs;
  int n_filters;
  //const struct sfcoeff *coeffs;
  char *debug_dump_filter_path;
  bool debug;
  void *rbuf;
  int render_postponed_index;
  bool std_bands;
  
  char * strtrim(char s[]);

  void coeff_final(int filter, int *coeff);

  bool finalise_equaliser(struct realtime_eq *eq,
			  double mfreq[],
			  double mag[],
			  int n_mag,
			  double pfreq[],
			  double phase[],
			  int n_phase,
			  double bands[],
			  int n_bands);

#define real_t float
#define REALSIZE 4
#define COSINE_INT_NAME cosine_int_f
#define DFT dft_f
#define IDFT idft_f
#define CONVOLVE_INPLACE_FFT convolve_inplace_fft_f
#define MINIMUMPHASE minimumphase_f
#define WRITEFILTER_NAME writefilter_f
#define RENDER_EQUALISER_NAME render_equaliser_f
#include "renderaux.hpp"
#include "rendereq.hpp"
#undef DFT
#undef IDFT
#undef CONVOLVE_INPLACE_FFT
#undef MINIMUMPHASE
#undef WRITEFILTER_NAME
#undef COSINE_INT_NAME
#undef RENDER_EQUALISER_NAME
#undef REALSIZE
#undef real_t

#define real_t double
#define REALSIZE 8
#define COSINE_INT_NAME cosine_int_d
#define DFT dft_d
#define IDFT idft_d
#define CONVOLVE_INPLACE_FFT convolve_inplace_fft_d
#define MINIMUMPHASE minimumphase_d
#define WRITEFILTER_NAME writefilter_d
#define RENDER_EQUALISER_NAME render_equaliser_d
#include "renderaux.hpp"
#include "rendereq.hpp"
#undef DFT
#undef IDFT
#undef CONVOLVE_INPLACE_FFT
#undef MINIMUMPHASE
#undef WRITEFILTER_NAME
#undef COSINE_INT_NAME
#undef RENDER_EQUALISER_NAME
#undef REALSIZE
#undef real_t

public:

  SFLOGIC_EQ(struct sfconf *_sfconf,
	     struct intercomm_area *_icomm,
	     SfAccess *_sfaccess);

  ~SFLOGIC_EQ(void) {};

  int preinit(xmlNodePtr sfparams, int _debug);

  int config(int n_eq, string _coeff_name, bool _minphase, string _std);

  int init(int event_fd, sem_t *synch_sem);

  bool change_magnitude(int n_eq, int nfreq, double mag);

  bool change_rendering(int n_eq);

  //int command(const char params[]);

  //const char *message(void);

};


#endif
