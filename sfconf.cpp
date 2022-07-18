/*
 * (c) Copyright 2013/2022 -- Raul Fernandez
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <sys/types.h>
#include <inttypes.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pwd.h>
#include <sys/shm.h>
#include <sched.h>
#include <dlfcn.h>
#include <stddef.h>

#include "sfconf.hpp"

#define PATH_SEPARATOR_CHAR '/'
#define PATH_SEPARATOR_STR "/"
#define CONVOLVER_NEEDS_CONFIGFILE 1

#define MINFILTERLEN 4
#define MAXFILTERLEN (1 << 30)

void parse_error(const char msg[])
{
  fprintf(stderr, "Parse error:\n  %s",msg);
}

void parse_error_exit(const char msg[])
{
  fprintf(stderr, "Parse error:\n  %s",msg);
  exit(SF_EXIT_INVALID_CONFIG);
}

char* tilde_expansion(char path[])
{
  static char real_path[PATH_MAX];
  struct passwd *pw;
  char *homedir = NULL, *p;
  int pos, slashpos = -1;
  
  /* FIXME: cleanup the code */
  
  if (path[0] != '~') {
    strncpy(real_path, path, PATH_MAX);
    real_path[PATH_MAX-1] = '\0';
    return real_path;
  }
  
  if ((p = strchr(path, PATH_SEPARATOR_CHAR)) != NULL) {
    slashpos = p - path;
  }
  
  if (path[1] == '\0' || slashpos == 1) {
    if ((homedir = getenv("HOME")) == NULL)
      {
	if ((pw = getpwuid(getuid())) != NULL) {
	  homedir = pw->pw_dir;
	}
      }
  } else {
    strncpy(real_path, &path[1], PATH_MAX);
    real_path[PATH_MAX-1] = '\0';
    if (slashpos != -1) {
      real_path[slashpos-1] = '\0';
    }
    if ((pw = getpwnam(real_path)) != NULL) {
      homedir = pw->pw_dir;
    }
  }
  if (homedir == NULL) {
    strncpy(real_path, path, PATH_MAX);
    real_path[PATH_MAX-1] = '\0';
    return real_path;
  }
  
  real_path[PATH_MAX-1] = '\0';
  strncpy(real_path, homedir, PATH_MAX - 2);
  pos = strlen(homedir);
  if (homedir[0] == '\0' || homedir[strlen(homedir)-1] != PATH_SEPARATOR_CHAR) {
    strcat(real_path, PATH_SEPARATOR_STR);
    pos += 1;
  }
  if (slashpos != -1) {
    strncpy(&real_path[pos], &path[slashpos+1], PATH_MAX - pos - 1);
  } else {
    strncpy(&real_path[pos], &path[1], PATH_MAX - pos - 1);
  }
  real_path[PATH_MAX-1] = '\0';
  
  return real_path;
}

bool parse_sample_format(struct sample_format *sf, const string s, bool allow_auto)
{

  bool little_endian, native_endian;

  little_endian = true;
  native_endian = false;
  memset(sf, 0, sizeof(struct sample_format));
  
  if (strcasecmp(s.c_str(), "AUTO") == 0) {
    if (allow_auto) {
      return true;
    }
    parse_error("Cannot have \"AUTO\" sample format here.\n");
  }
    
  /* new sample format string format */
  if (strcasecmp(s.c_str(), "S8") == 0) {
    sf->format = SF_SAMPLE_FORMAT_S8;
    sf->bytes = 1;
    sf->sbytes = 1;
    little_endian = false;
  } else if (strcasecmp(s.c_str(), "S16_LE") == 0) {
    sf->format = SF_SAMPLE_FORMAT_S16_LE;
    sf->bytes = 2;
    sf->sbytes = 2;
  } else if (strcasecmp(s.c_str(), "S16_BE") == 0) {
    sf->format = SF_SAMPLE_FORMAT_S16_BE;
    sf->bytes = 2;
    sf->sbytes = 2;
    little_endian = false;
  } else if (strcasecmp(s.c_str(), "S16_NE") == 0) {
    sf->format = SF_SAMPLE_FORMAT_S16_NE;
    sf->bytes = 2;
    sf->sbytes = 2;
    native_endian = true;
  } else if (strcasecmp(s.c_str(), "S16_4LE") == 0) {
    sf->format = SF_SAMPLE_FORMAT_S16_4LE;
    sf->bytes = 4;
    sf->sbytes = 2;
  } else if (strcasecmp(s.c_str(), "S16_4BE") == 0) {
    sf->format = SF_SAMPLE_FORMAT_S16_4BE;
    sf->bytes = 4;
    sf->sbytes = 2;
    little_endian = false;
  } else if (strcasecmp(s.c_str(), "S16_4NE") == 0) {
    sf->format = SF_SAMPLE_FORMAT_S16_4NE;
    sf->bytes = 4;
    sf->sbytes = 2;
    native_endian = true;
  } else if (strcasecmp(s.c_str(), "S24_LE") == 0 || strcasecmp(s.c_str(), "S24_3LE") == 0) {
    sf->format = SF_SAMPLE_FORMAT_S24_LE;
    sf->bytes = 3;
    sf->sbytes = 3;
  } else if (strcasecmp(s.c_str(), "S24_BE") == 0 || strcasecmp(s.c_str(), "S24_3BE") == 0) {
    sf->format = SF_SAMPLE_FORMAT_S24_BE;
    sf->bytes = 3;
    sf->sbytes = 3;
    little_endian = false;
  } else if (strcasecmp(s.c_str(), "S24_NE") == 0 || strcasecmp(s.c_str(), "S24_3NE") == 0) {
    sf->format = SF_SAMPLE_FORMAT_S24_NE;
    sf->bytes = 3;
    sf->sbytes = 3;
    native_endian = true;
  } else if (strcasecmp(s.c_str(), "S24_4LE") == 0) {
    sf->format = SF_SAMPLE_FORMAT_S24_4LE;
    sf->bytes = 4;
    sf->sbytes = 3;
  } else if (strcasecmp(s.c_str(), "S24_4BE") == 0) {
    sf->format = SF_SAMPLE_FORMAT_S24_4BE;
    sf->bytes = 4;
    sf->sbytes = 3;
    little_endian = false;
  } else if (strcasecmp(s.c_str(), "S24_4NE") == 0) {
    sf->format = SF_SAMPLE_FORMAT_S24_4NE;
    sf->bytes = 4;
    sf->sbytes = 3;
    native_endian = true;
  } else if (strcasecmp(s.c_str(), "S32_LE") == 0) {
    sf->format = SF_SAMPLE_FORMAT_S32_LE;
    sf->bytes = 4;
    sf->sbytes = 4;
  } else if (strcasecmp(s.c_str(), "S32_BE") == 0) {
    sf->format = SF_SAMPLE_FORMAT_S32_BE;
    sf->bytes = 4;
    sf->sbytes = 4;
    little_endian = false;
  } else if (strcasecmp(s.c_str(), "S32_NE") == 0) {
    sf->format = SF_SAMPLE_FORMAT_S32_NE;
    sf->bytes = 4;
    sf->sbytes = 4;
    native_endian = true;
  } else if (strcasecmp(s.c_str(), "FLOAT_LE") == 0) {
    sf->format = SF_SAMPLE_FORMAT_FLOAT_LE;
    sf->bytes = 4;
    sf->sbytes = 4;
    sf->isfloat = true;
  } else if (strcasecmp(s.c_str(), "FLOAT_BE") == 0) {
    sf->format = SF_SAMPLE_FORMAT_FLOAT_BE;
    sf->bytes = 4;
    sf->sbytes = 4;
    sf->isfloat = true;
    little_endian = false;
  } else if (strcasecmp(s.c_str(), "FLOAT_NE") == 0) {
    sf->format = SF_SAMPLE_FORMAT_FLOAT_NE;
    sf->bytes = 4;
    sf->sbytes = 4;
    native_endian = true;
  } else if (strcasecmp(s.c_str(), "FLOAT64_LE") == 0) {
    sf->format = SF_SAMPLE_FORMAT_FLOAT64_LE;
    sf->bytes = 8;
    sf->sbytes = 8;
    sf->isfloat = true;
  } else if (strcasecmp(s.c_str(), "FLOAT64_BE") == 0) {
    sf->format = SF_SAMPLE_FORMAT_FLOAT64_BE;
    sf->bytes = 8;
    sf->sbytes = 8;
    sf->isfloat = true;
    little_endian = false;
  } else if (strcasecmp(s.c_str(), "FLOAT64_NE") == 0) {
    sf->format = SF_SAMPLE_FORMAT_FLOAT64_NE;
    sf->bytes = 4;
    sf->sbytes = 4;
    native_endian = true;
  } else {
    parse_error("Unknown sample format.\n");
  }
  if (sf->isfloat) {
    sf->scale = 1.0;
  } else {
    sf->scale = 1.0 / (double)(1 << ((sf->sbytes << 3) - 1));
  }
#ifdef __BIG_ENDIAN__
  if (native_endian) {
        sf->swap = false;
        switch (sf->format) {
        case SF_SAMPLE_FORMAT_S16_NE:
	  sf->format = SF_SAMPLE_FORMAT_S16_BE;
	  break;
        case SF_SAMPLE_FORMAT_S16_4NE:
	  sf->format = SF_SAMPLE_FORMAT_S16_4LE;
	  break;
        case SF_SAMPLE_FORMAT_S24_NE:
	  sf->format = SF_SAMPLE_FORMAT_S24_BE;
	  break;
        case SF_SAMPLE_FORMAT_S24_4NE:
	  sf->format = SF_SAMPLE_FORMAT_S24_4LE;
	  break;
        case SF_SAMPLE_FORMAT_S32_NE:
	  sf->format = SF_SAMPLE_FORMAT_S32_BE;
	  break;
        case SF_SAMPLE_FORMAT_FLOAT_NE:
	  sf->format = SF_SAMPLE_FORMAT_FLOAT_BE;
	  break;
        case SF_SAMPLE_FORMAT_FLOAT64_NE:
	  sf->format = SF_SAMPLE_FORMAT_FLOAT64_BE;
	  break;
        }
  } else {
    sf->swap = little_endian;
  }
#endif        
#ifdef __LITTLE_ENDIAN__
  if (native_endian) {
    switch (sf->format) {
    case SF_SAMPLE_FORMAT_S16_NE:
      sf->format = SF_SAMPLE_FORMAT_S16_LE;
      break;
    case SF_SAMPLE_FORMAT_S16_4NE:
      sf->format = SF_SAMPLE_FORMAT_S16_4LE;
      break;
    case SF_SAMPLE_FORMAT_S24_NE:
      sf->format = SF_SAMPLE_FORMAT_S24_LE;
      break;
    case SF_SAMPLE_FORMAT_S24_4NE:
      sf->format = SF_SAMPLE_FORMAT_S24_4LE;
      break;
    case SF_SAMPLE_FORMAT_S32_NE:
      sf->format = SF_SAMPLE_FORMAT_S32_LE;
      break;
    case SF_SAMPLE_FORMAT_FLOAT_NE:
      sf->format = SF_SAMPLE_FORMAT_FLOAT_LE;
      break;
    case SF_SAMPLE_FORMAT_FLOAT64_NE:
      sf->format = SF_SAMPLE_FORMAT_FLOAT64_LE;
      break;
    }
  } else {
    sf->swap = !little_endian;
  }
#endif
  
  return false;
}


