/*
 * (c) Copyright 2013 -- Raul Fernandez Ortega
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */

#include "sflogic_py.hpp"

SFLOGIC_PY::SFLOGIC_PY(struct sfconf *_sfconf,
		       struct intercomm_area *_icomm,
		       SfAccess *_sfaccess)  : SFLOGIC(_sfconf, _icomm, _sfaccess)
{
  fork_mode = SF_FORK_DONT_FORK;
  name = "py";
  inPeak.resize(sfconf->n_channels[IN], 0);
  outPeak.resize(sfconf->n_channels[OUT], 0);
}

int SFLOGIC_PY::preinit(xmlNodePtr sfparams, int _debug)
{ 
  debug = !!_debug;
  return 0;
}

int SFLOGIC_PY::init(int event_fd, sem_t *synch_sem)
{
  return 0;
}

vector<struct sfcoeff> SFLOGIC_PY::getCoeffData(void)
{
  return sfconf->coeffs;
}

vector<struct sffilter> SFLOGIC_PY::getFilterData(void)
{
  return sfconf->filters;
}

struct sffilter_control* SFLOGIC_PY::getFilterControl(int n)
{
  return &icomm->fctrl[n];
}

struct vector<sfchannel> SFLOGIC_PY::getChannels(int io)
{
  return sfconf->channels[io];
}

bool SFLOGIC_PY::is_running(void) 
{
  if (sfconf->io != NULL)
    return sfconf->io->is_running();
  else
    return false;
}

int SFLOGIC_PY::getFilternChannels(struct sffilter* filter, int io)
{
  return filter->n_channels[io];
}

int SFLOGIC_PY::getFilternFilters(struct sffilter* filter, int io)
{
  return filter->n_filters[io];
}

struct sfcoeff* SFLOGIC_PY::getFilterCoeff(string filtername)
{
  unsigned int i;

  for(i = 0; i < sfconf->filters.size(); i++) {
    if(sfconf->filters[i].name.compare(filtername) == 0) {
      if(icomm->fctrl[i].coeff >= 0)
	return &sfconf->coeffs[icomm->fctrl[i].coeff];
    }
  } 
  return NULL;
}

vector<int> SFLOGIC_PY::getFilterChannels(struct sffilter* filter, int io)
{
  return filter->channels[io];
}

vector<int> SFLOGIC_PY::getFilterFilters(struct sffilter* filter, int io)
{
  return filter->filters[io];
}

bool SFLOGIC_PY::connect_port(string port_name, string dest_name)
{
  return sfconf->io->connect_port(port_name, dest_name);
} 
bool SFLOGIC_PY::disconnect_port(string port_name, string dest_name)
{
  return sfconf->io->disconnect_port(port_name, dest_name);
}

const char** SFLOGIC_PY::get_jack_port_connections(string port_name) 
{
  return sfconf->io->get_jack_port_connections(port_name);
}

const char **SFLOGIC_PY::get_jack_ports(void)
{
  return sfconf->io->get_jack_ports();
}

const char **SFLOGIC_PY::get_jack_input_physical_ports(void)
{
  return sfconf->io->get_jack_input_physical_ports();
}

const char **SFLOGIC_PY::get_jack_input_ports(void)
{
  return sfconf->io->get_jack_input_ports();
}

const char **SFLOGIC_PY::get_jack_output_physical_ports(void)
{
  return sfconf->io->get_jack_output_physical_ports();
}

const char **SFLOGIC_PY::get_jack_output_ports(void)
{
  return sfconf->io->get_jack_output_ports();
}

bool SFLOGIC_PY::change_input_attenuation(string inputchannel, double scale)
{
  unsigned int i, j;
  for(i = 0; i < sfconf->filters.size(); i++) {
    for(j = 0; j < sfconf->filters[i].channels[IN].size(); j++) {
      if(sfconf->channels[IN][sfconf->filters[i].channels[IN][j]].name.compare(inputchannel) == 0) {
	change_filter_attenuation(sfconf->filters[i].name, IN, j, scale);
      }
    }
  }
  return true;
}

double SFLOGIC_PY::get_input_attenuation(string inputchannel)
{
  unsigned int i, j;
  for(i = 0; i < sfconf->filters.size(); i++) {
    for(j = 0; j < sfconf->filters[i].channels[IN].size(); j++) {
      if(sfconf->channels[IN][j].name.compare(inputchannel) == 0) {
	return get_filter_attenuation(sfconf->filters[i].name, IN, j);
      }
    }
  }
  return -90;
}

