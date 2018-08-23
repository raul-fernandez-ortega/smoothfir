/*
 * (c) Copyright 2013 -- Raul Fernandez Ortega
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */

#include "sflogic_eq.hpp"

char * SFLOGIC_EQ::strtrim(char s[])
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

void SFLOGIC_EQ::coeff_final(int filter, int *coeff)
{
  int n, c[2], active;

  for (n = 0; n < n_equalisers; n++) {
    c[0] = equalisers[n].coeff[0];
    c[1] = equalisers[n].coeff[1];
    active = c[equalisers[n].active_coeff];
    if (*coeff == c[0] || *coeff == c[1]) {
      *coeff = active;
      equalisers[n].not_changed = false;
    }
  }
}

bool SFLOGIC_EQ::finalise_equaliser(struct realtime_eq *eq,
				    double mfreq[],
				    double mag[],
				    int n_mag,
				    double pfreq[],
				    double phase[],
				    int n_phase,
				    double bands[],
				    int n_bands)
{
  int n, i, band_count;
  
  band_count = n_bands + 2;
  eq->freq = new double [band_count];
  eq->mag = new double [band_count];
  eq->phase = new double [band_count];
  eq->freq[0] = 0.0;
  for (n = 0; n < n_bands; n++) {
    eq->freq[1+n] = bands[n];
  }
  eq->freq[1+n] = (double)sfconf->sampling_rate / 2.0;                
  
  MEMSET(eq->mag, 0, band_count * sizeof(double));
  for (n = 0, i = 0; n < n_mag; n++) {
    while (mfreq[n] > eq->freq[i]) {
      i++;
    }
    if (mfreq[n] != eq->freq[i]) {
      fprintf(stderr, "EQ: %.1f Hz is not a band frequency, use %.1f instead.\n", mfreq[n], eq->freq[i]);
      return false;
    }
    eq->mag[i++] = mag[n];
  }
  eq->mag[0] = eq->mag[1];
  eq->mag[band_count - 1] = eq->mag[band_count - 2];
  
  MEMSET(eq->phase, 0, band_count * sizeof(double));
  for (n = 0, i = 0; n < n_phase; n++) {
    while (pfreq[n] > eq->freq[i]) {
      i++;
    }
    if (pfreq[n] != eq->freq[i]) {
      fprintf(stderr, "EQ: %.1f Hz is not a band frequency, use %.1f instead.\n", pfreq[n], eq->freq[i]);
      return false;
    }
    eq->phase[i++] = phase[n];
  }
  for (n = 0; n < band_count; n++) {
    eq->freq[n] /= (double)sfconf->sampling_rate;
    eq->mag[n] = pow(10, eq->mag[n] / 20);
    eq->phase[n] = eq->phase[n] / (180 * M_PI);           
  }
  eq->band_count = band_count;
  for (n = i = 0; n < 2; n++) {
    if ((i = log2_get(sfconf->filter_length * sfconf->coeffs[eq->coeff[n]].n_blocks)) == -1)
      {
	fprintf(stderr, "EQ: Coefficient %d length is not a power of two.\n", eq->coeff[n]);
	return false;
      }
  }
  eq->taps = 1 << i;
  if (sfconf->coeffs[eq->coeff[0]].n_blocks != sfconf->coeffs[eq->coeff[1]].n_blocks) {
    fprintf(stderr, "EQ: Coefficient %d and %d must be the same length.\n", eq->coeff[0], eq->coeff[1]);
    return false;
  }
  return true;
}

SFLOGIC_EQ::SFLOGIC_EQ(struct sfconf *_sfconf,
		       struct intercomm_area *_icomm,
		       SfAccess *_sfaccess)  : SFLOGIC(_sfconf, _icomm, _sfaccess)
{
  int n;

  unique = false;
  n_equalisers = 0;
  debug_dump_filter_path = NULL;
  debug = false;
  render_postponed_index = -1;
  name = "eq";

  fork_mode = SF_FORK_DONT_FORK;  
  MEMSET(msg, 0, MAX_MSG_LEN);

  equalisers = new struct realtime_eq [MAX_EQUALISERS];
  for(n = 0; n < MAX_EQUALISERS; n++) {
    MEMSET(&equalisers[n], 0, sizeof(struct realtime_eq));
    equalisers[n].coeff[0] = -1;
    equalisers[n].coeff[1] = -1;
  }
}

