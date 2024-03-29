/*
 * (c) Copyright 2013/2022 -- Raul Fernandez Ortega
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */

#include "convolver.hpp"

#if defined(__ARCH_IA32__)  || defined(__ARCH_X86_64__) || defined(__ARCH_IA64__)
inline void cpuid(uint32_t op,
			 uint32_t *eax,
			 uint32_t *ebx,
			 uint32_t *ecx,
			 uint32_t *edx)
{
    asm volatile ("cpuid" : "=a" (*eax), "=b" (*ebx), "=c" (*ecx),"=d" (*edx) : "a" (op));
}
int decide_opt_code(int realsize)
{
  uint32_t level, junk, cap;
  numunion_t vendor[2];
  int opt_code;

  opt_code = OPT_CODE_GCC;
  
  vendor[1].u8[4] = '\0';
  cpuid(0x00000000, &level, &vendor->u32[0], &vendor->u32[2], &vendor->u32[1]);
  if (strcmp((char *)vendor->u8, "GenuineIntel") == 0 && level >= 0x00000001) {
    cpuid(0x00000001, &junk, &junk, &junk, &cap);
    if (realsize == 8) {
      if ((cap & (1 << 26)) != 0) {
	opt_code = OPT_CODE_SSE2;
	pinfo("SSE2 capability detected -- optimisation enabled.\n");
      }
    } else {
      if ((cap & (1 << 25)) != 0) {
	opt_code = OPT_CODE_SSE;
	pinfo("SSE capability detected -- optimisation enabled.\n");
      }
    }
  }
  return opt_code;
}
#else
int decide_opt_code(void)
{
  return OPT_CODE_GCC;
}
#endif


void *create_fft_plan(int realsize, int length, bool inplace, bool invert)
{
  void *plan, *buf[2];
  
  buf[0] = emallocaligned(length * realsize);
  memset(buf[0], 0, length * realsize);
  buf[1] = buf[0];
  if (!inplace) {
    buf[1] = emallocaligned(length * realsize);
    memset(buf[1], 0, length * realsize);
  }
  if (realsize == 4) {
    plan = (float*)fftwf_plan_r2r_1d(length, (float*)buf[0], (float*)buf[1], invert ? FFTW_HC2R : FFTW_R2HC, FFTW_MEASURE||FFTW_PATIENT);
  } else {
    plan = (double*)fftw_plan_r2r_1d(length, (double*)buf[0], (double*)buf[1],invert ? FFTW_HC2R : FFTW_R2HC, FFTW_MEASURE||FFTW_PATIENT);
  }
  efree(buf[0]);
  if (!inplace) {
    efree(buf[1]);
  }
  return plan;
}

#define real_t float
#define REALSIZE 4
#define RAW2REAL_NAME raw2realf
#define MIXNSCALE_NAME mixnscalef
#define CONVOLVE_INPLACE_NAME convolve_inplacef
#define CONVOLVE_NAME convolvef
#define CONVOLVE_ADD_NAME convolve_addf
#define DIRAC_CONVOLVE_INPLACE_NAME dirac_convolve_inplacef
#define DIRAC_CONVOLVE_NAME dirac_convolvef
//#include "raw2real.h"
#include "fftw_convfuns.h"
#undef real_t
#undef REALSIZE
#undef RAW2REAL_NAME
#undef MIXNSCALE_NAME
#undef CONVOLVE_INPLACE_NAME
#undef CONVOLVE_NAME
#undef CONVOLVE_ADD_NAME
#undef DIRAC_CONVOLVE_INPLACE_NAME
#undef DIRAC_CONVOLVE_NAME

#define real_t double
#define REALSIZE 8
#define RAW2REAL_NAME raw2reald
#define MIXNSCALE_NAME mixnscaled
#define CONVOLVE_INPLACE_NAME convolve_inplaced
#define CONVOLVE_NAME convolved
#define CONVOLVE_ADD_NAME convolve_addd
#define DIRAC_CONVOLVE_INPLACE_NAME dirac_convolve_inplaced
#define DIRAC_CONVOLVE_NAME dirac_convolved
//#include "raw2real.h"
#include "fftw_convfuns.h"
#undef real_t
#undef REALSIZE
#undef RAW2REAL_NAME
#undef MIXNSCALE_NAME
#undef CONVOLVE_INPLACE_NAME
#undef CONVOLVE_NAME
#undef CONVOLVE_ADD_NAME
#undef DIRAC_CONVOLVE_INPLACE_NAME
#undef DIRAC_CONVOLVE_NAME

