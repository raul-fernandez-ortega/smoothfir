# smoothfir
Smoothfir is a fork of Brutefir, a convolution device developed for digital audio processing that it's used as python module. \New features are included such as ambiophonics and LADSPA plugin machine.

## Install

As usual for python modules:

python setup.py build    ----> As user for compiling code.\
python setup.py install ----> As root for install python module smoothfir.

Libraries needed:

- fftw3     https://www.fftw.org/
- fftw3f    https://www.fftw.org/
- xml2      https://cran.r-project.org/web/packages/xml2/index.html
- sndfile   https://github.com/libsndfile/libsndfile
- jack      https://jackaudio.org/

## Usage

There is a sample scripy in python for running smoothfir (run_DRCOP.py) using an input xml config file (config sample config_DRCOP_1.xml):

python run_DRCoP.py config_DRCOP_1.xml

