/*
 * (c) Copyright 2013/2022 -- Raul Fernandez Ortega
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */

#include "sflogic_ladspa.hpp"

int LADSPA_PLUGIN::analysePlugin(void)
{
  unsigned long lPortIndex;

  fprintf(stderr,"Plugin Name: \"%s\"\n", PluginDescriptor->Name);
  fprintf(stderr,"Plugin Label: \"%s\"\n", PluginDescriptor->Label);
  fprintf(stderr,"Plugin Unique ID: %lu\n", PluginDescriptor->UniqueID);
  fprintf(stderr,"Maker: \"%s\"\n", PluginDescriptor->Maker);
  fprintf(stderr,"Copyright: \"%s\"\n", PluginDescriptor->Copyright);
      
  fprintf(stderr,"Must Run Real-Time: ");
  if (LADSPA_IS_REALTIME(PluginDescriptor->Properties))
    fprintf(stderr,"Yes\n");
  else
    fprintf(stderr,"No\n");
      
  fprintf(stderr,"Has activate() Function: ");
  if (PluginDescriptor->activate != NULL)
    fprintf(stderr,"Yes\n");
  else
    fprintf(stderr,"No\n");    
  fprintf(stderr,"Has deactivate() Function: ");
  if (PluginDescriptor->deactivate != NULL)
    fprintf(stderr,"Yes\n");
  else
    fprintf(stderr,"No\n");
  fprintf(stderr,"Has run_adding() Function: ");
  if (PluginDescriptor->run_adding != NULL)
    fprintf(stderr,"Yes\n");
  else
    fprintf(stderr,"No\n");
  
  if (PluginDescriptor->instantiate == NULL) {
    fprintf(stderr,"ERROR: PLUGIN HAS NO INSTANTIATE FUNCTION.\n");
    return -1;
  }
  if (PluginDescriptor->connect_port == NULL) {
    fprintf(stderr,"ERROR: PLUGIN HAS NO CONNECT_PORT FUNCTION.\n");
    return -1;
  }
  if (PluginDescriptor->run == NULL) {
    fprintf(stderr,"ERROR: PLUGIN HAS NO RUN FUNCTION.\n");
    return -1;
  }
  if (PluginDescriptor->run_adding != NULL && PluginDescriptor->set_run_adding_gain == NULL) {
    fprintf(stderr,"ERROR: PLUGIN HAS RUN_ADDING FUNCTION BUT NOT SET_RUN_ADDING_GAIN.\n");
    return -1;
  }
  if (PluginDescriptor->run_adding == NULL && PluginDescriptor->set_run_adding_gain != NULL) {
    fprintf(stderr,"ERROR: PLUGIN HAS SET_RUN_ADDING_GAIN FUNCTION BUT NOT RUN_ADDING.\n");
    return -1;
  }
  if (PluginDescriptor->cleanup == NULL) {
    fprintf(stderr,"ERROR: PLUGIN HAS NO CLEANUP FUNCTION.\n");
    return -1;
  }
  
  fprintf(stderr,"Environment: ");
  if (LADSPA_IS_HARD_RT_CAPABLE(PluginDescriptor->Properties))
    fprintf(stderr,"Normal or Hard Real-Time\n");
  else
    fprintf(stderr,"Normal\n");
  
  if (LADSPA_IS_INPLACE_BROKEN(PluginDescriptor->Properties))
    fprintf(stderr,"This plugin cannot use in-place processing. It will not work with all hosts.\n");
  
  fprintf(stderr,"Ports:\n");
  
  if (PluginDescriptor->PortCount == 0) {
    fprintf(stderr,"\tERROR: PLUGIN HAS NO PORTS.\n");
    return -1;
  }

  for (lPortIndex = 0; lPortIndex < PluginDescriptor->PortCount; lPortIndex++) { 
    fprintf(stderr,"\t\"%s\" ", PluginDescriptor->PortNames[lPortIndex]);
    if (LADSPA_IS_PORT_INPUT(PluginDescriptor->PortDescriptors[lPortIndex]) && LADSPA_IS_PORT_OUTPUT(PluginDescriptor->PortDescriptors[lPortIndex])) {
      fprintf(stderr,"ERROR: INPUT AND OUTPUT");
      return -1;
    }
    else if (LADSPA_IS_PORT_INPUT(PluginDescriptor->PortDescriptors[lPortIndex])) {
      fprintf(stderr,"input");
      if(!LADSPA_IS_PORT_CONTROL(PluginDescriptor->PortDescriptors[lPortIndex]))
	nPluginInputs++;
    }
    else if (LADSPA_IS_PORT_OUTPUT(PluginDescriptor->PortDescriptors[lPortIndex])) {
      fprintf(stderr,"output");
      if(!LADSPA_IS_PORT_CONTROL(PluginDescriptor->PortDescriptors[lPortIndex]))
	nPluginOutputs++;
    }
    else { 
      fprintf(stderr,"ERROR: NEITHER INPUT NOR OUTPUT");
      return -1;
    }
    if (LADSPA_IS_PORT_CONTROL(PluginDescriptor->PortDescriptors[lPortIndex])&& LADSPA_IS_PORT_AUDIO(PluginDescriptor->PortDescriptors[lPortIndex])) {
      fprintf(stderr,", ERROR: CONTROL AND AUDIO");
      return -1;
    } else if (LADSPA_IS_PORT_CONTROL(PluginDescriptor->PortDescriptors[lPortIndex])) {
      fprintf(stderr,", control");
      PluginControls[nPluginControls].control_value = new LADSPA_Data();
      PluginControls[nPluginControls].control_label = PluginDescriptor->PortNames[lPortIndex];
      PluginControls[nPluginControls].index = lPortIndex;
      nPluginControls++;
    }
    else if (LADSPA_IS_PORT_AUDIO(PluginDescriptor->PortDescriptors[lPortIndex]))
      fprintf(stderr,", audio");
    else { 
      fprintf(stderr,", ERROR: NEITHER CONTROL NOR AUDIO");
      return -1;
    }
    fprintf(stderr,"\n");
  }
  return 0;
}

