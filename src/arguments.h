#ifndef arguments_h_
#define arguments_h_

#include <argtable2.h>
#include "config.h"

/** 
* Hands the commandline arguments to argtable2. 
* Parses the initial arguments like help, version and config file..
* 
* @param argc number of arguments
* @param argv the array of strings of the arguments
* 
* @return zero of no error was encountered; one if there was.
*/
int arg_parseprimairy(int argc, char **argv);

/** 
* Parses the secondary arguments. This should be called after the defaults have
* been set and the config file has been read.
* 
* @return zero on success; one if an error was encountered.
*/
int arg_parsesecondary();

#endif /* arguments_h_ */
