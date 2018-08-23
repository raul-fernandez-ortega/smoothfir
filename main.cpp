/*
 * (c) Copyright 2001 - 2006, 2009 -- Anders Torger
 *
 * This program is open source. For license terms, see the LICENSE file.
 *
 */
extern "C" {

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

}

#include "brutefir.hpp"

#define PRESENTATION_STRING \
"\n\
SmoothFIR v0.1 (March 2012)                                   \
(c) Raul Fernandez Ortega\n\
\n"

#define USAGE_STRING \
"Usage: %s [-quiet] [-nodefault] [-daemon] [configuration file]\n"

int
main(int argc,
     char *argv[])
{
    char *config_filename = NULL;
    bool quiet = false;
    bool nodefault = false;
    bool run_as_daemon = false;
    int n;

    Brutefir *brutefir;

    if (sizeof(float) != 4) {
        fprintf(stderr, "Unexpected sizeof(float): %d.\n", (int)sizeof(float));
        exit(BF_EXIT_OTHER);
    }
    if (sizeof(double) != 8) {
        fprintf(stderr, "Unexpected sizeof(double): %d.\n",(int)sizeof(double));
        exit(BF_EXIT_OTHER);
    }
    
    for (n = 1; n < argc; n++) {
	if (strcmp(argv[n], "-quiet") == 0) {
	    quiet = true;
	} else if (strcmp(argv[n], "-nodefault") == 0) {
            nodefault = true;
	} else if (strcmp(argv[n], "-daemon") == 0) {
            run_as_daemon = true;
	} else {
	    if (config_filename != NULL) {
		break;
	    }
	    config_filename = argv[n];
	}
    }
    if (n != argc) {
	fprintf(stderr, PRESENTATION_STRING);
	fprintf(stderr, USAGE_STRING, argv[0]);
	return BF_EXIT_INVALID_CONFIG;
    }
    
    if (!quiet) {
	fprintf(stderr, PRESENTATION_STRING);
    }
    
    //emalloc_set_exit_function(bf_exit, BF_EXIT_NO_MEMORY);
    brutefir = new Brutefir();

    brutefir->bfConf->bfconf_init(config_filename);

    /* start! */
    brutefir->bfRun->bfrun(brutefir->bfConf->bfConv, brutefir->bfConf->bfDelay, brutefir->bfConf->bflogic);
    brutefir->bfRun->post_start();

    fprintf(stderr, "Could not start filtering.\n");
    exit(BF_EXIT_OTHER);
    return BF_EXIT_OTHER;
}