void LADSPA_PLUGIN::ConnectControlPort(string port_name, LADSPA_Data port_value)
{
  unsigned long PortIndex;
  int n;
  LADSPA_Data mult;

  for (PortIndex = 0; PortIndex < PluginDescriptor->PortCount; PortIndex++) {
    if (port_name.compare(PluginDescriptor->PortNames[PortIndex]) == 0) {
      if (PluginDescriptor->PortRangeHints[PortIndex].HintDescriptor & LADSPA_HINT_TOGGLED && 
	  port_value != LADSPA_HINT_DEFAULT_0 &&  port_value != LADSPA_HINT_DEFAULT_1) {
	fprintf(stderr,"Error in %s value: it must be boolean.\n",port_name.c_str());
	exit(SF_EXIT_OTHER);
      } else {
	if (PluginDescriptor->PortRangeHints[PortIndex].HintDescriptor & LADSPA_HINT_SAMPLE_RATE)
	  mult = sfconf->sampling_rate;
	else
	  mult = 1;
	if (PluginDescriptor->PortRangeHints[PortIndex].HintDescriptor & LADSPA_HINT_BOUNDED_BELOW && 
	    port_value < mult * PluginDescriptor->PortRangeHints[PortIndex].LowerBound) {
	  fprintf(stderr,"Error in %s value: it is below lower bound:%.4f.\n",port_name.c_str(), PluginDescriptor->PortRangeHints[PortIndex].LowerBound);
	  exit(SF_EXIT_OTHER);
	} else 	if (PluginDescriptor->PortRangeHints[PortIndex].HintDescriptor & LADSPA_HINT_BOUNDED_ABOVE && 
		    port_value > mult * PluginDescriptor->PortRangeHints[PortIndex].UpperBound) {
	  fprintf(stderr,"Error in %s value: %.4f it is above upper bound:%.4f.\n",port_name.c_str(), port_value,PluginDescriptor->PortRangeHints[PortIndex].UpperBound);
	  exit(SF_EXIT_OTHER);
	}
      }
      for(n = 0; n < nPluginControls; n ++) {
	if(PluginControls[n].index == PortIndex) {
	  memcpy(PluginControls[n].control_value, &port_value, sizeof(LADSPA_Data));
	  PluginDescriptor->connect_port(PluginHandle, PortIndex, PluginControls[n].control_value);
	  break;
	}
      }
      break;
    }
  }
}

void LADSPA_PLUGIN::ConnectInputPort(LADSPA_Data* buffer)
{
  unsigned int PortIndex;
  LADSPA_PortDescriptor PortDescriptor;

  for (PortIndex = 0; PortIndex < PluginDescriptor->PortCount; PortIndex++) {
    PortDescriptor = PluginDescriptor->PortDescriptors[PortIndex];
    if (LADSPA_IS_PORT_INPUT(PortDescriptor) && LADSPA_IS_PORT_AUDIO(PortDescriptor)) {
      PluginDescriptor->connect_port(PluginHandle, PortIndex, buffer);
      nPluginInputs++;
      break;
    }
  }
}

