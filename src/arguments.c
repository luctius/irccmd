#include <string.h>

#include "arguments.h"
#include "configdefaults.h"

struct arg_file *config;
struct arg_str  *mode;
struct arg_int  *port;
struct arg_str  *server;
struct arg_str  *channel;
struct arg_str  *botname;
struct arg_str  *serverpassword;
struct arg_lit  *noninteractive;
struct arg_lit  *keepreading;
struct arg_lit  *showchannel;
struct arg_lit  *shownick;
struct arg_lit  *showjoins;
struct arg_int  *lines;
struct arg_int  *timeout;
struct arg_rem  *remark1;

struct arg_lit  *silent;
struct arg_lit  *verbose;
struct arg_lit  *debug;
struct arg_lit  *help;
struct arg_lit  *version;
struct arg_end  *end;
void *argtable[40];

/** 
* Frees the argtable memory allocations.
*/
static void arg_clean()
{
    /* deallocate each non-null entry in argtable[] */
    arg_freetable(argtable,sizeof(argtable)/sizeof(argtable[0]));
}

int arg_parseprimairy(int argc, char **argv)
{
    int exitcode = 0;
    int nerrors = 0;

    help            = arg_lit0("h"  , "help"            , "print this help and exit");
    verbose         = arg_lit0("v"  , "verbose"         , "verbose messaging");
    debug           = arg_lit0("d"  , "debug"           , "enables debug messages, implies -v");
    silent          = arg_lit0("s"  , "silent"          , "program will only output errors");
    config          = arg_file0("c" , "config"          , CONFIG_FILE                  , "override default config file");
    version         = arg_lit0("V"  , "version"         , "print version information and exit");
    remark1         = arg_rem("", "");


    mode            = arg_str0("m"  , "mode"            , "in/out/both"                , "set the mode, input, output or both");
    port            = arg_int0("p"  , "port"            , XSTR(CONFIG_PORT)            , "set the port of the irc server");
    botname         = arg_str0("n"  , "name"            , CONFIG_BOTNAME               , "set the botname");
    timeout         = arg_int0("t"  , "timeout"         , XSTR(CONFIG_CONNECTION_TIMEOUT), "set the maximum timeout of the irc connection");
    lines           = arg_int0("l"  , "lines"           , "0"                          , "quit when the number of messages has exceeded <lines>. "
                                                                                         "Off when set to zero.");
    noninteractive  = arg_lit0("N"  , "noninteractive"                                 , "will force a non-interactive session");
    keepreading     = arg_lit0("K"  , "keepreading"                                    , "will stay in the channel after "
                                                                                         "the writing end of stdin has closed.");
    showchannel     = arg_lit0("H"  , "showchannel"                                    , "show channel when printing irc messages to stdout");
    shownick        = arg_lit0("N"  , "shownick"                                       , "show nick from sender when printing irc messages to stdout");
    showjoins       = arg_lit0("J"  , "showjoins"                                      , "show joins from the connected channels");
    server          = arg_str0("S"  , "server"          , CONFIG_SERVER                , "set the irc server");
    serverpassword  = arg_str0("P"  , "serverpassword"  , "<password>"                 , "set the password for the server");
    channel         = arg_strn("C"  , "channel"         , CONFIG_CHANNEL ":<password>" , 0, MAX_CHANNELS, 
                                                                                        "set an irc channel, can be applied multiple "
                                                                                        "times, each for a new channel. An optional "
                                                                                        "password can be supplied using a column (:) as seperator.");
    end             = arg_end(40);


    {
        int i = 0;
        argtable[i++] = help;
        argtable[i++] = verbose;
        argtable[i++] = debug;
        argtable[i++] = silent;
        argtable[i++] = config;
        argtable[i++] = version;
        argtable[i++] = remark1;

        argtable[i++] = mode;
        argtable[i++] = port;
        argtable[i++] = botname;
        argtable[i++] = timeout;
        argtable[i++] = lines;
        argtable[i++] = noninteractive;
        argtable[i++] = keepreading;
        argtable[i++] = showchannel;
        argtable[i++] = shownick;
        argtable[i++] = showjoins;
        argtable[i++] = server;
        argtable[i++] = serverpassword;
        argtable[i++] = channel;

        argtable[i++] = end;
    }

    if (arg_nullcheck(argtable) != 0)
    {
        /* NULL entries were detected, some allocations must have failed */
        error("%s: insufficient memory\n",PROG_STRING);
        exitcode = 1;
        options.running = false;
    }

    /* Parse the command line as defined by argtable[] */
    nerrors = arg_parse(argc,argv,argtable);

    /* special case: '--help' takes precedence over error reporting */
    if (help->count > 0)
    {
        if (options.running)
        {
            nsilent("Usage: %s", PROG_STRING);
            arg_print_syntaxv(stdout,argtable,"\n");
            nsilent("This is a test irc commandline client\n\n");
            arg_print_glossary(stdout,argtable,"  %-30s %s\n");
            exitcode = 0;
            options.running = false;
        }
    }

    /* special case: '--version' takes precedence error reporting */
    if (version->count > 0)
    {
        if (options.running)
        {
            nsilent("'%s' example program for the \"argtable\" command line argument parser.\n",PROG_STRING);
            nsilent("test version\n");
            exitcode = 0;
            options.running = false;
        }
    }

    if (silent->count > 0)
    {
        if (options.running)
        {
            options.silent  = true;
            verbose("silent on\n");
        }
    }

    if (verbose->count > 0)
    {
        if (options.running)
        {
            options.verbose = true;
            verbose("verbose on\n");
        }
    }
    
    if (debug->count > 0)
    {
        if (options.running)
        {
            options.debug   = true;
            options.verbose = true;
            verbose("debug on\n");
        }
    }

    if (config->count > 0)
    {
        if (options.running)
        {
            strncpy(options.configfile, config->filename[0], 100);
            verbose("using %s as config file\n", options.configfile);
        }
    }

    /* If the parser returned any errors then display them and exit */
    if (nerrors > 0)
    {
        if (options.running)
        {
            /* Display the error details contained in the arg_end struct.*/
            arg_print_errors(stderr,end,PROG_STRING);
            nsilent("Try '%s --help' for more information.\n",PROG_STRING);
            exitcode = 1;
            options.running = false;
        }
    }

    return exitcode;
}