int SFLOGIC_EQ::preinit(xmlNodePtr sfparams, int _debug)
{
  double mag[2][MAX_BANDS];
  double phase[2][MAX_BANDS];
  double bands[MAX_BANDS];
  double freqval;
  int n_mag, n_phase, n_bands;
  int n, i, bandindex;
  xmlNodePtr children;

  n_mag = 0;
  n_phase = 0;
  n_bands = 0;
  bandindex = 0;

  debug = !!_debug;
  
  while (sfparams != NULL) {
    if (!xmlStrcmp(sfparams->name, (const xmlChar *)"debug_dump")) {
      debug_dump_filter_path = estrdup((char*)xmlNodeGetContent(sfparams));
    } else if (!xmlStrcmp(sfparams->name, (const xmlChar *)"coeff")) {
      for (n = 0; n < sfconf->n_coeffs; n++) {
	if (sfconf->coeffs[n].name.compare((char*)xmlNodeGetContent(sfparams)) == 0)  {
	  equalisers[n_equalisers].coeff[0] = sfconf->coeffs[n].intname;
	  equalisers[n_equalisers].coeff[1] = equalisers[n_equalisers].coeff[0];
	  break;
	}
      }
      if (n == sfconf->n_coeffs) {
	fprintf(stderr, "EQ: Unknown coefficient name.\n");
	return -1;
      }
    } else if (!xmlStrcmp(sfparams->name, (const xmlChar *)"phase")) {
      equalisers[n_equalisers].minphase = (!xmlStrcmp(xmlNodeGetContent(sfparams), (const xmlChar *)"minphase") ? true : false);
    } else if (!xmlStrcmp(sfparams->name, (const xmlChar *)"std")) {
      if (n_bands > 0) {
	fprintf(stderr, "Cannot set std equalizer. Bands were configure previously.\n");
      } else if (strcmp("ISO octave",(char*)xmlNodeGetContent(sfparams)) == 0) {
	std_bands = true;
	n_bands = 10;
	bands[0] = 31.5;
	bands[1] = 63;
	bands[2] = 125;
	bands[3] = 250;
	bands[4] = 500;
	bands[5] = 1000;
	bands[6] = 2000;
	bands[7] = 4000;
	bands[8] = 8000;
	bands[9] = 16000;
      } else if (strcmp("ISO 1/3 octave", (char*)xmlNodeGetContent(sfparams)) == 0) {
	std_bands = true;
	n_bands = 31;
	bands[0] = 20;
	bands[1] = 25;
	bands[2] = 31;
	bands[3] = 40;
	bands[4] = 50;
	bands[5] = 63;
	bands[6] = 80;
	bands[7] = 100;
	bands[8] = 125;
	bands[9] = 160;
	bands[10] = 200;
	bands[11] = 250;
	bands[12] = 315;
	bands[13] = 400;
	bands[14] = 500;
	bands[15] = 630;
	bands[16] = 800;
	bands[17] = 1000;
	bands[18] = 1250;
	bands[19] = 1600;
	bands[20] = 2000;
	bands[21] = 2500;
	bands[22] = 3150;
	bands[23] = 4000;
	bands[24] = 5000;
	bands[25] = 6300;
	bands[26] = 8000;
	bands[27] = 10000;
	bands[28] = 12500;
	bands[29] = 16000;
	bands[30] = 20000;
      } else {
	fprintf(stderr, "EQ: Parse error: expected \"ISO octave\" or \"ISO 1/3 octave\".\n");
	return -1;
      }
    }  else if (!xmlStrcmp(sfparams->name, (const xmlChar *)"band")) {
      if (bandindex == MAX_BANDS) {
	fprintf(stderr, "EQ: Parse error: band index out of brutefir limits.\n");
	return -1;
      }
      children = sfparams->children;
      while(children != NULL) {
	if (!xmlStrcmp(children->name, (const xmlChar *)"freq")) {
	  freqval = abs(strtof((char*)xmlNodeGetContent(children), NULL));
	  if(!std_bands)
	    bands[n_bands] = freqval;
	  else if(abs(freqval - bands[bandindex]) > 0.1) 
	    fprintf(stderr, "Usinf ISO band %f Hz instead of %f Hz.\n", bands[bandindex], freqval);
	} else if (!xmlStrcmp(children->name, (const xmlChar *)"mag")) {
	  mag[1][n_mag] = strtof((char*)xmlNodeGetContent(children), NULL);
	  mag[0][n_mag] = freqval;
	  if (mag[0][n_mag] > (double)sfconf->sampling_rate / 2.0) {
	    fprintf(stderr, "EQ: Parse error: frequency larger than nykvist.\n");
	    return -1;
	  }
	  if (n_mag > 0 && mag[n_mag] <= mag[n_mag-1]) {
	    fprintf(stderr, "EQ: Parse error: frequencies not sorted.\n");
	    return -1;
	  }
	  n_mag++;
	} else if (!xmlStrcmp(children->name, (const xmlChar *)"phase")) {
	  phase[1][n_phase] = strtof((char*)xmlNodeGetContent(children), NULL);
	  phase[0][n_phase] = freqval;
	  if (phase[0][n_phase] > (double)sfconf->sampling_rate / 2.0) {
	    fprintf(stderr, "EQ: Parse error: frequency larger than nykvist.\n");
	    return -1;
	  }
	  if (n_phase > 0 && phase[n_phase] <= phase[n_phase-1]) {
	    fprintf(stderr, "EQ: Parse error: frequencies not sorted.\n");
	    return -1;
	  }
	  n_phase++;
	}
	children = children->next;
      }
      n_bands++;
      bandindex++;
    }
    sfparams = sfparams->next;
  }
  if (bands[n_bands-1] >= (double)sfconf->sampling_rate / 2.0) {
    fprintf(stderr, "EQ: Parse error: band frequencies must be less than sample rate / 2.\n");
    return -1;
  }
    if (n_equalisers > 0) {
      if (n_equalisers == MAX_EQUALISERS) {
	fprintf(stderr, "EQ: Too many equalisers.\n");
	return -1;
      }
    }
    if (!finalise_equaliser(&equalisers[n_equalisers], mag[0], mag[1], n_mag, phase[0], phase[1], n_phase, bands, n_bands)) {
      return -1;
    }
    n_equalisers++;
    for (n = 0; n < n_equalisers; n++) {
      for (i = 0; i < n_equalisers; i++) {
	if (i != n &&
	    (equalisers[n].coeff[0] == equalisers[i].coeff[0] ||
	     equalisers[n].coeff[0] == equalisers[i].coeff[1] ||
	     equalisers[n].coeff[1] == equalisers[i].coeff[0] ||
	     equalisers[n].coeff[1] == equalisers[i].coeff[1]))
	  {
	    fprintf(stderr, "EQ: At least two equalisers has at least one coefficent set in common.\n");
	    return -1;
	  }
      }
    }
    return 0;
}

