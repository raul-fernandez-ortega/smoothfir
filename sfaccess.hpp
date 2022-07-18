/*
 * (c) Copyright 2013/2022 -- Raul Fernandez Ortega
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */

#ifndef _SFACCESS_HPP_
#define _SFACCESS_HPP_

#ifdef __cplusplus
extern "C" {
#endif 

#include "defs.h"
#include <stdarg.h>

#ifdef __cplusplus
}
#endif 

#include <string>

#include "sfmod.h"
#include "sfstruct.h"

using namespace std;

class SfAccess {

public:

  struct sfoverflow overflow[SF_MAXCHANNELS];
  vector<string> dummy;

  SfAccess(void) {};
  ~SfAccess(void) {};

  virtual void control_mutex(int lock) {};
  virtual void reset_peak(void) {};
  virtual void exit(int bf_exit_code) {};
/*
 * Mute/unmute the given input/output channel. If the channel index is invalid,
 * nothing happens.
 */
  virtual void toggle_mute(int io, int channel) { };  
  virtual bool ismuted(int io, int channel) {return false; };
/*
 * Change delay of the given channel. If the delay or channel is out of range,
 * -1 is returned, else 0.
 */
  virtual int set_delay(int io, int channel, int delay) {return 0; };
  virtual int get_delay(int io, int channel) {return 0; };

  virtual double realtime_index(void) {return 0; };

  virtual vector<string> sfio_names(int io, int *n_names) {return dummy; };
  virtual void sfio_range(int io, int modindex, int range[2]) { };
  virtual void sfio_command(int io, int modindex, const char params[], char **error) {};
  virtual vector<string> sflogic_names(int *n_names) { return dummy; };
  virtual bool sflogic_exec(int modindex, char* command, int nparams, ...) { return false; };
  virtual int sflogic_command(int modindex, const char params[], char **error) {return 0; };

  virtual void convolver_coeffs2cbuf(void *src, void *dest) { return; };

  virtual void *convolver_fftplan(int order, int invert, int inplace) { return NULL; };

  virtual int set_subdelay(int io, int channel, int subdelay) { return 0; };
  virtual int get_subdelay(int io, int channel) { return 0; };
};


#endif