Convolver::Convolver(int length, int _realsize)
{
  realsize = _realsize;
  if(length > 0) {
    convolver_init(length, realsize);
  }
}

Convolver::~Convolver(void)
{
}
 
void Convolver::convolver_time2freq(void *input_cbuf,void *output_cbuf)
{
  void *fftplan;

  if (input_cbuf == output_cbuf) {
    fftplan = fftplans_inplace[fft_order];
  } else {
    fftplan = fftplans[fft_order];
  }
  if (realsize == 4) {
    fftwf_execute_r2r((const fftwf_plan)fftplan, (float *)input_cbuf, (float *)output_cbuf);
  } else {
    fftw_execute_r2r((const fftw_plan)fftplan, (double *)input_cbuf, (double *)output_cbuf);
  }
}

 
void Convolver::convolver_mixnscale(void *input_cbufs[],
				    void *output_cbuf,
				    vector<double> scales,
				    int n_bufs,
				    int mixmode)
{
  if (realsize == 4) {
    mixnscalef(input_cbufs, output_cbuf, scales, n_bufs, mixmode, n_fft);
  } else {
    mixnscaled(input_cbufs, output_cbuf, scales, n_bufs, mixmode, n_fft);
  }
}

 
void Convolver::convolver_convolve_inplace(void *cbuf, void *coeffs)
{
  if (realsize == 4) {
    convolve_inplacef(cbuf, coeffs, n_fft);
  } else {
    convolve_inplaced(cbuf, coeffs, n_fft);
  }
}

 
void Convolver::convolver_convolve(void *input_cbuf, void *coeffs, void *output_cbuf)
{
  if (realsize == 4) {
    convolvef(input_cbuf, coeffs, output_cbuf, n_fft);
  } else {
    convolved(input_cbuf, coeffs, output_cbuf, n_fft);
  }
}


void Convolver::convolver_convolve_add(void *input_cbuf, void *coeffs, void *output_cbuf)
{
/*
  real_t *_b = (real_t *)emallocaligned(n_fft * sizeof(real_t));
  real_t *_c = (real_t *)emallocaligned(n_fft * sizeof(real_t));
  real_t *_d = (real_t *)emallocaligned(n_fft * sizeof(real_t));
  
  memcpy(_b, input_cbuf, n_fft * sizeof(real_t));
  memcpy(_c, coeffs, n_fft * sizeof(real_t));
  memcpy(_d, output_cbuf, n_fft * sizeof(real_t));
*/
  switch (opt_code) {
#if defined(__ARCH_IA32__)  || defined(__ARCH_X86_64__) || defined(__ARCH_IA64__)
  case OPT_CODE_SSE:
    convolver_sse_convolve_add(input_cbuf, coeffs, output_cbuf, n_fft >> 3);
    break;
#ifdef __SSE2__
  case OPT_CODE_SSE2:
    convolver_sse2_convolve_add(input_cbuf, coeffs, output_cbuf, n_fft >> 4);
    break;
#endif
#endif	
  default:
  case OPT_CODE_GCC:
    if (realsize == 4) {
      convolve_addf(input_cbuf, coeffs, output_cbuf, n_fft);
    } else {
      convolve_addd(input_cbuf, coeffs, output_cbuf, n_fft);
    }
  }
/*
  {
  real_t d1s, d2s, err, e;
  int n;
  
  d1s = _d[0] + _b[0] * _c[0];
  d2s = _d[4] + _b[4] * _c[4];
  for (n = 0; n < n_fft; n += 8) {
  _d[n+0] += _b[n+0] * _c[n+0] - _b[n+4] * _c[n+4];
  _d[n+1] += _b[n+1] * _c[n+1] - _b[n+5] * _c[n+5];
  _d[n+2] += _b[n+2] * _c[n+2] - _b[n+6] * _c[n+6];
  _d[n+3] += _b[n+3] * _c[n+3] - _b[n+7] * _c[n+7];
  
  _d[n+4] += _b[n+0] * _c[n+4] + _b[n+4] * _c[n+0];
  _d[n+5] += _b[n+1] * _c[n+5] + _b[n+5] * _c[n+1];
  _d[n+6] += _b[n+2] * _c[n+6] + _b[n+6] * _c[n+2];
  _d[n+7] += _b[n+3] * _c[n+7] + _b[n+7] * _c[n+3];
  }
  _d[0] = d1s;
  _d[4] = d2s;
  
  err = 0;
  for (n = 0; n < n_fft; n++) {
  e = (((real_t *)output_cbuf)[n] - _d[n]);
  if (e < 0.0) {
  err -= e;
  } else {
  err += e;
  }
  }
  fprintf(stderr, "err: %.10e\n", err);
  efree(_b);
  efree(_c);
  efree(_d);
  }
*/
}
  
