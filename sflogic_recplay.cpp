/*
 * (c) Copyright 2013/2022 -- Raul Fernandez Ortega
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */

#include "sflogic_recplay.hpp"

SndFileManager::SndFileManager(struct sfconf *_sfconf,
			       struct intercomm_area *_icomm,
			       SfAccess *_sfaccess)
{
  sfconf = _sfconf;
  icomm = _icomm;
  sfaccess = _sfaccess;
  isWrite = true;
  isRunning = false;
  isOpen = false;
  sem_init(&manager_sem, 0, 1);
}

int SndFileManager::preinit(xmlNodePtr sfparams, bool _debug)
{
  debug = _debug;
  
  while (sfparams != NULL) {
    if (!xmlStrcmp(sfparams->name, (const xmlChar *)"path")) {
      path = reinterpret_cast<char*>(xmlNodeGetContent(sfparams));
    } else if (!xmlStrcmp(sfparams->name, (const xmlChar *)"input")) {
      InputChannel = reinterpret_cast<char*>(xmlNodeGetContent(sfparams));
    } else if (!xmlStrcmp(sfparams->name, (const xmlChar *)"output")) {
      OutputChannel = reinterpret_cast<char*>(xmlNodeGetContent(sfparams));
    } else if (!xmlStrcmp(sfparams->name, (const xmlChar *)"recplay")) {
      isWrite = (!xmlStrcmp(xmlNodeGetContent(sfparams),(const xmlChar *)"rec")) ? true : false;
    }
    sfparams = sfparams->next;
  }
  return 0;
}

int SndFileManager::config(string _path, bool _is_in, string _channel, bool _recplay)
{
  path = _path;
  if(_is_in) 
    InputChannel = _channel;
  else
    OutputChannel = _channel;
  isWrite = _recplay;
  return 0;
} 

void* SndFileManager::readwriteProcessThread(void *args)
{
  reinterpret_cast<SndFileManager *>(args)->readwriteProcess();
  return NULL;
}

void SndFileManager::readwriteProcess(void)
{
  sf_count_t count;
  int buf_size;

  do {
    if(isRunning && isOpen && !isWrite) {
      if(sfconf->realsize == 4)
	count = sf_readf_float(sf_file,  &((float*)iobuffer)[0], sfconf->filter_length);
      else
	count = sf_readf_double(sf_file, &((double*)iobuffer)[0], sfconf->filter_length);
      if(count < sfconf->filter_length)
	stop();
    }
    sem_wait(&manager_sem);
    if(isRunning && isOpen && isWrite) {
      if (l_time - time_ms < sfconf->filter_length)
	buf_size = (int)(time_ms - (l_time + sfconf->filter_length));
      else
	buf_size = sfconf->filter_length;
      if(sfconf->realsize == 4)
	sf_writef_float(sf_file, (float*)iobuffer, buf_size);
      else
	sf_writef_double(sf_file, (double*)iobuffer, buf_size);
    } 
    time_ms += sfconf->filter_length;
    if(isOpen && l_time > 0 && time_ms >= l_time)
      stop();
  } while (true);
}

void SndFileManager::init(void)
{
  if(sfconf->realsize == 4) {
    iobuffer = emalloc(2 * sfconf->filter_length* sizeof(float));
    MEMSET((float*)iobuffer, 0, 2 * sfconf->filter_length * sizeof(float));
  } else {
    iobuffer = emalloc(2 * sfconf->filter_length* sizeof(double));
    MEMSET((double*)iobuffer, 0, 2 * sfconf->filter_length * sizeof(double));
  }
  pthread_create(&manager_thread, NULL,&SndFileManager::readwriteProcessThread, this);
}

void SndFileManager::start(int _l_time)
{
  openFilename();
  isRunning = true;
  time_ms = 0;
  l_time = _l_time*sfconf->sampling_rate;
}

void SndFileManager::stop(void)
{
  isRunning = false;
  closeFilename();
}

bool SndFileManager::openFilename(void)
{
  int short_mask;
 
  if(isWrite) { 
    sf_info.samplerate = sfconf->sampling_rate;
    sf_info.channels = 1;
    short_mask = SF_FORMAT_FLOAT | SF_ENDIAN_FILE;
    sf_info.format = SF_FORMAT_WAV | short_mask;
    
    if((sf_file = sf_open(path.data(), SFM_WRITE, &sf_info)) == NULL) {
      fprintf(stderr, "cannot open sndfile \"%s\" for output (%s)\n",path.data(), sf_strerror(sf_file));
      return false;
    }
  } else {
    if((sf_file = sf_open(path.data(), SFM_READ, &sf_info)) == NULL) {
      fprintf(stderr, "cannot open sndfile \"%s\" for output (%s)\n",path.data(), sf_strerror(sf_file));
      return false;
    }
    if(sf_info.samplerate != sfconf->sampling_rate) {
      fprintf(stderr, "sndfile sample rate:%d .Convolver sample rate:%d.\n",sf_info.samplerate, sfconf->sampling_rate);
      return false;
    }
  } 
  fprintf(stderr,"Snd File opened:%s\n",path.data());
  isOpen = true;
  return true;
}

