/*
 * (c) Copyright 2013/2022 -- Raul Fernandez Ortega
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */

#ifndef _SFLOGIC_RECPLAY_HPP_
#define _SFLOGIC_RECPLAY_HPP_

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

#include <sndfile.h>

#ifdef __cplusplus
}
#endif 


#include <libxml/parser.h>
#include <libxml/tree.h>
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
#include "timestamp.h"

class SndFileManager {

private:
  struct sfconf *sfconf;
  struct intercomm_area *icomm;
  SfAccess *sfaccess;

  SNDFILE* sf_file;
  SF_INFO sf_info;
  string path;
  bool isWrite;
  string InputChannel;
  string OutputChannel;
  string FileChannel;
  void* iobuffer; // n_fft * realsize = 2 * sfconf->filter_length * sfconf->realsize

  bool debug;
  bool isOpen;
  bool isRunning;
  float time_ms;
  float l_time;

  pthread_t manager_thread;
  sem_t manager_sem;

  static void* readwriteProcessThread(void *args);
  void readwriteProcess(void);

public:

  pthread_mutex_t mutex;

  SndFileManager(struct sfconf *_sfconf,
		 struct intercomm_area *_icomm,
		 SfAccess *_sfaccess);

  ~SndFileManager(void) { };
  
  int preinit(xmlNodePtr sfparams, bool _debug);
  void init(void);
  int config(string _path, bool _is_in, string _channel, bool _recplay);
  void start(int _l_time);
  void stop(void);
  bool openFilename(void);
  bool closeFilename(void);
  void input_timed(void *buffer, int channel);
  void output_timed(void *buffer, int channel);
};

class SFLOGIC_RECPLAY :  public SFLOGIC {

private:

  bool debug;
  vector<SndFileManager*> managers;
  

public:

  SFLOGIC_RECPLAY(struct sfconf *_sfconf,
		 struct intercomm_area *_icomm,
		 SfAccess *_sfaccess);

  ~SFLOGIC_RECPLAY(void) { };

  int preinit(xmlNodePtr sfparams, int _debug);

  void addManager(SndFileManager* _manager);

  int init(int event_fd, sem_t *synch_sem);

  int command(const char params[]);

  const char *message(void);

  void input_timed(void *buffer, int channel);
  void output_timed(void *buffer, int channel);

};

#endif