int SFLOGIC_EQ::config(int n_eq, string _coeff_name, bool _minphase, string _std)
{
  int n;
  double mag[2][MAX_BANDS];
  double phase[2][MAX_BANDS];
  double bands[MAX_BANDS];
  double freqval;
  int n_mag, n_phase, n_bands;

  if (n_eq == MAX_EQUALISERS) {
    fprintf(stderr, "EQ: Too many equalisers.\n");
    return -1;
  }
  for (n = 0; n < sfconf->n_coeffs; n++) {
    if (sfconf->coeffs[n].name.compare(_coeff_name) == 0)  {
      equalisers[n_eq].coeff[0] = sfconf->coeffs[n].intname;
      equalisers[n_eq].coeff[1] = equalisers[n_equalisers].coeff[0];
      break;
    }
  }
  if (n == sfconf->n_coeffs) {
    fprintf(stderr, "EQ: Unknown coefficient name.\n");
    return -1;
  }

  equalisers[n_eq].minphase = _minphase;

  if (strcmp("ISO octave", _std.data()) == 0) {
    std_bands = true;
    n_bands = 10;
    bands[0] = 31.5;
    bands[1] = 63;
    bands[2] = 125;
    bands[3] = 250;
    bands[4] = 500;
    bands[5] = 1000;
    bands[6] = 2000;
    bands[7] = 4000;
    bands[8] = 8000;
    bands[9] = 16000;
  } else if (strcmp("ISO 1/3 octave", _std.data()) == 0) {
    std_bands = true;
    n_bands = 31;
    bands[0] = 20;
    bands[1] = 25;
    bands[2] = 31;
    bands[3] = 40;
    bands[4] = 50;
    bands[5] = 63;
    bands[6] = 80;
    bands[7] = 100;
    bands[8] = 125;
    bands[9] = 160;
    bands[10] = 200;
    bands[11] = 250;
    bands[12] = 315;
    bands[13] = 400;
    bands[14] = 500;
    bands[15] = 630;
    bands[16] = 800;
    bands[17] = 1000;
    bands[18] = 1250;
    bands[19] = 1600;
    bands[20] = 2000;
    bands[21] = 2500;
    bands[22] = 3150;
    bands[23] = 4000;
    bands[24] = 5000;
    bands[25] = 6300;
    bands[26] = 8000;
    bands[27] = 10000;
    bands[28] = 12500;
    bands[29] = 16000;
    bands[30] = 20000;
  } else {
    fprintf(stderr, "EQ: Parse error: expected \"ISO octave\" or \"ISO 1/3 octave\".\n");
    return -1;
  }
  if (bands[n_bands-1] >= (double)sfconf->sampling_rate / 2.0) {
    fprintf(stderr, "EQ: Parse error: band frequencies must be less than sample rate / 2.\n");
    return -1;
  }
  for(n = 0; n < n_bands; n++) {
    phase[0][n] = bands[n];
    phase[1][n] = 0.0;
    mag[0][n] = bands[n];
    mag[1][n] = 0.0;
  }
  n_mag = n_bands;
  n_phase = n_bands;
  if (!finalise_equaliser(&equalisers[n_eq], mag[0], mag[1], n_mag, phase[0], phase[1], n_phase, bands, n_bands)) {
    return -1;
  }
  if(n_equalisers < n_eq + 1) 
    n_equalisers = n_eq + 1;
  return 0;
}

