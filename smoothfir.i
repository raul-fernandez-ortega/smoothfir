%module smoothfir

%{
#include "sfclasses.hpp"
#include "sfstruct.h"
#include "sfconf.hpp"
#include "sflogic_py.hpp"
#include "sflogic_recplay.hpp"
#include "sflogic_ladspa.hpp"
#include "sflogic_race.hpp"
#include "sflogic_eq.hpp"
#include "sfdai.hpp"
#include "sfrun.hpp"
#include "smoothfir.hpp"
#include "iojack.hpp"
%}

// Grab a Python function object as a Python object.
%typemap(in) PyObject *pyfunc {
  if (!PyCallable_Check($input)) {
      PyErr_SetString(PyExc_TypeError, "Need a callable object!");
      return NULL;
  }
  $1 = $input;
}

%typemap(in) char ** {
  /* Check if is a list */
  if (PyList_Check($input)) {
    int size = PyList_Size($input);
    int i = 0;
    $1 = (char **) malloc((size+1)*sizeof(char *));
    for (i = 0; i < size; i++) {
      PyObject *o = PyList_GetItem($input,i);
      if (PyString_Check(o))
        $1[i] = PyString_AsString(PyList_GetItem($input,i));
      else {
        PyErr_SetString(PyExc_TypeError,"list must contain strings");
        free($1);
        return NULL;
      }
    }
    $1[i] = 0;
  } else {
    PyErr_SetString(PyExc_TypeError,"not a list");
    return NULL;
  }
}

%typemap(out) char ** {
  int len,i;
  len = 0;
  if ($1==NULL) {
    $result = PyList_New(len);	
  }
  else  {
    while ($1[len]) len++;
    $result = PyList_New(len);
    for (i = 0; i < len; i++) {
      PyList_SetItem($result,i,PyString_FromString($1[i]));
    }
  }
}

%include "std_vector.i"
%include "std_string.i"

%include "sfclasses.hpp"
%include "sfstruct.h"
%include "sfconf.hpp"
%include "sfcallback.hpp"
%include "sfaccess.hpp"
%include "io.hpp"
%include "sflogic.hpp"
%include "sflogic_py.hpp"
%include "sflogic_recplay.hpp"
%include "sflogic_ladspa.hpp"
%include "sflogic_race.hpp"
%include "sflogic_eq.hpp"
%include "sfdai.hpp"
%include "sfrun.hpp"
%include "smoothfir.hpp"
%include "iojack.hpp"

namespace std {
    %template(intVector) vector<int>;
    %template(doubleVector) vector<double>;
    %template(sfchannelVector) vector<sfchannel>;
    %template(sfcoeffVector) vector<sfcoeff>;			
    %template(sffilterVector) vector<sffilter>;
    %template(sffilter_controlVector) vector<sffilter_control>;
    %template(sfoverflowVector) vector<sfoverflow>;
    %template(SFLOGICVector) vector<SFLOGIC*>;	
    %template(SndFileManagerVector) vector<SndFileManager*>;
}


	 
%pythoncode %{

class smoothfir_exec:
    
    def __init__(self, xml_config_file):
        # smoothfir convolver object creation
        self.convolver = smoothfir()
        # loading smoothfir config 
        self.convolver.sfConf.sfconf_init(xml_config_file)
        # link to smoothfir configuration data
        self.config = self.convolver.sfConf.sfconf
        # smoothfir agent object for realtime commands 
        self.agent = SFLOGIC_PY(self.convolver.sfConf.sfconf, self.convolver.sfConf.icomm, self.convolver.sfRun)
        self.convolver.sfConf.add_sflogic(self.agent)
        
    def start(self):
        self.convolver.sfRun.sfrun(self.convolver.sfConf.sfConv, self.convolver.sfConf.sfDelay, self.convolver.sfConf.sflogic)
       
    def stop(self):
        self.convolver.sfRun.sfstop()

    def getConvolver(self):
        return self.convolver
	  
%}
