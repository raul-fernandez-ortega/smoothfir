/*
 * (c) Copyright 2013/2022 -- Raul Fernandez Ortega
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */

#ifndef _IO_HPP_
#define _IO_HPP_

extern "C" {

#include <libxml/parser.h>
#include <libxml/tree.h>

}

#include <vector>

//#include "bfclasses.hpp"

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
		   //int period_size,
		   //int *device_period_size,
		   //bool *isinterleaved,
		   struct subdev *callback_subdev) { return 0; };

  //virtual int bfio_cb_init(void *params) { return 0; };

  //virtual int read(int fd, void *buf, int offset, int count) { return 0; };

  //virtual int write(int fd, const void *buf, int offset, int count) { return 0; };

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
  //virtual char *message(void) { return NULL; };

};

/*typedef BFIO* (*bfio_create_t)(struct bfconf *_bfconf,
			       struct intercomm_area *_icomm,
			       xmlNodePtr _xmldata,
			       int _io,
			       struct iodev *_iodev);

typedef void (*bfio_destroy_t)(BFIO* _bfio); 

struct bfio_module {
  void *handle;
  int iscallback;
  BFIO* bfio;
  bfio_create_t create;
  bfio_destroy_t destroy;
  }; 
*/
#endif
