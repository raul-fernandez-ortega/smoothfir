/*
 * (c) Copyright 2013 -- Raul Fernandez Ortega
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */

#include "smoothfir.hpp"

smoothfir::smoothfir(void)
{
  sfconf = new struct sfconf();
  icomm = new struct intercomm_area();
  sfRun = new SfRun(sfconf, icomm);
  sfConf = new SfConf(sfRun);
}

smoothfir::~smoothfir(void)
{
  delete sfConf;
  delete sfRun;
  delete icomm;
  delete sfconf;
}
