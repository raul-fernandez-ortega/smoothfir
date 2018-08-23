/*
 * (c) Copyright 2013 -- Raul Fernandez
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#ifndef _CONVOLVER_HPP_
#define _CONVOLVER_HPP_

#include "bit.h"

#ifdef __cplusplus
extern "C" {
#endif 

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>
#ifdef __OS_SUNOS__
#include <ieeefp.h>
#endif

#include <fftw3.h>

#include "log2.h"
#include "emalloc.h"
#include "swap.h"
#include "asmprot.h"
#include "timestamp.h"
#include "numunion.h"

#ifdef __cplusplus
}
#endif 

#include "sfstruct.h"
#include "sfmod.h"
#include "sfdither.h"
#include "sfinout.h"

#define ifftplans fftplan_table[1][0]
#define ifftplans_inplace fftplan_table[1][1]
#define fftplans fftplan_table[0][0]
#define fftplans_inplace fftplan_table[0][1]

#define OPT_CODE_GCC   0
#define OPT_CODE_SSE   1
#define OPT_CODE_SSE2  2
#define OPT_CODE_3DNOW 3
#define OPT_CODE_X87   4

#ifdef __cplusplus
extern "C" {
  void cpuid(uint32_t op, uint32_t *eax, uint32_t *ebx, uint32_t *ecx, uint32_t *edx);
  int decide_opt_code(int realsize);
}
#endif 

class Convolver {

private:
  void *fftplan_table[2][2][32];
  uint32_t fftplan_generated[2][2];
  int opt_code;

  int n_fft, n_fft2, fft_order;

public:

  int realsize;

  Convolver(int length, int realsize);
  ~Convolver(void);
  
  /* Transform from time-domain to frequency-domain. */
  void convolver_time2freq(void *input_cbuf,void *output_cbuf);

  /* Scale and mix in the frequency-domain. The 'mixmode' parameter may be used
     internally for possible reordering of data prior to or after convolution. */
#define CONVOLVER_MIXMODE_INPUT      1
#define CONVOLVER_MIXMODE_INPUT_ADD  2
#define CONVOLVER_MIXMODE_OUTPUT     3
  void convolver_mixnscale(void *input_cbufs[], void *output_cbuf, vector<double> scales, int n_bufs,int mixmode);

			  
/* Convolution in the frequency-domain, done in-place. */
  void convolver_convolve_inplace(void *cbuf, void *coeffs);

  /* Convolution in the frequency-domain. */
  void convolver_convolve(void *input_cbuf, void *coeffs, void *output_cbuf);

  /* Convolution in the frequency-domain, with the result added to the output. */
  void convolver_convolve_add(void *input_cbuf, void *coeffs, void *output_cbuf);

  void convolver_crossfade_inplace(void *input_cbuf, void *crossfade_cbuf, void *buffer_cbuf);

  /* Convolve with dirac pulse. */
  void convolver_dirac_convolve_inplace(void *cbuf);
  void convolver_dirac_convolve(void *input_cbuf, void *output_cbuf);

  /* Transform from frequency-domain to time-domain. */
  void convolver_freq2time(void *input_cbuf, void *output_cbuf);

  /* Evaluate convolution output by transforming it back to time-domain, do
     overlap-save and transform back to frequency-domain again. Used when filters
     are put in series. The 'buffer_cbuf' must be 1.5 times the cbufsize and must
     be cleared the first call, and be kept the following calls. Input and output
     buffers may be the same. */
  void convolver_convolve_eval(void *input_cbuf, void *buffer_cbuf, void *output_cbuf);

  /* Return the size of the convolver's internal format corresponding to the
     given number of samples. */
  int convolver_cbufsize(void);

  /* Convert a set of coefficients to the convolver's internal frequency domain
     format. */
  void * convolver_coeffs2cbuf(void *coeffs, int n_coeffs, double scale);

  /* Fast version of the above to be used in runtime */
  void convolver_runtime_coeffs2cbuf(void *src, void *dest);

  /* Make a quick sanity check */
  bool convolver_verify_cbuf(void *cbufs[], int n_cbufs);

  /* Dump the contents of a cbuf to a text-file (after conversion back to
     coefficient list) */
  void convolver_debug_dump_cbuf(const char filename[], void *cbufs[], int n_cbufs);

  /* Get a plan for the FFT lib used. Do not free it. */
  void *convolver_fftplan(int order, int invert, int inplace);

  typedef struct _td_conv_t_ td_conv_t;

  int convolver_td_block_length(int n_coeffs);

  td_conv_t * convolver_td_new(void *coeffs, int n_coeffs);

  void convolve_inplace_ordered(void *cbuf, void *coeffs, int size);

  void convolver_td_convolve(td_conv_t *tdc, void *overlap_block);

  bool convolver_init(int length, int realsize);

};
#endif