void Convolver::convolver_crossfade_inplace(void *input_cbuf, void *crossfade_cbuf, void *buffer_cbuf)
{
  void *buf1, *buf2;
  double d;
  vector <double> scale;
  float f;
  int n;
  
  buf1 = buffer_cbuf;
  buf2 = &((uint8_t *)buffer_cbuf)[n_fft * realsize];
  scale.push_back(1.0);
  convolver_mixnscale(&crossfade_cbuf, buffer_cbuf, scale, 1, CONVOLVER_MIXMODE_OUTPUT);
  convolver_freq2time(buffer_cbuf, crossfade_cbuf);
  convolver_mixnscale(&input_cbuf, buffer_cbuf, scale, 1, CONVOLVER_MIXMODE_OUTPUT);
  convolver_freq2time(buffer_cbuf, buffer_cbuf);
  if (realsize == 4) {
    f = 1.0 / (float)(n_fft2 - 1);
    for (n = 0; n < n_fft2; n++) {
      ((float *)buffer_cbuf)[n] = ((float *)crossfade_cbuf)[n] * (1.0 - f * (float)n) + ((float *)buffer_cbuf)[n] * f * (float)n;
    }
  } else {
    d = 1.0 / (double)(n_fft2 - 1);
    for (n = 0; n < n_fft2; n++) {
      ((double *)buf1)[n] = ((double *)buf1)[n] * (1.0 - d * (double)n) + ((double *)buf2)[n] * d * (double)n;
    }
  }
  convolver_time2freq(buffer_cbuf, buffer_cbuf);
  for (n = 0; n < (int)scale.size(); n++)
    scale[n] /= (double)n_fft;
  convolver_mixnscale(&buffer_cbuf, input_cbuf, scale, 1, CONVOLVER_MIXMODE_INPUT);
}

 
void Convolver::convolver_dirac_convolve_inplace(void *cbuf)
{
  if (realsize == 4) {
    dirac_convolve_inplacef(cbuf, n_fft);
  } else {
    dirac_convolve_inplaced(cbuf, n_fft);
  }
}

 
void Convolver::convolver_dirac_convolve(void *input_cbuf, void *output_cbuf)
{
  if (realsize == 4) {
    dirac_convolvef(input_cbuf, output_cbuf, n_fft);
  } else {
    dirac_convolved(input_cbuf, output_cbuf, n_fft);
  }
}

 
void Convolver::convolver_freq2time(void *input_cbuf, void *output_cbuf)
{
  void *ifftplan;

  if (input_cbuf == output_cbuf) {
    ifftplan = ifftplans_inplace[fft_order];
  } else {
    ifftplan = ifftplans[fft_order];
  }
  if (realsize == 4) {
    fftwf_execute_r2r((const fftwf_plan)ifftplan, (float *)input_cbuf, (float *)output_cbuf);
  } else {
    fftw_execute_r2r((const fftw_plan)ifftplan, (double *)input_cbuf, (double *)output_cbuf);
  }
}

 
void Convolver::convolver_convolve_eval(void *input_cbuf,
					void *buffer_cbuf, /* 1.5 x size */
					void *output_cbuf)
{
  if (realsize == 4) {
    fftwf_execute_r2r((const fftwf_plan)ifftplans[fft_order], (float *)input_cbuf, &((float *)buffer_cbuf)[n_fft2]);
    fftwf_execute_r2r((const fftwf_plan)fftplans[fft_order], (float *)buffer_cbuf, (float *)output_cbuf);
  } else {
    fftw_execute_r2r((const fftw_plan)ifftplans[fft_order], (double *)input_cbuf, &((double *)buffer_cbuf)[n_fft2]);
    fftw_execute_r2r((const fftw_plan)fftplans[fft_order], (double *)buffer_cbuf, (double *)output_cbuf);
  }
  memcpy(buffer_cbuf, &((uint8_t *)buffer_cbuf)[n_fft2 * realsize], n_fft2 * realsize);
}
 
