#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "main.h"
#include "configdefaults.h"
#include "ircmod.h"
#include "commands.h"
#include "input.h"

/* Filescope variables */
static char *prompt = NULL;
static int completion_index = 0;

/* Helper functions */
static void process_command(char *line);
static void send_irc_message(char *msg);
static size_t sgets(int fd, char *line, size_t size);
static int get_channel(char *channel);
static char **irccmd_completion(char *text, int start, int end);
static bool valid_argument(char *caller, char *arg, bool req_args);

static char* ltrim(char* s) 
{
    char* newstart = s;

    debug("old string: |%s|\n", s);

    while (isspace( *newstart) ) ++newstart; /* newstart points to first non-whitespace char (which might be '\0') */
    memmove(s, newstart, strlen(newstart) + 1); /* don't forget to move the '\0' terminator*/
    while (isspace(s[strlen(s) -1]) && strlen(s) > 0) s[strlen(s) -1] = '\0'; /* Remove trailing spaces */

    debug("new string: |%s|\n", s);

    return s;
}

void init_readline()
{
    debug("initializing readline library\n");

    if ( (isatty(fileno(stdin) ) == false) || ( (options.mode | input) == 0) )
    {
        if (options.interactive == true) warning("Turning interactive mode off; stdin is a pipe\n");
        options.interactive = false;
    }

    if (options.interactive)
    {
        /* Allow conditional parsing of the ~/.inputrc file. */
        rl_readline_name = PROG_STRING;

        /* Install input handler */
        change_prompt();

        /* Enable Completion */
        rl_bind_key('\t',rl_complete);

        /* Tell the completer that we want a crack first. */
        rl_attempted_completion_function = (CPPFunction *)irccmd_completion;


        verbose("interactive mode: turning on showchannel and shownick");
        options.showchannel = true;
        options.shownick = true;
        options.mode = output;
    }
}

void deinit_readline()
{
    debug("de-initializing readline library\n");
    if (options.interactive)
    {
        rl_callback_handler_remove();
        free(prompt);
    }
}

void process_input()
{
    if (options.interactive)
    {
        rl_callback_read_char();
    }
    else if (options.connected)
    {
        int result = 0;
        char buff[300];

        memset(buff, 0, sizeof(buff) );
        result = sgets(STDIN_FILENO, buff, sizeof(buff) );

        if (result == 0)
        {
            /*
               The writing end has closed.
               We either switch to output only
               or stop the application.
             */
            if (options.keepreading)
            {
                options.mode = output;
            }
            else
            {
                options.running = false;
            }
            
        }
        else if (result > 0)
        {
            if (options.running)
            {
                send_irc_message(buff);
            }
        }
    }
}

void change_prompt()
{
    if (options.interactive) 
    {
        free(prompt);

        /* Allocate and create prompt */
        prompt = malloc(strlen(options.botname) + strlen(options.channels[options.current_channel_id]) +3);
        sprintf(prompt, "%s@%s: ", options.botname, options.channels[options.current_channel_id]);
        debug("changing prompt to: %s", prompt);

        rl_callback_handler_install(prompt, process_command);
    }
}

static void process_command(char *line)
{
    if (line[0] == '/')
    {
        int index = -1;
        int counter = 0;
        char *command = NULL;
        char *arguments = NULL;

        debug("received %s as an command\n", line);

        /* Fetch command */
        command = strchr(line, '/');

        /* Fetch Arguments */
        arguments = strchr(line, ' ');
        if (arguments != NULL)
        {
            *arguments = '\0';
            arguments++;
            arguments = ltrim(arguments);
        }

        debug("command is %s\n", command);
        if (arguments != NULL) debug("arguments are %s\n", arguments);

        /* Fetch command information */
        for (counter = 0; commands[counter].name; counter++)
        {
            if (strcmp(command, commands[counter].name) == 0)
            {
                index = counter;
                break;
            }
        }

        if (index >= 0)
        {
            /* Execute command*/
            if (valid_argument(command, arguments, commands[index].req_args) )
            {
                verbose("executing command: %s %s\n", command, arguments);
                if (commands[index].func(arguments) == false)
                {
                    options.running = false;
                    error("error occured in %s\n", command);
                }
                else
                {
                    /* Restore original line */
                    if (arguments != NULL)
                    {
                        arguments--;
                        *arguments = ' ';
                    }

                    debug("adding \"%s\" to readline history\n", line);
                    add_history(line);
                }
            }
        }
        else nsilent("command %s not found\n", command);
    }
    else send_irc_message(line);
}

