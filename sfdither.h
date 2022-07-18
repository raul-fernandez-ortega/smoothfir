/*
 * (c) Copyright 2013/2022 -- Raul Fernandez Ortega
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#ifndef _SFDITHER_H_
#define _SFDITHER_H_

#ifdef __cplusplus
extern "C" {
#endif 

#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#ifdef __cplusplus
}
#endif 

#include "bit.h"
#include "sfmod.h"
#include <vector>
#include "shmalloc.h"
#include "emalloc.h"
#include "pinfo.h"

using namespace std;

struct dither_state {
  int randtab_ptr;
  int8_t *randtab;
  float sf[2];
  double sd[2];
};

extern int8_t *dither_randtab;
extern int dither_randtab_size;
extern void *dither_randmap;

static inline void dither_preloop_real2int_hp_tpdf(struct dither_state *state, int samples_per_loop)
{
  if (state->randtab_ptr + samples_per_loop >= dither_randtab_size) {
    dither_randtab[0] = dither_randtab[state->randtab_ptr - 1];
    state->randtab_ptr = 1;
  }
  state->randtab = &dither_randtab[state->randtab_ptr];
  state->randtab_ptr += samples_per_loop;
}

#define real_t float
#define REALSIZE 4
#define REAL2INT_HP_TPDF_NAME ditherf_real2int_hp_tpdf
#define REAL2INT_NO_DITHER_NAME ditherf_real2int_no_dither
#include "dither_funs.h"
#undef REAL2INT_HP_TPDF_NAME
#undef REAL2INT_NO_DITHER_NAME
#undef REALSIZE
#undef real_t

#define real_t double
#define REALSIZE 8
#define REAL2INT_HP_TPDF_NAME ditherd_real2int_hp_tpdf
#define REAL2INT_NO_DITHER_NAME ditherd_real2int_no_dither
#include "dither_funs.h"
#undef REAL2INT_HP_TPDF_NAME
#undef REAL2INT_NO_DITHER_NAME
#undef REALSIZE
#undef real_t

bool dither_init(int n_channels,
		 int sample_rate,
		 int realsize,
		 int max_samples_per_loop,
		 vector<struct dither_state*> dither_states);

#endif
