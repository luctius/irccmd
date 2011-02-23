#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <readline/readline.h>
#include <readline/history.h>

#include "main.h"
#include "configdefaults.h"
#include "ircmod.h"
#include "input.h"
#include "commands.h"

static bool com_help(char *arg);
static bool com_exit(char *arg);
static bool com_join(char *arg);
static bool com_list(char *arg);
static bool com_channel(char *arg);
static bool com_leave(char *arg);

struct commands commands[] = {
     { "/help"      , com_help     , "displays this help"                  , false } , 
     { "/exit"      , com_exit     , "quits the application"               , false } , 
     { "/quit"      , com_exit     , "quits the application"               , false } , 
     { "/join"      , com_join     , "joins a given channel"               , true  } , 
     { "/list"      , com_list     , "lists all joined channels"           , false } , 
     { "/channel"   , com_channel  , "switch to channel"                   , true  } , 
     { "/leave"     , com_leave    , "leaves the current or given channel" , false } , 
     { (char *)NULL , (void *)NULL , (char *)NULL                          , false }
};

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
        return true;
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

