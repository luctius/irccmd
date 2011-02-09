#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "config.h"
#include "configdefaults.h"

/** 
* Evaluates a Lua expression and returns the string result. 
* If an error occurs or the result is not string, def is returned. 
* 
* @param L the lua_State in which the string resides.
* @param expr the lua expression to retreive the string
* @param def the default string should the State not contain the expression given
* 
* @return a pointer towards the asked string.
*/
static const char* lua_stringexpr(lua_State *L, const char *expr, const char *def)
{
    const char* r = def;
    char buf[256] = "";

    /* Assign the Lua expression to a Lua global variable. */
    sprintf(buf, "evalExpr=%s", expr);

    if (!luaL_dostring(L, buf) )
    {
        /* Get the value of the global varibable */
        lua_getglobal(L, "evalExpr");

        if ( lua_isstring(L, -1) )
        {
            r = lua_tostring(L, -1);
        }

        /* remove lua_getglobal value */
        lua_pop(L, 1);
    }
    return r;
}  

/** 
* Evaluates a Lua expression and returns the number result. 
* 
* @param L the lua_State in which the number resides.
* @param expr the lua expression to retreive the number
* @param out the result of the expression.
* 
* @return zero on succes, otherwise 1.
*/
static int lua_numberexpr(lua_State *L, const char *expr, double *out)
{
    int ok = 0;
    char buf[256] = "";

    /* Assign the Lua expression to a Lua global variable. */
    sprintf( buf, "evalExpr=%s", expr );

    if (!luaL_dostring(L, buf) )
    {
        /* Get the value of the global varibable */
        lua_getglobal(L, "evalExpr");

        if (lua_isnumber(L, -1) )
        {
            *out = lua_tonumber(L, -1);
            ok = 1;
        }

        /* remove lua_getglobal value */
        lua_pop(L, 1);
    }

    return ok;
}

/** 
* Evaluates a Lua expression and returns the int result. 
* This uses lua_numberexpr underwater.
* 
* @param L the lua_State in which the int resides.
* @param expr the lua expression to retreive the int
* @param out the result of the expression
* 
* @return zero on succes, otherwise 1.
*/
static int lua_intexpr(lua_State* L, const char* expr, int* out)
{
    double d;
    int ok = lua_numberexpr(L, expr, &d);

    if (ok)
    {
        *out = (int) d;
    }

    return ok;
}

/** 
* Evaluates a Lua expression and returns the bool result. 
* If an error occurs or the result is not bool, def is returned. 
* 
* @param L the lua_State in which the bool resides.
* @param expr the lua expression to retreive the bool
* @param def the default bool should the State not contain the expression given
* 
* @return the bool retreived from lua via the expression
*/
static int lua_boolexpr(lua_State* L, const char* expr, bool def)
{
    int ok = def;
    char buf[256] = "";

    /* Assign the Lua expression to a Lua global variable. */
    sprintf(buf, "evalExpr=%s", expr);

    if (!luaL_dostring(L, buf) )
    {
        /* Get the value of the global varibable */
        lua_getglobal(L, "evalExpr");

        if (lua_isboolean(L, -1) )
        {
            ok = lua_toboolean(L, -1);
        }

        /* remove lua_getglobal value */
        lua_pop(L, 1);
    }

    return ok;
}

