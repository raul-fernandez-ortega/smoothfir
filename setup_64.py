# Distutils installer for smoothfir.
# Smoothfir is compiled using asmlib. See http://agner.org/optimize/ and http://agner.org/optimize/asmlib.zip
# for details.
 

from distutils.core import setup, Extension
from distutils import sysconfig
from os import system

save_init_posix = sysconfig._init_posix
C_OPTS = '-fPIC -Wall -O3'

def my_init_posix():
    print 'my_init_posix: changing gcc to g++'
    save_init_posix()
    g = sysconfig._config_vars
    g['CC'] = 'g++'
    g['LDSHARED'] = 'g++ -shared -Xlinker -export-dynamic'
    g['CFLAGS']='-m32 -fno-strict-aliasing -fpermissive -DNDEBUG -03 -Wall'
    g['OPT']= C_OPTS

   
sysconfig._init_posix = my_init_posix

def pybrutefir_precompile():
    compile_list = [
        "gcc -fPIC -o convolver_xmm.o -c -I/usr/local/include -Wall -O3 convolver_xmm.c",
        "g++ -fPIC -fno-strict-aliasing -Wall -O3 -I. -c fftw_convolver.cpp -o fftw_convolver.o -I/usr/include/libxml2",
        "gcc -fPIC -o firwindow.o -c -I/usr/local/include -Wall "
        "-Wpointer-arith -Wshadow -Wcast-qual -Wcast-align -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes "
        "-Wmissing-declarations -Wnested-externs  -O3 firwindow.c",
        "gcc -fPIC -o emalloc.o -c -I/usr/local/include -Wall "
        "-Wpointer-arith -Wshadow -Wcast-qual -Wcast-align -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes "
        "-Wmissing-declarations -Wnested-externs  -O3 emalloc.c",
        "gcc -fPIC -o shmalloc.o -c -I/usr/local/include -Wall "
        "-Wpointer-arith -Wshadow -Wcast-qual -Wcast-align -Wwrite-strings -Wstrict-prototypes -Wmissing-prototypes "
        "-Wmissing-declarations -Wnested-externs  -O3 shmalloc.c",
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
                             include_dirs=["."],
                             libraries=["fftw3", "fftw3f", "m", "dl", "xml2", "sndfile", "jack"],
                             extra_objects=[ "emalloc.o", "convolver_xmm.o", "fftw_convolver.o", "shmalloc.o", "firwindow.o"]
                             
                             )]
    )