bool SndFileManager::closeFilename(void)
{
  isOpen = false;
  sf_close(sf_file);
  fprintf(stderr, "Snd File closed:%s\n",path.data());
  return true;
}

void SndFileManager:: input_timed(void *buffer, int channel)
{
  if(InputChannel.empty())
    return;
  if(sfconf->channels[SF_IN][channel].name.compare(InputChannel) == 0) {
    if(isRunning && isOpen) {
      if(isWrite) {
	if(sfconf->realsize == 4) {
	  MEMCPY(&((float*)iobuffer)[0], &((float*)buffer)[sfconf->filter_length], sfconf->filter_length*sizeof(float));
	} else {
	  MEMCPY(&((double*)iobuffer)[0], &((double*)buffer)[sfconf->filter_length], sfconf->filter_length*sizeof(double));
	}
      } else {
       	if(sfconf->realsize == 4) {
	  MEMCPY(&((float*)buffer)[sfconf->filter_length], &((float*)iobuffer)[0], sfconf->filter_length*sizeof(float));
	} else {
	  MEMCPY(&((double*)buffer)[sfconf->filter_length], &((double*)iobuffer)[0], sfconf->filter_length*sizeof(double));
	}
      }
      sem_post(&manager_sem);
    }
  }
}
void SndFileManager::output_timed(void *buffer, int channel)
{
  if(OutputChannel.empty())
    return;
  if(sfconf->channels[SF_OUT][channel].name.compare(OutputChannel) == 0) {
    if(isRunning && isOpen) {
      if(isWrite) {
	if(sfconf->realsize == 4) {
	  MEMCPY(&((float*)iobuffer)[0], &((float*)buffer)[sfconf->filter_length], sfconf->filter_length*sizeof(float));
	} else {
	  MEMCPY(&((double*)iobuffer)[0], &((double*)buffer)[sfconf->filter_length], sfconf->filter_length*sizeof(double));
	}
      } else {
	if(sfconf->realsize == 4) {
	  MEMCPY(&((float*)buffer)[sfconf->filter_length], &((float*)iobuffer)[0], sfconf->filter_length*sizeof(float));
	} else {
	  MEMCPY(&((double*)buffer)[sfconf->filter_length], &((double*)iobuffer)[0], sfconf->filter_length*sizeof(double));
	}
      }
      sem_post(&manager_sem);
    }
  }
}

SFLOGIC_RECPLAY::SFLOGIC_RECPLAY(struct sfconf *_sfconf,
				 struct intercomm_area *_icomm,
				 SfAccess *_sfaccess) : SFLOGIC(_sfconf, _icomm, _sfaccess) 
{ 
  fork_mode = SF_FORK_DONT_FORK;
  debug = false;
  name = "recplay";
};

int SFLOGIC_RECPLAY::preinit(xmlNodePtr sfparams, int _debug)
{
  debug = !!_debug;
  
  MEMSET(msg, 0, MAX_MSG_LEN);
  while (sfparams != NULL) {
    if (!xmlStrcmp(sfparams->name, (const xmlChar *)"file")) {
      managers.push_back(new SndFileManager(sfconf,icomm, sfaccess));
      managers.back()->preinit(sfparams->children, debug);			
    }
    sfparams = sfparams->next;
  }
  return 0;
}

void SFLOGIC_RECPLAY::addManager(SndFileManager* _manager)
{
  managers.push_back(_manager);
}

int SFLOGIC_RECPLAY::init(int event_fd, sem_t *synch_sem)
{
  unsigned int n;
  for(n = 0; n < managers.size(); n++) {
    managers[n]->init();
  }
  return 0;
}

int SFLOGIC_RECPLAY::command(const char params[])
{
  return 0;
}

const char *SFLOGIC_RECPLAY::message(void)
{
  return msg;
}

void SFLOGIC_RECPLAY::output_timed(void *buffer, int channel) {
  unsigned int i;
  for(i = 0; i < managers.size(); i++) {
    managers[i]->output_timed(buffer, channel);
  }
}

void SFLOGIC_RECPLAY::input_timed(void *buffer, int channel) {
  unsigned int i;
  for(i = 0; i < managers.size(); i++) {
    managers[i]->input_timed(buffer, channel);
  }
}
