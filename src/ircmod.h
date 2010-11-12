#ifndef ircmod_h_
#define ircmod_h_

#include <libircclient/libircclient.h>

int create_irc_session();
int close_irc_session();

int add_irc_descriptors(fd_set *in_set, fd_set *out_set, int *maxfd);
int process_irc(fd_set *in_set, fd_set *out_set);

irc_callbacks_t *get_callback();
int irc_send_raw_msg(const char *message, const char *channel);

#endif /*ircmod_h_*/