int SFLOGIC_EQ::init(int event_fd, sem_t *synch_sem)
{
  int n, maxblocks;

   
  maxblocks = 0;
  for (n = 0; n < n_equalisers; n++) {
    if (maxblocks < sfconf->coeffs[equalisers[n].coeff[0]].n_blocks) {
      maxblocks = sfconf->coeffs[equalisers[n].coeff[0]].n_blocks;
    }
  }
  rbuf = emallocaligned(maxblocks * sfconf->filter_length * sfconf->realsize);
  
  for (n = 0; n < n_equalisers; n++) {
    equalisers[n].ifftplan = sfaccess->convolver_fftplan(log2_get(equalisers[n].taps), true, true);
    if (sfconf->realsize == 4) {
      render_equaliser_f(&equalisers[n]);
    } else {
      render_equaliser_d(&equalisers[n]);
    }
  }
  if (sem_post(synch_sem)== -1) {
    fprintf(stderr, "EQ: sem failed.\n");
    return -1;
  }
  return 0;
}

bool SFLOGIC_EQ::change_magnitude(int n_eq, int nfreq, double mag)
{
  struct realtime_eq *eq;

  eq = &equalisers[n_eq];
  if (nfreq > eq->band_count) {
    fprintf(stderr,"The given band does not exist. Limit is %i.\n",eq->band_count);
    return false;
  }
  eq->mag[nfreq] = pow(10, mag / 20);
  return true;
}