int Convolver::convolver_cbufsize(void)
{
  return n_fft * realsize;
}

 
void * Convolver::convolver_coeffs2cbuf(void *coeffs,
					int n_coeffs,
					double scale)
{
  vector <double> vscale;
  void *rcoeffs, *coeffs_data;
  int n, len;
  
  len = (n_coeffs > n_fft2) ? n_fft2 : n_coeffs;
  rcoeffs = emallocaligned(n_fft * realsize);
  memset(rcoeffs, 0, n_fft * realsize);
  
  if (realsize == 4) {
    for (n = 0; n < len; n++) {
      ((float *)rcoeffs)[n_fft2 + n] = ((float *)coeffs)[n] * (float)scale;
      if (!finite((double)((float *)rcoeffs)[n_fft2 + n])) {
	fprintf(stderr, "NaN or Inf value among coefficients.\n");
	return NULL;
      }
    }
    fftwf_execute_r2r((const fftwf_plan)fftplans_inplace[fft_order], (float *)rcoeffs, (float *)rcoeffs);
  } else {
    for (n = 0; n < len; n++) {
      ((double *)rcoeffs)[n_fft2 + n] = ((double *)coeffs)[n] * scale;
      if (!finite(((double *)rcoeffs)[n_fft2 + n])) {
	fprintf(stderr, "NaN or Inf value among coefficients.\n");
	return NULL;
      }
    }
    fftw_execute_r2r((const fftw_plan)fftplans_inplace[fft_order], (double *)rcoeffs, (double *)rcoeffs);
  }
  
  scale = 1.0 / (double)n_fft;
  coeffs_data = emallocaligned(n_fft * realsize);
  vscale.push_back(scale);
  convolver_mixnscale(&rcoeffs, coeffs_data, vscale, 1, CONVOLVER_MIXMODE_INPUT);
  efree(rcoeffs);
  
  return coeffs_data;
}

 
void Convolver::convolver_runtime_coeffs2cbuf(void *src,  /* nfft / 2 */
					      void *dest) /* nfft */
{
  vector <double> scale;
  static void *tmp = NULL;
  
  if (tmp == NULL) {
    tmp = emallocaligned(n_fft * realsize);
  }
  memset(dest, 0, n_fft2 * realsize);
  memcpy(&((uint8_t *)dest)[n_fft2 * realsize], src, n_fft2 * realsize);
  if (realsize == 4) {
    fftwf_execute_r2r((const fftwf_plan)fftplans[fft_order], (float *)dest, (float *)tmp);
  } else {
    fftw_execute_r2r((const fftw_plan)fftplans[fft_order], (double *)dest, (double *)tmp);
  }
  scale.push_back(1.0 / (double)n_fft);
  convolver_mixnscale(&tmp, dest, scale, 1, CONVOLVER_MIXMODE_INPUT);
}

 
bool Convolver::convolver_verify_cbuf(void *cbufs[], int n_cbufs)
{
  int n, i;
  
  for (n = 0; n < n_cbufs; n++) {
    if (realsize == 4) {
      for (i = 0; i < n_fft; i++) {
	if (!finite((double)((float *)cbufs[n])[i])) {
	  fprintf(stderr, "NaN or Inf value among coefficients.\n");
	  return false;
	}
      }
    } else {
      for (i = 0; i < n_fft; i++) {
	if (!finite(((double *)cbufs[n])[i])) {
	  fprintf(stderr, "NaN or Inf value among coefficients.\n");
	  return false;
	}
      }
    }
  }
  return true;
}

 
void Convolver::convolver_debug_dump_cbuf(const char filename[], void *cbufs[], int n_cbufs)
{
  void *coeffs;
  vector<double> scale;
  FILE *stream;
  int n, i;
  
  if ((stream = fopen(filename, "wt")) == NULL) {
    fprintf(stderr, "Could not open \"%s\" for writing: %s", filename, strerror(errno));
    return;
  }
  coeffs = emallocaligned(n_fft * realsize);
  scale.push_back(1.0);    
  for (n = 0; n < n_cbufs; n++) {
    convolver_mixnscale(&cbufs[n], coeffs, scale, 1, CONVOLVER_MIXMODE_OUTPUT);
    if (realsize == 4) {
      fftwf_execute_r2r((const fftwf_plan)ifftplans_inplace[fft_order], (float *)coeffs, (float *)coeffs);
      for (i = 0; i < n_fft2; i++) {
	fprintf(stream, "%.16e\n", ((float *)coeffs)[n_fft2 + i]);
      }
    } else {
      fftw_execute_r2r((const fftw_plan)ifftplans_inplace[fft_order], (double *)coeffs, (double *)coeffs);
      for (i = 0; i < n_fft2; i++) {
	fprintf(stream, "%.16e\n", ((double *)coeffs)[n_fft2 + i]);
      }
    }
  }
  efree(coeffs);
  fclose(stream);
}

 
void * Convolver::convolver_fftplan(int order, int invert, int inplace)
{
  invert = !!invert;
  inplace = !!inplace;
  if (!bit_isset(&fftplan_generated[invert][inplace], order)) {
    /*pinfo("Creating %s%sFFTW plan of size %d using wisdom...",
      invert ? "inverse " : "forward ",
      inplace ? "inplace " : "",
      1 << order);*/
    fprintf(stderr, "Creating %s%sFFTW plan of size %d using wisdom...",
	    invert ? "inverse " : "forward ", inplace ? "inplace " : "", 1 << order);
    fftplan_table[invert][inplace][order] = create_fft_plan(realsize, 1 << order, inplace, invert);
    //pinfo("finished\n");
    fprintf(stderr, "finished\n");
    bit_set(&fftplan_generated[invert][inplace], order);
  }
  return fftplan_table[invert][inplace][order];
}

