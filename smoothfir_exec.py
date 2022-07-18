# *
# * (c) Copyright 2013/2022 -- Raul Fernandez Ortega
# *
# * This program is open source. For license terms, see the LICENSE file.
# *
# *

#! /usr/bin/env python

import sys
import time
import smoothfir

class recplay:

    def __init__(self, smoothfirExec):
        self.convolver = smoothfir.getConvolver()
        self.module = smoothfir.SFLOGIC_RECPLAY(self.convolver.sfConf.sfconf, self.convolver.sfConf.icomm, self.convolver.sfRun)

    def addRECPLAY(self, filename, isInput, logicChannel, isRec):
        manager = smoothfir.SndFileManager(self.convolver.sfConf.sfconf, self.convolver.sfConf.icomm, self.convolver.sfRun)
        manager.config(filename, isInput, logicChannel, isRec)
        self.module.addManager(manager)
        return manager

    def addPlay(self, filename, isInput, logicChannel):
        return self.addRECPLAY(self, filename, isInput, logicChannel, False)

    def addRec(self, filename, isInput, logicChannel):
        return self.addRECPLAY(self, filename, isInput, logicChannel, True)
                
    def endConfig(self):
        self.convolver.sfConf.add_sflogic(self.module)


class smoothfir_exec:
    
    def __init__(self, xml_config_file):
        # smoothfir convolver object creation
        self.convolver = smoothfir.smoothfir()
        # loading smoothfir config 
        self.convolver.sfConf.sfconf_init(xml_config_file)
        # link to smoothfir configuration data
        self.config = self.convolver.sfConf.sfconf
        # smoothfir agent object for realtime commands 
        self.agent = smoothfir.SFLOGIC_PY(self.convolver.sfConf.sfconf, self.convolver.sfConf.icomm, self.convolver.sfRun)
        self.convolver.sfConf.add_sflogic(self.agent)
        
    def start(self):
        self.convolver.sfRun.sfrun(self.convolver.sfConf.sfConv, self.convolver.sfConf.sfDelay, self.convolver.sfConf.sflogic)
       
    def stop(self):
        self.convolver.sfRun.sfstop()

    def getConvolver(self):
        return self.convolver
        
def main(configFile):
    print "Smoothfir object creation."
    smoothfirRun = smoothfir_exec(configFile)
    print "Starting smoothfir." 
    smoothfirRun.start()
    print "Smoothfir is running..."
    DT = time.time()

    while True:
        time.sleep(10)

if __name__ == "__main__":
    main(sys.argv[1])

