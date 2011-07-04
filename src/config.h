#ifndef config_h_
#define config_h_

#include <stdbool.h>
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "main.h"

/** 
* Opens the configfile as set in options.
* Then evaluates the aqcuired Lua State for the possible
* settings and puts them into the options structure. When
* an setting is not found; the default is set.
*
* When no file is found or the file was not valid 
* 
* @return 0 on success or 1 on failure.
*/
int read_config_file(const char *path);
char *execute_str_plugins(char *string);

#endif /* config_h_ */
