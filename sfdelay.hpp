/*
 * (c) Copyright 2013 -- Raul Fernandez Ortega
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#ifndef _SFDELAY_HPP_
#define _SFDELAY_HPP_

#include "bit.h"

#ifdef __cplusplus
extern "C" {
#endif 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>
#include <fftw3.h>

#include "pinfo.h"
#include "firwindow.h"
#include "timestamp.h"

#ifdef __cplusplus
}
#endif 

#include "emalloc.h"
#include "sfstruct.h"

/* optional_target_buf has sample_spacing == 1 */

class Delay
{
private:
  struct sfconf* sfconf;

public:

  Delay(struct sfconf *_sfconf,
	int step_count);

  ~Delay(void) {};

  void copy_to_delaybuf(void *dbuf,
			void *buf,
			int sample_size,
			int sample_spacing,
			int n_samples);

  void copy_from_delaybuf(void *buf,
			  void *dbuf,		   
			  int sample_size,
			  int sample_spacing,
			  int n_samples);

  void shift_samples(void *buf,
		     int sample_size,
		     int sample_spacing,
		     int n_samples,
		     int distance);

  void update_delay_buffer(delaybuffer_t *db,
			   int sample_size,
			   int sample_spacing,
			   uint8_t *buf);

  void update_delay_short_buffer(delaybuffer_t *db,
				 int sample_size,
				 int sample_spacing,
				 uint8_t *buf);

  void change_delay(delaybuffer_t *db,
		    int sample_size,
		    int newdelay);

  void delay_update(delaybuffer_t *db,
		    void *buf,	     
		    int sample_size,
		    int sample_spacing,
		    int delay,
		    void *optional_target_buf);

  delaybuffer_t *delay_allocate_buffer(int fragment_size,
				       int initdelay,
				       int maxdelay,
				       int sample_size);

};

#endif