bool SFLOGIC_EQ::change_rendering(int n_eq) 
{
  struct realtime_eq *eq;

  eq = &equalisers[n_eq];
  if (sfconf->realsize == 4) {
    render_equaliser_f(eq);
  } else {
    render_equaliser_d(eq);
  }
  eq->not_changed = true;

  return true;
}

/*int SFLOGIC_EQ::command(const char params[])
{
  int command, coeff, n, i, n_bands, eq_index;
  char *p, *params_copy, *cmd;
  char rmsg[MSGSIZE];
  double bands[MAX_BANDS], values[MAX_BANDS], freq;
  struct realtime_eq *eq;
  
  string params_copy = params;
  string coeff, cmd;
  size_t begin = 0;
  size_t end;

  trim(params_copy);
  end = params_copy.substr(begin, params_copy.size()-begin).find(" ");
  if(end == string::npos) {
    msg = "Invalid coefficient.\n";
    return -1;
  }
  coeff = params_copy.substr(begin, end - begin);
  trim(coeff);
  if (coeff.find('\"') == 0) {
    coeff = coeff.substr(1, coeff.rfind('\'') - 1);
    for (n = 0; n < sfconf->n_coeffs; n++) {
      if ( sfconf->coeffs[n].name.compare(coeff) == 0) {
	ncoeff = sfconf->coeffs[n].intname;
	break;
      }
    }
    if (n == sfconf->n_coeffs) {
      msg = "Coefficient with name ";
      msg += coeff;
      msg += " does not exist.\n";
      return -1;
    }
  } else {
    ncoeff = strtol(coeff.c_str(), &p, 10);
    if (p == coeff.c_str()) {
      msg = "Invalid number.\n";
      return -1;
    }
  }
  for (n = 0; n < n_equalisers; n++) {
    if (equalisers[n].coeff[0] == ncoeff || equalisers[n].coeff[1] == ncoeff) {
      eq_index = n;
      break;
    }
  }
  if (n == n_equalisers) {
    msg = "The given coefficient is not controlled.\n";
    return -1;
  }

  trim(params_copy);
  begin = end;
  end = params_copy.substr(begin, params_copy.size()-begin).find(" ");
  
  cmd = params_copy.substr(begin, end - begin);
  trim(cmd);
  if (cmd.compare("mag") == 0) {
    command = CMD_CHANGE_MAGNITUDE;
  } else if (cmd.compare("phase") == 0) {
    command = CMD_CHANGE_PHASE;
  } else if (cmd.compare( "info") == 0) {
    command = CMD_GET_INFO;
  } else {
    msg = "Unknown command.\n";
    return -1;
  }

  switch (command) {
  case CMD_CHANGE_MAGNITUDE:
  case CMD_CHANGE_PHASE:
    for (n = 0; n < MAX_BANDS && cmd[0] != '\0'; n++) {
      bands[n] = strtod(cmd, &p);
      if (p == cmd || *p != '/') {
	sprintf(msg, "Invalid frequency/value list.\n");
	free(params_copy);
	return -1;
      }
      if (n > 1 && bands[n] <= bands[n-1]) {
	sprintf(msg, "Frequency bands not sorted.\n");
	free(params_copy);
	return -1;
      }
      cmd = p + 1;
      values[n] = strtod(cmd, &p);
      if (p == cmd) {
	sprintf(msg, "Invalid frequency/value list.\n");
	free(params_copy);
	return -1;
      }
      cmd = strtrim(p);
      if (cmd[0] != ',' && cmd[0] != '\0') {
	sprintf(msg, "Invalid frequency/value list.\n");
	free(params_copy);
	return -1;
      }
      if (cmd[0] == ',') {
	cmd++;
      }
    }
    free(params_copy);
    n_bands = n;
    eq = &equalisers[eq_index];
    for (n = 0, i = 0; i < n_bands && n < eq->band_count; n++) {
      if (bands[i] / (double)sfconf->sampling_rate > 0.99 * eq->freq[n] && bands[i] / (double)sfconf->sampling_rate < 1.01 * eq->freq[n]) {
	bands[i] /= (double)sfconf->sampling_rate;
	if (command == CMD_CHANGE_MAGNITUDE) {
	  eq->mag[n] = pow(10, values[i] / 20);
	} else {
	  eq->phase[n] = values[i] / (180 * M_PI);
	}
	i++;
      }
    }
    if (i != n_bands) {
      sprintf(msg, "At least one invalid frequency band.\n");
      return -1;
    }
    if (sfconf->realsize == 4) {
      render_equaliser_f(eq);
    } else {
      render_equaliser_d(eq);
    }
    eq->not_changed = true;
    sprintf(msg, "ok\n");
    break;
  case CMD_GET_INFO:
    eq = &equalisers[eq_index];
    p = msg;
    MEMSET(rmsg, 0, sizeof(rmsg));
    if (eq->coeff[0] == eq->coeff[1]) {
      sprintf(p, "coefficient %d:\n band: ", eq->coeff[0]);
    } else {
      sprintf(p, "coefficient %d,%d:\n band: ",eq->coeff[0], eq->coeff[1]);
    }
    p += strlen(p);
    for (n = 1; n < eq->band_count - 1; n++) {
      freq = eq->freq[n] * (double)sfconf->sampling_rate;
      if (freq < 100) {
	sprintf(p, "%6.1f", freq);
      } else {
	sprintf(p, "%6.0f", freq);
      }
      p += strlen(p);
    }
    sprintf(p, "\n  mag: ");
    p += strlen(p);
    for (n = 1; n < eq->band_count - 1; n++) {
      sprintf(p, "%6.1f", 20 * log10(eq->mag[n]));
      p += strlen(p);
    }
    sprintf(p, "\nphase: ");
    p += strlen(p);
    for (n = 1; n < eq->band_count - 1; n++) {
      sprintf(p, "%6.1f", M_PI * 180 * eq->phase[n]);
      p += strlen(p);
    }
    sprintf(p, "\n");
    break;
    if (render_postponed_index != -1) {
      if (sfconf->realsize == 4) {
	render_equaliser_f(&equalisers[render_postponed_index]);
      } else {
	render_equaliser_d(&equalisers[render_postponed_index]);
      }
      render_postponed_index = -1;
    }
  }
  return 0;

  /////////////////////////////////////////////////////////////////////
  params_copy = estrdup(params);
  cmd = strtrim(params_copy);
  coeff = -1;
  
  // <coeff> <mag | phase | info> <band>/<value>[,<band/value>, ...] 
  if (cmd[0] == '\"') {
    p = strchr(cmd + 1, '\"');
    if (p == NULL) {
      sprintf(msg, "Invalid coefficient.\n");
      free(params_copy);
      return -1;
    }
    *p = '\0';
    p++;
    for (n = 0; n < sfconf->n_coeffs; n++) {
      //if (strcmp(cmd + 1, sfconf->coeffs[n].name) == 0) {
      if ( sfconf->coeffs[n].name.compare(cmd + 1) == 0) {
	coeff = sfconf->coeffs[n].intname;
	break;
      }
    }
    if (n == sfconf->n_coeffs) {
      sprintf(msg, "Coefficient with name \"%s\" does not exist.\n", cmd + 1);
      free(params_copy);
      return -1;
    }
  } else {
    coeff = strtol(cmd, &p, 10);
    if (p == cmd) {
      sprintf(msg, "Invalid number.\n");
      free(params_copy);
      return -1;
    }
  }
  for (n = 0; n < n_equalisers; n++) {
    if (equalisers[n].coeff[0] == coeff || equalisers[n].coeff[1] == coeff) {
      eq_index = n;
      break;
    }
  }
  if (n == n_equalisers) {
    sprintf(msg, "The given coefficient is not controlled.\n");
    free(params_copy);
    return -1;
  }
  cmd = strtrim(p);
  if (strstr(cmd, "mag") == cmd) {
    command = CMD_CHANGE_MAGNITUDE;
    cmd = strtrim(cmd + 3);
  } else if (strstr(cmd, "phase") == cmd) {
    command = CMD_CHANGE_PHASE;
    cmd = strtrim(cmd + 5);
  } else if (strstr(cmd, "info") == cmd) {
    command = CMD_GET_INFO;
  } else {
    sprintf(msg, "Unknown command.\n");
    free(params_copy);
    return -1;
  }
  
  switch (command) {
  case CMD_CHANGE_MAGNITUDE:
  case CMD_CHANGE_PHASE:
    for (n = 0; n < MAX_BANDS && cmd[0] != '\0'; n++) {
      bands[n] = strtod(cmd, &p);
      if (p == cmd || *p != '/') {
	sprintf(msg, "Invalid frequency/value list.\n");
	free(params_copy);
	return -1;
      }
      if (n > 1 && bands[n] <= bands[n-1]) {
	sprintf(msg, "Frequency bands not sorted.\n");
	free(params_copy);
	return -1;
      }
      cmd = p + 1;
      values[n] = strtod(cmd, &p);
      if (p == cmd) {
	sprintf(msg, "Invalid frequency/value list.\n");
	free(params_copy);
	return -1;
      }
      cmd = strtrim(p);
      if (cmd[0] != ',' && cmd[0] != '\0') {
	sprintf(msg, "Invalid frequency/value list.\n");
	free(params_copy);
	return -1;
      }
      if (cmd[0] == ',') {
	cmd++;
      }
    }
    free(params_copy);
    n_bands = n;
    eq = &equalisers[eq_index];
    for (n = 0, i = 0; i < n_bands && n < eq->band_count; n++) {
      if (bands[i] / (double)sfconf->sampling_rate > 0.99 * eq->freq[n] && bands[i] / (double)sfconf->sampling_rate < 1.01 * eq->freq[n]) {
	bands[i] /= (double)sfconf->sampling_rate;
	if (command == CMD_CHANGE_MAGNITUDE) {
	  eq->mag[n] = pow(10, values[i] / 20);
	} else {
	  eq->phase[n] = values[i] / (180 * M_PI);
	}
	i++;
      }
    }
    if (i != n_bands) {
      sprintf(msg, "At least one invalid frequency band.\n");
      return -1;
    }
    if (sfconf->realsize == 4) {
      render_equaliser_f(eq);
    } else {
      render_equaliser_d(eq);
    }
    eq->not_changed = true;
    sprintf(msg, "ok\n");
    break;
  case CMD_GET_INFO:
    eq = &equalisers[eq_index];
    p = msg;
    MEMSET(rmsg, 0, sizeof(rmsg));
    if (eq->coeff[0] == eq->coeff[1]) {
      sprintf(p, "coefficient %d:\n band: ", eq->coeff[0]);
    } else {
      sprintf(p, "coefficient %d,%d:\n band: ",eq->coeff[0], eq->coeff[1]);
    }
    p += strlen(p);
    for (n = 1; n < eq->band_count - 1; n++) {
      freq = eq->freq[n] * (double)sfconf->sampling_rate;
      if (freq < 100) {
	sprintf(p, "%6.1f", freq);
      } else {
	sprintf(p, "%6.0f", freq);
      }
      p += strlen(p);
    }
    sprintf(p, "\n  mag: ");
    p += strlen(p);
    for (n = 1; n < eq->band_count - 1; n++) {
      sprintf(p, "%6.1f", 20 * log10(eq->mag[n]));
      p += strlen(p);
    }
    sprintf(p, "\nphase: ");
    p += strlen(p);
    for (n = 1; n < eq->band_count - 1; n++) {
      sprintf(p, "%6.1f", M_PI * 180 * eq->phase[n]);
      p += strlen(p);
    }
    sprintf(p, "\n");
    break;
    if (render_postponed_index != -1) {
      if (sfconf->realsize == 4) {
	render_equaliser_f(&equalisers[render_postponed_index]);
      } else {
	render_equaliser_d(&equalisers[render_postponed_index]);
      }
      render_postponed_index = -1;
    }
  }
  return 0;
}*/

 /*const char *SFLOGIC_EQ::message(void)
{
  return msg;
}*/
