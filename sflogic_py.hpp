/*
 * (c) Copyright 2013/2022 -- Raul Fernandez Ortega
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */

#ifndef _SFLOGIC_PY_HPP_
#define _SFLOGIC_PY_HPP_

#ifdef __cplusplus
extern "C" {
#endif 

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#ifdef __cplusplus
}
#endif 

#define IS_SFLOGIC_MODULE

#include "sflogic.hpp"
#include "sfinout.h"

#define FILTER_ID 1
#define INPUT_ID  2
#define OUTPUT_ID 3
#define COEFF_ID  4

class SFLOGIC_PY : public SFLOGIC {

private:
  
  bool print_peak_updates;
  bool debug;
  vector<double> inPeak;
  vector<double> outPeak;
  struct sfoverflow *reset_overflow;

struct state newstate;


  void print_overflows(void);
  
public:

  SFLOGIC_PY(struct sfconf *_sfconf,
	     struct intercomm_area *_icomm,
	     SfAccess *_sfaccess);

  ~SFLOGIC_PY(void) {}; 

  int preinit(xmlNodePtr sfparams, int _debug); 
  int init(int event_fd, sem_t *synch_sem);

  vector<struct sfcoeff> getCoeffData(void);

  vector<struct sffilter> getFilterData(void);
  struct sffilter_control* getFilterControl(int n);
  struct vector<sfchannel> getChannels(int io);

  bool is_running(void);

  int getFilternChannels(struct sffilter* filter, int io);
  int getFilternFilters(struct sffilter* filter, int io);
  struct sfcoeff* getFilterCoeff(string filtername);
  vector<int> getFilterChannels(struct sffilter* filter, int io);
  vector<int> getFilterFilters(struct sffilter* filter, int io);
  
  bool connect_port(string port_name, string dest_name); 
  bool disconnect_port(string port_name, string dest_name); 
  const char **get_jack_port_connections(string port_name);
  const char **get_jack_ports(void);
  const char **get_jack_input_physical_ports(void);
  const char **get_jack_input_ports(void);
  const char **get_jack_output_physical_ports(void);
  const char **get_jack_output_ports(void);

  bool change_input_attenuation(string inputchannel, double scale);
  double get_input_attenuation(string inputchannel);
  bool change_output_attenuation(string outputchannel, double scale);
  double get_output_attenuation(string outputchannel);  

  bool change_filter_coeff(string filtername, int coeff); 
  bool change_filter_attenuation(string filtername, int INOUT, int nchannel, double scale);
  double get_filter_attenuation(string filtername, int INOUT, int nchannel);

  bool change_filter_filter_attenuation(string filtername, int nfilter, double scale);

  void toggle_mute(int INOUT, int nchannel);

  vector<double> get_inpeaks(void);
  vector<double> get_outpeaks(void);
  void reset_peaks(void);
};

#endif
