# Distutils installer for smoothfir.
# Smoothfir is compiled using asmlib. See http://agner.org/optimize/ and http://agner.org/optimize/asmlib.zip
# for details.
 

from distutils.core import setup, Extension
from distutils import sysconfig
from os import system

save_init_posix = sysconfig._init_posix

def my_init_posix():
    print 'my_init_posix: changing gcc to g++'
    save_init_posix()
    g = sysconfig._config_vars
    g['CC'] = 'g++'
    g['LDSHARED'] = 'g++ -shared -Xlinker -export-dynamic'
    g['CFLAGS']='-fno-strict-aliasing -fpermissive -DNDEBUG -O2 -Wall'
    g['OPT']='-DASMLIB -fno-pic -Wall -g' 

   
sysconfig._init_posix = my_init_posix

def pybrutefir_precompile():
    compile_list = [
        "g++ -fno-strict-aliasing -DASMLIB -Wall -O2 -I. -c fftw_convolver.cpp -o fftw_convolver.o -I/usr/include/libxml2",
        "gcc -o firwindow.o -c -I/usr/local/include -Wall -Wlong-long "
        "-Wpointer-arith -Wshadow -Wcast-qual -Wcast-align -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes "
        "-Wmissing-declarations -Wnested-externs  -O2 firwindow.c",
        "gcc -o emalloc.o -c -I/usr/local/include -Wall -Wlong-long "
        "-Wpointer-arith -Wshadow -Wcast-qual -Wcast-align -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes "
        "-Wmissing-declarations -Wnested-externs  -O2 emalloc.c",
        "gcc -o shmalloc.o -c -I/usr/local/include -Wall -Wlong-long "
        "-Wpointer-arith -Wshadow -Wcast-qual -Wcast-align -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes "
        "-Wmissing-declarations -Wnested-externs  -O2 shmalloc.c",
        "as -o convolver_sse2.o                  convolver_sse2.s",
        "as -o convolver_sse.o                   convolver_sse.s",
        "as -o convolver_3dnow.o                 convolver_3dnow.s",
        "as -o convolver_x87.o                   convolver_x87.s",
        ]
    for line in compile_list:
        print line
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
    ext_modules = [Extension("_smoothfir", [  "smoothfir.i", "sflogic_py.cpp", "sflogic_cli.cpp", "sflogic_ladspa.cpp", \
                                                  "sflogic_eq.cpp", "sflogic_race.cpp", "sflogic_recplay.cpp", "sfrun.cpp", \
                                                  "sfconf.cpp", "sfdai.cpp", "iojack.cpp", "smoothfir.cpp",   "sfdelay.cpp", "sfdither.cpp" ],
                             extra_compile_args=['-I/usr/include/libxml2'],
                             include_dirs=["."], #, "gsl"],
                             libraries=["fftw3", "fftw3f", "m", "dl", "xml2", "sndfile", "jack", "aelf32op"],
                             extra_objects=[ "emalloc.o", "fftw_convolver.o", \
                                                "convolver_sse.o", "convolver_sse2.o", "convolver_3dnow.o", \
                                                "convolver_x87.o", "shmalloc.o", \
                                                "firwindow.o"]
                             
                             )]
    )