bool parse_coeff_sample_format(SF_INFO *sf, const string s)
{
  MEMSET(sf, 0, sizeof(SF_INFO));

  sf->channels = 1;
  
  /* sndfile sample format */
  if (strcasecmp(s.c_str(), "S8") == 0) {
    sf->format = SF_FORMAT_RAW | SF_FORMAT_PCM_S8;
  } else if (strcasecmp(s.c_str(), "S16_LE") == 0) {
    sf->format = SF_FORMAT_RAW | SF_FORMAT_PCM_16 | SF_ENDIAN_LITTLE;
  } else if (strcasecmp(s.c_str(), "S16_BE") == 0) {
    sf->format = SF_FORMAT_RAW | SF_FORMAT_PCM_16 | SF_ENDIAN_BIG;
  } else if (strcasecmp(s.c_str(), "S16_NE") == 0) {
    sf->format = SF_FORMAT_RAW | SF_FORMAT_PCM_16 | SF_ENDIAN_CPU;
  } else if (strcasecmp(s.c_str(), "S24_LE") == 0) {
    sf->format = SF_FORMAT_RAW | SF_FORMAT_PCM_24 | SF_ENDIAN_LITTLE;
  } else if (strcasecmp(s.c_str(), "S24_BE") == 0) {
    sf->format = SF_FORMAT_RAW | SF_FORMAT_PCM_24 | SF_ENDIAN_BIG;
  } else if (strcasecmp(s.c_str(), "S24_NE") == 0) {
    sf->format = SF_FORMAT_RAW | SF_FORMAT_PCM_24 | SF_ENDIAN_CPU;
  } else if (strcasecmp(s.c_str(), "S32_LE") == 0) {
    sf->format = SF_FORMAT_RAW | SF_FORMAT_PCM_32 | SF_ENDIAN_LITTLE;
  } else if (strcasecmp(s.c_str(), "S32_BE") == 0) {
    sf->format = SF_FORMAT_RAW | SF_FORMAT_PCM_32 | SF_ENDIAN_BIG;
  } else if (strcasecmp(s.c_str(), "S32_NE") == 0) {
    sf->format = SF_FORMAT_RAW | SF_FORMAT_PCM_24 | SF_ENDIAN_CPU;
  } else if (strcasecmp(s.c_str(), "FLOAT_LE") == 0) {
    sf->format = SF_FORMAT_RAW | SF_FORMAT_FLOAT | SF_ENDIAN_LITTLE;
  } else if (strcasecmp(s.c_str(), "FLOAT_BE") == 0) {
    sf->format = SF_FORMAT_RAW | SF_FORMAT_FLOAT | SF_ENDIAN_BIG;
  } else if (strcasecmp(s.c_str(), "FLOAT_NE") == 0) {
    sf->format = SF_FORMAT_RAW | SF_FORMAT_FLOAT | SF_ENDIAN_CPU;
  } else if (strcasecmp(s.c_str(), "FLOAT64_LE") == 0) {
    sf->format = SF_FORMAT_RAW | SF_FORMAT_DOUBLE | SF_ENDIAN_LITTLE;
    } else if (strcasecmp(s.c_str(), "FLOAT64_BE") == 0) {
    sf->format = SF_FORMAT_RAW | SF_FORMAT_DOUBLE | SF_ENDIAN_BIG;
    } else if (strcasecmp(s.c_str(), "FLOAT64_NE") == 0) {
    sf->format = SF_FORMAT_RAW | SF_FORMAT_DOUBLE | SF_ENDIAN_CPU;
  } else {
    parse_error("Unknown sample format.\n");
  }
  return false;
}

void *real_read(FILE *stream, int *len, char filename[], int realsize, int maxitems)
{
  char str[1024], *p, *s;
  void *realbuf = NULL;
  int capacity = 0;
  
  *len = 0;
  
  str[1023] = '\0';
  while (fgets(str, 1023, stream) != NULL) {
    s = str;
    while (*s == ' ' || *s == '\t') s++;
    if (*s == '\n' || *s == '\0') {
      continue;
    }
    if (*len == capacity) {
      capacity += 1024;
      realbuf = erealloc(realbuf, capacity * realsize);
    }
    if (realsize == 4) {
      ((float *)realbuf)[*len] = (float)strtod(s, &p);
    } else {
      ((double *)realbuf)[*len] = strtod(s, &p);
    }
    (*len) += 1;
    if (p == s) {
      fprintf(stderr, "Parse error on line %d in file %s: invalid floating point number.\n", *len, filename);
      exit(SF_EXIT_INVALID_CONFIG);
    }
    if (maxitems > 0 && (*len) == maxitems) {
      break;
    }
  }
  realbuf = erealloc(realbuf, (*len) * realsize);
  return realbuf;
}

void * sndfile_read(const char* filename, int *totitems, struct coeff *coeff, int channel, SF_INFO snfinfo, int realsize, int maxitems)
{
  SNDFILE *infile;
  int n_items, i, j;
  float *fbuffer = NULL;
  double *dbuffer = NULL;
  void *realbuf;
  
  *totitems = 0;
  if ((infile = sf_open(filename, SFM_READ, &snfinfo)) == NULL) {
    return NULL;
  }
  if (realsize == 4) {
    fbuffer = new float [snfinfo.channels * snfinfo.frames]();
    if (fbuffer == NULL) {
      fprintf(stderr, "Could not allocate memory for sndfile data\n");
      sf_close(infile);
      return NULL;
    }
    if (coeff->skip > 0 && sf_seek(infile, coeff->skip, SEEK_SET) != coeff->skip) {
      fprintf(stderr, "Could not skip %i frames at %s.\n",coeff->skip, filename);
      sf_close(infile);
      return NULL;
    }
    if ((n_items = sf_readf_float(infile, fbuffer, maxitems)) < maxitems) {
      fprintf(stderr, "Did not read enough frames from %s: %d instead of %d\n", filename, n_items, maxitems);
      sf_close(infile);
      efree(fbuffer);
      return NULL;
    }
    *totitems += n_items;
    if (maxitems > 0 && *totitems >= maxitems) {
      *totitems = maxitems;
    }
    realbuf = (void*) emalloc((*totitems) * realsize);
    if (realbuf == NULL) {
      fprintf(stderr, "Could not allocate memory for sndfile data\n");
      sf_close(infile);
      return NULL;
    }
    for (i = channel, j = 0; i < *totitems * snfinfo.channels; i +=snfinfo.channels, j++) {
      ((float*)realbuf)[j] = fbuffer[i]; 
    }
  } else {
    dbuffer = new double [snfinfo.channels * snfinfo.frames]();
    if (dbuffer == NULL) {
      fprintf(stderr, "Could not allocate memory for sndfile data\n");
      sf_close(infile);
      return NULL;
    }
    if (coeff->skip > 0 && sf_seek(infile, coeff->skip, SEEK_SET) != coeff->skip) {
      fprintf(stderr, "Could not skip %i frames at %s.\n",coeff->skip, filename);
      sf_close(infile);
      return NULL;
    }
    if ((n_items = sf_readf_double(infile, dbuffer, maxitems)) < maxitems) {
      fprintf(stderr, "Did not read enough frames from %s: %d instead of %d\n", filename, n_items, maxitems);
      sf_close(infile);
      efree(fbuffer);
      return NULL;
    }
    *totitems += n_items;
    if (maxitems > 0 && *totitems >= maxitems) {
      *totitems = maxitems;
    }
    realbuf = (void*) emalloc((*totitems) * realsize);
    for (i = channel, j = 0; i < *totitems * snfinfo.channels;  i +=snfinfo.channels, j++) {
      ((double*)realbuf)[j] = dbuffer[i]; 
    }
    efree(dbuffer);
  }
  sf_close(infile);
  return realbuf;
}

int number_of_cpus(void)
{
  FILE *stream;
  char s[1000];
  int n_cpus = 0;

  /* This code is Linux specific... */
    
  if ((stream = fopen("/proc/cpuinfo", "rt")) == NULL) {
    fprintf(stderr, "Warning: could not determine number of processors, guessing 1.\n");
    return 1;
  }
  s[999] = '\0';
  while (fgets(s, 999, stream) != NULL) {
    if (strncasecmp(s, "processor", 9) == 0) {
      n_cpus++;
    }
  }
  fclose(stream);
  if (n_cpus == 0) {
    n_cpus = 1;
  }
  return n_cpus;
}

SfConf::SfConf(SfRun *_sfrun)
{
  sfconf = _sfrun->sfconf;
  icomm = _sfrun->icomm;
  sfDai = _sfrun->sfDai;
  sfRun = _sfrun;
}

