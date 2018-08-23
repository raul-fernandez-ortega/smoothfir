/*
 * (c) Copyright 2013 -- Raul Fernandez Ortega
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */

//#include <stdio.h>


void RENDER_RACE_NAME()
{    
  real_t dir_att, cross_att, att_step;
  real_t mag, rad, curfreq, divtaps, tapspi;
  real_t *eqmag, *eqfreq, *eqphase;
  struct timeval tv1, tv2;
  int n, i;
  fftw_complex *fftw_buf;
  real_t *minbuf, *rbuf;
  
  gettimeofday(&tv1, NULL);
  eqmag = new real_t [RACEFilter.band_count];
  eqfreq = new real_t [RACEFilter.band_count];
  eqphase = new real_t [RACEFilter.band_count];
  for (n = 0; n < RACEFilter.band_count; n++) {
    eqmag[n] = (real_t)RACEFilter.mag[n];
    eqfreq[n] = (real_t)RACEFilter.freq[n];
    eqphase[n] = (real_t)RACEFilter.phase[n];
  }
  divtaps = 1.0 / (real_t)RACEFilter.taps;
  tapspi = -(real_t)RACEFilter.taps * M_PI;

  att_step = pow(10., -scale/20);
  dir_att = att_step * att_step;
  cross_att = att_step;
  
  fftw_buf = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (RACEFilter.taps << 1) + 1);
  rbuf = (real_t*) fftw_malloc(sizeof(real_t) * RACEFilter.taps);
  
  memset(d_rbuf, 0, maxblocks * sfconf->filter_length * sfconf->realsize);
  memset(c_rbuf, 0, maxblocks * sfconf->filter_length * sfconf->realsize);

  for(n = delay; n < maxblocks * sfconf->filter_length - delay; n += 2*delay) {
    ((real_t*)d_rbuf)[n + delay] = dir_att;
    ((real_t*)c_rbuf)[n] = -cross_att;
    dir_att *= att_step * att_step;
    cross_att *= att_step * att_step;
  }

  // RACE Frequency Filter
  fftw_buf[n][0] = (double)eqmag[0] * divtaps;
  fftw_buf[n][1] = 0;
  for (n = 1, i = 0; n < (RACEFilter.taps << 1); n++) {
    curfreq = (real_t)n * divtaps;
    while (curfreq > eqfreq[i+1]) {
      i++;
    }
    mag = COSINE_INT_NAME(eqmag[i], eqmag[i+1], eqfreq[i], eqfreq[i+1], curfreq) * divtaps;
    rad = tapspi * curfreq + COSINE_INT_NAME(eqphase[i], eqphase[i+1], eqfreq[i], eqfreq[i+1], curfreq);
    fftw_buf[n][0] = (double)(cos(rad) * mag);
    fftw_buf[n][1] = (double)(sin(rad) * mag);
  }
  fftw_buf[RACEFilter.taps << 1][0] = (double)eqmag[RACEFilter.band_count - 1] * divtaps;
  fftw_buf[RACEFilter.taps << 1][1] = 0;

  IDFT(fftw_buf, rbuf, RACEFilter.taps);
  minbuf = MINIMUMPHASE(rbuf, RACEFilter.taps); 
 
  CONVOLVE_INPLACE_FFT((real_t*)d_rbuf, minbuf, RACEFilter.taps);
  CONVOLVE_INPLACE_FFT((real_t*)c_rbuf, minbuf, RACEFilter.taps);

  ((real_t*)d_rbuf)[0] = 1;

  // Dump new RACE Filter
  if (debug_dump_filter_path != NULL) {
    WRITEFILTER_NAME(debug_dump_filter_path, (real_t*)d_rbuf, sfconf->filter_length * sfconf->coeffs[RACEFilter.direct_coeff].n_blocks );
  }

  /* put to target BruteFIR coeffients */

  for (n = 0; n < sfconf->coeffs[RACEFilter.direct_coeff].n_blocks; n++) {
    sfaccess->convolver_coeffs2cbuf(&((real_t *)d_rbuf)[sfconf->filter_length * n], sfconf->coeffs_data[RACEFilter.direct_coeff][n]);
  }
  for (n = 0; n < sfconf->coeffs[RACEFilter.cross_coeff].n_blocks; n++) {
    sfaccess->convolver_coeffs2cbuf(&((real_t *)c_rbuf)[sfconf->filter_length * n], sfconf->coeffs_data[RACEFilter.cross_coeff][n]);
  }
  gettimeofday(&tv2, NULL);
  timersub(&tv2, &tv1, &tv1);
  
  if (debug) {
    fprintf(stderr, "RACE: rendering coefficient sets took %.2f ms\n",(double)tv1.tv_sec * 1000.0 + (double)tv1.tv_usec / 1000.0);
  }
}
