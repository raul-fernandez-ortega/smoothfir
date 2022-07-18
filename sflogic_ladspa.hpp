/*
 * (c) Copyright 2013/2022 -- Raul Fernandez Ortega
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */

#ifndef _SFLOGIC_LADSPA_HPP_
#define _SFLOGIC_LADSPA_HPP_

#include <vector>

#ifdef __cplusplus
extern "C" {
#endif 

#include <dirent.h>
#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/select.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include <ladspa.h>

#ifdef __cplusplus
}
#endif 

#include <string>
#include <vector>

#define IS_SFLOGIC_MODULE

#include "sflogic.hpp"

#include "sfmod.h"
#include "emalloc.h"
#include "shmalloc.h"
#include "log2.h"
#include "fdrw.h"
#include "timermacros.h"

#define LADSPA_CONTROLS_MAX 20
#define LADSPA_OUTPUT_MAX 4

using namespace std;

void field_repeat_test(uint32_t *bitset, const char name[], int bit);

void field_mandatory_test(uint32_t bitset, uint32_t bits, const char name[]);

struct LADSPA_Control {
  unsigned long index;
  string control_label;
  LADSPA_Data* control_value;
};

class LADSPA_PLUGIN {

private:

  struct sfconf *sfconf;
  struct intercomm_area *icomm;
  SfAccess *sfaccess;
  char msg[MAX_MSG_LEN];

  bool debug;
  string LADSPALabel;
  vector<string> OutputChannel;
  string InputChannel;
  int nOutputs;
  int processedOutputs;
  void * pvPluginLibrary;
  LADSPA_Descriptor_Function fDescriptorFunction;
  const LADSPA_Descriptor * PluginDescriptor;
  LADSPA_Data* BufferIN;
  vector<LADSPA_Data*>  BufferOUT;
  LADSPA_Handle PluginHandle;
  LADSPA_Control *PluginControls;
  int nPluginControls;
  int nPluginInputs;
  int nPluginOutputs;

  int analysePlugin(void);
  bool findPlugin(const string label,
		  const string pcFullFilename, 
		  void * pvPluginHandle);
  void ConnectControlPort(string port_name, LADSPA_Data port_value);
  void ConnectInputPort(LADSPA_Data *buffer);
  void ConnectOutputPort(vector<LADSPA_Data*>  buffer);
  bool LADSPADirectoryPluginSearch(string pcDirectory, const string label);  
  int LADSPAPluginSearch(const string label);
  
public:

  pthread_mutex_t mutex;

  LADSPA_PLUGIN(struct sfconf *_sfconf,
		 struct intercomm_area *_icomm,
		 SfAccess *_sfaccess);

  ~LADSPA_PLUGIN(void) { };

  int preinit(xmlNodePtr sfparams, bool _debug);

  int init(void);

  int setLabel(string label);
  void setInput(string _InputChannel);
  void addOutput(string _OutputChannel);
  int setPortValue(string port_name, float port_value);

  //int command(const char params[]);

  //const char *message(void);

  void input_timed(void *buffer, int channel);
  void output_timed(void *buffer, int channel);

};

class SFLOGIC_LADSPA :  public SFLOGIC {

private:

  bool debug;

public:

  vector<LADSPA_PLUGIN*> plugins;

  SFLOGIC_LADSPA(struct sfconf *_sfconf,
		 struct intercomm_area *_icomm,
		 SfAccess *_sfaccess);

  ~SFLOGIC_LADSPA(void) { };

  int preinit(xmlNodePtr sfparams, int _debug);

  int init(int event_fd, sem_t* _synch_sem);

  //int command(const char params[]);

  int addPlugin(LADSPA_PLUGIN* plugin);

  //const char *message(void);

  void input_timed(void *buffer, int channel);
  void output_timed(void *buffer, int channel);

};

#endif
