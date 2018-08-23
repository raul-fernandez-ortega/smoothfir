/*
 * (c) Copyright 2001 - 2003, 2005 -- Anders Torger
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */

#ifndef _SFDAI_HPP_
#define _SFDAI_HPP_

#include "bit.h"

#ifdef __cplusplus
extern "C" {
#endif 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <sched.h>
#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>
#include <pthread.h>
#include <semaphore.h>

#include "emalloc.h"
#include "shmalloc.h"
#include "timestamp.h"
#include "fdrw.h"
#include "pinfo.h"
#include "timermacros.h"
#include "numunion.h"

#ifdef __cplusplus
}
#endif 

#include <vector>

#include "sfclasses.hpp"
#include "sfstruct.h"
#include "sfinout.h"
#include "sfcallback.hpp"
#include "sfio.hpp"

#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif

using namespace std;

#define CB_MSG_START 1
#define CB_MSG_STOP 2

struct dai_params {
  int subdev_params;
  int subdev_index;
  int psize;
  char *params;
  int ans;
  int msize;
  char *msgstr;
};

struct comarea {
    bool blocking_stopped;
    int lastbuf_index;
    int frames_left;
    int cb_lastbuf_index;
    int cb_frames_left;
    bool is_muted[2][SF_MAXCHANNELS];
    int delay[2][SF_MAXCHANNELS];
    pid_t pid[2];
    pid_t callback_pid;
    struct subdev dev[2][SF_MAXCHANNELS];
    struct dai_buffer_format buffer_format[2];
    int buffer_id;
    int cb_buf_index[2];
};

class Dai {

  SfCallback  *sfRun;
  struct sfconf *sfconf;
  intercomm_area *icomm;
  uint8_t *buffer;
  struct comarea ca;
  void *iobuffers[2][2];
  int n_devs[2];
  int n_fd_devs[2];
  fd_set dev_fds[2];
  fd_set clocked_wfds;
  int n_clocked_devs;
  int dev_fdn[2];
  int min_block_size[2];
  int cb_min_block_size[2];
  bool input_poll_mode;
  struct subdev* dev[2];
  struct subdev *fd2dev[2][FD_SETSIZE];
  struct subdev *ch2dev[2][SF_MAXCHANNELS];
  int monitor_rate_fd;

  pthread_t dai_thread;
  sem_t synchsem[2], paramssem_s[2], paramssem_r[2];
  sem_t cbsem_s, cbsem_r;
  sem_t cbmutex_pipe[2];
  sem_t cbreadywait_sem[2];
  int callback_ready_waiting[2];

  struct dai_params dai_handle_params[2];
  char callback_init_msg;

  bool input_isfirst, input_startmeasure;
  int input_buf_index, input_frames, input_curbuf;
  struct timeval starttv;

  bool output_isfirst, output_islast;
  int output_buf_index, output_curbuf;
  fd_set readfds;

  char *msgstr;

  void Dai_exit(void);

  void trigger_callback_ready(int io);

  void wait_callback_ready(int io);

  void cbmutex(int io, bool lock);

  bool output_finish(void);
  
  void update_devmap(int io);

/* if noninterleaved, update channel layout to fit the noninterleaved access
   mode (it is setup for interleaved layout per default). */
 
  void noninterleave_modify(int io);

  void do_mute(struct subdev *sd,
	       int io,
	       int wsize,
	       void *_buf,
	       int offset);

  bool init_input(struct dai_subdevice *subdev);
  
  bool init_output(struct dai_subdevice *subdev);

  void calc_buffer_format(int fragsize,
			  int io,
			  struct dai_buffer_format *format);
  
  bool callback_init(void);

  void callback_process(void);
  
public:
  /*
   * The subdevs structures are used internally, so they must not be deallocated
   * nor modified.
   */
  struct dai_buffer_format *dai_buffer_format[2];
  void *buffers[2][2];

  Dai(struct sfconf *_sfconf,
      intercomm_area *_icomm,
      SfCallback *_callbackClass);

  void Dai_init(void);
  
  ~Dai(void);
  
  void trigger_callback_io(void);

  int minblocksize(void);

  bool get_input_poll_mode(void);

  bool isinit(void);

  void toggle_mute(int io, int channel);

  int change_delay(int io,
		   int channel,
		   int delay);

  /*
   * Shut down prematurely. Must be called by the input and output process
   * separately. For any other process it has no effect.
   */
  void die(void);
  
  static void *callback_process_thread(void *args);
  
  void process_callback_input(struct subdev *sd,
			      void *cbbufs[],
			      int frame_count);
  
  void process_callback_output(struct subdev *sd,
			       void *cbbufs[],
			       int frame_count,
			       bool iodelay_fill);

  int process_callback(vector<struct subdev*> states[2],
		       int state_count[2],
		       void **buffers[2],
		       int frame_count,
		       int event);  
};

#endif
