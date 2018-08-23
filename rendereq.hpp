/*
 * (c) Copyright 2013 -- Raul Fernandez Ortega
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */

void RENDER_EQUALISER_NAME(struct realtime_eq *eq)
{    
  real_t mag, rad, curfreq, scale, divtaps, tapspi;
  real_t *eqmag, *eqfreq, *eqphase;
  struct timeval tv1, tv2;
  int n, i;
  real_t *minbuf;
  /* poll until ready */
  /*while (eq->not_changed) {
    usleep(10000);
    }*/
  
  gettimeofday(&tv1, NULL);
  /* generate smoothed frequency domain filter */
  //eqmag = alloca(eq->band_count * sizeof(real_t));
  //eqfreq = alloca(eq->band_count * sizeof(real_t));
  //eqphase = alloca(eq->band_count * sizeof(real_t));
  eqmag = new real_t [eq->band_count];
  eqfreq = new real_t [eq->band_count];
  eqphase = new real_t [eq->band_count];
  for (n = 0; n < eq->band_count; n++) {
    eqmag[n] = (real_t)eq->mag[n];
    eqfreq[n] = (real_t)eq->freq[n];
    eqphase[n] = (real_t)eq->phase[n];
  }
  scale = 1.0 / (real_t)eq->taps;
  divtaps = 1.0 / (real_t)eq->taps;
  tapspi = -(real_t)eq->taps * M_PI;
  ((real_t *)rbuf)[0] = eqmag[0] * scale;
  for (n = 1, i = 0; n < eq->taps >> 1; n++) {
    curfreq = (real_t)n * divtaps;
    while (curfreq > eqfreq[i+1]) {
      i++;
    }
    mag = COSINE_INT_NAME(eqmag[i], eqmag[i+1], eqfreq[i], eqfreq[i+1], curfreq) * scale;
    rad = tapspi * curfreq + COSINE_INT_NAME(eqphase[i], eqphase[i+1], eqfreq[i], eqfreq[i+1], curfreq);
    ((real_t *)rbuf)[n] = cos(rad) * mag;
    ((real_t *)rbuf)[eq->taps-n] = sin(rad) * mag;
  }
  ((real_t *)rbuf)[eq->taps>>1] = eqmag[eq->band_count - 1] * scale;

  /* convert to time-domain */
#if REALSIZE == 4
  fftwf_execute_r2r((const fftwf_plan)eq->ifftplan, (real_t *)rbuf, (real_t *)rbuf);
#elif REALSIZE == 8
  fftw_execute_r2r((const fftw_plan)eq->ifftplan, (real_t *)rbuf, (real_t *)rbuf);
#else
#error invalid REALSIZE
#endif
  if(eq->minphase)
    minbuf = MINIMUMPHASE((real_t*)rbuf, eq->taps); 
  else
    minbuf = (real_t*)rbuf;
  /*
  if (debug_dump_filter_path != NULL) {
    snprintf(path, 1024, debug_dump_filter_path, eq->coeff[0]);
    path[1023] = '\0';
    stream = fopen(path, "wt");
    if (stream != NULL) {
      for (n = 0; n < eq->taps; n++) {
	fprintf(stream, "%.16e\n", ((real_t *)rbuf)[n]);
      }
      fclose(stream);
    }
  }*/
  // Dump new RACE Filter
  if (debug_dump_filter_path != NULL) {
    WRITEFILTER_NAME(debug_dump_filter_path, (real_t*)minbuf, eq->taps);
  }
  /* put to target BruteFIR coeffients */
  for (n = 0; n < sfconf->coeffs[eq->coeff[0]].n_blocks; n++) {
    sfaccess->convolver_coeffs2cbuf(&((real_t *)minbuf)[sfconf->filter_length * n], sfconf->coeffs_data[eq->coeff[!eq->active_coeff]][n]);
  }
  gettimeofday(&tv2, NULL);
  timersub(&tv2, &tv1, &tv1);
  eq->active_coeff = !eq->active_coeff;
  eq->not_changed = true;
  
  if (debug) {
    fprintf(stderr, "EQ: rendering coefficient set %d took %.2f ms\n", eq->coeff[eq->active_coeff], (double)tv1.tv_sec * 1000.0 + (double)tv1.tv_usec / 1000.0);
  }
}

