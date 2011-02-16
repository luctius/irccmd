#include <string.h>
#include <unistd.h>

#include "ircmod.h"
#include "configdefaults.h"

irc_session_t *session;
irc_callbacks_t callbacks;
bool init_callbacks = false;

void irc_general_event(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
    if (count == 0) debug("irc general event: %s: %s\n", event, origin);
    if (count == 1) debug("irc general event: %s: %s: %s\n", event, origin, params[0]);
    if (count == 2) debug("irc general event: %s: %s: %s: %s\n", event, origin, params[0], params[1]);
    if (count >= 3) debug("irc general event: %s: %s: %s: %s: %s\n", event, origin, params[0], params[1], params[2]);
}

void irc_general_event_numeric (irc_session_t * session, unsigned int event, const char * origin, const char ** params, unsigned int count)
{
	debug("irc numeric event: %d: %s\n", event, origin);

    if (event == 433)
    {
        error("Nick allready in use\n");

        if (options.botname_nr == 0)
        {
            if (strlen(options.botname) <= (MAX_BOT_NAMELEN -2) )
            {
                sprintf(options.botname, "%s_", options.botname);
            }

            if (strlen(options.botname) <= (MAX_BOT_NAMELEN -1) )
            {
                sprintf(options.botname, "%sX", options.botname);
                debug("new nick is %s", options.botname);
            }
            else if (strlen(options.botname) >= (MAX_BOT_NAMELEN) )
            {
                options.running = false;
            }
        }

        if (options.running)
        {
            if (options.botname_nr < 0xF)
            {
                options.botname[strlen(options.botname) -1] = '\0';
                sprintf(options.botname, "%s%X", options.botname, options.botname_nr++);

                verbose("retrying with nick: %s\n", options.botname);
                create_irc_session();
            }
            else options.running = false;
        }
    }
}

irc_callbacks_t *get_callback()
{
	if (init_callbacks == false) 
	{
		memset(&callbacks, 0, sizeof(callbacks) );

		callbacks.event_connect     = irc_general_event;
		callbacks.event_join        = irc_general_event;
		callbacks.event_nick        = irc_general_event;
		callbacks.event_quit        = irc_general_event;
		callbacks.event_part        = irc_general_event;
		callbacks.event_mode        = irc_general_event;
		callbacks.event_topic       = irc_general_event;
		callbacks.event_kick        = irc_general_event;
		callbacks.event_channel     = irc_general_event;
		callbacks.event_privmsg     = irc_general_event;
		callbacks.event_notice      = irc_general_event;
		callbacks.event_invite      = irc_general_event;
		callbacks.event_umode       = irc_general_event;
		callbacks.event_ctcp_rep    = irc_general_event;
		callbacks.event_ctcp_action = irc_general_event;
		callbacks.event_unknown     = irc_general_event;
		callbacks.event_numeric     = irc_general_event_numeric;
		init_callbacks = true;
	}

	return &callbacks;
}

bool join_irc_channel(char *channel, char *password)
{
    int retval = 0;
    verbose("joining channel: %s\n", channel);
    retval = irc_cmd_join(session, channel, password);
    if (retval != 0)
    {
        error("%d: %s\n", retval, irc_strerror(irc_errno(session) ) );
        return false;
    }
    return true;
}

bool part_irc_channel(char *channel)
{
    int retval = 0;
    verbose("leaving channel: %s\n", channel);
    retval = irc_cmd_part(session, channel);
    if (retval != 0)
    {
        error("%d: %s\n", retval, irc_strerror(irc_errno(session) ) );
        return false;
    }
    return true;
}

int create_irc_session()
{
	int retval = 0;

	debug("setting up irc connection\n");
	session = irc_create_session(&callbacks);

	verbose("connecting to server: %s:%d\n", options.server, options.port);
	retval = irc_connect(session, options.server, options.port, options.serverpassword, options.botname, PROG_STRING, PROG_STRING);
	if (retval != 0) error("%d: %s\n", retval, irc_strerror(irc_errno(session) ) );

	debug("irc session is ready\n");
	return irc_is_connected(session);
}

int add_irc_descriptors(fd_set *in_set, fd_set *out_set, int *maxfd)
{
    int retval = 0;

    if ( (retval = irc_add_select_descriptors(session, in_set, out_set, maxfd) ) != 0)
    {
        error("add irc descriptors: %s\n", irc_strerror(irc_errno(session) ) );
    }
    return retval;
}

int process_irc(fd_set *in_set, fd_set *out_set)
{
    int retval =0;

    if ( (retval = irc_process_select_descriptors(session, in_set, out_set) ) != 0)
    {
        error("process irc: %s\n", irc_strerror(irc_errno(session) ) );
    }

    return retval;
}

int close_irc_session()
{
	verbose("close irc connection\n");
	irc_disconnect(session);
	irc_destroy_session(session);
	return 0;
}

int irc_send_raw_msg(const char *message, const char *channel)
{
	if (irc_is_connected(session) )
	{
		if (irc_cmd_msg(session, channel, message) != 0)
		{
			error("irc message: %s\n", irc_strerror(irc_errno(session) ) );
		}
		return 0;
	}

	return 1;
}