SfConf::~SfConf(void)
{
}

struct coeff* SfConf::parse_coeff(xmlNodePtr xmlnode, int intname)
{
  struct coeff *coeff;
  uint32_t bitset = 0;
  char* coeffName;

  coeff = new struct coeff();
  coeff->scale = 1.0;
  coeff->coeff.n_blocks = -1;
  coeff->coeff.intname = intname;
  
  while (xmlnode != NULL) {
    if (!xmlStrcmp(xmlnode->name, (const xmlChar *)"name")) { 
      coeffName = estrdup((char*)xmlNodeGetContent(xmlnode));
      coeff->coeff.name = static_cast<char*>(coeffName);
      field_repeat_test(&bitset, "name", 0);
    } else if (!xmlStrcmp(xmlnode->name, (const xmlChar *)"format")) { 
      field_repeat_test(&bitset, "format", 1);
      if (strcasecmp((char*)xmlNodeGetContent(xmlnode), "text") == 0) {
	coeff->format = COEFF_FORMAT_TEXT;
      } else if (strcasecmp((char*)xmlNodeGetContent(xmlnode), "sndfile") == 0) {
	coeff->format = COEFF_FORMAT_SNDFILE;
      } else {
	coeff->format = COEFF_FORMAT_RAW;
	parse_coeff_sample_format(&coeff->sfinfo, reinterpret_cast<char*>(xmlNodeGetContent(xmlnode)));
      }
    } else if  (!xmlStrcmp(xmlnode->name, (const xmlChar *)"attenuation")) {
      field_repeat_test(&bitset, "attenuation", 2);
      coeff->scale = FROM_DB(strtof((char*)xmlNodeGetContent(xmlnode), NULL));
    } else if  (!xmlStrcmp(xmlnode->name, (const xmlChar *)"channel")) {
      coeff->channel = strtol((char*)xmlNodeGetContent(xmlnode), NULL, 10);
    } else if  (!xmlStrcmp(xmlnode->name, (const xmlChar *)"filename")) {
      field_repeat_test(&bitset, "filename", 3);
      strncpy(coeff->filename,(char*)xmlNodeGetContent(xmlnode) , PATH_MAX);
      coeff->filename[PATH_MAX-1] = '\0';
    } else if (!xmlStrcmp(xmlnode->name, (const xmlChar *)"blocks")) {
      field_repeat_test(&bitset, "blocks", 4);
      coeff->coeff.n_blocks = strtol((char*) xmlNodeGetContent(xmlnode), NULL, 10);
    }  else if (!xmlStrcmp(xmlnode->name, (const xmlChar *)"skip")) {
      field_repeat_test(&bitset, "skip", 5);
      coeff->skip = strtol((char*) xmlNodeGetContent(xmlnode), NULL, 10);
    }
    xmlnode = xmlnode->next;
  }
  if (strcmp(coeff->filename, "dirac pulse") == 0) {
    coeff->coeff.n_blocks = sfconf->n_blocks;
    field_mandatory_test(bitset, 0x01, "coeff");
  } else {
    field_mandatory_test(bitset, 0x0B, "coeff");
  } 
  return coeff;
}

void SfConf::parse_filter_io_array(xmlNodePtr xmlnode, struct filter *filter, int io, bool isfilter)
{
  int len = -1;
    
  do {
    if (!xmlStrcmp(xmlnode->name, (const xmlChar *)"name")) {
      len++;
      if (isfilter) {
	filter->filter.filters[io].push_back(0);
	filter->filter_name[io].push_back(reinterpret_cast<char*>(xmlNodeGetContent(xmlnode)));
	if (io == IN) {
	  filter->fctrl.fscale.push_back(1.0);
	}
      } else {
	filter->filter.channels[io].push_back(0);
	filter->channel_name[io].push_back(reinterpret_cast<char*>(xmlNodeGetContent(xmlnode)));
	filter->fctrl.scale[io].push_back(1.0);
      }
    } else if (!xmlStrcmp(xmlnode->name, (const xmlChar *)"attenuation")) 
      if (io == OUT && isfilter) {
	parse_error("cannot scale filter outputs which are connected to other filter inputs.\n");
      } else if (isfilter) {
	filter->fctrl.fscale[len] *= FROM_DB((float)strtof((char *) xmlNodeGetContent(xmlnode), NULL));
      } else {
	filter->fctrl.scale[io][len] *= FROM_DB((float)strtof((char *) xmlNodeGetContent(xmlnode), NULL));
      }
    else if (!xmlStrcmp(xmlnode->name, (const xmlChar *)"multiplier"))
      if (io == OUT && isfilter) {
	parse_error("cannot scale filter outputs which are connected to other filter inputs.\n");
      } else if (isfilter) {
	filter->fctrl.fscale[len] *= FROM_DB((float)strtof((char *) xmlNodeGetContent(xmlnode), NULL));
      } else {
	filter->fctrl.scale[io][len] *= FROM_DB((float)strtof((char *) xmlNodeGetContent(xmlnode), NULL));
      }
    else if (xmlStrcmp(xmlnode->name, (const xmlChar *)"text")) {
      fprintf(stderr,"Unexpected token:%s\n",xmlnode->name);
      parse_error("unexpected token.\n");
    } 
    xmlnode = xmlnode->next;
  } while (xmlnode != NULL);

  if (isfilter) {
    filter->filter.n_filters[io] = filter->filter.filters[io].size();
  } else {
    filter->filter.n_channels[io] = filter->filter.channels[io].size();
  }
}

struct filter* SfConf::parse_filter(xmlNodePtr xmlnode, int intname)
{
  struct filter *filter;
  uint32_t bitset = 0;
  char msg[200];
  
  filter = new struct filter();       
  filter->fctrl.coeff = -1;
  filter->process = 0;    
  filter->filter.intname = intname;
    
  do {
    if (!xmlStrcmp(xmlnode->name, (const xmlChar *)"name")) {
      filter->filter.name = reinterpret_cast<char*>(xmlNodeGetContent(xmlnode));
    } else if (!xmlStrcmp(xmlnode->name, (const xmlChar *)"process")) { 
      field_repeat_test(&bitset, "process", 0);
      filter->process = strtol((char*)xmlNodeGetContent(xmlnode), NULL, 10);
      if (filter->process >= SF_MAXPROCESSES) {
	sprintf(msg, "process is less than 0 or larger than %d.\n", SF_MAXPROCESSES - 1);
	parse_error(msg);
      }
      if (filter->process < 0) {
	filter->process = -1;
      }
    } else if (!xmlStrcmp(xmlnode->name, (const xmlChar *)"coeff_name")) { 
      field_repeat_test(&bitset, "coeff_name", 1);
      filter->coeff_name = reinterpret_cast<char*>(xmlNodeGetContent(xmlnode));
    } else if (!xmlStrcmp(xmlnode->name, (const xmlChar *)"coeff_index")) { 
      field_repeat_test(&bitset, "coeff_index", 1);
      filter->fctrl.coeff = strtol((char*)xmlNodeGetContent(xmlnode), NULL, 10);
    } else if (!xmlStrcmp(xmlnode->name, (const xmlChar *)"from_inputs")) {
      field_repeat_test(&bitset, "from_inputs", 2);
      parse_filter_io_array(xmlnode->children, filter, IN, false); 
    } else if  (!xmlStrcmp(xmlnode->name, (const xmlChar *)"to_outputs")) {
      field_repeat_test(&bitset, "to_outputs", 3);
	parse_filter_io_array(xmlnode->children, filter, OUT, false);
    } else if (!xmlStrcmp(xmlnode->name, (const xmlChar *)"from_filters")) {
      field_repeat_test(&bitset, "from_filters", 4);
      parse_filter_io_array(xmlnode->children, filter, IN, true);
    } else if (!xmlStrcmp(xmlnode->name, (const xmlChar *)"to_filters")) {
      field_repeat_test(&bitset, "to_filters", 5);
      parse_filter_io_array(xmlnode->children, filter, OUT, true);
    } else if (!xmlStrcmp(xmlnode->name, (const xmlChar *)"delay")) {
      field_repeat_test(&bitset, "delay", 6);
      filter->fctrl.delayblocks = strtol((char*)xmlNodeGetContent(xmlnode), NULL, 10);
      if (filter->fctrl.delayblocks < 0) {
	filter->fctrl.delayblocks = 0;
      }
    } else if (!xmlStrcmp(xmlnode->name, (const xmlChar *)"crossfade")) {
      field_repeat_test(&bitset, "crossfade", 7);
      filter->filter.crossfade = (!xmlStrcmp(xmlNodeGetContent(xmlnode), (const xmlChar *)"true") ? true : false); 
    } else if (xmlStrcmp(xmlnode->name, (const xmlChar *)"text") && xmlStrcmp(xmlnode->name, (const xmlChar *)"comment")) {
      parse_error("unrecognized filter field token\n");
    }
    xmlnode = xmlnode->next;
  } while(xmlnode != NULL);

  if (!bit_isset(&bitset, 5) && !bit_isset(&bitset, 3)) {
    parse_error("no outputs for filter.\n");
  }
  if (!bit_isset(&bitset, 4) && !bit_isset(&bitset, 2)) {
    parse_error("no inputs for filter.\n");
  }
  field_mandatory_test(bitset, 0x2, "filter");

  /* some sanity checks and completion of inputs, outputs and coeff fields
     cannot be done until whole configuration has been read */

  return filter;
}

struct iodev* SfConf::parse_iodev(xmlNodePtr xmlnode, int io)
{
  xmlNodePtr xmlchild1, xmlchild2;
  struct iodev *iodev;
  uint32_t bitset = 0; 
  int maxdelay_setting = -2, indmaxd_count = 0;
  int n;

  iodev = new struct iodev();
  MEMSET(iodev, 0, sizeof(struct iodev));
  iodev->auto_format = true;

