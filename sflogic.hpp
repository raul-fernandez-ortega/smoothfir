/*
 * (c) Copyright 2013 -- Raul Fernandez Ortega
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */

#ifndef _SFLOGIC_HPP_
#define _SFLOGIC_HPP_

#include "bit.h"

#ifdef __cplusplus
extern "C" {
#endif 

#include <string.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#ifdef __cplusplus
}
#endif 

#include <string>
#include <vector>

#include "sfclasses.hpp"
#include "sfaccess.hpp"

#define MAX_MSG_LEN         500
#define SF_FORK_DONT_FORK   0
#define SF_FORK_PRIO_MAX    1
#define SF_FORK_PRIO_FILTER 2
#define SF_FORK_PRIO_OTHER  3

struct state {
    struct sffilter_control fctrl[SF_MAXFILTERS];
    bool fchanged[SF_MAXFILTERS];
    int delay[2][SF_MAXCHANNELS];
    int subdelay[2][SF_MAXCHANNELS];
    bool toggle_mute[2][SF_MAXCHANNELS];
};

struct sleep_task {
    bool do_sleep;
    bool block_sleep;
    unsigned int blocks;
    unsigned int seconds;
    unsigned int useconds;
};


class SFLOGIC {

public:

  struct sfconf *sfconf;
  struct intercomm_area *icomm;
  SfAccess *sfaccess;
  bool unique;
  string name;
  sem_t* synch_sem;

  int fork_mode;
  int event_pipe[2];
  char msg[MAX_MSG_LEN];

  SFLOGIC(struct sfconf *_sfconf,
	  intercomm_area *_icomm,
	  SfAccess *_sfaccess) : sfconf(_sfconf), icomm(_icomm), sfaccess(_sfaccess), unique(false) {};
  
  ~SFLOGIC(void) {};

  virtual bool iscallback(void) { return false; }

  virtual int preinit(xmlNodePtr sfparams, int debug) { return 0; };

  virtual int init(int event_fd, sem_t *_synch_sem) { synch_sem = _synch_sem; return 0; };

  //virtual int command(const char params[]) { return 0; };

  //virtual const char *message(void) { return msg; };

  virtual void input_timed(void *buffer, int channel) { return; };

  virtual void output_timed(void *buffer, int channel) { return; };

  virtual void print_overflows(void) { return; };
}; 

#endif
