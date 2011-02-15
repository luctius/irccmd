#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "main.h"
#include "configdefaults.h"
#include "ircmod.h"
#include "input.h"

/* Filescope variables */
static char *prompt = NULL;
static int completion_index = 0;

/* Helper functions */
static void process_command(char *line);
static void send_irc_message(char *msg);
static size_t sgets(int fd, char *line, size_t size);
static int get_channel(char *channel);
static void change_prompt();
static char **irccmd_completion(char *text, int start, int end);
static bool valid_argument(char *caller, char *arg, bool req_args);

/* Command definitions*/
struct commands
{
    char *name;             /* User printable name of the function. */
    bool (*func)(char *);   /* Function to call to do the job. */
    char *doc;              /* Documentation for this function.  */
    bool req_args;          /* True if the commands expects arguments */
};

static bool com_help(char *arg);
static bool com_exit(char *arg);
static bool com_join(char *arg);
static bool com_list(char *arg);
static bool com_channel(char *arg);
static bool com_leave(char *arg);

static struct commands commands[] = {
     { "/help"      , com_help     , "displays this help"    , false } ,
     { "/exit"      , com_exit     , "quits the application" , false } ,
     { "/join"      , com_join     , "quits the application" , true  } ,
     { "/list"      , com_list     , "quits the application" , false } ,
     { "/channel"   , com_channel  , "quits the application" , true  } ,
     { "/leave"     , com_leave    , "quits the application" , false } ,
     { (char *)NULL , (void *)NULL , (char *)NULL            , false } 
};

void init_readline()
{
    debug("initializing readline library\n");

    if (isatty(fileno(stdin) ) == false)
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
    else
    {
        int result = 0;
        char buff[300];

        memset(buff, 0, sizeof(buff) );
        result = sgets(STDIN_FILENO, buff, sizeof(buff) );

        if (result == 0)
        {
            debug("stdin reset");
        }
        else if (result > 0)
        {
            debug("received message %s\n", buff);
            if (options.running)
            {
                send_irc_message(buff);
            }
        }
    }
}