  while (xmlnode != NULL) {
    if (!xmlStrcmp(xmlnode->name, (const xmlChar *)"params")) {
      iodev->device_params = xmlnode->children;
      xmlchild1 = xmlnode->children;
      while(xmlchild1 != NULL) {
	if (!xmlStrcmp(xmlchild1->name, (const xmlChar *)"port")) {
	  xmlchild2 = xmlchild1->children;
	  while(xmlchild2 != NULL) {
	    if (!xmlStrcmp(xmlchild2->name, (const xmlChar *)"index")) {
	      iodev->ch.channel_selection.push_back(strtol((char*)xmlNodeGetContent(xmlchild2), NULL, 10));
	    } else if (!xmlStrcmp(xmlchild2->name, (const xmlChar *)"mute")) {
	      sfconf->mute[io].resize(sfconf->n_channels[io] + iodev->ch.used_channels + 1);
	      sfconf->mute[io][sfconf->n_channels[io] + iodev->ch.used_channels] = (!xmlStrcmp(xmlNodeGetContent(xmlchild2),(const xmlChar *)"true") ? true : false);
	      // individual_maxdelay sub-tag
	    } else if (!xmlStrcmp(xmlchild2->name, (const xmlChar *)"individual_maxdelay")) {
	      sfconf->maxdelay[io][sfconf->n_channels[io] + iodev->ch.used_channels] = strtol((char*) xmlNodeGetContent(xmlchild2), NULL, 10);
	      indmaxd_count++;
	      sfconf->maxdelay[io].resize(sfconf->n_channels[io] + iodev->ch.used_channels + 1);
	      if (sfconf->maxdelay[io][sfconf->n_channels[io] + iodev->ch.used_channels] < 0) {
		sfconf->maxdelay[io][sfconf->n_channels[io] + iodev->ch.used_channels] = -1;
	      }
	      if (sfconf->maxdelay[io][sfconf->n_channels[io] + iodev->ch.used_channels] > maxdelay[io]) {
		maxdelay[io] = sfconf->maxdelay[io][sfconf->n_channels[io] + iodev->ch.used_channels];
	      }
	    } 
	    xmlchild2 = xmlchild2->next;
	  }
	  iodev->ch.used_channels++;
	  iodev->ch.open_channels++;
	}
	xmlchild1 = xmlchild1->next;
      }
      if(sfconf->delay[io].empty())
	sfconf->delay[io].resize(sfconf->n_channels[io] + iodev->ch.used_channels + 1, 0);
      if(sfconf->mute[io].empty())
	sfconf->mute[io].resize(sfconf->n_channels[io] + iodev->ch.used_channels + 1, false);
      if(sfconf->maxdelay[io].empty())
	sfconf->maxdelay[io].resize(sfconf->n_channels[io] + iodev->ch.used_channels + 1, 0);
    } else if (!xmlStrcmp(xmlnode->name, (const xmlChar *)"dither")) {
      field_repeat_test(&bitset, "dither", 4);
      if (io == IN) {
	parse_error("input field does not support dither.\n");
      }
      iodev->apply_dither = (!xmlStrcmp(xmlNodeGetContent(xmlnode), (const xmlChar *)"true") ? true : false);
    } else if (!xmlStrcmp(xmlnode->name, (const xmlChar *)"channel")) {
      xmlchild1 = xmlnode->children;
      sfconf->delay[io].resize(sfconf->n_channels[io] + iodev->virtual_channels + 1);
      while(xmlchild1 != NULL) {
	if (!xmlStrcmp(xmlchild1->name, (const xmlChar *)"name")) {
	  iodev->channel_intname.push_back(sfconf->n_channels[io] + iodev->virtual_channels);
	  iodev->virt2phys.push_back(iodev->virtual_channels);
	  iodev->channel_name.push_back(string((char*)xmlNodeGetContent(xmlchild1)));
	} else  if (!xmlStrcmp(xmlchild1->name, (const xmlChar *)"mapping")) {
	  iodev->virt2phys[iodev->virt2phys.size()-1] = strtol((char*)xmlNodeGetContent(xmlchild1), NULL, 10);
	} else if (!xmlStrcmp(xmlchild1->name, (const xmlChar *)"delay")) {
	  sfconf->delay[io][sfconf->n_channels[io] + iodev->virtual_channels] = strtol((char*) xmlNodeGetContent(xmlchild1), NULL, 10);
	  if (sfconf->delay[io][sfconf->n_channels[io] + iodev->virtual_channels] < 0) {
	    parse_error("negative delay.\n");
	  }
	  if (sfconf->delay[io][sfconf->n_channels[io] +iodev->virtual_channels] > maxdelay[io]) {
	    maxdelay[io] = sfconf->delay[io][sfconf->n_channels[io] + iodev->virtual_channels];
	  }
	}
	xmlchild1 = xmlchild1->next;
      }
      iodev->virtual_channels++;
    }
    xmlnode = xmlnode->next;
  } 
  if (iodev->ch.used_channels > iodev->virtual_channels) {
    parse_error("channel amount exceeds allocated.\n");
  }
  if (iodev->ch.used_channels > iodev->ch.open_channels) {
    parse_error("channel amount mismatch.\n");
  }
  for (n = 0; n < iodev->ch.used_channels; n++) {
    iodev->ch.channel_name.push_back(sfconf->n_physical_channels[io] + n);
  }
  if (maxdelay_setting != -2) {
    for (n = indmaxd_count; n < iodev->virtual_channels; n++) {
      sfconf->maxdelay[io][sfconf->n_channels[io] + n] = maxdelay_setting;
    }
  } 
  for (n = 0; n < iodev->virtual_channels; n++) {
    if (iodev->virt2phys[n] < 0 || iodev->virt2phys[n] >= iodev->ch.used_channels) {
      parse_error("invalid channel mapping.\n");
    }
  }	
  if (bit_isset(&bitset, 8) && iodev->virtual_channels <= iodev->ch.used_channels) {
    parse_error("virtual mapping only allowed when virtual channel amount exceeds physical.\n");
  }
  for (n = 0; n < iodev->virtual_channels; n++) {
    if (sfconf->maxdelay[io][sfconf->n_channels[io] + n] >= 0 && sfconf->delay[io][iodev->virt2phys[n]] > sfconf->maxdelay[io][sfconf->n_channels[io] + n]) {
      parse_error("delay exceeds specified maximum delay.\n");
    }
  }
  return iodev;
}

