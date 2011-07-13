#ifndef main_h_
#define main_h_

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#define MAX_CHANNELS (20)
#define MAX_CHANNELS_NAMELEN (20)
#define MAX_SERVER_NAMELEN (20)
#define MAX_BOT_NAMELEN (9)
#define MAX_PASSWD_LEN (20)
#define MAX_PATH_LEN (100)

#define OUTPUT_TIME_DIV 10000

#define verbose(...) { if (options.verbose) { printf("v[%lu]- ", (time(NULL) % OUTPUT_TIME_DIV) ); printf(__VA_ARGS__); (void) fflush(stdout); } }
#define debug(...) { if (options.debug) { printf("d[%lu]- ", (time(NULL) % OUTPUT_TIME_DIV) ); printf(__VA_ARGS__); (void) fflush(stdout); } }
#define nsilent(...) { if (options.silent == false) { printf(__VA_ARGS__); (void) fflush(stdout); } }
#define error(...) { fprintf(stderr, "e[%lu]- ", (time(NULL) % OUTPUT_TIME_DIV) ); fprintf(stderr, "" __VA_ARGS__); (void) fflush(stderr); }
#define warning(...) { if (options.silent == false) { fprintf(stderr, "w[%lu]- ", (time(NULL) % OUTPUT_TIME_DIV) ); fprintf(stderr,__VA_ARGS__); (void) fflush(stderr); } }


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

    char configfile[100];
    enum modes mode;
    bool verbose;
    bool debug;
    bool silent;

    bool interactive;    
    bool keepreading;    

    bool showchannel;
    bool shownick;
    bool showjoins;
    int maxlines;

    int port;
    int no_channels;
    char botname[MAX_BOT_NAMELEN];
    char server[MAX_SERVER_NAMELEN];
    char serverpassword[MAX_PASSWD_LEN];
    char channels[MAX_CHANNELS][MAX_CHANNELS_NAMELEN];
    char channelpasswords[MAX_CHANNELS][MAX_PASSWD_LEN];

    bool enableplugins;
    int no_pluginpaths;
    int no_plugins;
    char pluginpaths[MAX_CHANNELS][MAX_PATH_LEN];     /* Size does not relate to nr of channels */
    char plugins[MAX_CHANNELS][MAX_CHANNELS_NAMELEN];   /* Size does not relate to nr of channels */

    int botname_nr;
    int current_channel_id;
    time_t connection_timeout;

    uint64_t ping_count;

    int output_flood_timeout;
};

extern struct config_options options;

#endif /* main_h_ */

