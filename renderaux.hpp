/*
 * (c) Copyright 2002 - 2005 -- Anders Torger
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */

#ifndef M_PI
#define M_PI ((double) 3.14159265358979323846264338327950288)
#endif

#ifndef M_2PI
#define M_2PI ((double) 6.28318530717958647692528676655900576)
#endif

#if REALSIZE == 4
#define FSIN_NAME sinf
#define FCOS_NAME cosf
#define FTAN_NAME tanf
#define FATAN_NAME atanf
#else
#define FSIN_NAME sin
#define FCOS_NAME cos
#define FTAN_NAME tan
#define FATAN_NAME atan
#endif

#if REALSIZE == 4
//Complex Absolute Value 
double cabs(double x, double y)
{
  return sqrt((x * x) + (y * y));
};

// Complex Exponential 
void cexp(double x, double y, double *zx, double *zy)
{
  double expx;
  
  expx = exp(x);
  *zx = expx * cos(y);
  *zy = expx * sin(y);
};
#endif

real_t COSINE_INT_NAME(real_t mag1,
		       real_t mag2,
		       real_t freq1,
		       real_t freq2,
		       real_t curfreq)
{
  return (mag1 - mag2) * 0.5 * cos(M_PI * (curfreq - freq1) / (freq2 - freq1)) + (mag1 + mag2) * 0.5;
}

/* Discrete Fourier Transform */
fftw_complex *DFT(real_t* inTime, int n)
{
  int i;
  fftw_complex* outFreq;
  fftw_plan fftplan;
  double* inBuf;

  inBuf = (double*) fftw_malloc(sizeof(double) * n);
  outFreq = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * (n >> 1) + 1);

  for(i = 0; i < n; i++) {
    inBuf[i] = (double)inTime[i];
  }
  fftplan = fftw_plan_dft_r2c_1d(n, inBuf, outFreq, FFTW_ESTIMATE);
  fftw_execute(fftplan);
  fftw_destroy_plan(fftplan);

  for(i = 0; i < (n >> 1) + 1; i++) {
    outFreq[i][0] /= (real_t)n;
    outFreq[i][1] /= (real_t)n;
  }
  return outFreq;
}

/* Inverse Discrete Fourier Transform */
void IDFT(fftw_complex *inFreq, real_t* outTime, int n)
{
  int i;
  fftw_plan fftplan;
  double *outBuf;
  outBuf = (double*) fftw_malloc(sizeof(double) * n);

  fftplan = fftw_plan_dft_c2r_1d(n, inFreq, outBuf, FFTW_ESTIMATE);
  fftw_execute(fftplan);
  fftw_destroy_plan(fftplan);

#if REALSIZE == 4
  for(i = 0; i < n; i++) {
    outTime[i] = (float)outBuf[i];
  }
#elif REALSIZE == 8
  for(i = 0; i < n; i++) {
    outTime[i] = outBuf[i];
  }
#else
#error invalid REALSIZE
#endif
}

void CONVOLVE_INPLACE_FFT(real_t* inSignal, real_t* inCoeffs, int size)
{
  int n;
  double *bufSignal, *bufCoeffs; 
  fftw_complex *freqSignal, *freqCoeffs;
  fftw_complex a;

  bufSignal = (double*) fftw_malloc(sizeof(double)*(size << 1));
  bufCoeffs = (double*) fftw_malloc(sizeof(double)*(size << 1));

  memset(bufSignal, 0, sizeof(double)*size << 1);
  memset(bufCoeffs, 0, sizeof(double)*size << 1);

  for(n = 0; n < size; n++) {
    bufSignal[n] = (double)inSignal[n];
    bufCoeffs[n] = (double)inCoeffs[n];
  }

  freqSignal = dft_d(bufSignal, size << 1);
  freqCoeffs = dft_d(bufCoeffs, size << 1);
  
  for(n = 0; n < size + 1; n++) {
      a[0] = freqSignal[n][0];
      a[1] = freqSignal[n][1];
      freqSignal[n][0] = (a[0] * freqCoeffs[n][0] - a[1] * freqCoeffs[n][1]) * (double)(size >> 1);
      freqSignal[n][1] = (a[0] * freqCoeffs[n][1] + a[1] * freqCoeffs[n][0]) * (double)(size >> 1);
    }	 
	idft_d(freqSignal, bufSignal,size << 1);
      for(n = 0; n < size; n++) {
	inSignal[n] = (double)bufSignal[n];
      }
  
}

