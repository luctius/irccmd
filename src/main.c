#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "config.h"
#include "arguments.h"
#include "main.h"
#include "configdefaults.h"
#include "ircmod.h"
#include "input.h"

/** 
* This is the config structure where all the important configuration options are located.
* This structure is used globally.
*/
struct config_options options =
{
    .connected        = false,
    .silent           = CONFIG_SILENT,            /**< If this member is set, the aplpication should only output irc output or errors. */ 
    .debug            = CONFIG_DEBUG,             /**< If this is set, debug output will be send to stdout */ 
    .verbose          = CONFIG_VERBOSE,           /**< if this is set, messages which would not be interresting to normal users can be shown */ 
    .configfile       = CONFIG_FILE,              /**< This will define where to look for the config file */ 

    .interactive      = true,
    .showchannel      = CONFIG_SHOWCHANNEL,       /**< This will enable showing of the channel in the irc output */ 
    .shownick         = CONFIG_SHOWNICK,          /**< This will enable showing of the nickname in the irc output */ 

    .mode             = CONFIG_MODE,              /**< this will define the mode of the application */ 
    .port             = CONFIG_PORT,              /**< this will hold the port which should be used to connect to the irc server */ 
    .server           = CONFIG_SERVER,            /**< this will hold the server url or ip which should be used to connect to the irc server */ 
    .serverpassword   = CONFIG_SERVERPASSWORD,    /**< this will hold the password neccesary to connect to the irc server; this can be empty */ 
    .channels         = {CONFIG_CHANNEL},         /**< this will hold the channel name, including '#' the bot would like to  join */ 
    .channelpasswords = {CONFIG_CHANNELPASSWORD}, /**< this will hold the password neccesary to join the channel; this can be empty */ 
    .no_channels      = 1,                        /**< this will hold the number of channels the bot would like to join */
    .botname          = CONFIG_BOTNAME,           /**< this will hold the bot nick name and should be a unique identifier */ 
    .botname_nr       = 1,

    .current_channel_id  = 0,
};
     
/** 
* A simple signal replacement for SIGINT and SIGHUP.
* when called, this will gracefully end the program.
*/
static void sigfunc()
{
    debug("Received signal\n");
    options.running = false;
}

/** 
* This callback is called when the irc connection with the server is established.
* We will do things here like joining a channel and logging in at userserv
* 
* @param session This will provide the irc session
* @param event This contains what kind of event the callback triggers
* @param origin Contains the sender of this event
* @param params The parameters of the event, can be zero.
* @param count The number of parameters.
*/
static void irc_server_connect(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) 
{
    int counter = 0;

    irc_send_raw_msg("login bot bone", "userserv");
    verbose("connected to server\n");

    for (counter = 0; counter < options.no_channels; counter++)
    {
    	join_irc_channel(options.channels[counter], options.channelpasswords[counter]);
    }
}

/** 
* This callback is called when we have joined a channel.
* We will notify the configuration structure that we are active now and ready to process input and output.
* 
* @param session This will provide the irc session
* @param event This contains what kind of event the callback triggers
* @param origin Contains the sender of this event
* @param params The parameters of the event, can be zero.
* @param count The number of parameters.
*/
static void irc_mode_callback(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) 
{
    options.connected = true;
    verbose("joined channel %s\n", params[0]);
}

/** 
* This callback is called when there is activity on the channel.
* We will print out the activity, including channel name and sender when the config struct asks for it.
* 
* @param session This will provide the irc session
* @param event This contains what kind of event the callback triggers
* @param origin Contains the sender of this event
* @param params The parameters of the event, can be zero.
* @param count The number of parameters.
*/
static void irc_channel_callback(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) 
{
    if ( (options.mode & output) > 0)
    {
        if (count >= 2)
        {
            char nick[100];
            irc_target_get_nick(origin, nick, sizeof(nick) -1);
            if (options.interactive)
            {
                if (strncmp(params[0], options.channels[options.current_channel_id], strlen(options.channels[options.current_channel_id]) ) == 0)
                {
                    printf("%s@%s: %s\n", nick, params[0], params[1]);
                }
            }
            else if (options.showchannel && options.shownick)
            {
                printf("%s - %s: %s\n", params[0], nick, params[1]);
            }
            else if (options.showchannel)
            {
                printf("%s - %s\n", params[0], params[1]);
            }
            else if (options.shownick)
            {
                printf("%s: %s\n", nick, params[1]);
            }
            else printf("%s\n", params[1]);
        }
    }
    fflush(stdout);
}

