import smoothfir

# DRCop XML configuration file. Including full path.
xml_config_file="/home/drcop/Measurements/config_DRCOP_1.xml"

# Configuring smoothfir convolver 
def configure(convolver, xml_config_file):
    convolver.sfConf.sfconf_init(xml_config_file)

#starting convolver 
def run(convolver):
    convolver.sfRun.sfrun(convolver.sfConf.sfConv, convolver.sfConf.sfDelay, convolver.sfConf.sflogic)

convolver = smoothfir.smoothfir()
configure(convolver, xml_config_file)
config = convolver.sfConf.sfconf
agent = smoothfir.SFLOGIC_PY(convolver.sfConf.sfconf,convolver.sfConf.icomm,convolver.sfConf.sfRun)
convolver.sfConf.add_sflogic(agent)
#run(convolver)
