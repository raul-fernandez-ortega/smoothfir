/*
 * (c) Copyright 2013/2022 -- Raul Fernandez Ortega
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#ifndef _SMOOTHFIR_HPP_
#define _SMOOTHFIR_HPP_

#include "sfclasses.hpp"
#include "sfconf.hpp"
#include "sfrun.hpp"
#include "sfdai.hpp"

class smoothfir {

public:

  struct sfconf *sfconf;
  intercomm_area* icomm;
  struct sfaccess *sfaccess;

  smoothfir(void);
  ~smoothfir(void);
  
  SfConf *sfConf;
  SfRun *sfRun;
};

#endif
