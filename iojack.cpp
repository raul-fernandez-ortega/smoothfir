/*
 * (c) Copyright 2013 -- Raul Fernandez Ortega
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */

#include "iojack.hpp"

#define DEFAULT_CLIENTNAME "smoothfir"
#define DEFAULT_JACK_CB_THREAD_PRIORITY 5

ioJack::ioJack(struct sfconf *_sfconf,
	       struct intercomm_area *_icomm,
	       Dai *_sfDai): IO(_sfconf, _icomm)
{
  sfconf = _sfconf;
  icomm = _icomm;
  sfDai = _sfDai;
  debug = !!sfconf->debug;
  has_started = false;

  MEMSET(hasio, 0, sizeof(hasio));
  MEMSET(handles, 0, sizeof(handles));
  MEMSET(n_handles, 0, sizeof(n_handles));

  io_idx[SF_IN] = 0;
  io_idx[SF_OUT] = 0;
  
  expected_priority = -1;
  client = NULL;
  client_name = NULL;
  frames_left = 0;
}

ioJack::~ioJack(void)
{
  synch_stop();
}

void ioJack::preinit(int io, struct iodev *iodev)
{
  vector<int> subdevs;
  int rate, n;
  xmlNodePtr sfsubparams;
  xmlNodePtr xmldata;
  struct jack_state *jackState;
  struct sched_param *callback_sched_param;
  
#ifdef __LITTLE_ENDIAN__
  if (sizeof(jack_default_audio_sample_t) == 4) {
    iodev->ch.sf.format = SF_SAMPLE_FORMAT_FLOAT_LE;
  } else if (sizeof(jack_default_audio_sample_t) == 8) {
    iodev->ch.sf.format = SF_SAMPLE_FORMAT_FLOAT64_LE;
  }
#endif        
#ifdef __BIG_ENDIAN__
  if (sizeof(jack_default_audio_sample_t) == 4) {
    iodev->ch.sf.format = SF_SAMPLE_FORMAT_FLOAT_BE;
  } else if (sizeof(jack_default_audio_sample_t) == 8) {
    iodev->ch.sf.format = SF_SAMPLE_FORMAT_FLOAT64_BE;
  }
#endif       
  if (sizeof(jack_default_audio_sample_t) == 4) {
#ifdef __LITTLE_ENDIAN__
    if (iodev->ch.sf.format != SF_SAMPLE_FORMAT_FLOAT_LE) {
      fprintf(stderr, "JACK I/O: Sample format must be %s or %s.\n", sf_strsampleformat(SF_SAMPLE_FORMAT_FLOAT_LE), sf_strsampleformat(SF_SAMPLE_FORMAT_AUTO));
      return;
    }
#endif        
#ifdef __BIG_ENDIAN__
    if (iodev->ch.sf.format != SF_SAMPLE_FORMAT_FLOAT_BE) {
      fprintf(stderr, "JACK I/O: Sample format must be %s or %s.\n", sf_strsampleformat(SF_SAMPLE_FORMAT_FLOAT_BE), sf_strsampleformat(SF_SAMPLE_FORMAT_AUTO));
      return;
    }
#endif        
  } else if (sizeof(jack_default_audio_sample_t) == 8) {
#ifdef __LITTLE_ENDIAN__
    if (iodev->ch.sf.format != SF_SAMPLE_FORMAT_FLOAT64_LE) {
      fprintf(stderr, "JACK I/O: Sample format must be %s or %s.\n", sf_strsampleformat(SF_SAMPLE_FORMAT_FLOAT64_LE), sf_strsampleformat(SF_SAMPLE_FORMAT_AUTO));
      return;
    }
#endif        
#ifdef __BIG_ENDIAN__
    if (sample_format != SF_SAMPLE_FORMAT_FLOAT64_BE) {
      fprintf(stderr, "JACK I/O: Sample format must be %s or %s.\n", sf_strsampleformat(SF_SAMPLE_FORMAT_FLOAT64_BE), sf_strsampleformat(SF_SAMPLE_FORMAT_AUTO));
      return;
    }
#endif        
  }
  jackState = new struct jack_state();
  jackState->n_channels = iodev->ch.open_channels;
  jackState->local_port_name.resize(iodev->ch.open_channels);
  jackState->dest_name.resize(iodev->ch.open_channels);
  jackState->port_name.resize(iodev->ch.open_channels);

  xmldata = iodev->device_params;
  while (xmldata != NULL) {
    if (!xmlStrcmp(xmldata->name, (const xmlChar *)"port")) { 
      sfsubparams = xmldata->children;
      while (sfsubparams != NULL) {
	if(!xmlStrcmp(sfsubparams->name, (const xmlChar *)"index")) {
	  n = atoi((char*)xmlNodeGetContent(sfsubparams));
	  if (n > iodev->ch.open_channels - 1) {
	    fprintf(stderr, "JACK I/O: Parse error: wrong port index.\n");
	    return;
	  }
	  subdevs.push_back(n);
	} else if(!xmlStrcmp(sfsubparams->name, (const xmlChar *)"destname")) {
	  jackState->dest_name[n] = reinterpret_cast<char*>(xmlNodeGetContent(sfsubparams));
	} else if(!xmlStrcmp(sfsubparams->name, (const xmlChar *)"localname")) {
	  jackState->local_port_name[n] = reinterpret_cast<char*>(xmlNodeGetContent(sfsubparams));
	}
	sfsubparams = sfsubparams->next;
      }
    } else if(!xmlStrcmp(xmldata->name, (const xmlChar *)"clientname")) {
      if (client != NULL && strcmp((char*)xmlNodeGetContent(xmldata), client_name) != 0) {
	fprintf(stderr, "JACK I/O: clientname setting is global and must be set in the first jack device.\n");
	//return;
      }
      if (client_name == NULL) {
	client_name = estrdup((char*)xmlNodeGetContent(xmldata));
      }
    } else if (!xmlStrcmp(xmldata->name, (const xmlChar *)"priority")) {
      if (client != NULL && expected_priority != atoi((char*)xmlNodeGetContent(xmldata))) {
	fprintf(stderr, "JACK I/O: priority setting is global and must be set in the first jack device.\n");
	//return;
      }
      expected_priority = atoi((char*)xmlNodeGetContent(xmldata));
    } else if (xmlStrcmp(xmldata->name, (const xmlChar *)"text") && xmlStrcmp(xmldata->name, (const xmlChar *)"comment")) {
      fprintf(stderr, "JACK I/O: Parse error: expected field.\n");
      return;
    }
    xmldata = xmldata->next;
  }
  hasio[io] = true;
  
  if (expected_priority < 0) {
    expected_priority = DEFAULT_JACK_CB_THREAD_PRIORITY;
  }
  if (client == NULL) {
    if (client_name == NULL) {
      client_name = estrdup(DEFAULT_CLIENTNAME);
    }
    if (!global_init()) {
      return;
    }
  } 
  
  callback_sched_param = &sfconf->subdevs[io].sched_param;
  MEMSET(callback_sched_param, 0, sizeof(*callback_sched_param));
  sfconf->subdevs[io].sched_param.sched_priority = expected_priority;
  sfconf->subdevs[io].sched_policy = SCHED_FIFO;
    
  if ((rate = (int)jack_get_sample_rate(client)) == 0) {
    sfconf->subdevs[io].uses_clock = 0;
  } else {
    sfconf->subdevs[io].uses_clock = 1;
  }
  sfconf->subdevs[io].i_handle = n_handles[io];

  handles[io].push_back(jackState);

  if (rate != 0 && rate != sfconf->sampling_rate) {
    fprintf(stderr, "JACK I/O: JACK sample rate is %d, smoothfir is %d, they must be same.\n", rate, sfconf->sampling_rate);
    return;
  }
  n_handles[io]++; 
  return;
}

