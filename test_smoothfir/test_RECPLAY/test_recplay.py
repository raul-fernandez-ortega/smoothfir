#! /usr/bin/env python

import time
import sys
import smoothfir

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
    print "Configuring REC/PLAY logic..."
    rec =  smoothfir.SFLOGIC_RECPLAY(cv.sfConf.sfconf, cv.sfConf.icomm, cv.sfRun)
    managerREC = smoothfir.SndFileManager(cv.sfConf.sfconf, cv.sfConf.icomm, cv.sfRun)
    managerREC.config('./rec_sweep.wav', True, 'left', True)
    managerPLAY = smoothfir.SndFileManager(cv.sfConf.sfconf, cv.sfConf.icomm, cv.sfRun)
    managerPLAY.config('./sweep.wav', True, 'left', False)
    rec.addManager(managerREC)
    rec.addManager(managerPLAY)
    cv.sfConf.add_sflogic(rec)
    
    cv.sfRun.sfrun(cv.sfConf.sfConv, cv.sfConf.sfDelay, cv.sfConf.sflogic)
    
    print "Sleeping 10 sec."
    time.sleep(10)
    print "RECORDING SWEEP...."
    managerREC.start(7)
    managerPLAY.start(7)
    time.sleep(7)
    print "END OF RECORD"
    time.sleep(2)
    cv.sfRun.sf_stop()
    print "STOP"

    #b.bfRun.bfrun(b.bfConf.bfConv, b.bfConf.bfDelay, b.bfConf.bflogic)
    #while True:
        #time.sleep(3)
        #print "OK"

if __name__ == "__main__":
    main(sys.argv[1])
