#ifndef ircmod_h_
#define ircmod_h_

#include <libircclient/libircclient.h>

int create_irc_session();
int close_irc_session();

int irc_update();

irc_callbacks_t *get_callback();
int irc_send_raw_msg(const char *message, const char *channel);

#endif /*ircmod_h_*/
