/*
 * (c) Copyright 2013 -- Raul Fernandez Ortega
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */

#ifndef _EMALLOC_H_
#define _EMALLOC_H_

#ifdef ASMLIB
#include "asmlib.h"
  #define MEMCPY A_memcpy
  #define MEMSET A_memset
  #define STRSTR A_strstr
  #define STRCMP A_strcmp
#else
  #define MEMCPY memcpy
  #define MEMSET memset
  #define STRSTR strstr
  #define STRCMP strcmp
#endif


#ifdef __cplusplus
 extern "C" {
#endif

#include <stdlib.h>

#include "defs.h"
   
   void *emallocaligned(size_t size);
   
   void *emalloc(size_t size);
   
   void *erealloc(void *p, size_t size);
   
   char *estrdup(const char str[]);
   
   void emalloc_set_exit_function(void (*exit_function)(int), int status);

   void efree(void *p);

#ifdef __cplusplus
 }
#endif 
#endif
