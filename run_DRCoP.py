#! /usr/bin/env python

import sys
import time
import os
import smoothfir
import thread

IN = 0
OUT = 1

def print_filter_output_att(agent, config):
    outch = agent.getChannels(OUT)
    filters = agent.getFilterData()
    for n in range(config.n_filters):
        print "Filter number:",n
        bfilter = filters[n] 
        print "Name:",bfilter.name
        fcontrol = agent.getFilterControl(n)
        outp_index = 0
        print "Outputs:", agent.getFilternChannels(bfilter,1)
        for outp in agent.getFilterChannels(bfilter,1):
            print outp, outch[outp].name
            print "Output attenuation: %.1f dB."%agent.get_filter_attenuation(bfilter.name, OUT, outp_index)
            outp_index +=1

def print_filter_data(agent, config):
    inch = agent.getChannels(IN)
    outch = agent.getChannels(OUT)
    filters = agent.getFilterData()
    for n in range(config.n_filters):
        print "Filter number:",n
        bfilter = filters[n] 
        print "Name:",bfilter.name
        fcontrol = agent.getFilterControl(n)
        if fcontrol.coeff > -1:
            print "Coeff. Index:",fcontrol.coeff, "Name:", config.coeffs[fcontrol.coeff].name
        else:
            print "No Coeff Index."
        print "Inputs:",agent.getFilternChannels(bfilter,0)
        inp_index = 0
        for inp in agent.getFilterChannels(bfilter,0):
            print inp, inch[inp].name
            print "Input attenuation: %.1f dB."%agent.get_filter_attenuation(bfilter.name, IN, inp_index)
            inp_index +=1
        print "Outputs:", agent.getFilternChannels(bfilter,1)
        outp_index = 0
        for outp in agent.getFilterChannels(bfilter,1):
            print outp, outch[outp].name
            print "Output attenuation: %.1f dB."%agent.get_filter_attenuation(bfilter.name, OUT, outp_index)
            outp_index +=1
        print "Filter Inputs:",agent.getFilternFilters(bfilter,0)
        for finp in agent.getFilterFilters(bfilter,0):
            print finp, filters[finp].name
        print "Filter Outputs:",agent.getFilternFilters(bfilter,1)
        for foutp in agent.getFilterFilters(bfilter,1):
            print foutp, filters[foutp].name      
        print "=============================================="

class smoothfir_exec:
    
    def __init__(self, xml_config_file):
        print "Smoothfir object creation."
        self.convolver = smoothfir.smoothfir()
        print "Loading smoothfir config." 
        self.convolver.sfConf.sfconf_init(xml_config_file)
        # link to smoothfir configuration data
        self.config = self.convolver.sfConf.sfconf
        # smoothfir agent object for realtime commands 
        self.agent = smoothfir.SFLOGIC_PY(self.convolver.sfConf.sfconf, self.convolver.sfConf.icomm, self.convolver.sfRun)
        self.convolver.sfConf.add_sflogic(self.agent)
        
    def start(self):
        print "Starting smoothfir." 
        self.convolver.sfRun.sfrun(self.convolver.sfConf.sfConv, self.convolver.sfConf.sfDelay, self.convolver.sfConf.sflogic)
        print "Smoothfir is running..."
        
    def print_data(self):
        print_filter_data(self.agent, self.config)

    def print_output_att(self):
        print_filter_output_att(self.agent, self.config)

def main(configFile):
    smoothfirRun = smoothfir_exec(configFile)
    smoothfirRun.start()
    print "Smoothfir is running..."
    smoothfirRun.print_data()
    DT = time.time()

    while True:
        time.sleep(30)
        #smoothfirRun.print_output_att()
        #agent.change_input_attenuation('left',2.0)
        #agent.change_input_attenuation('right',3.0)
        print "Running time: %d s."%(time.time()-DT)

if __name__ == "__main__":
    main(sys.argv[1])

