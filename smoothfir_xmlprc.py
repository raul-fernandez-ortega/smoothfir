# *
# * (c) Copyright 2013/2022 -- Raul Fernandez Ortega
# *
# * This program is open source. For license terms, see the LICENSE file.
# *
# *

import sys
import os
import time
import smoothfir
import thread
import SocketServer
from SimpleXMLRPCServer import SimpleXMLRPCServer,SimpleXMLRPCRequestHandler

class smoothfir_exec (SimpleXMLRPCServer):

    def __init__(self, configFile, port):

        self.convolver = smoothfir.smoothfir()
        self.count = 1
        if os.path.isfile(configFile):
            self.convolver.sfConf.sfconf_init(configFile)
            self.config = self.convolver.sfConf.sfconf
            self.race = smoothfir.SFLOGIC_RACE(self.convolver.sfConf.sfconf, self.convolver.sfConf.icomm, self.convolver.sfRun)
            self.race.set_config('race direct', 'race cross', 5.0, 5, 200., 12., 6000., 24.)
            self.convolver.sfConf.add_sflogic(self.race)
            self.agent =  smoothfir.SFLOGIC_PY(self.convolver.sfConf.sfconf, self.convolver.sfConf.icomm, self.convolver.sfRun)
            self.convolver.sfConf.add_sflogic(self.agent)
            if port != 0:
                SimpleXMLRPCServer.__init__(self,('', port))
                self.register_function(self.Hello)
        else:
            return False

    def Hello(self):
        print "Hello",self.count
        self.count += 1
        return True

    def start(self):
        print "Starting smoothfir." 
        self.convolver.sfRun.sfrun(self.convolver.sfConf.sfConv, self.convolver.sfConf.sfDelay, self.convolver.sfConf.sflogic)
        print "Smoothfir is running..."
        
    def XMLRPCstart(self):
        self.serve_forever()

    def _dispatch(self, method, params):
         """Extend dispatch, adding client info to some parameters."""
         return SimpleXMLRPCServer._dispatch(self, method, params);

    
def main(configFile):
    DT = time.time()
    smoothfirRun =  smoothfir_exec(configFile, 8000)
    smoothfirRun.start()
    smoothfirRun.XMLRPCstart()
    while True:
        time.sleep(30)
        print "Running time: %d s."%(time.time()-DT)

if __name__ == "__main__":
    main(sys.argv[1])