void LADSPA_PLUGIN::ConnectOutputPort(vector<LADSPA_Data*> buffer)
{
  unsigned int PortIndex;
  LADSPA_PortDescriptor PortDescriptor;
  int connectOutput = 0;

  for (PortIndex = 0; PortIndex < PluginDescriptor->PortCount; PortIndex++) {
    PortDescriptor = PluginDescriptor->PortDescriptors[PortIndex];
    if (LADSPA_IS_PORT_OUTPUT(PortDescriptor) && LADSPA_IS_PORT_AUDIO(PortDescriptor)) {
      PluginDescriptor->connect_port(PluginHandle, PortIndex, buffer[connectOutput++]);
      nPluginOutputs++;
      break;
    }
  }
}

bool LADSPA_PLUGIN::findPlugin(const string label,
			       const string pcFullFilename, 
			       void * pvPluginHandle) 
{

  const LADSPA_Descriptor * psDescriptor;
  long lIndex;
  for (lIndex = 0; (psDescriptor = fDescriptorFunction(lIndex)) != NULL; lIndex++) {
    if(label.compare(psDescriptor->Label) == 0) {
      fprintf(stderr,"Found LADSPA plugin.\n");
      fprintf(stderr,"%s:\n", pcFullFilename.c_str());
      fprintf(stderr,"\t%s (%lu/%s)\n", psDescriptor->Name, psDescriptor->UniqueID, psDescriptor->Label);
      pvPluginLibrary = dlopen(pcFullFilename.c_str(), RTLD_NOW);
      PluginDescriptor = psDescriptor;
      PluginHandle = PluginDescriptor->instantiate(PluginDescriptor, sfconf->sampling_rate);
      return true;
    }
  }
  return false;
}

bool LADSPA_PLUGIN::LADSPADirectoryPluginSearch(string pcDirectory, const string label) 
{
  string pcFilename;
  DIR * psDirectory;
  long lDirLength;
  long iNeedSlash;
  struct dirent *psDirectoryEntry;
  void * pvPluginHandle;

  lDirLength = pcDirectory.size();
  if (!lDirLength) {
    return false;
  }
  if (pcDirectory[lDirLength - 1] == '/') {
    iNeedSlash = 0;
  }
  else {
    iNeedSlash = 1;
  }

  psDirectory = opendir(pcDirectory.c_str());
  if (!psDirectory) {
    return false;
  }

  while (1) {

    psDirectoryEntry = readdir(psDirectory);
    if (!psDirectoryEntry) {
      closedir(psDirectory);
      return false;
    }

   
    pcFilename = pcDirectory;
    if (iNeedSlash) {
      pcFilename.append("/");
    }
    pcFilename.append(psDirectoryEntry->d_name);
    
    pvPluginHandle = dlopen(pcFilename.c_str(), RTLD_LAZY);
    if (pvPluginHandle) {
      /* This is a file and the file is a shared library! */
      dlerror();
      fDescriptorFunction = (LADSPA_Descriptor_Function)dlsym(pvPluginHandle,"ladspa_descriptor");
      if (dlerror() == NULL && fDescriptorFunction) {
	/* We've successfully found a ladspa_descriptor function. Pass
           it to the callback function. */
	if (findPlugin(label, pcFilename, pvPluginHandle)) {
	  return true;
	}
      } else {
	dlclose((void*)pcFilename.c_str());
      }
    }
  }
  return false;
}

int LADSPA_PLUGIN::LADSPAPluginSearch(const string label) {

  string pcBuffer;
  string pcLADSPAPath;
  char *envpath;
  size_t pcEnd;
  size_t pcStart = 0;

  envpath = getenv("LADSPA_PATH");
  if (envpath == NULL) {
    fprintf(stderr,"Warning: You do not have a LADSPA_PATH environment variable set.\n");
    pcBuffer = "/usr/lib/ladspa";
    if (LADSPADirectoryPluginSearch(pcBuffer, label)) {
      fprintf(stderr,"Found LADSPA plugin:%s",label.data());
      return 0;
    }
  } else {
    pcLADSPAPath = envpath;
    do {
      pcEnd = pcLADSPAPath.find(":");
      if (pcEnd < 0)
	pcEnd = pcLADSPAPath.size();
      pcBuffer = pcLADSPAPath.substr(pcStart, pcEnd - pcStart);
      if (LADSPADirectoryPluginSearch(pcBuffer, label)) {
	fprintf(stderr,"Found LADSPA plugin:%s",label.data());
	return 0;
      }
      pcStart = pcEnd;
      pcLADSPAPath = pcLADSPAPath.substr(pcEnd, pcLADSPAPath.size() - pcEnd);
    } while(pcEnd > 0);
  }
  fprintf(stderr,"ERROR: LADSPA plugin not found:%s",label.data());
  return -1;
}