struct _td_conv_t_ {
  void *fftplan;
  void *ifftplan;    
  void *coeffs;
  int blocklen;
};

int Convolver::convolver_td_block_length(int n_coeffs)
{
  if (n_coeffs < 1) {
    return -1;
  }
  return 1 << log2_roof(n_coeffs);
}
 
td_conv_t* Convolver::convolver_td_new(void *coeffs, int n_coeffs)
{
  int blocklen, n;
  td_conv_t *tdc;
  double scaled;
  float scalef;
  
  if ((blocklen = convolver_td_block_length(n_coeffs)) == -1) {
    return NULL;
  }
  tdc = (_td_conv_t_*)emalloc(sizeof(td_conv_t));
  memset(tdc, 0, sizeof(td_conv_t));
  tdc->fftplan = convolver_fftplan(log2_get(blocklen) + 1, false, true);
  tdc->ifftplan = convolver_fftplan(log2_get(blocklen) + 1, true, true);
  tdc->coeffs = emallocaligned(2 * blocklen * realsize);
  tdc->blocklen = blocklen;
  
  memset(tdc->coeffs, 0, blocklen * realsize);
  memset(&((uint8_t *)tdc->coeffs)[(blocklen + n_coeffs) * realsize], 0, (blocklen - n_coeffs) * realsize);
  memcpy(&((uint8_t *)tdc->coeffs)[blocklen * realsize], coeffs, n_coeffs * realsize);
  if (realsize == 4) {
    fftwf_execute_r2r((fftwf_plan_s*)tdc->fftplan, (float*)tdc->coeffs, (float*)tdc->coeffs);
    scalef = 1.0 / (float)(blocklen << 1);
    for (n = 0; n < blocklen << 1; n++) {
      ((float *)tdc->coeffs)[n] *= scalef;
    }
  } else {
    fftw_execute_r2r((fftw_plan_s*)tdc->fftplan, (double*)tdc->coeffs, (double*)tdc->coeffs);
    scaled = 1.0 / (double)(blocklen << 1);
    for (n = 0; n < blocklen << 1; n++) {
      ((double *)tdc->coeffs)[n] *= scaled;
    }
  }
  return tdc;
}
 
