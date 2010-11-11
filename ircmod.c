#include <string.h>
#include <unistd.h>

#include "ircmod.h"
#include "main.h"
#include "configdefaults.h"

irc_session_t *session;
irc_callbacks_t callbacks;
bool init_callbacks = false;

void irc_general_event(irc_session_t *session, const char *event, const char *origin, const char **params, unsigned int count)
{
	printf("irc general event: %s: %s\n", event, origin);
}

void irc_general_event_numeric (irc_session_t * session, unsigned int event, const char * origin, const char ** params, unsigned int count)
{
	printf("irc numeric event: %d: %s\n", event, origin);
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

int create_irc_session()
{
	int retval = 0;

	verbose_printf("setting up irc connection\n");
	session = irc_create_session(&callbacks);

	verbose_printf("connecting to server: %s:%d\n", options.server, options.port);
	retval = irc_connect(session, options.server, options.port, NULL, options.botname, PROG_STRING, PROG_STRING);
	//retval = irc_connect(session, options.server, options.port, options.serverpassword, options.botname, NULL, NULL);
	if (retval != 0) error("%d: %s\n", retval, irc_strerror(irc_errno(session) ) );

	verbose_printf("irc session is ready\n");
	return irc_is_connected(session);
}

int add_irc_descriptors(fd_set *in_set, fd_set *out_set, int *maxfd)
{
    int retval = 0;

    //verbose_printf("adding irc descriptors\n");
    if ( (retval = irc_add_select_descriptors(session, in_set, out_set, maxfd) ) != 0)
    {
        error("add irc descriptors: %s\n", irc_strerror(irc_errno(session) ) );
    }
    return retval;
}

int process_irc(fd_set *in_set, fd_set *out_set)
{
    int retval =0;

//    verbose_printf("processing  irc descriptors\n");
    if ( (retval = irc_process_select_descriptors(session, in_set, out_set) ) != 0)
    {
        error("process irc: %s\n", irc_strerror(irc_errno(session) ) );
    }

    return retval;
}

int close_irc_session()
{
	verbose_printf("close irc connection\n");
	irc_disconnect(session);
	irc_destroy_session(session);
	return 0;
}

int irc_send_raw_msg(const char *message, const char *channel)
{
	if (irc_is_connected(session) )
	{
        verbose_printf("sending irc message\n");
		if (irc_cmd_msg(session, channel, message) != 0)
		//if (irc_cmd_msg(session, "cor", message) != 0)
		{
			error("irc message: %s\n", irc_strerror(irc_errno(session) ) );
		}
		return 0;
	}

	debug_printf("sending irc message -- not connected\n");
	return 1;
}

int irc_update()
{
    struct timeval tv;
    fd_set readset;
    fd_set writeset;
    int maxfd = 0;
    int result;

    tv.tv_sec = 0;
    tv.tv_usec = 2;
    FD_ZERO(&readset);
    FD_ZERO(&writeset);

	if (irc_is_connected(session) )
    {
        add_irc_descriptors(&readset, &writeset, &maxfd);
        result = select(maxfd +1, &readset, &writeset, NULL, &tv);

        if (result == 0)
        {
        } 
        else if (result < 0)
        {
            error("we've got an error on stdin\n");
            return 1;
        }
        else
        {
            process_irc(&readset, &writeset);
        }
    }
    return 0;
}