/** 
* Opens the given file, and executes it within a Lua Context.
* 
* @param file A path and file towards the config file.
* 
* @return returns the lua_State on succes or NULL otherwise.
*/
static lua_State *conf_open(const char *file)
{
    int err = 0;
    lua_State *L = luaL_newstate();
    if (L == NULL)
    {
        error("Bailing out! No memory.");
        return L;
    }

    luaL_openlibs(L);
    if ( (err = luaL_loadfile(L, file) ) != 0) 
    {
        if (err == LUA_ERRFILE)
        {
            verbose("cannot access config file: %s\n", file);
        }
        else if (err == LUA_ERRSYNTAX)
        {
            error("syntax error in config file: %s\n", file);
        }
        else if (err == LUA_ERRERR)
        {
            error("general LUA error when accessing config file: %s\n", file);
        }
        else if (err == LUA_ERRMEM)
        {
            error("out of memory when accessing config file: %s\n", file);
        }
        else
        {
            error("Unkown LUA error(%d) when accessing config file: %s\n", err, file);
        }
        lua_close(L);
        L = NULL;
    }
    else
    {
        if ( (err = lua_pcall(L, 0, LUA_MULTRET, 0) ) != 0)
        {
            if (err == LUA_ERRRUN)
            {
                error("Run error when executing config file: %s\n", file);
            }
            else if (err == LUA_ERRERR)
            {
                error("general LUA error when executing config file: %s\n", file);
            }
            else if (err == LUA_ERRMEM)
            {
                error("out of memory when executing config file: %s\n", file);
            }
            else
            {
                error("unkown error(%d) when executing config file: %s\n", err, file);
            }

            lua_close(L);
            L = NULL;
        }
        else debug("loaded config file in memory; parsing now...\n");
    }

    return L;
}

/** 
* This reads the default config file, or the system default if the first is not found.
* When either is read, the application settings are read from file or their defaults are used.
* 
* @return returns 0 on succes or 1 on failure.
*/
int read_config_file(const char *path)
{
    lua_State *L = conf_open(path);
    int errorcode = 0;
    
    if (L != NULL)
    {
        verbose("found the config file %s\n", path);

        if (!options.silent)  options.silent    = lua_boolexpr(L , "settings.silent"         , options.silent);
        if (!options.verbose) options.verbose   = lua_boolexpr(L , "settings.verbose"        , options.verbose);
        if (!options.debug)   options.debug     = lua_boolexpr(L , "settings.debug"          , options.debug);

        options.showchannel                 = lua_boolexpr(L     , "settings.showchannel"    , options.showchannel);
        options.shownick                    = lua_boolexpr(L     , "settings.shownick"       , options.shownick);
        lua_intexpr(L                                            , "settings.port"           , &options.port);
        strncpy(options.server              , lua_stringexpr(L   , "settings.server"         , options.server)         , MAX_SERVER_NAMELEN);
        strncpy(options.botname             , lua_stringexpr(L   , "settings.name"           , options.botname)        , MAX_BOT_NAMELEN);
        strncpy(options.serverpassword      , lua_stringexpr(L   , "settings.serverpassword" , options.serverpassword) , MAX_PASSWD_LEN);

        int counter = 0;
        char *basestr = "settings.channels[%d].%s";
        char *namestr = "name";
        char *passwdstr = "password";
        for (counter = 0; counter < MAX_CHANNELS; counter++)
        {
            char name_buff[strlen(basestr) + strlen(namestr) +2];
            char passwd_buff[strlen(basestr) + strlen(passwdstr) +2];

            snprintf(name_buff, sizeof(name_buff) -1, basestr, counter +1, namestr);
            snprintf(passwd_buff, sizeof(passwd_buff) -1, basestr, counter +1, passwdstr);

            strncpy(options.channels[counter],         lua_stringexpr(L, name_buff,   options.channels[counter]),         MAX_CHANNELS_NAMELEN);
            strncpy(options.channelpasswords[counter], lua_stringexpr(L, passwd_buff, options.channelpasswords[counter]), MAX_PASSWD_LEN);

            if (strlen(options.channels[counter]) == 0) counter = MAX_CHANNELS;
            else
            {
                options.no_channels = counter +1;
                debug("fetching %s: %s\n", name_buff, options.channels[counter]);
                debug("fetching %s: %s\n", passwd_buff, options.channelpasswords[counter]);
            }
        }

        debug("number of channels to join: %d\n", options.no_channels);
        lua_close(L);
    }
    else
    {
        errorcode = 1;
    }

    return  errorcode;
}

