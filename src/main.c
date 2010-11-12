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

struct config_options options;
     
void sigfunc()
{
    debug_printf("Received signal\n");
    options.running = false;
}

void irc_server_connect(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) 
{
    int retval = 0;

    irc_send_raw_msg("login bot bone", "userserv");

	verbose_printf("joining channel: %s\n", options.channel);
	retval = irc_cmd_join(session, options.channel, options.channelpassword);
	if (retval != 0) error("%d: %s\n", retval, irc_strerror(irc_errno(session) ) );
}

void irc_mode_callback(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) 
{
    options.connected = true;
    verbose_printf("irc joined channel\n");
}

void irc_channel_callback(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count) 
{
    if ( (options.mode & output) > 0)
    {
        if (count >= 2)
        {
            if (options.showchannel && options.shownick)
            {
                printf("%s - %s: %s\n", params[0], origin, params[1]);
            }
            else if (options.showchannel)
            {
                printf("%s - %s\n", params[0], params[1]);
            }
            else if (options.shownick)
            {
                printf("%s: %s\n", origin, params[1]);
            }
            else printf("%s\n", params[1]);
        }
    }
}

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

int prog_main()
{
    char buff[300];
    int stdin_fd = STDIN_FILENO;
    int maxfd = stdin_fd;
    fd_set readset;
    fd_set writeset;
    struct timeval tv;
    irc_callbacks_t *callbacks = get_callback();

    callbacks->event_connect = irc_server_connect;
    callbacks->event_channel = irc_channel_callback;
    callbacks->event_mode    = irc_mode_callback;

    tv.tv_sec = 10;
    tv.tv_usec = 0;

    verbose_printf("starting main loop\n");

    if (create_irc_session() != 1)
    {
        debug_printf("irc connection setup has failed\n");
    }

    verbose_printf("starting loop\n");

    int counter = 0;
    while (options.running)
    {
        int result = 0;

        FD_ZERO(&readset);
        FD_ZERO(&writeset);
        if ( ( (options.mode & input) > 0) && (options.connected) ) FD_SET(stdin_fd, &readset);

        add_irc_descriptors(&readset, &writeset, &maxfd);
        result = select(maxfd +1, &readset, &writeset, NULL, &tv);

        if (result == 0)
        {
        } 
        else if (result < 0)
        {
            error("we've got an error on select\n");
        }
        else 
        {
            process_irc(&readset, &writeset);
            if (FD_ISSET(stdin_fd, &readset) )
            {
                result = sgets(stdin_fd, buff, sizeof(buff) );

                if (result == 0) options.running = false;
                else if (result > 0)
                {
                    if (options.verbose) printf(".");
                    irc_send_raw_msg(buff, options.channel);
                }
            }
        }
        usleep(100);
    }

    return close_irc_session();
}

int main(int argc, char **argv)
{
    struct sigaction setmask;
    int exitcode = 0;

    memset(&options, 0, sizeof(options) );

    /*initialise signal handler*/
    sigemptyset( &setmask.sa_mask );
    setmask.sa_handler = sigfunc;
    setmask.sa_flags   = 0;
    sigaction( SIGHUP,  &setmask, (struct sigaction *) NULL );      /* Hangup */
    sigaction( SIGINT,  &setmask, (struct sigaction *) NULL );      /* Interrupt (Ctrl-C) */

/*---------------- Configuration code -----------------*/
    /*set options to defaults*/
    setdefaults(&options);
    options.running = true;

    /*parse command line and set primairy options*/
    if (options.running)
    {
        if ( (exitcode = arg_parseprimairy(argc, argv) ) != 0)
        {
            debug_printf("parsing secondaries failed\n");
        }
    }

    /*parse (updated) config file*/
    if (options.running)
    {
        if ( (exitcode = read_config_file(&options) ) != 0)
        {
            //verbose_printf("config file: %s is not parsed succesfully\n", options.configfile);
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
            debug_printf("parsing secondaries failed\n");
        }
    }
/*---------------- Configuration code end--------------*/
    {
        const char *temp[] = {"none", "input", "output", "both"};

        /*give warnings about experimental options*/
        verbose_printf("mode is set to '%s'\n", temp[options.mode]);
    }

    /*let's fire it up*/
    if (options.running)
    {
        /* normal case: take the command line options at face value */
        exitcode = prog_main();
    }

    /*exitting gracefully*/
    debug_printf("exiting with: %d\n", exitcode);
    return exitcode;
}