LADSPA_PLUGIN::LADSPA_PLUGIN(struct sfconf *_sfconf,
			     struct intercomm_area *_icomm,
			     SfAccess *_sfaccess)
{
  sfconf = _sfconf;
  icomm = _icomm;
  sfaccess = _sfaccess;

  debug = false;
  nPluginControls = 0;
  nPluginOutputs = 0;
  nPluginInputs = 0;
  nOutputs = 0;
  PluginControls = new LADSPA_Control [LADSPA_CONTROLS_MAX]();
  pthread_mutex_init(&mutex, NULL);
}

int LADSPA_PLUGIN::preinit(xmlNodePtr sfparams, bool _debug)
{

  xmlNodePtr children;
  string port_name;
  uint32_t bitset, bitset2;
  LADSPA_Data port_value = 0;

  debug = _debug;
  bitset = 0;
  
  while (sfparams != NULL) {
    if (!xmlStrcmp(sfparams->name, (const xmlChar *)"label")) {
      field_repeat_test(&bitset, "label", 0);
      LADSPALabel = reinterpret_cast<char*>(xmlNodeGetContent(sfparams));
      LADSPAPluginSearch(LADSPALabel);
      analysePlugin();
    } else if (!xmlStrcmp(sfparams->name, (const xmlChar *)"input")) {
      field_repeat_test(&bitset, "io", 1);
      InputChannel = reinterpret_cast<char*>(xmlNodeGetContent(sfparams));
    } else if (!xmlStrcmp(sfparams->name, (const xmlChar *)"output")) {
      field_repeat_test(&bitset, "io", 1);
      OutputChannel.push_back(reinterpret_cast<char*>(xmlNodeGetContent(sfparams)));
    } else if (!xmlStrcmp(sfparams->name, (const xmlChar *)"port")) {
      bitset2 = 0;
      children = sfparams->children;
      while(children != NULL) {
	if (!xmlStrcmp(children->name, (const xmlChar *)"name")) {
	  field_repeat_test(&bitset2, "port:name", 0);
	  port_name = reinterpret_cast<char*>(xmlNodeGetContent(children));
	} else if (!xmlStrcmp(children->name, (const xmlChar *)"value")) {
	  field_repeat_test(&bitset2, "port:value", 1);
	  fprintf(stderr, "children:%s %f\n",xmlNodeGetContent(children), strtof((char*)xmlNodeGetContent(children), NULL));
	  port_value = (LADSPA_Data) strtof((char*)xmlNodeGetContent(children), NULL);
	}
	children = children->next;
      } 
      fprintf(stderr, "Setting Control Port %s: %e\n",port_name.c_str(), port_value);
      ConnectControlPort(port_name, port_value);
    }
    sfparams = sfparams->next;
  }
  return 0;
}

int LADSPA_PLUGIN::init(void)
{
  int n;
  
  BufferIN = new LADSPA_Data [2 * sfconf->filter_length];
  BufferOUT.resize(nPluginOutputs);
  for(n = 0; n < nPluginOutputs; n++)
    BufferOUT[n]= new LADSPA_Data [2 * sfconf->filter_length];
  ConnectInputPort(BufferIN);
  ConnectOutputPort(BufferOUT);
  processedOutputs = 0;
  if (PluginDescriptor->activate != NULL)
    PluginDescriptor->activate(PluginHandle);

  /*if (sem_post(synch_sem)== -1) {
    fprintf(stderr, "LADSPA: sem failed.\n");
    return -1;
    }*/
  return 0;
}

int LADSPA_PLUGIN::setLabel(string label)
{
  LADSPALabel = label;
  LADSPAPluginSearch(LADSPALabel);
  analysePlugin();
  return 0;
}

void LADSPA_PLUGIN::setInput(string _InputChannel)
{
  pthread_mutex_lock(&mutex);
  InputChannel = _InputChannel;
  pthread_mutex_unlock(&mutex);
}

void LADSPA_PLUGIN::addOutput(string _OutputChannel)
{
  pthread_mutex_lock(&mutex);
  OutputChannel.push_back(_OutputChannel);
  pthread_mutex_unlock(&mutex);
}

int LADSPA_PLUGIN::setPortValue(string port_name, float port_value)
{
  pthread_mutex_lock(&mutex);
  fprintf(stderr, "ConnectControlPort:%s %e\n",port_name.c_str(), port_value);
  ConnectControlPort(port_name, port_value);
  pthread_mutex_unlock(&mutex);
  return 0;
}