void ioJack::init_callback(void *arg)
{
  ioJack *CallbackJackObject;
  CallbackJackObject = (ioJack*) arg;

  static pthread_once_t once_control = PTHREAD_ONCE_INIT;

  struct sched_param schp;
  int policy;

  pthread_getschedparam(pthread_self(), &policy, &schp);

  if (policy != SCHED_FIFO && policy != SCHED_RR) {
    //fprintf(stderr, "JACK I/O: Warning: JACK is not running with SCHED_FIFO or SCHED_RR (realtime).\n");
  } else if (schp.sched_priority != CallbackJackObject->expected_priority) {
    fprintf(stderr, "\
JACK I/O: Warning: JACK thread has priority %d, but smoothfir expected %d.\n \
  In order to make correct realtime scheduling smoothfir must know what\n \
  priority JACK uses. At the time of writing the JACK API does not support\n \
  getting that information so smoothfir must be manually configured with that.\n	\
  Use the \"priority\" setting in the first \"jack\" device clause in your\n \
  smoothfir configuration file.\n",
	    (int)schp.sched_priority, (int)CallbackJackObject->expected_priority);
  }

  pthread_once(&once_control, (void (*)())ioJack::init_callback_); 
}


void ioJack::init_callback_(void)
{
  return;
}

void ioJack::latency_callback(jack_latency_callback_mode_t mode, void *arg)
{
  ioJack *CallbackJackObject;
  CallbackJackObject = (ioJack*) arg;
  CallbackJackObject->sf_latency_callback(mode);
}

