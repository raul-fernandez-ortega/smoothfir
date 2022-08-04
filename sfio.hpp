/*
 * (c) Copyright 2013/2022 -- Raul Fernandez Ortega
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */

#ifndef _SFIO_HPP_
#define _SFIO_HPP_

#include <libxml/parser.h>
#include <libxml/tree.h>
#include <vector>

#include "sfclasses.hpp"

using namespace std;

class IO {

public:

  struct sfconf *sfconf;
  struct intercomm_area *icomm;
  struct iodev *iodev;
  int sample_format;
  int sample_rate;

  IO(struct sfconf *_sfconf,
     struct intercomm_area *_icomm) : sfconf(_sfconf), icomm(_icomm) {};
  
  ~IO(void) {};

  virtual void preinit(int io, struct iodev *_iodev) {};

  virtual int init(int io,
		   int i_handle,
		   int used_channels,
		   vector<int> channel_selection,
		   struct subdev *callback_subdev) { return 0; };

  virtual int sfio_cb_init(void *params) { return 0; };

  virtual int synch_start(void) { return 0; };

  virtual void synch_stop(void) { };

  virtual int start(int io) { return 0; } ;

  virtual void stop(int io) { };

  virtual bool connect_port(string port_name, string dest_name) { return true; }; 

  virtual bool disconnect_port(string port_name, string dest_name) { return true; }; 

  virtual const char **get_jack_port_connections(string port_name) { return NULL; };
  virtual const char **get_jack_ports(void) { return NULL; };
  virtual const char **get_jack_input_physical_ports(void) { return NULL; };
  virtual const char **get_jack_input_ports(void) { return NULL; };
  virtual const char **get_jack_output_physical_ports(void) { return NULL; };
  virtual const char **get_jack_output_ports(void) { return NULL; };

  virtual bool is_running(void) { return false; };

};

#endif