/* Compute Minimum Phase Reconstruction Of Signal */
real_t* MINIMUMPHASE(real_t* inSignal, int size)
{
  fftw_complex* bufFreq;
  double *bufTime;
  real_t* realTime;
  int n;

  bufTime = (double*) fftw_malloc(sizeof(double) * size << 1);
  realTime = (real_t*) fftw_malloc(sizeof(real_t) * size);

  memset(bufTime, 0, sizeof(double)*size << 1);

  for(n = 0; n < size; n++) {
    bufTime[n] = (double)inSignal[n];
  }
 
  bufFreq = dft_d(bufTime, size << 1);
 
  /* Calculate Log Of Absolute Value */ 
  for(n = 0; n < size + 1; n++) {
    bufFreq[n][0] = log(cabs(bufFreq[n][0], bufFreq[n][1]));
    bufFreq[n][1] = 0.0;
  }

  idft_d(bufFreq, bufTime, size << 1);

  for(n = 1; n < size; n++) {
    bufTime[n] = 2.0 * bufTime[n];
  }
  for(n = size + 1; n < size << 1; n++) {
    bufTime[n] = 0;
  }

  bufFreq = dft_d(bufTime, size << 1);

  for(n = 0; n < size + 1; n++) {
    cexp(bufFreq[n][0], bufFreq[n][1], &bufFreq[n][0], &bufFreq[n][1]);
  }

  idft_d(bufFreq, bufTime, size << 1);

#if REALSIZE == 4
  for(n = 0; n < size; n++) {
    realTime[n] = (float)bufTime[n];
  }
#elif REALSIZE == 8
  for(n = 0; n < size; n++) {
    realTime[n] = bufTime[n];
  }
#else
#error invalid REALSIZE
#endif
  return realTime;

}

bool WRITEFILTER_NAME(const char * FName, const real_t *Src, const int SSize)
{
  FILE * IOF;
  int i;
  real_t RW;
  
  if ((IOF = fopen(FName,"wb")) == NULL) {
    fprintf(stderr, "\nUnable to open ouput file");
    return false;
  }
  
  for (i = 0; i < SSize; i++) {
    RW = (real_t) Src[i];
    if (fwrite((void *) &RW, sizeof(real_t), 1, IOF) != 1) {
      fprintf(stderr, "\nError writing ouput file");
      return false;
    }
  }
  fclose(IOF);
  return true;
}


void LOWPASSFIR_NAME(real_t *Filter, unsigned int Order, real_t Freq)
{
  unsigned int i, j, k;
  unsigned int HalfOrder = Order/2;
  real_t c = (real_t) (M_PI*Freq);
  real_t fv;

  if (Order > 1) {
    if (Order%2 == 0) {
      for(i = 1,j = HalfOrder - 1,k = HalfOrder; i <= HalfOrder;i++,j--,k++) {
	fv = (real_t) (i-((real_t) 0.5));
	fv = (real_t) (FSIN_NAME(fv*c)/fv*((real_t) M_PI));
	Filter[j] = fv;
	Filter[k] = fv;
      }
    } else {
      for(i = 1,j = HalfOrder - 1,k = HalfOrder + 1; i <= HalfOrder;i++,j--,k++) {
	fv = (real_t) (FSIN_NAME(i*c)/(i*((real_t) M_PI)));
	Filter[j] = fv;
	Filter[k] = fv;
      }
      Filter[HalfOrder] = Freq;
    }
  }
}


void HIGHPASSFIR_NAME(real_t* Filter, unsigned int Order, real_t Freq)
{
  unsigned int i,HalfOrder;
  real_t c = (real_t) (M_PI*Freq);
  
  if (Order > 2) {
    if (Order%2 == 0) {
      Order--;
      Filter[Order] = 0;
    }

    HalfOrder = Order/2;

    for(i = 1; i <= HalfOrder;i++) {
      Filter[HalfOrder - i] = (real_t) (-FSIN_NAME(i*c)/(i*M_PI));
      Filter[HalfOrder + i] = Filter[HalfOrder-i];
    }
    Filter[HalfOrder] = 1-Freq;
  }
}

void BANDPASSFIR_NAME(real_t* Filter, unsigned int Order, real_t Low, real_t High)
{
  unsigned int i, HalfOrder = Order/2;
  real_t d = (real_t) (M_PI*(High - Low)/2), s = (real_t) (M_PI*(High + Low)/2);

  if (Order > 1) {
    if (Order%2 == 0) {
      for(i = 1;i <= HalfOrder;i++) {
	Filter[HalfOrder - i] = (real_t) (M_2PI*FSIN_NAME((i-0.5)*d)* FCOS_NAME((i-0.5)*s)/(i-0.5));
	Filter[HalfOrder+ i - 1] = Filter[HalfOrder - i];
      }
    } else {
      for(i = 1;i <= HalfOrder;i++) {
	Filter[HalfOrder - i] = (real_t) (M_2PI*FSIN_NAME(i*d)* FCOS_NAME(i*s)/i);
	Filter[HalfOrder + i] = Filter[HalfOrder - i];
      }
      Filter[HalfOrder] = M_PI*M_PI*(High-Low);
    }
  }
}

#undef FSIN_NAME
#undef FCOS_NAME
#undef FTAN_NAME
#undef FATAN_NAME 
