#include <string.h>

#include "arguments.h"
#include "configdefaults.h"

struct arg_file *config;
struct arg_str  *mode;
struct arg_int  *port;
struct arg_str  *server;
struct arg_str  *channel;
struct arg_str  *botname;
struct arg_str  *password;

struct arg_lit  *silent;
struct arg_lit  *verbose;
struct arg_lit  *debug;
struct arg_lit  *help;
struct arg_lit  *version;
struct arg_end  *end;
void *argtable[20];

static int arg_clean()
{
    /* deallocate each non-null entry in argtable[] */
    arg_freetable(argtable,sizeof(argtable)/sizeof(argtable[0]));
    return 0;
}

int arg_parseprimairy(int argc, char **argv)
{
    help     = arg_lit0(NULL , "help"    , "print this help and exit");
    version  = arg_lit0(NULL , "version" , "print version information and exit");
    verbose  = arg_lit0("v"  , "verbose" , "verbose messages");
    debug    = arg_lit0("d"  , "debug"   , "debug messages");
    silent   = arg_lit0("s"  , "silent"  , "program will only output errors");
    config   = arg_file0("c" , "config"  , CONFIG_FILE                            , "override default config file");
    end      = arg_end(20);

    port     = arg_int0("p"  , "port"    , "port" XSTR(CONFIG_PORT)               , "set the port of the irc server");
    mode     = arg_str0("m"  , "mode"    , "in/out/auto/both"                     , "set the mode, input, output, auto detect or both");
    server   = arg_str0(NULL , "server"  , CONFIG_SERVER                          , "set the irc server");
    channel  = arg_str0(NULL , "channel" , CONFIG_CHANNEL                         , "set the irc channel");
    botname  = arg_str0(NULL , "name"    , CONFIG_BOTNAME                         , "set the botname");
    password = arg_str0(NULL , "pasword" , "password"                             , "set the password");

    const char* progname = PROG_STRING;
    int exitcode = 0;
    int nerrors = 0;

    {
        int i = 0;
        argtable[i++] = help;
        argtable[i++] = version;
        argtable[i++] = verbose;
        argtable[i++] = debug;
        argtable[i++] = silent;
        argtable[i++] = config;

        argtable[i++] = mode;
        argtable[i++] = port;
        argtable[i++] = server;
        argtable[i++] = channel;
        argtable[i++] = botname;
        argtable[i++] = password;

        argtable[i++] = end;
    }

    if (arg_nullcheck(argtable) != 0)
    {
        /* NULL entries were detected, some allocations must have failed */
        error("%s: insufficient memory\n",progname);
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
            printf("Usage: %s", progname);
            arg_print_syntax(stdout,argtable,"\n");
            printf("This is a test irc commandline client\n\n");
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
            printf("'%s' example program for the \"argtable\" command line argument parser.\n",progname);
            printf("test version\n");
            exitcode = 0;
            options.running = false;
        }
    }

    if (silent->count > 0)
    {
        if (options.running)
        {
            options.silent  = true;
            options.verbose = false;
            options.debug   = false;
            verbose_printf("silent on\n");
        }
    }

    if (verbose->count > 0)
    {
        if (options.running)
        {
            options.silent  = false;
            options.verbose = true;
            verbose_printf("verbose on\n");
        }
    }
    
    if (debug->count > 0)
    {
        if (options.running)
        {
            options.silent  = false;
            options.verbose = true;
            options.debug   = true;
            verbose_printf("debug on\n");
        }
    }

    if (config->count > 0)
    {
        if (options.running)
        {
            options.configfile = *config->filename;
            verbose_printf("using %s as config file\n", options.configfile);
        }
    }

    /* If the parser returned any errors then display them and exit */
    if (nerrors > 0)
    {
        if (options.running)
        {
            /* Display the error details contained in the arg_end struct.*/
            arg_print_errors(stderr,end,progname);
            nsilent_printf("Try '%s --help' for more information.\n",progname);
            exitcode = 1;
            options.running = false;
        }
    }

    return exitcode;
}

int arg_parsesecondary()
{
	int	exitcode = 0;

    if (mode->count > 0)
    {
        if (options.running)
        {
			options.mode = none;
			if (strncmp(mode->sval[0], "in", 2) == 0)
			{
				options.mode = input;
				verbose_printf("setting mode to input\n");
			}
			else if (strncmp(mode->sval[0], "out", 3) == 0)
			{
				options.mode = output;
				verbose_printf("setting mode to output\n");
			}
			else if (strncmp(mode->sval[0], "both", 4) == 0)
			{
				options.mode = both;
				verbose_printf("setting mode to both\n");
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
			verbose_printf("setting port to %d\n", port->ival[0]);
		}
	}
	
	if (server->count > 0)
	{
        if (options.running)
        {
			options.server = server->sval[0];
			verbose_printf("setting server to %s\n", server->sval[0]);
		}
	}
	
	if (channel->count > 0)
	{
        if (options.running)
        {
			options.channel = channel->sval[0];
			verbose_printf("setting channel to #%s\n", channel->sval[0]);
		}
	}
	
	if (botname->count > 0)
	{
        if (options.running)
        {
			options.botname = botname->sval[0];
			verbose_printf("setting name to %s\n", botname->sval[0]);
		}
	}
	
    if (password->count > 0)
	{
        if (options.running)
        {
//			options.password = password->sval[0];
			verbose_printf("using different password\n");
			verbose_printf("password not supported at this moment\n");
		}
	}
    arg_clean();

    return exitcode;
}