void ioJack::sf_latency_callback(jack_latency_callback_mode_t mode)
{
  jack_latency_range_t range;
  struct jack_state *js;
  int n, i;
  
  // same latency for all ports, regardless of how they are connected
  if (mode == JackPlaybackLatency) {
    // do nothing
  } else if (mode == JackCaptureLatency) {
    for (n = 0; n < n_handles[OUT]; n++) {
      js = handles[OUT][n];
      for (i = 0; i < js->n_channels; i++) {
	range.min = period_size;
	range.max = period_size;
	jack_port_set_latency_range(js->ports[i], mode, &range);
      }
    }
  }
}

void ioJack::jack_shutdown_callback(void *arg)
{
  ioJack *CallbackJackObject;
  CallbackJackObject = (ioJack*) arg;
  CallbackJackObject->sf_shutdown_callback();
}

void ioJack::sf_shutdown_callback(void)
{
  fprintf(stderr, "JACK I/O: JACK daemon shut down.\n");
  has_started = false;
  sfDai->process_callback(states, n_handles, NULL, 0, SF_CALLBACK_EVENT_ERROR);
}

int ioJack::jack_process_callback(jack_nframes_t n_frames, void *arg)
{
  return reinterpret_cast<ioJack *>(arg)->sf_process_callback(n_frames);
}

void ioJack::error_callback(const char *msg)
{   
  if (msg[strlen(msg)-1] ==  '\n') {
    fprintf(stderr, "JACK I/O: JACK reported an error: %s", msg);
  } else {
    fprintf(stderr, "JACK I/O: JACK reported an error: %s\n", msg);
  }
}


int ioJack::sf_process_callback(jack_nframes_t n_frames)
{
  int IO;
  struct jack_state *jackState;
    
  void *in_bufs[SF_MAXCHANNELS], *out_bufs[SF_MAXCHANNELS], **iobufs[2];
  void *buffer = NULL;
  int n, i;

  iobufs[IN] = n_handles[IN] > 0 ? in_bufs : NULL;
  iobufs[OUT] = n_handles[OUT] > 0 ? out_bufs : NULL;
  FOR_IN_AND_OUT {
    for (n = 0; n < n_handles[IO]; n++) {
      jackState = handles[IO][n];
      for (i = 0; i < jackState->n_channels; i++) {
	iobufs[IO][i] = jack_port_get_buffer(jackState->ports[i], n_frames);
      }
    }
  }
  if (frames_left != 0) {
    sfDai->process_callback(states, n_handles, NULL, 0, SF_CALLBACK_EVENT_FINISHED);
    for (n = 0; n < n_handles[OUT]; n++) {
      jackState = handles[OUT][n];
      for (i = 0; i < jackState->n_channels; i++) {
	buffer = jack_port_get_buffer(jackState->ports[i], n_frames);
	MEMSET(buffer, 0, n_frames * sizeof(jack_default_audio_sample_t));
      }
    }
    has_started = false;
    return -1;
  }
  frames_left = sfDai->process_callback(states, n_handles, iobufs, n_frames, SF_CALLBACK_EVENT_NORMAL);
  if (frames_left == -1) {
    has_started = false;
    return -1;
  }
  return 0;
}