void SfConf::parse_setting(xmlNodePtr xmlnode, uint32_t *repeat_bitset)
{
  char msg[200];
  int n; 
  xmlNodePtr xmlchild;

  //field_repeat_test(repeat_bitset, 1);
  //field_repeat_test(repeat_bitset, 15);
  while (xmlnode!=NULL) {
    if (!xmlStrcmp(xmlnode->name, (const xmlChar *)"sampling_rate")) {
      field_repeat_test(repeat_bitset, "logic", 0);
      if (sfconf->sampling_rate == 0)
	sfconf->sampling_rate = strtof((char*)xmlNodeGetContent(xmlnode), NULL);
      else {
	sprintf(msg, "Repeated sampling_rate field.\n");
	parse_error_exit(msg);
      }
      if (sfconf->sampling_rate <= 0) {
	sprintf(msg,"invalid sampling_rate %d.\n",sfconf->sampling_rate);
	parse_error_exit(msg);
      }
    } else if (!xmlStrcmp(xmlnode->name, (const xmlChar *)"logic")) {
      field_repeat_test(repeat_bitset, "logic", 2);
      xmlchild = xmlnode->children;
      while (xmlchild != NULL) {
	if (!xmlStrcmp(xmlchild->name, (const xmlChar *)"name")) {
	  for (n = 0; n < sfconf->n_logicmods; n++) {
	    if (logic_names[n].compare(reinterpret_cast<char*>(xmlNodeGetContent(xmlchild))) == 0) {
	      pinfo("Warning: overriding parameters for module \"%s\".\n", (char*)xmlNodeGetContent(xmlchild));
	      break;
	    }
	  }
	  logic_names.push_back((char*)xmlNodeGetContent(xmlchild));
	  sfconf->n_logicmods++;
	  if (strchr((char*)xmlNodeGetContent(xmlchild), PATH_SEPARATOR_CHAR) != NULL) {
	    parse_error("path separator not allowed in module name.\n");
	  }
	} else if (!xmlStrcmp(xmlchild->name, (const xmlChar *)"params")) {
	  logic_data[sfconf->n_logicmods - 1] = xmlchild->children;
	}
	xmlchild = xmlchild->next;
      }
    } else if (!xmlStrcmp(xmlnode->name, (const xmlChar *)"overflow_warnings")) {
      field_repeat_test(repeat_bitset, "overflow_warnings", 3);
      sfconf->overflow_warnings =  (!xmlStrcmp(xmlNodeGetContent(xmlnode), (const xmlChar *)"true") ? true : false);
    } else if (!xmlStrcmp(xmlnode->name, (const xmlChar *)"overflow_control")) {
      field_repeat_test(repeat_bitset, "overflow_control", 5);
      sfconf->overflow_control =  (!xmlStrcmp(xmlNodeGetContent(xmlnode), (const xmlChar *)"true") ? true : false);
    } else if (!xmlStrcmp(xmlnode->name, (const xmlChar *)"show_progress")) {
      field_repeat_test(repeat_bitset, "show_progress", 4);
      sfconf->show_progress = (!xmlStrcmp(xmlNodeGetContent(xmlnode), (const xmlChar *)"true") ? true : false);
    } else if (!xmlStrcmp(xmlnode->name, (const xmlChar *)"filter_length")) {
      field_repeat_test(repeat_bitset, "filter_length", 7);
      //sfconf->n_blocks = 1;
      xmlchild = xmlnode->children;
      while (xmlchild != NULL) {
	if (!xmlStrcmp(xmlchild->name, (const xmlChar *)"block_length")) {
	  if (sfconf->filter_length == 0) 
	    sfconf->filter_length = strtol((char*)xmlNodeGetContent(xmlchild), NULL, 10);
	  else {
	    sprintf(msg, "Repeated field: %s.\n", xmlchild->name);
	    parse_error(msg);
	  }
	} else if (!xmlStrcmp(xmlchild->name, (const xmlChar *)"n_blocks")) {
	  if (sfconf->n_blocks == 0) 
	    sfconf->n_blocks = strtol((char*)xmlNodeGetContent(xmlchild), NULL, 10);
	  else {
	    sprintf(msg, "Repeated field: %s.\n", xmlchild->name);
	    parse_error(msg);
	  }
	} else if (xmlStrcmp(xmlchild->name, (const xmlChar *)"text")) {
	  sprintf(msg, "Unexpected field on filter_length setting: %s.\n", xmlchild->name);
	  parse_error(msg);
	}
	xmlchild = xmlchild->next;
      }
      if (sfconf->n_blocks == 0 || sfconf->filter_length == 0) {
	sprintf(msg, "filter_length field: block_length and n_blocks must be set.");
	parse_error_exit(msg);
      }
      if (log2_get(sfconf->filter_length) == -1 || sfconf->n_blocks * sfconf->filter_length < MINFILTERLEN || 
	  sfconf->n_blocks * sfconf->filter_length > MAXFILTERLEN) {
	sprintf(msg, "filter length is not within %d - %d or not a power of 2.\n",MINFILTERLEN, MAXFILTERLEN);
	parse_error(msg);
      }
      /*} else if (!xmlStrcmp(xmlnode->name, (const xmlChar *)"lock_memory")) {
      field_repeat_test(repeat_bitset, "lock_memory", 8);
      sfconf->lock_memory = (!xmlStrcmp(xmlNodeGetContent(xmlnode), (const xmlChar *)"true") ? true : false);*/
    } else if (!xmlStrcmp(xmlnode->name, (const xmlChar *)"debug")) {
      field_repeat_test(repeat_bitset, "debug", 10);
      sfconf->debug = (!xmlStrcmp(xmlNodeGetContent(xmlnode), (const xmlChar *)"true") ? true : false);
    } else if (!xmlStrcmp(xmlnode->name, (const xmlChar *)"powersave")) {
      field_repeat_test(repeat_bitset, "powersave", 11);
      sfconf->analog_powersave = 1.0;
      if (!xmlStrcmp(xmlNodeGetContent(xmlnode), (const xmlChar *)"true")) {
	sfconf->powersave = true;
      } else if (!xmlStrcmp(xmlNodeGetContent(xmlnode), (const xmlChar *)"false")) {
	sfconf->powersave = false;
      } else {
	sfconf->analog_powersave = FROM_DB((float)strtof((char*)xmlNodeGetContent(xmlnode), NULL));
	if (sfconf->analog_powersave < 1.0) {
	  sfconf->powersave = true;
	}
      }  
    } else if (!xmlStrcmp(xmlnode->name, (const xmlChar *)"allow_poll_mode")) {
      field_repeat_test(repeat_bitset, "allow_poll_mode", 12);
      sfconf->allow_poll_mode = (!xmlStrcmp(xmlNodeGetContent(xmlnode), (const xmlChar *)"true") ? true : false);
    } else if (!xmlStrcmp(xmlnode->name, (const xmlChar *)"float_bits")) {
      field_repeat_test(repeat_bitset, "float_bits", 13);
      sfconf->realsize = strtol((char*)xmlNodeGetContent(xmlnode), NULL, 10);
      if (sfconf->realsize != sizeof(float) * 8 && sfconf->realsize != sizeof(double) * 8) {
	sprintf(msg, "invalid float_bits, must be %d or %d.\n",sizeof(float) * 8, sizeof(double) * 8); 
	parse_error(msg);
      }
      sfconf->realsize /= 8;
    } else  if (xmlStrcmp(xmlnode->name, (const xmlChar *)"text") && xmlStrcmp(xmlnode->name, (const xmlChar *)"comment") ) {
      sprintf(msg, "unrecognised setting field: %s\n", xmlnode->name);
      parse_error(msg);
    } 
    xmlnode = xmlnode->next;
  }
  if (sfconf->sampling_rate == 0) {
    sprintf(msg, "globals --> sampling_rate field not set. Exiting.\n");
    parse_error_exit(msg);
  }
  if (sfconf->filter_length == 0) {
    sprintf(msg, "globals --> block_length field not set. Exiting.\n");
    parse_error_exit(msg);
  }
  if (sfconf->n_blocks == 0) {
    sprintf(msg, "globals --> n_blocks field not set. Exiting.\n");
    parse_error_exit(msg);
  }
}

void* SfConf::load_coeff(struct coeff *lcoeff, int cindex)
{
    void *coeffsbuf, *zbuf = NULL;
    FILE *stream = NULL;
    void **cbuf;
    int n, len;

    cbuf = (void**)emalloc(lcoeff->coeff.n_blocks * sizeof(void **));

    if (strcmp(lcoeff->filename, "dirac pulse") == 0) {
	len = lcoeff->coeff.n_blocks * sfconf->filter_length;
	coeffsbuf = emalloc(len * sfconf->realsize);
	MEMSET(coeffsbuf, 0, len * sfconf->realsize);
        if (sfconf->realsize == 4) {
            ((float *)coeffsbuf)[0] = 1.0;
        } else {
            ((double *)coeffsbuf)[0] = 1.0;
        }
    } else {
      switch (lcoeff->format) {
      case COEFF_FORMAT_TEXT:
	if ((stream = fopen(lcoeff->filename, lcoeff->format == COEFF_FORMAT_TEXT ? "rt" : "rb")) == NULL) {
	  fprintf(stderr, "Could not open \"%s\" for reading.\n", lcoeff->filename);
	  exit(SF_EXIT_OTHER);
	}
        if (lcoeff->skip > 0) {
	  fseek(stream, lcoeff->skip, SEEK_SET);
	  if (ftell(stream) != lcoeff->skip) {
	    fprintf(stderr, "Failed to skip %d bytes of file \"%s\".\n", lcoeff->skip, lcoeff->filename);
	    exit(SF_EXIT_OTHER);
	  }
        }
	coeffsbuf = real_read(stream, &len, lcoeff->filename, sfconf->realsize, lcoeff->coeff.n_blocks * sfconf->filter_length);
	break;
      case COEFF_FORMAT_SNDFILE:
	coeffsbuf = sndfile_read(lcoeff->filename, &len,  lcoeff, lcoeff->channel, lcoeff->sfinfo, sfconf->realsize, lcoeff->coeff.n_blocks * sfconf->filter_length);
	if(coeffsbuf == NULL) {
	  fprintf(stderr, "Error reading %s coeff file\n",lcoeff->filename);
	  exit(SF_EXIT_INVALID_CONFIG);
	}
	break;
      case COEFF_FORMAT_RAW:
	coeffsbuf = sndfile_read(lcoeff->filename, &len,  lcoeff, lcoeff->channel, lcoeff->sfinfo, sfconf->realsize, lcoeff->coeff.n_blocks * sfconf->filter_length);
	if(coeffsbuf == NULL) {
	  fprintf(stderr, "Error reading %s coeff file\n",lcoeff->filename);
	  exit(SF_EXIT_INVALID_CONFIG);
	}
	break;
      default:
	fprintf(stderr, "Invalid format: %d.\n", lcoeff->format);
	exit(SF_EXIT_INVALID_CONFIG);
      }
    }
    
    if (len < lcoeff->coeff.n_blocks * sfconf->filter_length) {
      zbuf = emalloc(sfconf->filter_length * sfconf->realsize);
      MEMSET(zbuf, 0, sfconf->filter_length * sfconf->realsize);
    }
    for (n = 0; n < lcoeff->coeff.n_blocks; n++) {
      if (n * sfconf->filter_length > len) {
	cbuf[n] = sfConv->convolver_coeffs2cbuf(zbuf, sfconf->filter_length, lcoeff->scale);
      } else if ((n + 1) * sfconf->filter_length > len) {
	cbuf[n] = sfConv->convolver_coeffs2cbuf(&((uint8_t *)coeffsbuf)[n * sfconf->filter_length * sfconf->realsize], len - n * sfconf->filter_length, lcoeff->scale);
      } else {
	cbuf[n] = sfConv->convolver_coeffs2cbuf(&((uint8_t *)coeffsbuf)[n * sfconf->filter_length * sfconf->realsize], sfconf->filter_length, lcoeff->scale);
      }
      if (cbuf[n] == NULL) {
	fprintf(stderr, "Failed to preprocess coefficients in file %s.\n", lcoeff->filename);
	exit(SF_EXIT_OTHER);
      }
    }
    efree(zbuf);
    efree(coeffsbuf);
#if 0    
    if (sfconf->debug) {
      char filename[1024];                
      sprintf(filename, "brutefir-%d-coeffs-%d.txt", getpid(), cindex);
      sfConv->convolver_debug_dump_cbuf(filename, cbuf, lcoeff->coeff.n_blocks);
    }
#endif    
    return cbuf;
}

bool SfConf::filter_loop(int source_intname, int search_intname)
{
  int n;
  
  for (n = 0; n < sfconf->filters[search_intname].n_filters[OUT]; n++) {
    if (sfconf->filters[search_intname].filters[OUT][n] == source_intname ||
	filter_loop(source_intname, sfconf->filters[search_intname].filters[OUT][n])) {
      return true;
    }
  }
  return false;
}


