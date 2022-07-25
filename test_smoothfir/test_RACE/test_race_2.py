import smoothfir
import time

convolver = smoothfir.smoothfir()
xml_config_file="/home/drcop/Measurements/config_DRCOP_1.xml"
convolver.sfConf.sfconf_init(xml_config_file)
config = convolver.sfConf.sfconf
race = smoothfir.SFLOGIC_RACE(convolver.sfConf.sfconf, convolver.sfConf.icomm, convolver.sfRun)
race.set_config('race direct', 'race cross', 5.0, 5, 400., 6., 14000., 12.)
convolver.sfConf.add_sflogic(race)
agent = smoothfir.SFLOGIC_PY(convolver.sfConf.sfconf,convolver.sfConf.icomm,convolver.sfConf.sfRun)
convolver.sfConf.add_sflogic(agent)
convolver.sfRun.sfrun(convolver.sfConf.sfConv, convolver.sfConf.sfDelay, convolver.sfConf.sflogic)
time.sleep(4)
race.change_config(4.0, 5, 400., 6., 14000., 12.)