bool ioJack::global_init(void)
{
  jack_status_t status;
  jack_set_error_function(error_callback);
  while ((client = jack_client_open(client_name, JackNoStartServer, &status)) == NULL) {
    fprintf(stderr, "JACK I/O: Could not become JACK client (status: 0x%2.0x). Error message(s):\n", status);
    if ((status & JackFailure) != 0) {
      fprintf(stderr, "Overall operation failed.\n");
      //exit(SF_EXIT_OTHER);
    }
    if ((status & JackInvalidOption) != 0) {
      fprintf(stderr, "  Likely bug in smoothfir: the operation contained an invalid or unsupported\noption.\n");
      exit(SF_EXIT_OTHER);
    }
    if ((status & JackNameNotUnique) != 0) {
      fprintf(stderr, "  Client name \"%s\" not unique, try another name.\n", client_name);
      exit(SF_EXIT_OTHER);
    }
    if ((status & JackServerFailed) != 0) {
      fprintf(stderr, "  Unable to connect to the JACK server. Perhaps it is not running? smoothfir\nrequires that a JACK server is started in advance.\n");
      //exit(SF_EXIT_OTHER);
    }
    if ((status & JackServerError) != 0) {
      fprintf(stderr, "  Communication error with the JACK server.\n");
      exit(SF_EXIT_OTHER);
    }
    if ((status & JackNoSuchClient) != 0) {
      fprintf(stderr, "  Requested client does not exist.\n");
      exit(SF_EXIT_OTHER);
    }
    if ((status & JackLoadFailure) != 0) {
      fprintf(stderr, "  Unable to load internal client.\n");
      exit(SF_EXIT_OTHER);
    }
    if ((status & JackInitFailure) != 0) {
      fprintf(stderr, "  Unable initialize client.\n");
      exit(SF_EXIT_OTHER);
    }
    if ((status & JackShmFailure) != 0) {
      fprintf(stderr, "  Unable to access shared memory.\n");
      exit(SF_EXIT_OTHER);
    }
    if ((status & JackVersionError) != 0) {
      fprintf(stderr, "The version of the JACK server is not compatible with the JACK client\nlibrary used by smoothfir.\n");
      exit(SF_EXIT_OTHER);
    }
    sleep(1);
    //return false;
  }
  jack_set_thread_init_callback(client,init_callback, this);
  jack_set_process_callback(client, ioJack::jack_process_callback, this);
  jack_on_shutdown(client, ioJack::jack_shutdown_callback, this);
  
  return true;
}

int ioJack::init(int io,
		 int i_handle,
		 int used_channels,
		 vector<int> channel_selection,
		 struct subdev *callback_subdev)
{ 
  const char *name;
  char _name[128], longname[1024];
  struct jack_state *jackState;
  jack_port_t *port;
  int n;
  
  callback_subdev->block_size_frames = jack_get_buffer_size(client);
  callback_subdev->isinterleaved = false;

  jackState = handles[io][i_handle];
  
  if (used_channels != jackState->n_channels) {
    fprintf(stderr, "JACK I/O: Open channels must be equal to used channels for this I/O module.\n");
    return -1;
  }

  _states[io].resize(used_channels);
  for (n = 0; n < used_channels; n++) {
    if (!jackState->dest_name[n].empty()) {
      if ((port = jack_port_by_name(client, jackState->dest_name[n].data())) == NULL) {
	fprintf(stderr, "JACK I/O: Failed to open JACK port \"%s\".\n", jackState->dest_name[n].data());
	return -1;
      }
      if ((io == IN && (jack_port_flags(port) & JackPortIsOutput) == 0) || (io == OUT && (jack_port_flags(port) & JackPortIsInput) == 0)) {
	fprintf(stderr, "JACK I/O: JACK port \"%s\" is not an %s.\n", jackState->dest_name[n].data(), io == IN ? "Output" : "Input");
	return -1;
      }
    }
    
    if (!jackState->local_port_name[n].empty()) {
      name = jackState->local_port_name[n].data();
    } else {
      name = _name;
      sprintf((char*)name, "%s-%d", io == IN ? "input" : "output", io_idx[io]++);
    }
    
    port = jack_port_register(client, name, JACK_DEFAULT_AUDIO_TYPE, (io == IN ? JackPortIsInput : JackPortIsOutput), 0);
    if (port == NULL) {
      fprintf(stderr, "JACK I/O: Failed to open new JACK port %s:%s.\n", name);
      return -1;
    }
    /*if (io == OUT) {
      jack_port_set_latency(port, sfconf->filter_length);
      } */
    jackState->ports.push_back(port);
    snprintf(longname, sizeof(longname), "%s:%s", client_name, name);
    longname[sizeof(longname) - 1] = '\0';
    jackState->port_name[n] = estrdup(longname);
    _states[io][n] = callback_subdev;
  }
  states[io] = _states[io];    

