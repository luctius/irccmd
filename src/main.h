#ifndef main_h_
#define main_h_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_CHANNELS (20)
#define MAX_CHANNELS_NAMELEN (20)
#define MAX_SERVER_NAMELEN (20)
#define MAX_BOT_NAMELEN (8)
#define MAX_PASSWD_LEN (20)

#define verbose(...) { if (options.verbose) { printf("v- " __VA_ARGS__); fflush(stdout); } }
#define debug(...) { if (options.debug) { printf("d- " __VA_ARGS__); fflush(stdout); } }
#define nsilent(...) { if (options.silent == false) { printf(__VA_ARGS__); fflush(stdout); } }
#define error(...) { fprintf(stderr, "e- " __VA_ARGS__); fflush(stderr); }
#define warning(...) { if (options.silent == false) { fprintf(stderr, "WARNING: " __VA_ARGS__); fflush(stderr); } }


/** 
* This enum is used to determine in which mode the application is running.
*this is ued by the application to turn off or on several functions.
*/
enum modes
{
    none       = 0,
    input      = 1,
    output     = 2,
    both       = 3,
};

/** 
* This struct contains the application specific settings.
*/
struct config_options
{
    bool running;
    bool connected;

    const char *configfile;
    enum modes mode;
    bool verbose;
    bool debug;
    bool silent;

    bool interactive;    

    bool showchannel;
    bool shownick;

    int port;
    int no_channels;
    char botname[MAX_BOT_NAMELEN];
    char server[MAX_SERVER_NAMELEN];
    char serverpassword[MAX_PASSWD_LEN];
    char channels[MAX_CHANNELS][MAX_CHANNELS_NAMELEN];
    char channelpasswords[MAX_CHANNELS][MAX_PASSWD_LEN];

    int botname_nr;
};

extern struct config_options options;

#endif /* main_h_ */

