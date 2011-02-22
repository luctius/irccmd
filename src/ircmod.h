#ifndef ircmod_h_
#define ircmod_h_

#include "def.h"
#include "main.h"

#ifdef HAVE_LIBIRCCLIENT_H
    #include <libircclient.h>
#elif HAVE_LIBIRCCLIENT_LIBIRCCLIENT_H
    #include <libircclient/libircclient.h>
#else
    #error "ircclibclient.h not available"
#endif

bool create_irc_session();
int close_irc_session();
bool join_irc_channel(char *channel, char *password);
bool part_irc_channel(char *channel);
bool is_irc_connected();

int add_irc_descriptors(fd_set *in_set, fd_set *out_set, int *maxfd);
int process_irc(fd_set *in_set, fd_set *out_set);
bool check_irc_connection();

irc_callbacks_t *get_callback();
int irc_send_raw_msg(const char *message, const char *channel);

#endif /*ircmod_h_*/