void LADSPA_PLUGIN::output_timed(void *buffer, int channel) 
{
  unsigned long n;

  pthread_mutex_lock(&mutex);
  memset(BufferIN, 0, sizeof(LADSPA_Data) * 2 * sfconf->filter_length);
  for(n = 0; n < OutputChannel.size(); n++) {
    if(sfconf->channels[SF_OUT][channel].name.compare(OutputChannel[n]) == 0) {
      if (processedOutputs == 0) {
	if (nPluginInputs > 0)
	  memcpy(BufferIN, (LADSPA_Data*)buffer, sfconf->filter_length*sizeof(LADSPA_Data));
	PluginDescriptor->run(PluginHandle, sfconf->filter_length);
	processedOutputs = ( processedOutputs == 6 ? 0 : processedOutputs++);
      }
      if (nPluginOutputs > 0) {
	memcpy((LADSPA_Data*)buffer, BufferOUT[n], sfconf->filter_length*sizeof(LADSPA_Data));
      }
    }
  }
  pthread_mutex_unlock(&mutex);
  return;
}

void LADSPA_PLUGIN::input_timed(void *buffer, int channel) 
{
  if(InputChannel.empty())
    return;

  pthread_mutex_lock(&mutex);
  memset(BufferIN, 0, sizeof(LADSPA_Data) * 2 * sfconf->filter_length);
  if(sfconf->channels[SF_IN][channel].name.compare(InputChannel) == 0) {
    if (nPluginInputs > 0) {
      memcpy(&BufferIN[0], &((LADSPA_Data*)buffer)[sfconf->filter_length], sfconf->filter_length*sizeof(LADSPA_Data));
      memcpy(&BufferIN[sfconf->filter_length], &((LADSPA_Data*)buffer)[0], sfconf->filter_length*sizeof(LADSPA_Data));
    }
    PluginDescriptor->run(PluginHandle, 2 * sfconf->filter_length);
    if (nPluginOutputs > 0) {
      memcpy(&((LADSPA_Data*)buffer)[sfconf->filter_length], &BufferOUT[0][0], sfconf->filter_length*sizeof(LADSPA_Data));
      memcpy(&((LADSPA_Data*)buffer)[0], &BufferOUT[0][sfconf->filter_length], sfconf->filter_length*sizeof(LADSPA_Data));
    }
  }
  pthread_mutex_unlock(&mutex);
  return;
}

SFLOGIC_LADSPA::SFLOGIC_LADSPA(struct sfconf *_sfconf,
			       struct intercomm_area *_icomm,
			       SfAccess *_sfaccess) : SFLOGIC(_sfconf, _icomm, _sfaccess) 
{ 
  name = "ladspa";
  debug = false;
  fork_mode = SF_FORK_DONT_FORK;
  
  memset(msg, 0, MAX_MSG_LEN);
};

int SFLOGIC_LADSPA::preinit(xmlNodePtr sfparams, int _debug)
{
  debug = !!_debug;

  while (sfparams != NULL) {
    if (!xmlStrcmp(sfparams->name, (const xmlChar *)"plugin")) {
      plugins.push_back(new LADSPA_PLUGIN(sfconf,icomm, sfaccess));
      plugins.back()->preinit(sfparams->children, debug);			
    }
    sfparams = sfparams->next;
  }
  return 0;
}

int SFLOGIC_LADSPA::init(int event_fd, sem_t* _synch_sem)
{
  unsigned int i;
  for(i = 0; i < plugins.size(); i++) {
    if( plugins[i]->init()==-1) {
      return -1;
    }
  }
  return 0;
}

int SFLOGIC_LADSPA::addPlugin(LADSPA_PLUGIN* plugin)
{
  plugins.push_back(plugin);
  return 0;
}

/*int SFLOGIC_LADSPA::command(const char params[])
{
  unsigned int i;
  for(i = 0; i < plugins.size(); i++) {
    plugins[i]->command(params);
  }
  return 0;
}

const char *SFLOGIC_LADSPA::message(void)
{
  return msg;
  }*/

void SFLOGIC_LADSPA::output_timed(void *buffer, int channel) 
{
  unsigned int i;

  for(i = 0; i < plugins.size(); i++) {
    plugins[i]->output_timed(buffer, channel);
  }
}

void SFLOGIC_LADSPA::input_timed(void *buffer, int channel) 
{
  unsigned int i;

  for(i = 0; i < plugins.size(); i++) {
    plugins[i]->input_timed(buffer, channel);
  }
}
