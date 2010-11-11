#include "configdefaults.h"

void setdefaults(struct config_options *op)
{
    op->connected       = false;
    op->silent          = CONFIG_SILENT;
    op->debug           = CONFIG_DEBUG;
    op->verbose         = CONFIG_VERBOSE;
    op->configfile      = CONFIG_FILE;

    op->mode            = CONFIG_MODE;
    op->port            = CONFIG_PORT;
    op->server          = CONFIG_SERVER;
    op->serverpassword  = CONFIG_SERVERPASSWORD;
    op->channel         = CONFIG_CHANNEL;
    op->channelpassword = CONFIG_CHANNELPASSWORD;
    op->botname         = CONFIG_BOTNAME;
}