int arg_parsesecondary()
{
	int	exitcode = 0;

    if (silent->count > 0)
    {
        if (options.running)
        {
            options.silent  = true;
        }
    }

    if (verbose->count > 0)
    {
        if (options.running)
        {
            options.verbose = true;
        }
    }
    
    if (debug->count > 0)
    {
        if (options.running)
        {
            options.debug   = true;
            options.verbose = true;
        }
    }

    if (mode->count > 0)
    {
        if (options.running)
        {
			options.mode = none;
			if (strncmp(mode->sval[0], "in", 2) == 0)
			{
				options.mode = input;
				verbose("setting mode to input\n");
			}
			else if (strncmp(mode->sval[0], "out", 3) == 0)
			{
				options.mode = output;
				verbose("setting mode to output\n");
			}
			else if (strncmp(mode->sval[0], "both", 4) == 0)
			{
				options.mode = both;
				verbose("setting mode to both\n");
			}
			else
			{
				options.mode = both;
				exitcode = 1;
			}
        }
    }

	if (port->count > 0)
	{
        if (options.running)
        {
			options.port = port->ival[0];
			verbose("setting port to %d\n", port->ival[0]);
		}
	}
	
	if (server->count > 0)
	{
        if (options.running)
        {
            strncpy(options.server, server->sval[0], MAX_SERVER_NAMELEN);
			verbose("setting server to %s\n", server->sval[0]);
		}
	}
	
    if (serverpassword->count > 0)
	{
        if (options.running)
        {
            strncpy(options.serverpassword, server->sval[0], MAX_PASSWD_LEN);
			verbose("using different server password\n");
		}
	}

	if (botname->count > 0)
	{
        if (options.running)
        {
            strncpy(options.botname, botname->sval[0], MAX_BOT_NAMELEN -1);
            options.botname[MAX_BOT_NAMELEN -1] = '\0';
			verbose("setting name to %s\n", botname->sval[0]);
		}
	}
	
	if (channel->count > 0)
	{
        if (options.running)
        {
            int counter = 0;
            options.no_channels = channel->count;
            for (counter = 0; counter < options.no_channels; counter++)
            {
                char *passwd_start = NULL;
                strncpy(options.channels[counter], channel->sval[counter], MAX_CHANNELS_NAMELEN);
                passwd_start = strchr(options.channels[counter], ':');

                if (passwd_start != NULL)
                {
                    *passwd_start = '\0';
                    strncpy(options.channelpasswords[counter], passwd_start++, MAX_PASSWD_LEN);
                    verbose("changing password for channel %s\n", options.channels[counter]);
                }
                verbose("setting channel to %s\n", options.channels[counter]);
            }
            debug("number of channels to join: %d\n", options.no_channels);
		}
	}
	
    if (showchannel->count > 0)
    {
        if (options.running)
        {
            options.showchannel = true;
			verbose("messages from irc now contain the originating channel\n");
        }
    }

    if (shownick->count > 0)
    {
        if (options.running)
        {
            options.shownick = true;
			verbose("messages from irc now contain the originating nick\n");
        }
    }

    if (showjoins->count > 0)
    {
        if (options.running)
        {
            options.showjoins = true;
			verbose("joins will now be printed\n");
        }
    }

    if (noninteractive->count > 0)
    {
        if (options.running)
        {
            options.interactive = false;
            verbose("interactive mode off\n");
        }
    }

    if (keepreading->count > 0)
    {
        if (options.running)
        {
            options.keepreading = true;
            verbose("interactive mode off\n");
        }
    }

	if (timeout->count > 0)
	{
        if (options.running)
        {
			options.connection_timeout = timeout->ival[0];
			verbose("setting timeout to %d\n", timeout->ival[0]);
		}
	}

	if (lines->count > 0)
	{
        if (options.running)
        {
			options.maxlines = lines->ival[0];
			verbose("setting lines to %d\n", lines->ival[0]);
		}
	}
	
    arg_clean();

    return exitcode;
}