bool SFLOGIC_PY::change_output_attenuation(string outputchannel, double scale)
{
  unsigned int i, j;
  for(i = 0; i < sfconf->filters.size(); i++) {
    for(j = 0; j < sfconf->filters[i].channels[OUT].size(); j++) {
      if(sfconf->channels[OUT][sfconf->filters[i].channels[OUT][j]].name.compare(outputchannel) == 0) {
	change_filter_attenuation(sfconf->filters[i].name, OUT, j, scale);
      }
    }
  }
  return true;
}

bool SFLOGIC_PY::change_filter_coeff(string filtername, int coeff)
{
  unsigned int n;
  for(n = 0; n < sfconf->filters.size(); n++) {
    if(sfconf->filters[n].name.compare(filtername) == 0) {
      icomm->fctrl[n].coeff = coeff;
      return true;
    }
  }
  fprintf(stderr,"Filter name does not exist.\n");
  return false;
}

bool SFLOGIC_PY::change_filter_attenuation(string filtername, int INOUT, int nchannel, double scale)
{
  unsigned int n;
  for(n = 0; n < sfconf->filters.size(); n++) {
    if(sfconf->filters[n].name.compare(filtername) == 0) {
      if((unsigned int)nchannel < icomm->fctrl[n].scale[INOUT].size()) {
	icomm->fctrl[n].scale[INOUT][nchannel] = pow(10, -scale / 20);
	return true;
      } else {
	fprintf(stderr,"Channel number out of scope.\n");
	return false;
      } 
    }
  }
  fprintf(stderr,"Filter name does not exist.\n");
  return false;
}

double SFLOGIC_PY::get_filter_attenuation(string filtername, int INOUT, int nchannel)
{
  unsigned int n;
  for(n = 0; n < sfconf->filters.size(); n++) {
    if(sfconf->filters[n].name.compare(filtername) == 0) {
      if((unsigned int)nchannel < icomm->fctrl[n].scale[INOUT].size()) {
	return -20*log10(icomm->fctrl[n].scale[INOUT][nchannel]);
	
      } else {
	fprintf(stderr,"Channel number out of scope.\n");
	return -90;
      } 
    }
  }
  fprintf(stderr,"Filter name does not exist.\n");
  return -90;
}

bool SFLOGIC_PY::change_filter_filter_attenuation(string filtername, int nfilter, double scale)
{
  unsigned int n;
  for(n = 0; n < sfconf->filters.size(); n++) {
    if(sfconf->filters[n].name.compare(filtername) == 0) {
      if((unsigned int)nfilter < icomm->fctrl[n].fscale.size()) {
	icomm->fctrl[n].fscale[nfilter] = pow(10, -scale / 20);
	return true;
      } else {
	fprintf(stderr,"Filter number out of scope.\n");
	return false;
      } 
    }
  }
  fprintf(stderr,"Filter name does not exist.\n");
  return false;
}

void SFLOGIC_PY::toggle_mute(int INOUT, int nchannel)
{
  sfaccess->toggle_mute(INOUT, nchannel);
}

void SFLOGIC_PY::print_overflows(void)
{
  return;
}

vector<double> SFLOGIC_PY::get_inpeaks(void)
{
  int n;
  double peak;
  for (n = 0; n < sfconf->n_channels[IN]; n++) {
    peak = icomm->in_overflow[n].largest;
    if (peak < (double)icomm->in_overflow[n].intlargest) {
      peak = (double)icomm->in_overflow[n].intlargest;
    }
    inPeak[n] = 20.0 * log10(peak / icomm->in_overflow[n].max);
  }
  return inPeak;
}

vector<double> SFLOGIC_PY::get_outpeaks(void)
{
  int n;
  double peak;
  for (n = 0; n < sfconf->n_channels[OUT]; n++) {
    peak = icomm->out_overflow[n].largest;
    if (peak < (double)icomm->out_overflow[n].intlargest) {
      peak = (double)icomm->out_overflow[n].intlargest;
    }
    outPeak[n] = 20.0 * log10(peak / icomm->out_overflow[n].max);
  }
  return outPeak;
}
void SFLOGIC_PY::reset_peaks(void)
{
  sfaccess->reset_peak();
}


