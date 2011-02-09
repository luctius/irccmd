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
};
     
/** 
* A simple signal replacement for SIGINT and SIGHUP.
* when called, this will gracefully end the program.
*/
void sigfunc()
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
void irc_server_connect(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) 
{
    int retval = 0;
    int counter = 0;

    irc_send_raw_msg("login bot bone", "userserv");
    verbose("connected to server\n");

    for (counter = 0; counter < options.no_channels; counter++)
    {
	    verbose("joining channel: %s\n", options.channels[counter]);
    	retval = irc_cmd_join(session, options.channels[counter], options.channelpasswords[counter]);
        if (retval != 0) error("%d: %s\n", retval, irc_strerror(irc_errno(session) ) );
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
void irc_mode_callback(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) 
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
void irc_channel_callback(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) 
{
    if ( (options.mode & output) > 0)
    {
        if (count >= 2)
        {
            char nick[100];
            irc_target_get_nick(origin, nick, sizeof(nick) -1);
            if (options.showchannel && options.shownick)
            {
                nsilent("%s - %s: %s\n", params[0], nick, params[1]);
            }
            else if (options.showchannel)
            {
                nsilent("%s - %s\n", params[0], params[1]);
            }
            else if (options.shownick)
            {
                nsilent("%s: %s\n", nick, params[1]);
            }
            else nsilent("%s\n", params[1]);
        }
    }
    fflush(stdout);
}

/** 
* reads a line from a file
* 
* @param fd file descriptor of the file to read
* @param line pointer to the storage where the read line should reside
* @param size the maximum size of the given storage
* 
* @return the number of characters read
*/
size_t sgets(int fd, char *line, size_t size)
{
    size_t i;
    for ( i = 0; i < size - 1; ++i )
    {
        char ch = 0;
        if (read(fd, &ch, sizeof(ch) ) == 0) ch = EOF;
        if ( ch == '\n' || ch == EOF )
        {
            break;
        }
        line[i] = ch;
    }
    line[i] = '\0';

    return i;
}

bool send_irc_message(char *msg)
{
    bool retval = false;
    int channel_id = 0;
    char channel[MAX_CHANNELS_NAMELEN];
    char *msg_start = msg;
    char *channel_start = msg;

    debug("send_irc-message\n");
    if (msg != NULL)
    {
        //check if the first non-white-space character is a '#'
        channel_start = strchr(msg, '#');
        if (channel_start != NULL)
        {
            msg_start = strchr(channel_start, ' ');

            if (msg_start != NULL)
            {
                *msg_start = '\0';
                msg_start++;
            }
        }

        if (msg_start != NULL)
        {
            debug("msg: %s\n", msg_start);

            //if so, find the channel in the known channels
            if (channel_start != NULL)
            {
                int len = msg_start - channel_start;

                debug("channel: %s\n", channel_start);
                while (strncmp(options.channels[channel_id], channel_start, len) != 0)
                {
                    channel_id++;
                    if (channel_id >= options.no_channels)
                    {
                        verbose("channel %s not found, defaulting to %s\n", channel_start, options.channels[0]);
                        channel_id = 0;
                        break;
                    }
                }
            }

            //send the message to the correct channel
            memset(channel, 0, sizeof(channel));
            strncpy(channel, options.channels[channel_id], sizeof(channel) );
            debug("sending: %s to channel %s\n", msg_start, channel);
            irc_send_raw_msg(msg_start, channel);
            verbose(".");
        }
        retval = true;
    }
    return retval;
}

/** 
* Main application loop.
* 
* @return the succes or failure of closing the irc connection
*/
int prog_main()
{
    int counter = 0;
    char buff[300];
    int stdin_fd = STDIN_FILENO;
    int maxfd = stdin_fd;
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
                debug("setting stdin in select readset\n");
                FD_SET(stdin_fd, &readset);
            }
            else debug("not setting stdin; waiting for channel join\n");
        }

        add_irc_descriptors(&readset, &writeset, &maxfd);
        result = select(maxfd +1, &readset, &writeset, NULL, &tv);

        debug("post select\n");

        if (result == 0)
        {
            debug("select result 0\n");
        } 
        else if (result < 0)
        {
            if (options.running) error("error on select\n");
        }
        else 
        {
            process_irc(&readset, &writeset);
            if (FD_ISSET(stdin_fd, &readset) )
            {
                debug("stdin is set\n");
                memset(buff, 0, sizeof(buff) );
                result = sgets(stdin_fd, buff, sizeof(buff) );

                if (result == 0)
                {
//                    options.running = false;
//                    error("error in fetching message from stdin\n");
                }
                else if (result > 0)
                {
                    debug("received message %s\n", buff);
                    if (options.running)
                    {
                        if( (options.running = send_irc_message(buff) ) == false)
                        {
                            error("failed to parse message\n");
                        }
                    }
                }
            }
        }
        usleep(100);

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
            debug("parsing primairies failed\n");
        }
    }

    /*parse (updated) config file*/
    if (options.running)
    {
        if ( (exitcode = read_config_file("/etc/irccmd.cnf") ) != 0)
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
        /* normal case: take the command line options at face value */
        exitcode = prog_main();
    }

    /*exitting gracefully*/
    verbose("exiting with: %d\n", exitcode);
    return exitcode;
}