void Convolver::convolve_inplace_ordered(void *cbuf, void *coeffs, int size)
{
  int n, size2 = size >> 1;
  if (realsize == 4) {
    float a, *b = (float *)cbuf, *c = (float *)coeffs;
    
    b[0] *= c[0];
    for (n = 1; n < size2; n++) {
      a = b[n];
      b[n] = a * c[n] - b[size - n] * c[size - n];
      b[size - n] = a * c[size - n] + b[size - n] * c[n];
    }
    b[size2] *= c[size2];
  } else {
    double a, *b = (double *)cbuf, *c = (double *)coeffs;
    
    b[0] *= c[0];
    for (n = 1; n < size2; n++) {
      a = b[n];
      b[n] = a * c[n] - b[size - n] * c[size - n];
      b[size - n] = a * c[size - n] + b[size - n] * c[n];
    }
    b[size2] *= c[size2];
  }
}
 
void Convolver::convolver_td_convolve(td_conv_t *tdc, void *overlap_block)
{
  if (realsize == 4) {
    fftwf_execute_r2r((fftwf_plan_s*)tdc->fftplan, (float*)overlap_block, (float*)overlap_block);
    convolve_inplace_ordered(overlap_block, tdc->coeffs, tdc->blocklen << 1);
    fftwf_execute_r2r((fftwf_plan_s*)tdc->ifftplan, (float*)overlap_block, (float*)overlap_block);
  } else {
    fftw_execute_r2r((fftw_plan_s*)tdc->fftplan, (double*)overlap_block, (double*)overlap_block);
    convolve_inplace_ordered(overlap_block, tdc->coeffs, tdc->blocklen << 1);
    fftw_execute_r2r((fftw_plan_s*)tdc->ifftplan, (double*)overlap_block, (double*)overlap_block);
  }
}
 
bool Convolver::convolver_init(int length, int _realsize)
{
  int order;
  // int order, ret;
  //FILE *stream;
  //bool quiet;
  
  realsize = _realsize;
  opt_code = decide_opt_code(realsize);
  
  if (realsize != 4 && realsize != 8) {
    fprintf(stderr, "Invalid real size %d.\n", realsize);
    return false;
  }
  if ((order = log2_get(length)) == -1) {
    fprintf(stderr, "Invalid length %d.\n", length);
    return false;
  }
  order++;
  fft_order = order;
  n_fft = 2 * length;
  n_fft2 = length;
  
  /*if ((stream = fopen(config_filename, "rt")) == NULL) {
    if (errno != ENOENT) {
      fprintf(stderr, "Could not open \"%s\" for reading: %s.\n", config_filename, strerror(errno));
      return false;
    }
  } else {
    if (realsize == 4) {
      ret = fftwf_import_wisdom_from_file(stream);
    } else {
      ret = fftw_import_wisdom_from_file(stream);
    }
    fclose(stream);
    }*/
  
  memset(fftplan_generated, 0, sizeof(fftplan_generated));
  fprintf(stderr, "Creating 4 FFTW plans of size %d...\n", 1 << fft_order);
  convolver_fftplan(fft_order, false, false);
  convolver_fftplan(fft_order, false, true);
  convolver_fftplan(fft_order, true, false);
  convolver_fftplan(fft_order, true, true);
  fprintf(stderr, "finished.\n");
  
  /* Wisdom is cumulative, save it each time (and get wiser) */
  /*if ((stream = fopen(config_filename, "wt")) == NULL) {
    fprintf(stderr, "Warning: could not save wisdom:\n  could not open \"%s\" for writing: %s.\n", config_filename, strerror(errno));
  } else {
    if (realsize == 4) {
      fftwf_export_wisdom_to_file(stream);
    } else {
      fftw_export_wisdom_to_file(stream);
    }
    fclose(stream);
    }*/
  
  return true;
}
