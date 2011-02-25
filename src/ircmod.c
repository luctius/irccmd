#include <string.h>
#include <unistd.h>

#include <sys/select.h>
#include <sys/types.h>

#include "ircmod.h"
#include "configdefaults.h"

static irc_session_t *session;
static irc_callbacks_t callbacks;
static bool init_callbacks = false;
static time_t last_contact = 0;

void irc_general_event(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
    if (count == 0) debug("irc general event[0]: %s: %s\n", event, origin);
    if (count == 1) debug("irc general event[1]: %s: %s: %s\n", event, origin, params[0]);
    if (count == 2) debug("irc general event[2]: %s: %s: %s: %s\n", event, origin, params[0], params[1]);
    if (count >= 3) debug("irc general event[3]: %s: %s: %s: %s: %s\n", event, origin, params[0], params[1], params[2]);

    if (strstr(event, "PONG") == event)
    {
        last_contact = time(NULL);
    }
}

void irc_general_event_numeric (irc_session_t * session, unsigned int event, const char * origin, const char ** params, unsigned int count)
{
	debug("irc numeric event: %d: %s\n", event, origin);

    if (event == LIBIRC_RFC_ERR_NICKNAMEINUSE) /* Nick allready in use */
    {
        warning("Nick allready in use\n");

        if (options.botname_nr == -1)
        {
            options.botname_nr = 0;
            debug("first try in creating a new nick\n");
            if (strlen(options.botname) >= (MAX_BOT_NAMELEN -1) )
            {
                options.botname[MAX_BOT_NAMELEN -1] = '\0';
                verbose("shortened nick to %s\n", options.botname);
            }

            if (strlen(options.botname) < (MAX_BOT_NAMELEN -2) )
            {
                debug("adding seperator\n");
                sprintf(options.botname, "%s_", options.botname);
            }

            if (strlen(options.botname) < (MAX_BOT_NAMELEN -1) )
            {
                debug("nick too long for a seperator\n");
                sprintf(options.botname, "%sX", options.botname);
                verbose("new nick is %s\n", options.botname);
            }
        }

        if (options.running)
        {
            if (options.botname_nr < 0xF)
            {
                debug("Nick rename try nr %d\n", options.botname_nr);
                options.botname[strlen(options.botname) -1] = '\0';
                sprintf(options.botname, "%s%X", options.botname, ++options.botname_nr);

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
        error("join: %d: %s\n", retval, irc_strerror(irc_errno(session) ) );
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
        error("part: %d: %s\n", retval, irc_strerror(irc_errno(session) ) );
        return false;
    }
    return true;
}

static bool setup_irc_session()
{
	debug("setting up irc connection\n");
	session = irc_create_session(&callbacks);

    if(options.debug) irc_option_set(session, LIBIRC_OPTION_DEBUG);
    return (session != NULL) ? true : false;
}

static int connect_irc_session()
{
    int retval = 0;
    options.connected = false;
    last_contact = time(NULL);

	verbose("connecting to server: %s:%d\n", options.server, options.port);
	retval = irc_connect(session, options.server, options.port, options.serverpassword, options.botname, PROG_STRING, PROG_STRING);
	if (retval != 0) 
    {
        error("connect: %d: %s\n", retval, irc_strerror(irc_errno(session) ) ); 
    }
    else debug("irc session is ready\n");

	return (irc_is_connected(session) == 1) ? true : false;
}

bool create_irc_session()
{
    if (setup_irc_session() )
    {
        return connect_irc_session();
    }
    return false;
}

int add_irc_descriptors(fd_set *in_set, fd_set *out_set, int *maxfd)
{
    int retval = 0;

    if (is_irc_connected() )
    {
        if ( (retval = irc_add_select_descriptors(session, in_set, out_set, maxfd) ) != 0)
        {
            error("add irc descriptors: %s\n", irc_strerror(irc_errno(session) ) );
        }
    }
    return retval;
}

bool check_irc_connection()
{
    time_t current_time = time(NULL);
    time_t timeout = current_time - last_contact;
    irc_send_raw(session, "PING %s\n", options.channels[0]);

    if (options.connected)
    {
        if (timeout > (options.connection_timeout / 5) ) debug("timeout [%ld]\n", timeout);
        if (timeout > options.connection_timeout)
        {
            error("connection timed-out (%ld seconds)\n", timeout);
            options.botname_nr = 0;
            return create_irc_session();
        }
    }
    else
    {
        if (timeout > (options.connection_timeout) )
        {
            warning("no connection with the server yet; retrying (%ld seconds)\n", timeout);
            options.botname_nr = 0;
            return create_irc_session();
        }
    }
    return true;
}

int process_irc(fd_set *in_set, fd_set *out_set)
{
    int retval = 0;

    if (is_irc_connected() )
    {
        if ( (retval = irc_process_select_descriptors(session, in_set, out_set) ) != 0)
        {
            int err = irc_errno(session);
            if (err != 0) error("process irc[%d]: %s\n", retval, irc_strerror(err ) );
        }
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

bool is_irc_connected()
{
    return (irc_is_connected(session) == 1) ? true : false;
}

int irc_send_raw_msg(const char *message, const char *channel)
{
	if (is_irc_connected() && options.connected)
	{
        int retval = 0;
		if ( (retval = irc_cmd_msg(session, channel, message) ) != 0)
		{
			error("irc message[%d]: %s\n", retval, irc_strerror(irc_errno(session) ) );
		}
		return 0;
	}

	return 1;
}

