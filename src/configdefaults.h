#ifndef configdefaults_h_
#define configdefaults_h_

#include "main.h"
#include "def.h"

#define STR(s) #s
#define XSTR(s) STR(s)
#define PROG_STRING PACKAGE

#define CONFIG_FILE     "~/.irccmd.conf"
#define CONFIG_VERBOSE  false
#define CONFIG_DEBUG    false
#define CONFIG_SILENT   false

#define CONFIG_SHOWCHANNEL  false
#define CONFIG_SHOWNICK     false

#define CONFIG_MODE     both
#define CONFIG_PORT     6667
#define CONFIG_SERVER   "irc.incas3.nl"
#define CONFIG_BOTNAME  "omega"
#define CONFIG_CHANNEL  "#spam"
#define CONFIG_SERVERPASSWORD ""
#define CONFIG_CHANNELPASSWORD ""

#endif /* configdefaults_h_ */