void SfConf::sfconf_init(char *filename)
{
  char current_filename[PATH_MAX];
  //vector<struct iodev *> iodevs[2];
  struct iodev *iodevs[2];
  vector<struct filter*> pfilters;
  vector<struct coeff*> coeffslist;
  //struct sffilter filters[SF_MAXFILTERS];
  //struct dither_state *dither_state[SF_MAXCHANNELS];
  struct timeval tv1, tv2;
  int largest_process = -1;
  int min_prio, max_prio;
  uint32_t used_channels[2][SF_MAXCHANNELS / 32 + 1];
  uint32_t used_filters[SF_MAXFILTERS / 32 + 1];
  uint32_t local_used_channels[SF_MAXCHANNELS / 32 + 1];
  uint32_t apply_dither[SF_MAXCHANNELS / 32 + 1];
  uint32_t used_processes[SF_MAXPROCESSES / 32 + 1];
  uint32_t repeat_bitset = 0;
  int n, i, j, k, io, virtch, physch;
  unsigned int l;
  bool load_balance = false;
  int *channels[2];
  uint64_t t1, t2;
  char str[200];
  int IO;
  int rlogic;
  
  xmlDocPtr xmlconf;
  xmlNodePtr xmlnode, xmlchildren, xmlsmoothfir;
  
  gettimeofday(&tv1, NULL);
  timestamp(&t1);
  
  channels[0] = new int [SF_MAXCHANNELS]();
  channels[1] = new int [SF_MAXCHANNELS]();

  sfconf->quiet = quiet;
  sfconf->realsize = sizeof(float);
  
  strcpy(current_filename, filename);
  current_filename[PATH_MAX-1] = '\0';
  
  // Open document
  xmlconf = xmlReadFile(current_filename, NULL, 0);
  if (xmlconf == NULL ) {
    fprintf(stderr,"Document cannot be not parsed.\n");
    exit(SF_EXIT_OTHER);
  }
  
  xmlnode = xmlDocGetRootElement(xmlconf);
  if (xmlnode == NULL) {
    fprintf(stderr,"Empty document found.\n");
    xmlCleanupParser();
    xmlFreeDoc(xmlconf);
    exit(SF_EXIT_OTHER);
  }
  xmlnode = xmlnode->children;
  xmlsmoothfir = NULL;

  while (xmlnode !=NULL) {
    if (!xmlStrcmp(xmlnode->name, (const xmlChar *)"smoothfir")) {
      fprintf(stderr,"<smoothfir> token found.\n");
      xmlsmoothfir = xmlnode;
      break;
    } else {
      xmlchildren = xmlnode->children;
      while (xmlchildren != NULL) {
	if (!xmlStrcmp(xmlchildren->name, (const xmlChar *)"smoothfir")) {
	  fprintf(stderr,"<smoothfir> token found.\n");
	  xmlsmoothfir = xmlchildren;
	  break;
	}	
	xmlchildren = xmlchildren->next;
      }
    }
    xmlnode = xmlnode->next;
  }
  if (xmlsmoothfir == NULL) {
    fprintf(stderr,"<smoothfir> token not found.\n");
    xmlCleanupParser();
    xmlFreeDoc(xmlconf);
    exit(SF_EXIT_OTHER);
  }
  maxdelay[IN] = maxdelay[OUT] = 0;
  xmlnode = xmlsmoothfir->children;
  while (xmlnode != NULL)  {
    if (!xmlStrcmp(xmlnode->name, (const xmlChar *)"globals")) {
      parse_setting(xmlnode->children, &repeat_bitset);
    }
    else if (!xmlStrcmp(xmlnode->name, (const xmlChar *)"coeff")) {
      coeffslist.push_back(parse_coeff(xmlnode->children, sfconf->n_coeffs));
      sfconf->n_coeffs++;
    }
    else if (!xmlStrcmp(xmlnode->name, (const xmlChar *)"input") || !xmlStrcmp(xmlnode->name, (const xmlChar *)"output")) {
      io = (!xmlStrcmp(xmlnode->name, (const xmlChar *)"input")) ? IN : OUT;
      if(sfconf->n_physical_channels[io] > 0) {
	sprintf(str, "%s config repeated.\n",(io == IN) ? "Input" : "Output");
	parse_error(str);
      }
      iodevs[io] = parse_iodev(xmlnode->children, io);
      sfconf->n_physical_channels[io] += iodevs[io]->ch.used_channels;
      sfconf->n_channels[io] += iodevs[io]->virtual_channels;
    }
    else if (!xmlStrcmp(xmlnode->name, (const xmlChar *)"filter")) {
      if (sfconf->n_filters == SF_MAXFILTERS) {
	parse_error("too many filters.\n");
      }
      pfilters.push_back(parse_filter(xmlnode->children, sfconf->n_filters));
      sfconf->n_filters++;
    }
    else if (xmlStrcmp(xmlnode->name, (const xmlChar *)"comment") && xmlStrcmp(xmlnode->name, (const xmlChar *)"text")) {
      fprintf(stderr,"Unexpected token:%s\n",xmlnode->name);
      parse_error("unexpected token.\n");
      }
    xmlnode = xmlnode->next;
  }
  
  field_mandatory_test(repeat_bitset, 0x0081, filename);
  /*#ifdef CONVOLVER_NEEDS_CONFIGFILE
  field_mandatory_test(repeat_bitset, 0x4081, filename);
#else
  field_mandatory_test(repeat_bitset, 0x0081, filename);
#endif*/
  
  pinfo("Internal resolution is %d bit floating point.\n",sfconf->realsize * 8);
  
  /* do some sanity checks */
  FOR_IN_AND_OUT {
    if (sfconf->n_physical_channels[IO] == 0) {
      fprintf(stderr, "No %s defined.\n",(IO == IN) ? "inputs" : "outputs");
      exit(SF_EXIT_INVALID_CONFIG);
    }
  }
  if (sfconf->n_filters == 0) {
    fprintf(stderr, "No filters defined.\n");
    exit(SF_EXIT_INVALID_CONFIG);
  }
  if (sfconf->benchmark && sfconf->powersave) {
    fprintf(stderr, "The benchmark and powersave setting cannot both be set to true.\n");
    exit(SF_EXIT_INVALID_CONFIG);
  }

  /* create the channel arrays */
  FOR_IN_AND_OUT {
    sfconf->channels[IO].resize(sfconf->n_channels[IO]);
    sfconf->virt2phys[IO].resize(sfconf->n_channels[IO]);
    sfconf->n_virtperphys[IO].resize(sfconf->n_physical_channels[IO]);
    sfconf->phys2virt[IO].resize(sfconf->n_physical_channels[IO]);
    for (i = 0; i < iodevs[IO]->virtual_channels; i++) {
      virtch = iodevs[IO]->channel_intname[i];
      physch = iodevs[IO]->ch.channel_name[0] + iodevs[IO]->virt2phys[i];
      sfconf->channels[IO][virtch].intname = virtch;
      sfconf->virt2phys[IO][virtch] = physch;
      sfconf->n_virtperphys[IO][physch]++;
      sfconf->channels[IO][virtch].name =  iodevs[IO]->channel_name[i];
    }
    for (i = 0; i < iodevs[IO]->ch.used_channels; i++) {
      physch = iodevs[IO]->ch.channel_name[i];
      sfconf->phys2virt[IO][physch].resize(sfconf->n_virtperphys[IO][physch]);
      /* set all values to -1 */
      for(j = 0; j < sfconf->n_virtperphys[IO][physch]; j++)
	sfconf->phys2virt[IO][physch][j] = -1;
    }
    for (i = 0; i < iodevs[IO]->virtual_channels; i++) {
      physch = iodevs[IO]->ch.channel_name[0] + iodevs[IO]->virt2phys[i];
      for (k = 0; sfconf->phys2virt[IO][physch][k] != -1; k++);
      sfconf->phys2virt[IO][physch][k] = iodevs[IO]->channel_intname[i];
    }
  }
  //}
  /* check for duplicate names */
  for (n = 0; n < sfconf->n_coeffs; n++) {
    for (i = 0; i < sfconf->n_coeffs; i++) {
      if (n != i && coeffslist[i]->coeff.name.compare(coeffslist[n]->coeff.name) == 0) {
	fprintf(stderr, "Duplicate coefficient set names.\n");
	exit(SF_EXIT_INVALID_CONFIG);
      }				 
    }
  }
  for (n = 0; n < sfconf->n_filters; n++) {
    for (i = 0; i < sfconf->n_filters; i++) {
      if (n != i && pfilters[i]->filter.name.compare(pfilters[n]->filter.name) == 0) {
	fprintf(stderr, "Duplicate filter names.\n");
	exit(SF_EXIT_INVALID_CONFIG);
      }				 
    }
  }
  FOR_IN_AND_OUT {
    for (n = 0; n < sfconf->n_channels[IO]; n++) {
      for (i = 0; i < sfconf->n_channels[IO]; i++) {
	if (n != i && sfconf->channels[IO][i].name.compare(sfconf->channels[IO][n].name) == 0) {
	  fprintf(stderr, "Duplicate channel names.\n");
	  exit(SF_EXIT_INVALID_CONFIG);
	}		 
      }
    }
  }
  
  /* finish and sanity check filter structures */
  MEMSET(used_channels, 0, sizeof(used_channels));
  MEMSET(used_processes, 0, sizeof(used_processes));
  for (n = 0; n < sfconf->n_filters; n++) {
    /* coefficient name */
    if (pfilters[n]->coeff_name[0] == '\0') {
      if (pfilters[n]->fctrl.coeff >= sfconf->n_coeffs) {
	fprintf(stderr, "Coeff index %d in filter %d/\"%s\" is out of range.\n", pfilters[n]->fctrl.coeff, n, pfilters[n]->filter.name.data());
	exit(SF_EXIT_INVALID_CONFIG);
      }
    } else {
      pfilters[n]->fctrl.coeff = -1;
      for (l = 0; l < coeffslist.size(); l++) {
	if (coeffslist[l]->coeff.name.compare(pfilters[n]->coeff_name) == 0) {
	  pfilters[n]->fctrl.coeff = l;
	  break;
	}
      }
      if (pfilters[n]->fctrl.coeff == -1 && !(pfilters[n]->coeff_name.compare("dirac pulse")==0)) {
	fprintf(stderr, "Coeff with name \"%s\" (in filter %d/\"%s\") does not exist.\n", pfilters[n]->coeff_name.data(), n, pfilters[n]->filter.name.data());
	exit(SF_EXIT_INVALID_CONFIG);
      }
    }
    
    if (pfilters[n]->process == -1) {
      if (n > 0 && !load_balance) {
	fprintf(stderr, "Cannot mix manual process settings with automatic.\n");
	exit(SF_EXIT_INVALID_CONFIG);
      }
      load_balance = true;
    } else {
      if (pfilters[n]->process > largest_process) {
	largest_process = pfilters[n]->process;
      }
      bit_set(used_processes, pfilters[n]->process);
      if (n > 0 && load_balance) {
	fprintf(stderr, "Cannot mix manual process settings with automatic.\n");
	exit(SF_EXIT_INVALID_CONFIG);
      }
      load_balance = false;
    }
    
    /* input/output names */
    FOR_IN_AND_OUT {
      /* channel input/outputs */
      MEMSET(local_used_channels, 0, sizeof(local_used_channels));
      for (j = 0; j < pfilters[n]->filter.n_channels[IO]; j++) {
	if (pfilters[n]->channel_name[IO][j].empty()) {
	  if (pfilters[n]->filter.channels[IO][j] < 0 || pfilters[n]->filter.channels[IO][j] >= sfconf->n_channels[IO]) {
	    fprintf(stderr, "%s channel index %d in filter %d/\"%s\" is out of range.\n", (IO == IN) ? "Input" : "Output",
		    pfilters[n]->filter.channels[IO][j],n, pfilters[n]->filter.name.data());
	    exit(SF_EXIT_INVALID_CONFIG);
	  }
	} else {
	  pfilters[n]->filter.channels[IO][j] = -1;
	  for (i = 0; i < sfconf->n_channels[IO]; i++) {
	    if (sfconf->channels[IO][i].name.compare(pfilters[n]->channel_name[IO][j]) == 0) {
	      pfilters[n]->filter.channels[IO][j] = i;
	      break;
	    }
	  }
	  if (pfilters[n]->filter.channels[IO][j] == -1) {
	    fprintf(stderr, "%s channel with name \"%s\" (in filter %d/\"%s\") does not exist.\n", (IO == IN) ? "Input" : "Output",
		    pfilters[n]->channel_name[IO][j].data(), n, pfilters[n]->filter.name.data());
	    exit(SF_EXIT_INVALID_CONFIG);
	  }
	}
	bit_set(used_channels[IO], pfilters[n]->filter.channels[IO][j]);
	if (bit_isset(local_used_channels, pfilters[n]->filter.channels[IO][j])) {
	  fprintf(stderr, "Duplicate channels in filter %d/\"%s\".\n", n, pfilters[n]->filter.name.data());
	  exit(SF_EXIT_INVALID_CONFIG);		    
	}
	bit_set(local_used_channels, pfilters[n]->filter.channels[IO][j]);
      }
      
      /* filter input/outputs */
      MEMSET(used_filters, 0, sizeof(used_filters));
      for (j = 0; j < pfilters[n]->filter.n_filters[IO]; j++) {
	if (pfilters[n]->filter_name[IO][j].empty()) {
	  if (pfilters[n]->filter.filters[IO][j] < 0 || pfilters[n]->filter.filters[IO][j] >= sfconf->n_filters) {
	    fprintf(stderr, "%s filter index %d in filter %d/\"%s\" is out of range.\n", (IO == IN) ? "Input" : "Output",
		    pfilters[n]->filter.filters[IO][j], n, pfilters[n]->filter.name.data());
	    exit(SF_EXIT_INVALID_CONFIG);
	  }
	} else {
	  pfilters[n]->filter.filters[IO][j] = -1;
	  for (i = 0; i < sfconf->n_filters; i++) {
	    if (pfilters[i]->filter.name.compare(pfilters[n]->filter_name[IO][j]) == 0) {
	      pfilters[n]->filter.filters[IO][j] = i;
	      break;
	    }
	  }
	  if (pfilters[n]->filter.filters[IO][j] == -1) {
	    fprintf(stderr, "%s filter with name \"%s\" (in filter %d/\"%s\") does not exist.\n",(IO == IN) ? "Input" : "Output",
		    pfilters[n]->filter_name[IO][j].data(), n, pfilters[n]->filter.name.data());
	    exit(SF_EXIT_INVALID_CONFIG);
	  }
	}
	if (bit_isset(used_filters, pfilters[n]->filter.filters[IO][j])) {
	  fprintf(stderr, "Duplicate filters in filter %d/\"%s\".\n", n, pfilters[n]->filter.name.data());
	  exit(SF_EXIT_INVALID_CONFIG);
	}
	bit_set(used_filters, pfilters[n]->filter.filters[IO][j]);
      }	    
    }
    
    /* finish delayblocks number */
    if (pfilters[n]->fctrl.delayblocks > sfconf->n_blocks - 1) {
      fprintf(stderr, "Delay in filter %d/\"%s\" is too large (max allowed is %d blocks, max blocks - 1).\n", n, pfilters[n]->filter.name.data(), sfconf->n_blocks - 1);
      exit(SF_EXIT_INVALID_CONFIG);
    }
  }
  
  /* check if all in/out channels are used in the filters */
  FOR_IN_AND_OUT {
    for (n = 0; n < sfconf->n_channels[IO]; n++) {
      if (!bit_isset(used_channels[IO], n)) {
	pinfo("Warning: %s channel \"%s\" is not used.\n",(IO == IN) ? "input" : "output", sfconf->channels[IO][n].name.data());
      }
    }
  }
  
  /* check if all processes are used (if manually set) */
  for (n = 0; n <= largest_process; n++) {
    if (!bit_isset(used_processes, n)) {
      fprintf(stderr, "The range of process indexes among filters is broken.\n");
      exit(SF_EXIT_INVALID_CONFIG);
    }
    }
  
  /* fill in the filters and initfctrl array */
  sfconf->filters.resize(sfconf->n_filters);
  sfconf->initfctrl.resize(sfconf->n_filters);
  for (n = 0; n < sfconf->n_filters; n++) {
    sfconf->filters[n] = pfilters[n]->filter;
    sfconf->initfctrl[n] = pfilters[n]->fctrl;
  }
  
  /* check so filters are connected, that is that output to a filter shows
     as input from source filter at the destination filter */
  for (n = 0; n < sfconf->n_filters; n++) {
    for (i = 0; i < sfconf->filters[n].n_filters[OUT]; i++) {
      k = sfconf->filters[n].filters[OUT][i];
      for (j = 0; j < sfconf->filters[k].n_filters[IN]; j++) {
	if (sfconf->filters[k].filters[IN][j] == n) {
	  break;
	}
      }
      if (j == sfconf->filters[k].n_filters[IN]) {
	fprintf(stderr,"Output to filter %d/\"%s\" from filter %d/\"%s\" must exist\nas input at at the destination filter %d/\"%s\".\n",
		k, sfconf->filters[k].name.data(), n, sfconf->filters[n].name.data(), k, sfconf->filters[k].name.data());
	exit(SF_EXIT_INVALID_CONFIG);
      }
    }
    for (i = 0; i < sfconf->filters[n].n_filters[IN]; i++) {
      k = sfconf->filters[n].filters[IN][i];
      for (j = 0; j < sfconf->filters[k].n_filters[OUT]; j++) {
	if (sfconf->filters[k].filters[OUT][j] == n) {
	  break;
	}
      }
      if (j == sfconf->filters[k].n_filters[OUT]) {
	fprintf(stderr,"Input from filter %d/\"%s\" in filter %d/\"%s\" must exist\nas output in the source filter %d/\"%s\".\n",
		k, sfconf->filters[k].name.data(), n, sfconf->filters[n].name.data(), k, sfconf->filters[k].name.data());
	exit(SF_EXIT_INVALID_CONFIG);
      }
    }
  }
  
  /* check so there are no filter loops */
  for (n = 0; n < sfconf->n_filters; n++) {
    if (filter_loop(n, n)) {
      fprintf(stderr, "Filter %d is involved in a loop.\n", n);
      exit(SF_EXIT_INVALID_CONFIG);
    }
  }
  
  /* estimate a load balancing for filters (if not manually set) */
  sfconf->n_cpus = number_of_cpus();
 
  sfConv = new Convolver(sfconf->filter_length, sfconf->realsize);
  //efree(convolver_config);
  
  sfDelay = new Delay(sfconf, SF_SAMPLE_SLOTS);
 
  /* load coefficients */
  sfconf->coeffs_data = (void***) emalloc(sfconf->n_coeffs * sizeof(void **));
  sfconf->coeffs.resize(sfconf->n_coeffs);
  if (sfconf->n_coeffs == 1) {
    pinfo("Loading coefficient set...\n");
  } else if (sfconf->n_coeffs > 1) {
    pinfo("Loading %d coefficient sets...\n", sfconf->n_coeffs);
  }
  for (n = 0; n < sfconf->n_coeffs; n++) {
    coeffslist[n]->sfinfo.samplerate = sfconf->sampling_rate;
    if (coeffslist[n]->coeff.n_blocks <= 0) {
      coeffslist[n]->coeff.n_blocks = sfconf->n_blocks;
    } else if (coeffslist[n]->coeff.n_blocks > sfconf->n_blocks) {
      fprintf(stderr, "Too many blocks in coeff %d.\n", n);
      exit(SF_EXIT_INVALID_CONFIG);
    }
    sfconf->coeffs_data[n] = (void**)load_coeff(coeffslist[n], n);
    sfconf->coeffs[n] = coeffslist[n]->coeff;
  }
  if (sfconf->n_coeffs > 0) {
    pinfo("finished.\n");
  }
    
  n_channels[IN] = n_channels[OUT] = 0;
  MEMSET(used_channels, 0, sizeof(used_channels));
  for (i = 0; i < sfconf->n_filters; i++) {
    filters.push_back(pfilters[i]->filter);
    sfconf->fproc.n_filters++;
    FOR_IN_AND_OUT {
      for (j = 0; j < pfilters[i]->filter.n_channels[IO]; j++) {
	if (!bit_isset(used_channels[IO], pfilters[i]->filter.channels[IO][j])) {
	  channels[IO][n_channels[IO]] = pfilters[i]->filter.channels[IO][j];
	  n_channels[IO]++;
	}
	bit_set(used_channels[IO], pfilters[i]->filter.channels[IO][j]);
      }
    }
  }
    
  FOR_IN_AND_OUT {
    sfconf->fproc.n_unique_channels[IO] = n_channels[IO];
    sfconf->fproc.unique_channels[IO].resize(n_channels[IO]);
    for(i = 0; i < n_channels[IO]; i++)
      sfconf->fproc.unique_channels[IO][i] = channels[IO][i];
  }
  for (i = 0; i < sfconf->fproc.n_filters; i++) {
    sfconf->fproc.filters.push_back(filters[i]);
  }
  
  /* load sflogic modules */
  if (sfconf->n_logicmods > 0) {
    sfconf->logicnames.resize(sfconf->n_logicmods);
  }
  for (n = i = 0; n < sfconf->n_logicmods; n++) {
    if (logic_names[n].compare("cli") == 0) {
      sflogic.push_back(new SFLOGIC_CLI(sfconf, icomm, sfRun));
    }
    else if (logic_names[n].compare("eq") == 0) {
      sflogic.push_back(new SFLOGIC_EQ(sfconf, icomm, sfRun));
    }
    else if (logic_names[n].compare("ladspa") == 0) {
      sflogic.push_back(new SFLOGIC_LADSPA(sfconf, icomm, sfRun));
    }
    else if (logic_names[n].compare("race") == 0) {
      sflogic.push_back(new SFLOGIC_RACE(sfconf, icomm, sfRun));
    }
    else if (logic_names[n].compare("py") == 0) {
      sflogic.push_back(new SFLOGIC_PY(sfconf, icomm, sfRun));
    }
    else if (logic_names[n].compare("recplay") == 0) {
      sflogic.push_back(new SFLOGIC_RECPLAY(sfconf, icomm, sfRun));
    }
    int l;
    for (l = n + 1; l < sfconf->n_logicmods; l++) {
      if (logic_names[l].compare(logic_names[n]) == 0 &&  sflogic[n]->unique) {
	fprintf(stderr, "Repeated logic module: %s.\n",logic_names[n].data());
	exit(SF_EXIT_OTHER);
      }
    }
    if ((rlogic = sflogic[n]->preinit(logic_data[n], (int)sfconf->debug)) == -1) {
      fprintf(stderr,"Error in logic module configuration: %s.\n",logic_names[n].data());
      exit(SF_EXIT_OTHER);
    }
    if (sflogic[n]->fork_mode != SF_FORK_DONT_FORK) {
      i++;
    }
    sfconf->logicnames[n] = logic_names[n];
  }
  /* load sfio modules */
  sfconf->io = new ioJack(sfconf, icomm, sfDai);

  /* derive dai_subdevices from inputs and outputs */
  
  FOR_IN_AND_OUT {
    iodevs[IO]->ch.sf.format = SF_SAMPLE_FORMAT_AUTO;
    //config_data = iodevs[IO]->device_params;
    sfconf->io->preinit(IO, iodevs[IO]);
    parse_sample_format(&iodevs[IO]->ch.sf, sf_strsampleformat(iodevs[IO]->ch.sf.format), false);
    sfconf->subdevs[IO].channels = iodevs[IO]->ch;
    sfconf->subdevs[IO].sched_policy = -1;
  }
  
  /* check which output channels that have dither applied */
  MEMSET(apply_dither, 0, sizeof(apply_dither));
  /* verify if it is feasible to apply dither */
  if (iodevs[OUT]->ch.sf.isfloat) {
    if (!iodevs[OUT]->auto_format) {
      pinfo("Warning: cannot dither floating point format (outputs %d - %d).\n", iodevs[OUT]->channel_intname[0],
	    iodevs[OUT]->channel_intname[iodevs[OUT]->virtual_channels - 1]);
    }
  }
  if (sfconf->realsize == sizeof(float) && iodevs[OUT]->ch.sf.sbytes > 2) {
    if (!iodevs[OUT]->auto_format) {
      pinfo("Warning: internal resolution not high enough to dither (outputs %d - %d).\n",iodevs[OUT]->channel_intname[0],
	    iodevs[OUT]->channel_intname[iodevs[OUT]->virtual_channels - 1]);
    }
  }
  if (iodevs[OUT]->ch.sf.sbytes >= 4) {
    if (!iodevs[OUT]->auto_format) {
      pinfo("Warning: cannot apply dither to 32 bit format (outputs %d - %d).\n",iodevs[OUT]->channel_intname[0],
	    iodevs[OUT]->channel_intname[iodevs[OUT]->virtual_channels - 1]);
    }
  }
  /* which physical channels to apply dither on */
  for (i = 0; i < iodevs[OUT]->ch.used_channels; i++) {
   if (iodevs[OUT]->apply_dither) { 
     bit_set(apply_dither, iodevs[OUT]->ch.channel_name[i]);
     j++;
   }
  }
  
  /* initialise dither array */
  sfconf->dither_state.resize(sfconf->n_physical_channels[OUT]);
  if (j > 0) {
    if (!dither_init(j, sfconf->sampling_rate, sfconf->realsize, sfconf->filter_length, dither_state)) {
      exit(SF_EXIT_OTHER);
    }
    for (n = j = 0; n < sfconf->n_physical_channels[OUT]; n++) {
      if (bit_isset(apply_dither, n)) {
	sfconf->dither_state[n] = dither_state[j];
	j++;
      } else {
	sfconf->dither_state[n] = NULL;
      }
    }
  }
  
  /* calculate which sched priorities to use */
  sfconf->realtime_maxprio = 4;
  sfconf->realtime_midprio = 3;
  sfconf->realtime_minprio = 2;
  sfconf->realtime_usermaxprio = 1;
  min_prio = -1;
  max_prio = 0;
  FOR_IN_AND_OUT {
    if (sfconf->subdevs[IO].sched_policy != -1) {
      if (sfconf->realtime_priority && sfconf->subdevs[IO].uses_clock && sfconf->subdevs[IO].sched_policy != SCHED_FIFO && 
	  sfconf->subdevs[IO].sched_policy != SCHED_RR) {
	exit(SF_EXIT_INVALID_CONFIG);
      }
      if (sfconf->realtime_priority) {
	if (max_prio < sfconf->subdevs[IO].sched_param.sched_priority) {
	  max_prio = sfconf->subdevs[IO].sched_param.sched_priority;
	}
	if (sfconf->subdevs[IO].sched_param.sched_priority < min_prio || min_prio == -1)  {
	  min_prio = sfconf->subdevs[IO].sched_param.sched_priority;
	}
	if (sfconf->realtime_midprio > sfconf->subdevs[IO].sched_param.sched_priority) {
	  exit(SF_EXIT_INVALID_CONFIG);
	}
      }
    }
  }

  if (min_prio != -1) {
    sfconf->realtime_midprio = max_prio;
    sfconf->realtime_minprio = min_prio - 1;
    sfconf->realtime_usermaxprio = min_prio - 2;
  }
  if (sfconf->realtime_midprio > sfconf->realtime_maxprio) {
    sfconf->realtime_maxprio = sfconf->realtime_midprio + 1;
  }
  if (sfconf->realtime_maxprio > sched_get_priority_max(SCHED_FIFO)) {
    fprintf(stderr, "Max priority exceeds maximum possible (%d > %d).\n", sfconf->realtime_maxprio, sched_get_priority_max(SCHED_FIFO));
    exit(SF_EXIT_INVALID_CONFIG);
  }
  if (sfconf->realtime_priority) {
    pinfo("Realtime priorities are min = %d, usermax = %d, mid = %d and max = %d.\n", sfconf->realtime_minprio, sfconf->realtime_usermaxprio,
	  sfconf->realtime_midprio, sfconf->realtime_maxprio);
  }
  
  /* calculate maximum possible flowthrough blocks */
  sfconf->flowthrough_blocks = sfconf->n_blocks + (maxdelay[IN] + sfconf->filter_length - 1) / sfconf->filter_length + (maxdelay[OUT] + sfconf->filter_length - 1) / sfconf->filter_length;
  
  /* estimate CPU clock rate */
  gettimeofday(&tv2, NULL);
  timestamp(&t2);
  timersub(&tv2, &tv1, &tv2);
  if (tv2.tv_sec == 0 && tv2.tv_usec < 100000) {
    usleep(100000 - tv2.tv_usec);
    gettimeofday(&tv2, NULL);
    timestamp(&t2);
    timersub(&tv2, &tv1, &tv2);
  }
  sfconf->cpu_mhz = (double)((long double)(t2 - t1) / (long double)(tv2.tv_sec * 1000000 + tv2.tv_usec));
#ifdef TIMESTAMP_NOT_CLOCK_CYCLE_COUNTER
  pinfo("Warning: no support for clock cycle counter on this platform.\n  Timers for benchmarking may be unreliable.\n");
#else    
  pinfo("Estimated CPU clock rate is %.3f MHz. CPU count is %d.\n", sfconf->cpu_mhz, sfconf->n_cpus);
#endif    
  
  if (load_balance && sfconf->n_cpus > 1) {
    for (n = 0; n <= largest_process; n++) {
      pinfo("Filters in process %d: ", n);
      for (i = 0; i < sfconf->n_filters; i++) {
	if (pfilters[i]->process == n) {
	  pinfo("%d ", i);
	}
      }
      pinfo("\n");
    }
  }
  
  /* clean config reader struct */
  xmlCleanupParser();
  xmlFreeDoc(xmlconf);
}

void SfConf::add_sflogic(SFLOGIC* _sflogic)
{
  int l, n;
  logic_names.push_back(_sflogic->name);
  sfconf->n_logicmods++;
  sfconf->logicnames.resize(sfconf->n_logicmods);
  sflogic.push_back(_sflogic);
  n = sfconf->n_logicmods - 1;
  for (l = 0; l < n; l++) {
    if (logic_names[l].compare(logic_names[n]) == 0 &&  sflogic[n]->unique) {
      fprintf(stderr, "Repeated logic module: %s.\n",logic_names[n].data());
      exit(SF_EXIT_OTHER);
    }
  }
  sfconf->logicnames[n] = logic_names[n];
}
