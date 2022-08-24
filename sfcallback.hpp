/*
 * (c) Copyright 2013/2022 -- Raul Fernandez Ortega
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
#ifndef _SFCALLBACK_HPP_
#define _SFCALLBACK_HPP_

class SfCallback {

public:

  SfCallback(void) {};

  virtual ~SfCallback(void) {};

  virtual void sf_callback_ready(int io) { return; };
  
  virtual void rti_and_overflow(void) {return; };

};

#endif