  if (io == OUT && hasio[IN]) {
    jack_set_latency_callback(client, ioJack::latency_callback, this);
  }

  return 0;
}

int ioJack::synch_start(void)
{
  struct jack_state *jackState;
  int n, i;

  if (has_started) {
    return 0;
  }
  if (client == NULL) {
    fprintf(stderr, "JACK I/O: client is NULL\n");
    return -1;
  }
  has_started = true;
  /*
   * jack_activate() will start a new pthread. We block all signals before
   * calling to make sure that we get all signals to our thread. This is a
   * bit ugly, since it assumes that the JACK library does not mess up the
   * signal handlers later.
   */
  n = jack_activate(client);
  if (n != 0) {
    fprintf(stderr, "JACK I/O: Could not activate local JACK client.\n");
    has_started = false;
    return -1;
  }
  for (n = 0; n < n_handles[IN]; n++) {
    jackState = handles[IN][n];
    for (i = 0; i < jackState->n_channels; i++) {
      if (jackState->dest_name[i].empty()) {
	continue;
      }
      if (jack_connect(client, jackState->dest_name[i].c_str(), jackState->port_name[i].c_str()) != 0) {
	fprintf(stderr, "JACK I/O: Could not connect local port \"%s\" to \"%s\".\n", jackState->port_name[i].c_str(), jackState->dest_name[i].c_str());
	has_started = false;
	return -1;
      }
    }
  }
  for (n = 0; n < n_handles[OUT]; n++) {
    jackState = handles[OUT][n];
    for (i = 0; i < jackState->n_channels; i++) {
      if (jackState->dest_name[i].empty()) {
	continue;
      }
      if (jack_connect(client, jackState->port_name[i].data(), jackState->dest_name[i].data()) != 0) {
	fprintf(stderr, "JACK I/O: Could not connect local port \"%s\" to \"%s\".\n", jackState->port_name[i].data(), jackState->dest_name[i].data());
	has_started = false;
	return -1;
      }
    }
  }
  return 0;
}

bool ioJack::connect_port(string port_name, string dest_name) 
{
  if (jack_connect(client, port_name.c_str(), dest_name.c_str()) != 0) {
    fprintf(stderr, "JACK I/O: Could not connect local port \"%s\" to \"%s\".\n", port_name.c_str(), dest_name.c_str());
    return false;
  } else {
    return true;
  }
}

bool ioJack::disconnect_port(string port_name, string dest_name) 
{
  if (jack_disconnect(client, port_name.c_str(), dest_name.c_str()) != 0) {
    fprintf(stderr, "JACK I/O: Could not disconnect local port \"%s\" to \"%s\".\n", port_name.c_str(), dest_name.c_str());
    return false;
  } else {
    return true;
  }
}

const char ** ioJack::get_jack_port_connections(string port_name)
{
  jack_port_t *port;

  port = jack_port_by_name(client, port_name.c_str());
  if (port == NULL) 
    {
      fprintf(stderr, "JACK I/O: Can't find port '%s'\n", port_name.c_str());
      return NULL;
    }
  return jack_port_get_all_connections(client, port);
}

const char **ioJack::get_jack_ports(void)
{
  return jack_get_ports (client, NULL, NULL, 0);
}

const char **ioJack::get_jack_input_physical_ports(void)
{
  return jack_get_ports (client, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput | JackPortIsPhysical);
}

const char **ioJack::get_jack_input_ports(void)
{
  return jack_get_ports (client, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput);
}

const char **ioJack::get_jack_output_physical_ports(void)
{
  return jack_get_ports (client, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput | JackPortIsPhysical);
}

const char **ioJack::get_jack_output_ports(void)
{
  return jack_get_ports (client, NULL, JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput);
}

void ioJack::synch_stop(void)
{
  fprintf(stderr,"synch_stop\n");
  has_started = false;
  jack_client_close(client);
  global_init();
}    