static char* ltrim(char* s) 
{
    char* newstart = s;

    debug("old string: %s\n", s);
    while (isspace( *newstart)) {
            ++newstart;
    }

    /* newstart points to first non-whitespace char (which might be '\0') */
    memmove(s, newstart, strlen(newstart) + 1); /* don't forget to move the '\0' terminator*/
    s[strlen(s)] = '\0';
    debug("new string: %s\n", s);

    return s;
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
            debug("arguments are %s\n", arguments);
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
                verbose("executing command: %s\n", command);
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

static void change_prompt()
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


static void send_irc_message(char *msg)
{
    bool error = options.connected;
    int channel_id = 0;
    char channel[MAX_CHANNELS_NAMELEN];
    char *msg_start = msg;
    char *channel_start = msg;

    if (options.connected == true)
    {
        debug("send_irc-message\n");
        if (msg != NULL)
        {
            /* Check if the first non-white-space character is a '#' */
            channel_start = strchr(msg, '#');
            if (channel_start != NULL)
            {
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
                debug("msg: %s\n", msg_start);

                /* If so, find the channel in the known channels */
                if (channel_start != NULL)
                {
                    debug("channel: %s\n", channel_start);
                    channel_id = get_channel(channel_start);
                }

                if (strlen(msg_start) > 0)
                {
                    /* Send the message to the correct channel */
                    memset(channel, 0, sizeof(channel));
                    strncpy(channel, options.channels[channel_id], sizeof(channel) );
                    debug("sending: %s to channel %s\n", msg_start, channel);

                    irc_send_raw_msg(msg_start, channel);

                    if (options.interactive)
                    {
                        add_history(msg);
                    }
                    else verbose(".");
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

/* Commands */
static bool com_help(char *arg)
{
    int index = 0;
    nsilent("This is the commands overview of %s\n\n", PROG_STRING);
    
    while (commands[index].name)
    {
        nsilent("%s\t\t%s\n", commands[index].name, commands[index].doc);
        index++;
    }
    nsilent("\n");

    return true;
}

static bool com_exit(char *arg)
{
    options.running = false;
    return true;
}

static bool com_list(char *arg)
{
    int counter = 0;

    for (counter = 0; counter < options.no_channels; counter++)
    {
        nsilent("%s\n", options.channels[counter]);
    }
    nsilent("\n");

    return true;
}

static bool com_channel(char *arg)
{
    int counter = 0;

    for (counter = 0; counter < options.no_channels; counter++)
    {
        if (strncmp(options.channels[counter], arg, MAX_CHANNELS_NAMELEN) == 0)
        {
            options.current_channel_id = counter;
            change_prompt();
            return true;
        }
    }
    warning("channel %s not found\n", arg);

    return true;
}

static bool com_join(char *arg)
{
    int counter = 0;
    int index = options.no_channels;
    char *channel = NULL;
    char *password = NULL;

    /* Fetch channel and password */
    channel = arg;
    password = strchr(arg, ' ');
    if (password != NULL)
    {
        *password = '\0';
        password++;
    }

    /* Check if the channel is allready known */
    for (counter = 0; counter < options.no_channels; counter++)
    {
        if (strncmp(options.channels[counter], channel, MAX_CHANNELS_NAMELEN) == 0)
        {
            warning("Allready joined channel %s\n", channel);
            return true;
        }
    }

    /* Clearing new channel entry */
    debug("clearing last channel\n");
    if (options.no_channels < MAX_CHANNELS)
    {
        memset(options.channels[options.no_channels], '\0', MAX_CHANNELS_NAMELEN);
        memset(options.channelpasswords[options.no_channels], '\0', MAX_CHANNELS_NAMELEN);
    }
    else
    {
        error("Channel list if full\n");
        return true;
    }

    /* Put the channel in the channel list */
    strncpy(options.channels[index], channel, MAX_CHANNELS_NAMELEN);
    if (password != NULL) strncpy(options.channelpasswords[index], password, MAX_PASSWD_LEN);
    debug("joining %s\n", options.channels[index]);

    /* Join the channel */
    if (join_irc_channel(options.channels[index], options.channelpasswords[index]) == false)
    {
        warning("unable to join %s\n", options.channels[index]);
    }
    else
    {
        debug("old no channels[%d] and current channel id[%d]\n", options.no_channels, options.current_channel_id);
        options.no_channels = index +1;
        options.current_channel_id = index;
        debug("new no channels[%d] and current channel id[%d]\n", options.no_channels, options.current_channel_id);
        change_prompt();
    }

    return true;
}

static bool com_leave(char *arg)
{
    int temp_channel = 0;

    if (arg != NULL)
    {
        if (strlen(arg) > 0)
        {
            /* find channel id */
            /* Check if the channel is allready known */
            int counter = 0;
            for (counter = 0; counter < options.no_channels; counter++)
            {
                if (strncmp(options.channels[counter], arg, MAX_CHANNELS_NAMELEN) == 0)
                {
                    options.current_channel_id = counter;
                    counter = 100;
                }
            }
        }
    }

    if (options.no_channels == 1)
    {
        error("Cannot close last channel!\n");
        return true;
    }

    debug("channel to leave %s[%d]\n", options.channels[options.current_channel_id], options.current_channel_id);

    /* Part from channel*/
    part_irc_channel(options.channels[options.current_channel_id]);
    verbose("leaving channel: %s", options.channels[options.current_channel_id]);

    /* Copy another channel in place */
    if (options.current_channel_id < (options.no_channels -1) )
    {
        int cc_id = options.current_channel_id;
        int lc_id = options.no_channels -1;

        debug("copying channel[%d]: %s in place of chanel[%d]: %s", cc_id, options.channels[cc_id], lc_id, options.channels[lc_id]);

        /* Copy last channel to current channel position */
        strncpy(options.channels[cc_id], options.channels[lc_id], MAX_CHANNELS_NAMELEN);
        memset(options.channelpasswords[cc_id], '\0', MAX_CHANNELS_NAMELEN);

        if (strlen(options.channelpasswords[lc_id]) > 0)
        {
            debug("copying channel password\n");
            strncpy(options.channelpasswords[cc_id], options.channelpasswords[lc_id], MAX_CHANNELS_NAMELEN);
        }
        
    }
    /* Clear last channel */
    debug("clearing last channel\n");
    memset(options.channels[options.no_channels -1], '\0', MAX_CHANNELS_NAMELEN);
    memset(options.channelpasswords[options.no_channels -1], '\0', MAX_CHANNELS_NAMELEN);

    debug("number of channels lowered to %d\n", options.no_channels);
    options.no_channels--;

    /* Go to default channel*/
    options.current_channel_id = temp_channel;
    change_prompt();

    return true;
}

