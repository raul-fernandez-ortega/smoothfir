/*
 * (c) Copyright 2013/2022 -- Raul Fernandez Ortega
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */

#include "sflogic_race.hpp"

char * SFLOGIC_RACE::strtrim(char s[])
{
  char *p;
  
  while (*s == ' ' || *s == '\t') {
    s++;
  }
  if (*s == '\0') {
    return s;
  }
  p = s + strlen(s) - 1;
  while ((*p == ' ' || *p == '\t') && p != s) {
    p--;
  }
  *(p + 1) = '\0';
  return s;
}

void trim(string& str)
{
  size_t pos = str.find_last_not_of(' ');
  if(pos != string::npos) {
    str.erase(pos + 1);
    pos = str.find_first_not_of(' ');
    if(pos != string::npos) str.erase(0, pos);
  }
  else str.erase(str.begin(), str.end());
}

void SFLOGIC_RACE::coeff_final(int filter, int *coeff)
{
}

bool SFLOGIC_RACE::finalise_RACE(void)
{
  int n, i;
  RACEFilter.band_count = 8;
  RACEFilter.freq = new double [RACEFilter.band_count];
  RACEFilter.mag = new double [RACEFilter.band_count];
  RACEFilter.phase = new double [RACEFilter.band_count];

  memset(RACEFilter.phase, 0, RACEFilter.band_count * sizeof(double));
  RACEFilter.freq[0] = 0;
  RACEFilter.mag[0] = 0;
  
  RACEFilter.freq[1] = lowFreq/2/sfconf->sampling_rate;
  RACEFilter.mag[1] = pow(10, (double)-lowSlope/20);

  RACEFilter.freq[2] = lowFreq/sfconf->sampling_rate;
  RACEFilter.mag[2] = 1;

  RACEFilter.freq[3] = 2 * lowFreq/sfconf->sampling_rate;
  RACEFilter.mag[3] = 1;

  RACEFilter.freq[4] = highFreq/2/sfconf->sampling_rate;
  RACEFilter.mag[4] = 1;
  
  RACEFilter.freq[5] = highFreq/sfconf->sampling_rate;
  RACEFilter.mag[5] = 1;

  RACEFilter.freq[6] = (0.5 + highFreq/sfconf->sampling_rate)/2;
  RACEFilter.mag[6] = pow(10, (double)-highSlope/10*(((0.5 + highFreq/sfconf->sampling_rate)/2/(highFreq/sfconf->sampling_rate))-1));
  
  RACEFilter.freq[7] = 0.5;
  RACEFilter.mag[7] = pow(10, (double)-highSlope/10*(0.50*sfconf->sampling_rate/highFreq-1));;

  if (debug) {
    fprintf(stderr, "RACE Filter frequency coeffs:\n");
    fprintf(stderr, "Frequency (Hz)       Coeffs\n");
    for(n = 0; n < 8; n++) {
      fprintf(stderr, "%f %f\n",RACEFilter.freq[n]*sfconf->sampling_rate, (double)10*log10(RACEFilter.mag[n]));
    }
  }
  for (n = i = 0; n < 2; n++) {
    if ((i = log2_get(sfconf->filter_length * sfconf->coeffs[RACEFilter.direct_coeff].n_blocks)) == -1) {
      fprintf(stderr, "RACE: Coefficient %d length is not a power of two.\n", RACEFilter.direct_coeff);
      return false;
    }
  }
  RACEFilter.taps = 1 << i;
  return true;
}

SFLOGIC_RACE::SFLOGIC_RACE(struct sfconf *_sfconf,
			   struct intercomm_area *_icomm,
			   SfAccess *_sfaccess)  : SFLOGIC(_sfconf, _icomm, _sfaccess)
{
  unique = false;
  dump_direct_file = NULL;
  dump_cross_file = NULL;
  //debug_dump_filter_path = new char [12];
  //strcpy(debug_dump_filter_path, "./direct.raw");
  debug = false;
  name = "race";
  memset(msg, 0, MAX_MSG_LEN);
}

