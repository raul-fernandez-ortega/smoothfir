/*
 * (c) Copyright 2013 -- Raul Fernandez Ortega
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */

#ifndef _SFLOGIC_CLI_HPP_
#define _SFLOGIC_CLI_HPP_

#ifdef __cplusplus
extern "C" {
#endif 

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <math.h>
#include <fcntl.h>
#include <termios.h>
#include <semaphore.h>

#ifdef __cplusplus
}
#endif 

#include "sflogic.hpp"

#define IS_SFLOGIC_MODULE
#include "timermacros.h"
#include "sfmod.h"
#include "log2.h"
#include "sfinout.h"
#include "fdrw.h"

#define WELCOME_TEXT "\nWelcome to smoothfir, type \"help\" for help.\n\n"
#define PROMPT_TEXT "> "

#define HELP_TEXT "\n\
Commands:\n\
\n\
lf -- list filters.\n\
lc -- list coeffient sets.\n\
li -- list inputs.\n\
lo -- list outputs.\n\
lm -- list modules.\n\
\n\
cfoa -- change filter output attenuation.\n\
        cfoa <filter> <output> <attenuation|Mmultiplier>\n\
cfia -- change filter input attenuation.\n\
        cfia <filter> <input> <attenuation|Mmultiplier>\n\
cffa -- change filter filter-input attenuation.\n\
        cffa <filter> <filter-input> <attenuation|Mmultiplier>\n\
cfc  -- change filter coefficients.\n\
        cfc <filter> <coeff>\n\
cfd  -- change filter delay. (may truncate coeffs!)\n\
        cfd <filter> <delay blocks>\n\
cod  -- change output delay.\n\
        cod <output> <delay> [<subdelay>]\n\
cid  -- change input delay.\n\
        cid <input> <delay> [<subdelay>]\n\
tmo  -- toggle mute output.\n\
        tmo <output>\n\
tmi  -- toggle mute input.\n\
        tmi <input>\n\
imc  -- issue input module command.\n\
        imc <index> <command>\n\
omc  -- issue output module command.\n\
        omc <index> <command>\n\
lmc  -- issue logic module command.\n\
        lmc <module> <command>\n\
\n\
sleep -- sleep for the given number of seconds [and ms], or blocks.\n\
         sleep 10 (sleep 10 seconds).\n\
         sleep b10 (sleep 10 blocks).\n\
         sleep 0 300 (sleep 300 milliseconds).\n\
abort -- terminate immediately.\n\
tp    -- toggle prompt.\n\
ppk   -- print peak info, channels/samples/max dB.\n\
rpk   -- reset peak meters.\n\
upk   -- toggle print peak info on changes.\n\
rti   -- print current realtime index.\n\
quit  -- close connection.\n\
help  -- print this text.\n\
\n\
Notes:\n\
\n\
- When entering several commands on a single line,\n\
  separate them with semicolons (;).\n\
- Inputs/outputs/filters can be given as index\n\
  numbers or as strings between quotes (\"\").\n\
\n\
"

#define MAXCMDLINE 4096

#define FILTER_ID 1
#define INPUT_ID  2
#define OUTPUT_ID 3
#define COEFF_ID  4

class SFLOGIC_CLI : public SFLOGIC {

private:

  int line_speed;
  int port, port2;
  char *lport;
  char *script;
  bool print_peak_updates;
  bool print_prompt;
  bool print_commands;
  bool debug;
  char error[1024];

  char *p, *p1, *p2;
  bool do_quit, do_sleep, sleep_block;
  unsigned int sleep_block_index;
  struct timeval sleep_time;
  bool retchr, cmdchr;
  char restore_char;

struct state newstate;

  void clear_changes(void);
  void commit_changes(FILE *stream);
  bool are_changes(void);
  void print_overflows(FILE *stream);
  char* strtrim(char s[]);
  bool get_id(FILE *stream,
	      char str[],
	      char **p,
	      int *id,
	      int type,
	      int rid);

  bool parse_command(FILE *stream,
		     char cmd[],
		     struct sleep_task *_sleep_task);
  
  void wait_data(FILE *client_stream,
		 int client_fd,
		 int callback_fd);

  bool parse(FILE *stream,
	     const char cmd[],
	     struct sleep_task *sleep_task);
  
  void block_start(unsigned int block_index,
		   struct timeval *current_time);

  void parse_string(FILE *stream,
		    const char inbuf[MAXCMDLINE],
		    char cmd[MAXCMDLINE]);

  void stream_loop(int event_fd,
		   int infd,
		   FILE *instream,
		   FILE *outstream);

  void socket_loop(int event_fd,
		   int lsock);
  
public:

  SFLOGIC_CLI(struct sfconf *_sfconf,
	      intercomm_area *_icomm,
	      SfAccess *_sfaccess);

  ~SFLOGIC_CLI(void);

  int preinit(xmlNodePtr sfparams, int _debug);

  int init(int event_fd, sem_t *synch_sem);

};


#endif
