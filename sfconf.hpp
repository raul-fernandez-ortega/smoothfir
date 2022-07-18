/*
 * (c) Copyright 2013/2022 - Raul Fernandez Ortega
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#ifndef _SFCONF_HPP_
#define _SFCONF_HPP_

#include "bit.h"

#ifdef __cplusplus
extern "C" {
#endif 

#include <limits.h>
#include <sndfile.h>

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "timestamp.h"
#include "emalloc.h"
#include "log2.h"
#include "bit.h"
#include "timermacros.h"
#include "shmalloc.h"
#include "swap.h"
#include "pinfo.h"
#include "numunion.h"

#ifdef __cplusplus
}
#endif 

#include <vector>
#include <string>

#include "sfclasses.hpp"
#include "sfinout.h"
#include "sfdither.h"
#include "sfmod.h"
#include "sfstruct.h"
#include "sfdai.hpp"
#include "sfrun.hpp"
#include "iojack.hpp"
#include "sfdelay.hpp"
#include "convolver.hpp"
#include "sflogic_cli.hpp"
#include "sflogic_eq.hpp"
#include "sflogic_ladspa.hpp"
#include "sflogic_race.hpp"
#include "sflogic_py.hpp"
#include "sflogic_recplay.hpp"

struct coeff {
  struct sfcoeff coeff;
#define COEFF_FORMAT_RAW 1    
#define COEFF_FORMAT_SNDFILE 2
#define COEFF_FORMAT_TEXT 3
  int format;
  int skip;
  int channel;
  SF_INFO sfinfo;
  char filename[PATH_MAX];
  int shm_shmids[SF_MAXCOEFFPARTS];
  int shm_offsets[SF_MAXCOEFFPARTS];
  int shm_blocks[SF_MAXCOEFFPARTS];
  int shm_elements;
  double scale;
};

struct filter {
  struct sffilter filter;
  struct sffilter_control fctrl;
  string coeff_name;
  vector<string> channel_name[2];
  vector<string> filter_name[2];
  int process;
};

#define FROM_DB(db) (pow(10, (-db) / 20.0))

void parse_error(const char msg[]);

void parse_error_exit(const char msg[]);

char* tilde_expansion(char path[]);

//void field_repeat_test(uint32_t *bitset, int bit);

//void field_mandatory_test(uint32_t bitset, uint32_t bits, const char name[]);

bool parse_sample_format(struct sample_format *sf, const string s, bool allow_auto);

bool parse_coeff_sample_format(SF_INFO *sf, const string s);

void *real_read(FILE *stream, int *len, char filename[], int realsize, int maxitems);

void * sndfile_read(const char* filename, int *totitems, struct coeff *coeff, int channel, SF_INFO snfinfo, int realsize, int maxitems);

int number_of_cpus(void);

class SfConf {

private:

  char found_module_path[SF_MAXOBJECTNAME + PATH_MAX + 100];
  vector<string> logic_names;
  xmlNodePtr logic_data[SF_MAXMODULES];
  xmlNodePtr config_data;
  
  struct coeff *parse_coeff(xmlNodePtr xmlnode, int intname);
  void parse_filter_io_array(xmlNodePtr xmlnode, struct filter *filter, int io, bool isfilter);
  struct filter *parse_filter(xmlNodePtr xmlnode, int intname);
  struct iodev *parse_iodev(xmlNodePtr xmlnode, int io);
  void parse_setting(xmlNodePtr xmlnode, uint32_t *repeat_bitset);
  void* load_coeff(struct coeff *lcoeff, int cindex);
  bool filter_loop(int source_intname, int search_intname);
  
public:

  SfConf(SfRun *_sfrun);
  ~SfConf(void);

  bool quiet;
  vector<struct dither_state*> dither_state;
  void ***coeffs_data;
  int n_channels[2];
  int maxdelay[2];
  vector<struct sffilter> filters;

  struct sfconf *sfconf;
  struct intercomm_area *icomm;
  
  Convolver *sfConv;
  Dai *sfDai;
  Delay *sfDelay;
  SfRun *sfRun;
  IO *iojack;
  vector<SFLOGIC*> sflogic;
  
  void sfconf_init(char filename[]);
  
  void add_sflogic(SFLOGIC* _sflogic);

};

#endif