int SFLOGIC_RACE::preinit(xmlNodePtr sfparams, int _debug)
{
  int n;
  
  debug = !!_debug;
  fork_mode = SF_FORK_DONT_FORK;
  
  while (sfparams != NULL) {
    if (!xmlStrcmp(sfparams->name, (const xmlChar *)"direct_coeff")) {
      for (n = 0; n < sfconf->n_coeffs; n++) {
	if (sfconf->coeffs[n].name.compare((char*)xmlNodeGetContent(sfparams)) == 0)  {
	  RACEFilter.direct_coeff = sfconf->coeffs[n].intname;
	  break;
	}
      }
      if (n == sfconf->n_coeffs) {
	fprintf(stderr, "RACE: Unknown coefficient name for direct RACE filter.\n");
	return -1;
      }
    } else if (!xmlStrcmp(sfparams->name, (const xmlChar *)"cross_coeff")) {
      for (n = 0; n < sfconf->n_coeffs; n++) {
	if (sfconf->coeffs[n].name.compare((char*)xmlNodeGetContent(sfparams)) == 0)  {
	  RACEFilter.cross_coeff = sfconf->coeffs[n].intname;
	  break;
	}
      }
      if (n == sfconf->n_coeffs) {
	fprintf(stderr, "RACE: Unknown coefficient name for direct RACE filter.\n");
	return -1;
      }
    } else if (!xmlStrcmp(sfparams->name, (const xmlChar *)"dump_direct_file")) {
      dump_direct_file = estrdup((char*)xmlNodeGetContent(sfparams));
    } else if (!xmlStrcmp(sfparams->name, (const xmlChar *)"dump_cross_file")) {
      dump_cross_file = estrdup((char*)xmlNodeGetContent(sfparams));   
    } else if (!xmlStrcmp(sfparams->name, (const xmlChar *)"attenuation")) {
      scale = strtof((char*)xmlNodeGetContent(sfparams), NULL);
    } else if (!xmlStrcmp(sfparams->name, (const xmlChar *)"delay")) {
      delay = strtol((char*)xmlNodeGetContent(sfparams), NULL, 10);
    } else if (!xmlStrcmp(sfparams->name, (const xmlChar *)"low_cut_freq")) {
      lowFreq = strtof((char*)xmlNodeGetContent(sfparams), NULL);
    } else if (!xmlStrcmp(sfparams->name, (const xmlChar *)"low_cut_slope")) {
      lowSlope = strtof((char*)xmlNodeGetContent(sfparams), NULL);
    } else if (!xmlStrcmp(sfparams->name, (const xmlChar *)"high_cut_freq")) {
      highFreq = strtof((char*)xmlNodeGetContent(sfparams), NULL);
    } else if (!xmlStrcmp(sfparams->name, (const xmlChar *)"high_cut_slope")) {
      highSlope = strtof((char*)xmlNodeGetContent(sfparams), NULL);
    } 
    sfparams = sfparams->next;
  }
  if (!finalise_RACE()) {
    return -1;
  }
  return 0;
}

int SFLOGIC_RACE::init(int event_fd, sem_t *synch_sem)
{ 
  maxblocks = sfconf->coeffs[RACEFilter.direct_coeff].n_blocks;
  if (maxblocks < sfconf->coeffs[RACEFilter.cross_coeff].n_blocks) {
    maxblocks = sfconf->coeffs[RACEFilter.cross_coeff].n_blocks;
  }
  d_rbuf = emallocaligned(maxblocks * sfconf->filter_length * sfconf->realsize);
  c_rbuf = emallocaligned(maxblocks * sfconf->filter_length * sfconf->realsize);

  if (sfconf->realsize == 4) {
    render_race_f();
  } else {
    render_race_d();
  }
  if (sem_post(synch_sem)== -1) {
    fprintf(stderr, "RACE: sem failed.\n");
    return -1;
  }
  return 0;
}

bool SFLOGIC_RACE::set_config(string direct_coeff, 
			      string cross_coeff,
			      float nscale, 
			      float ndelay, 
			      float nlowFreq, 
			      float nlowSlope, 
			      float nhighFreq, 
			      float nhighSlope)
{
  int n;

  for (n = 0; n < sfconf->n_coeffs; n++) {
    if (sfconf->coeffs[n].name.compare(direct_coeff) == 0)  {
      RACEFilter.direct_coeff = sfconf->coeffs[n].intname;
      break;
    }
  }
  if (n == sfconf->n_coeffs) {
    fprintf(stderr, "RACE: Unknown coefficient name for direct RACE filter.\n");
    return false;
  }
  for (n = 0; n < sfconf->n_coeffs; n++) {
    if (sfconf->coeffs[n].name.compare(cross_coeff) == 0)  {
      RACEFilter.cross_coeff = sfconf->coeffs[n].intname;
      break;
    }
  }
  if (n == sfconf->n_coeffs) {
    fprintf(stderr, "RACE: Unknown coefficient name for cross RACE filter.\n");
    return false;
  }

  scale = nscale;
  delay = ndelay;
  lowFreq = nlowFreq;
  lowSlope = nlowSlope;
  highFreq = nhighFreq;
  highSlope = nhighSlope;

  if (!finalise_RACE()) {
    return false;
  }

  return true;
}
  
bool SFLOGIC_RACE::change_config(float nscale, float ndelay, float nlowFreq, float nlowSlope, float nhighFreq, float nhighSlope)
{
  scale = nscale;
  delay = ndelay;
  lowFreq = nlowFreq;
  lowSlope = nlowSlope;
  highFreq = nhighFreq;
  highSlope = nhighSlope;

  if (!finalise_RACE()) {
    return false;
  }
  
  if (sfconf->realsize == 4) {
    render_race_f();
  } else {
    render_race_d();
  }
  return true;
  }


