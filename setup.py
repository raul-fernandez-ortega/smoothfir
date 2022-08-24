# *
# * (c) Copyright 2013/2022 -- Raul Fernandez Ortega
# *
# * This program is open source. For license terms, see the LICENSE file.
# *
# *

# Distutils installer for smoothfir.
 
from distutils.core import setup, Extension
from distutils import sysconfig
from os import system

save_init_posix = sysconfig._init_posix
C_OPTS = '-fPIC -O3'

def my_init_posix():
    print('my_init_posix: changing gcc to g++')
    save_init_posix()
    g = sysconfig._config_vars
    g['CC'] = 'g++'
    g['LDSHARED'] = 'g++ -shared -Xlinker -export-dynamic -Wundef'
    g['CFLAGS']='-m32 -O3 -fno-strict-aliasing -fpermissive -DNDEBUG -Wall -Wno-class-memaccess -Wno-sequence-point'
    g['OPT']= C_OPTS

sysconfig._init_posix = my_init_posix

def pybrutefir_precompile():
    compile_list = [
        #"g++ -o fftw_convolver.o -c -I/usr/local/include -I/usr/include/libxml2 -Wall -Wno-maybe-uninitialized -Wpointer-arith -Wshadow -Wcast-align -Wwrite-strings -O3 -msse fftw_convolver.cpp",
        "gcc -o firwindow.o -c -I/usr/local/include -Wall -Wno-maybe-uninitialized -Wpointer-arith -Wshadow -Wcast-align -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -O3 -msse firwindow.c",
        "gcc -o emalloc.o -c -I/usr/local/include -Wall -Wno-maybe-uninitialized -Wpointer-arith -Wshadow -Wcast-align -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -O3 -msse emalloc.c",
        "gcc -o shmalloc.o -c -I/usr/local/include -Wall -Wno-maybe-uninitialized -Wpointer-arith -Wshadow -Wcast-align -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -O3 -msse shmalloc.c",
        "gcc -o convolver_xmm.o -c -I/usr/local/include -Wall -Wno-maybe-uninitialized -Wpointer-arith -Wshadow -Wcast-align -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations -Wnested-externs -O3 -msse convolver_xmm.c"
        ]
    for line in compile_list:
        print(line)
        system(line)

pybrutefir_precompile()
setup(
    name = "smoothfir",
    version = "0.1",
    description = "Python bindings for brutefir convolver",
    author = "Raul Fernandez",
    author_email = "raul00fernandez@gmail.com",
    url = "http://www.matrixhifi.com",
    options={'build_ext':{'swig_opts':'-c++'}},
    py_modules = ['smoothfir'],
    ext_modules = [Extension("_smoothfir", [  "smoothfir.i", "fftw_convolver.cpp", "sflogic_py.cpp", "sflogic_cli.cpp", "sflogic_ladspa.cpp", \
                                                  "sflogic_eq.cpp", "sflogic_race.cpp", "sflogic_recplay.cpp", "sfrun.cpp", \
                                                  "sfconf.cpp", "sfdai.cpp", "iojack.cpp", "smoothfir.cpp", "sfdelay.cpp", "sfdither.cpp" ],
                             extra_compile_args=['-I/usr/include/libxml2'],
                             include_dirs=["."],
                             libraries=["fftw3", "fftw3f", "xml2", "sndfile", "jack"],
                             extra_objects=[ "emalloc.o", "shmalloc.o", "firwindow.o", "convolver_xmm.o"]
                             
                             )]
    )