/** 
* Main application loop.
* 
* @return the succes or failure of closing the irc connection
*/
static int prog_main()
{
    int maxfd = STDIN_FILENO;
    fd_set readset;
    fd_set writeset;
    struct timeval tv;
    irc_callbacks_t *callbacks = get_callback();

    callbacks->event_connect = irc_server_connect;
    callbacks->event_channel = irc_channel_callback;
	callbacks->event_join    = irc_mode_callback;

    tv.tv_sec = 2;
    tv.tv_usec = 0;

    debug("starting main loop\n");

    if (create_irc_session() != 1)
    {
        error("irc connection setup has failed\n");
        options.running = false;
    }

    debug("starting loop\n");

    while (options.running)
    {
        int result = 0;

        tv.tv_sec = 10;
        tv.tv_usec = 0;

        FD_ZERO(&readset);
        FD_ZERO(&writeset);

        if ( (options.mode & input) > 0)
        {
            if (options.connected)
            {
                FD_SET(STDIN_FILENO, &readset);
            }
            else debug("not setting stdin; waiting for channel join\n");
        }

        add_irc_descriptors(&readset, &writeset, &maxfd);
        result = select(maxfd +1, &readset, &writeset, NULL, &tv);

        if (result == 0)
        {
        } 
        else if (result < 0)
        {
            if (options.running) error("error on select\n");
        }
        else 
        {
            process_irc(&readset, &writeset);
            if (FD_ISSET(STDIN_FILENO, &readset) )
            {
                process_input();
            }
        }
//        usleep(100);

        /*counter++;
        if (counter > 1000) options.running = false;*/
    }

    return close_irc_session();
}

/** 
* This is the entry point of the application
* We will process the commandline, configuration file here and the start the main loop.
*
* Processing the commandline argumants will be done in two steps. First we will do the simple
* things like help and version. This will also include the configuration file should the user 
* point us towards a non-default file.
*
* When the first part is done, the config file is read. Afterwards, the secondary commandline 
* arguments are parsed which will override the config file. This ensures that both are read 
* and that the commandline has priority.
* 
* When the main loop is done, we will close gracefully.
*
* @param argc The number of commandline arguments
* @param argv the string array with commandline arguments.
* 
* @return returns succes or failure of the main application loop
*/
int main(int argc, char **argv)
{
    struct sigaction setmask;
    int exitcode = 0;

    /*initialise signal handler*/
    sigemptyset( &setmask.sa_mask );
    setmask.sa_handler = sigfunc;
    setmask.sa_flags   = 0;
    sigaction( SIGHUP,  &setmask, (struct sigaction *) NULL );      /* Hangup */
    sigaction( SIGINT,  &setmask, (struct sigaction *) NULL );      /* Interrupt (Ctrl-C) */

/*---------------- Configuration code -----------------*/
    /*set options to defaults*/
    options.running = true;

    /*parse command line and set primairy options*/
    if (options.running)
    {
        if ( (exitcode = arg_parseprimairy(argc, argv) ) != 0)
        {
            exitcode = 1;
            options.running = false;
            debug("parsing primairies failed\n");
        }
    }

    /*parse (updated) config file*/
    if (options.running)
    {
        if ( (exitcode = read_config_file(SYSTEM_CONFIG_FILE) ) != 0)
        {
        }
        if ( (exitcode = read_config_file(options.configfile) ) != 0)
        {
        }
    }

    {
        if (options.silent)
        {
            options.silent  = true;
            options.verbose = false;
            options.debug   = false;
        }
        else if (options.debug)
        {
            options.silent  = false;
            options.verbose = true;
            options.debug   = true;
        }
        else if (options.verbose)
        {
            options.verbose = true;
            options.silent  = false;
            options.debug   = false;
        }
    }

    /*parse secondary commandline arguments to overrule config settings*/
    if (options.running)
    {
        /* normal case: take the command line options at face value */
        if (arg_parsesecondary() != 0)
        {
            exitcode = 1;
            options.running = false;
            debug("parsing secondaries failed\n");
        }
    }
/*---------------- Configuration code end--------------*/
    {
        /*give warnings about experimental options*/
        const char *temp[] = {"none", "input", "output", "both"};
        verbose("mode is set to '%s'\n", temp[options.mode]);
    }


    /*let's fire it up*/
    if (options.running)
    {
        init_readline();
        exitcode = prog_main();
        deinit_readline();
    }


    /*exitting gracefully*/
    verbose("exiting with: %d\n", exitcode);

    printf("\n");
    return exitcode;
}

