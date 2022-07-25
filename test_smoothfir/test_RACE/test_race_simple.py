#! /usr/bin/env python

import sys
import time
import smoothfir
import thread

IN = 0
OUT = 1

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
    print "Loading smoothfit config." 
    cv.sfConf.sfconf_init(configFile)
    # link to smoothfir configuration data
    config = cv.sfConf.sfconf
    print "Configuring RACE coeffs..."
    race = smoothfir.SFLOGIC_RACE(cv.sfConf.sfconf, cv.sfConf.icomm, cv.sfRun)
    # RACE parameters:
    # Cross impulse attenuation: 5.0 dB
    # Cross impulse delay: 5 samples (5./44.1 ms)
    # Band filter from 200 Hz (12 dB/octave) to 6000 Hz (24 dB/octave)
    race.set_config('race direct', 'race cross', 5.0, 5, 200., 12., 6000., 24.)
    cv.sfConf.add_sflogic(race)
    # smoothfir agent object for realtime commands 
    agent = smoothfir.SFLOGIC_PY(cv.sfConf.sfconf, cv.sfConf.icomm, cv.sfRun)
    cv.sfConf.add_sflogic(agent)
    print "Starting smoothfir." 
    cv.sfRun.sfrun(cv.sfConf.sfConv, cv.sfConf.sfDelay, cv.sfConf.sflogic)
    print "Smoothfir is running..."

    print_filter_data(agent, config)
    DT = time.time()

    ACTIVE = True
    while True:
        # Every 10 seconds RACE effect is switch on/switch off
        time.sleep(10)
        if ACTIVE == True:
            print "RACE Inactive"
            agent.change_filter_attenuation('race left cross', IN, 0, 90)
            agent.change_filter_attenuation('race right cross', IN, 0, 90)
            agent.change_filter_coeff('race left direct', -1)
            agent.change_filter_coeff('race right direct', -1)
            ACTIVE = False
        else:
            print "RACE active"
            agent.change_filter_attenuation('race left cross', IN, 0, 0)
            agent.change_filter_attenuation('race right cross', IN, 0, 0)
            agent.change_filter_coeff('race left direct', 0)
            agent.change_filter_coeff('race right direct', 0)
            ACTIVE = True
        print_filter_data(agent, config)
        print "Running time: %d s."%(time.time()-DT)

if __name__ == "__main__":
    main(sys.argv[1])

