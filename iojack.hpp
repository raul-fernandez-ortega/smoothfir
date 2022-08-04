/*
 * (c) Copyright 2013/2022 -- Raul Fernandez Ortega
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */

#ifndef _SFIO_JACK_HPP_
#define _SFIO_JACK_HPP_

#define IS_SFIO_MODULE

extern "C" {

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <signal.h>

#include <jack/jack.h>

}
#include <libxml/parser.h>
#include <libxml/tree.h>


#include "defs.h"
#include "sfmod.h"
#include "sfinout.h"
#include "sfio.hpp"
#include "sfdai.hpp"

struct jack_state {
  int n_channels;
  vector <jack_port_t*> ports;
  vector <string> port_name;
  vector <string> local_port_name;
  vector <string> dest_name;
};

class ioJack : public IO {

private:

  struct sfconf *sfconf;
  struct intercomm_area *icomm;
  struct iodev *iodev;
  int sample_format;
  int sample_rate;
  int period_size;

  Dai *sfDai;
  bool debug;
  bool has_started;
  vector<struct jack_state *> handles[2];
  vector<struct subdev*> states[2];
  vector<struct subdev*> _states[2];
  int io_idx[2];
  int n_handles[2];
  bool hasio[2];
  jack_client_t *client;
  char *client_name;
  int frames_left;

  bool global_init(void);

public:

  int32_t expected_priority;

  ioJack(struct sfconf *_sfconf,
	 struct intercomm_area *_icomm,
	 Dai *_sfDai);

  void preinit(int io, struct iodev *_iodev);

  ~ioJack(void); 

  static void init_callback(void *arg);  
  static void init_callback_(void);
  static void latency_callback(jack_latency_callback_mode_t mode, void *arg);
  void sf_latency_callback(jack_latency_callback_mode_t mode);
  static void jack_shutdown_callback(void *arg);
  static void error_callback(const char *msg);
  void sf_shutdown_callback(void);
  static int jack_process_callback(jack_nframes_t n_frames, void* arg);
  int sf_process_callback(jack_nframes_t n_frames);

  //bool iscallback(void);

  int init(int io,
	   int i_handle,
	   int used_channels,
	   vector<int> channel_selection,
	   struct subdev *callback_subdev);
 
  int synch_start(void);

  void synch_stop(void);

  bool connect_port(string port_name, string dest_name); 

  bool disconnect_port(string port_name, string dest_name); 

  const char **get_jack_port_connections(string port_name);
  const char **get_jack_ports(void);
  const char **get_jack_input_physical_ports(void);
  const char **get_jack_input_ports(void);
  const char **get_jack_output_physical_ports(void);
  const char **get_jack_output_ports(void);
  bool is_running(void) { return has_started; };

};

#endif
