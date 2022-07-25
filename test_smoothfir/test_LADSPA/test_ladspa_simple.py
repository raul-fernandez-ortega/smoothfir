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
    print "Configuring LADSPA plugins..."
    ladspa =  smoothfir.SFLOGIC_LADSPA(cv.sfConf.sfconf, cv.sfConf.icomm, cv.sfRun)
    # Butterworth low filter for left input channel
    # Cut off frequency: 300 Hz
    # Resonance: 0.5
    plugin1 = smoothfir.LADSPA_PLUGIN(config, cv.sfConf.icomm, cv.sfRun)
    plugin1.setLabel("buttlow_iir")
    plugin1.setInput("left")
    plugin1.setPortValue("Cutoff Frequency (Hz)",300.)
    plugin1.setPortValue("Resonance",0.1)
    ladspa.addPlugin(plugin1)
    # Butterworth high filter for right output channel
    # Cut off frequency: 300 Hz
    # Resonance: 0.5
    plugin2 = smoothfir.LADSPA_PLUGIN(config, cv.sfConf.icomm, cv.sfRun)
    plugin2.setLabel("butthigh_iir")
    plugin2.addOutput("right")
    plugin2.setPortValue("Cutoff Frequency (Hz)",300.)
    plugin2.setPortValue("Resonance",0.1)
    ladspa.addPlugin(plugin2)
    # Inverse for right output channel
    # No config parameters
    plugin3 = smoothfir.LADSPA_PLUGIN(config, cv.sfConf.icomm, cv.sfRun)
    plugin3.setLabel("inv")
    plugin3.addOutput("right")
    ladspa.addPlugin(plugin3)
    # add ladspa logic to bflogic smoothfir module
    cv.sfConf.add_sflogic(ladspa)
    # smoothfir agent object for realtime commands 
    agent = smoothfir.SFLOGIC_PY(cv.sfConf.sfconf, cv.sfConf.icomm, cv.sfRun)
    cv.sfConf.add_sflogic(agent)
    print "Starting smoothfir." 
    cv.sfRun.sfrun(cv.sfConf.sfConv, cv.sfConf.sfDelay, cv.sfConf.sflogic)
    print "Smoothfir is running..."

    print_filter_data(agent, config)
    DT = time.time()

    Resonance = 0.1
    while True:
        Resonance += 0.1
        if Resonance > 1.4:
            print "Exiting..."
            sys.exit()
        # Every 20 seconds Butterworth filters resonance is increased by 0.1
        time.sleep(20)
        print "Change resonance parameter for both LADSPA plugins..."
        plugin1.setPortValue("Resonance",Resonance)
        plugin2.setPortValue("Resonance",Resonance)
        print "Running since: %d s ago..."%(time.time()-DT)


if __name__ == "__main__":
    main(sys.argv[1])