static void send_irc_message(char *msg)
{
    bool error = options.connected;
    int channel_id = 0;
    char channel[MAX_CHANNELS_NAMELEN];
    char *msg_start = msg;
    char *channel_start = msg;

    if (options.connected == true)
    {
        if (msg != NULL)
        {
            msg = ltrim(msg);

            /* Check if the first non-white-space character is a '#' */
            if (msg[0] == '#')
            {
                channel_start = strchr(msg, '#');
                /* Find space after channel; then put a \0 character there and go to the next 
                   character which should be the start of the message */
                msg_start = strchr(channel_start, ' ');

                if (msg_start != NULL)
                {
                    *msg_start = '\0';
                    msg_start++;
                }
            }

            if (msg_start != NULL)
            {
                /* If so, find the channel in the known channels */
                if (channel_start != NULL)
                {
                    channel_id = get_channel(channel_start);
                }

                if (strlen(msg_start) > 0)
                {
                    /* Send the message to the correct channel */
                    memset(channel, 0, sizeof(channel));
                    strncpy(channel, options.channels[channel_id], sizeof(channel) );

                    irc_send_raw_msg(msg_start, channel);

                    if (options.interactive)
                    {
                        add_history(msg);
                    }
                }
                error = false;
            }
            else debug("could not succesfully parse message; message NULL?\n");
        }
        else debug("could not succesfully parse message; message NULL?\n");
    }
    else
    {
        debug("not connected to a channel yet; forgetting message\n");
    }

    if (error)
    {
        error("failed to parse message\n");
        options.running = false;
    }
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
static size_t sgets(int fd, char *line, size_t size)
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

static int get_channel(char *channel)
{
    int channel_id = 0;
    int len = strlen(channel);

    while (strncmp(options.channels[channel_id], channel, len) != 0)
    {
        channel_id++;
        if (channel_id >= options.no_channels)
        {
            channel_id = options.current_channel_id;
            debug("channel %s not found, defaulting to %s\n", channel, options.channels[channel_id]);
            break;
        }
    }

    return channel_id;
}

/* Return non-zero if ARG is a valid argument for CALLER, else print
      an error message and return zero. */
static bool valid_argument(char *caller, char *arg, bool req_args)
{
    if (!arg || !*arg)
    {
        if (req_args)
        {
            fprintf (stderr, "%s: Argument required.\n", caller);
            return false;
        }
    }

    return true;
}

char *duplicate_string (char* s)
{
    char *r;

    r = malloc(strlen(s) + 1);
    strcpy(r, s);
    return r;
}


static char *command_generator(const char *text, int state)
{
    char *match = NULL;
    int len = strlen(text);
    if (!state) completion_index = 0;

    while ( (match = commands[completion_index++].name) )
    {
        if (strncmp(match, text, len) == 0) return duplicate_string(match);
    }

    return NULL;
}

static char *channel_generator(const char *text, int state)
{
    char *match = NULL;
    int len = strlen(text);
    if (!state) completion_index = 0;

    while (completion_index < MAX_CHANNELS)
    {
        match = options.channels[completion_index++];
        if (strncmp(match, text, len) == 0) return duplicate_string(match);
    }

    return NULL;
}

static char **irccmd_completion(char *text, int start, int end)
{
    char **matches;

    matches = (char **)NULL;

    /* If this word is at the start of the line, then it is a command
        to complete.  Otherwise it is the name of a file in the current
        directory. */
    if (text[0] == '/')
    {
        matches = rl_completion_matches(text, command_generator);
    }
    else if (strchr(text, '#') != NULL)
    {
        matches = rl_completion_matches(text, channel_generator);
    }

    return matches;
}

