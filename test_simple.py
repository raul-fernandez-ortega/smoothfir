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

def main(configFile):
    print "Smoothfir object creation."
    cv = smoothfir.smoothfir()
    print "Loading smoothfir config." 
    cv.sfConf.sfconf_init(configFile)
    # link to smoothfir configuration data
    config = cv.sfConf.sfconf
    # smoothfir agent object for realtime commands 
    agent = smoothfir.SFLOGIC_PY(cv.sfConf.sfconf, cv.sfConf.icomm, cv.sfRun)
    cv.sfConf.add_sflogic(agent)
    print "Starting smoothfir." 
    cv.sfRun.sfrun(cv.sfConf.sfConv, cv.sfConf.sfDelay, cv.sfConf.sflogic)
    print "Smoothfir is running..."
    print_filter_data(agent, config)
    DT = time.time()

    while True:
        time.sleep(10)
        print_filter_output_att(agent, config)
        #agent.change_input_attenuation('left',2.0)
        #agent.change_input_attenuation('right',3.0)
        print "Running time: %d s."%(time.time()-DT)

if __name__ == "__main__":
    main(sys.argv[1])

