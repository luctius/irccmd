#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <errno.h>

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
    (void) snprintf(buf, 255, "evalExpr=%s", expr);

    if (luaL_dostring(L, buf) == 0)
    {
        /* Get the value of the global varibable */
        lua_getglobal(L, "evalExpr");

        if (lua_isstring(L, -1) == 1)
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
    (void) snprintf(buf, 255, "evalExpr=%s", expr);

    if (luaL_dostring(L, buf) == 0)
    {
        /* Get the value of the global varibable */
        lua_getglobal(L, "evalExpr");

        if (lua_isnumber(L, -1) == 1)
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
    double d = 0;
    int ok = lua_numberexpr(L, expr, &d);

    if (ok == 1)
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
static bool lua_boolexpr(lua_State* L, const char* expr, bool def)
{
    bool ok = def;
    char buf[256] = "";

    /* Assign the Lua expression to a Lua global variable. */
    (void) snprintf(buf, 255, "evalExpr=%s", expr);

    if (luaL_dostring(L, buf) == 0)
    {
        /* Get the value of the global varibable */
        lua_getglobal(L, "evalExpr");

        if (lua_isboolean(L, -1) == true)
        {
            ok = (bool) lua_toboolean(L, -1);
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
        int counter = 0;
        char *basestr = "settings.channels[%d].%s";
        char *namestr = "name";
        char *passwdstr = "password";

        verbose("found the config file %s\n", path);

        if (!options.silent)  options.silent    = lua_boolexpr(L , "settings.silent"         , options.silent);
        if (!options.verbose) options.verbose   = lua_boolexpr(L , "settings.verbose"        , options.verbose);
        if (!options.debug)   options.debug     = lua_boolexpr(L , "settings.debug"          , options.debug);

        options.interactive                 = lua_boolexpr(L     , "settings.interactive"    , options.interactive);
        options.showchannel                 = lua_boolexpr(L     , "settings.showchannel"    , options.showchannel);
        options.shownick                    = lua_boolexpr(L     , "settings.shownick"       , options.shownick);
        options.showjoins                   = lua_boolexpr(L     , "settings.showjoins"      , options.showjoins);
        options.enableplugins               = lua_boolexpr(L     , "settings.plugins"        , options.enableplugins);
        (void) lua_intexpr(L                                     , "settings.port"           , &options.port);
        (void) lua_intexpr(L                                     , "settings.oflood"         , &options.output_flood_timeout);
        (void) lua_intexpr(L                                     , "settings.timeout"        , &options.connection_timeout);
        strncpy(options.server              , lua_stringexpr(L   , "settings.server"         , options.server)         , MAX_SERVER_NAMELEN);
        strncpy(options.botname             , lua_stringexpr(L   , "settings.name"           , options.botname)        , MAX_BOT_NAMELEN);
        strncpy(options.serverpassword      , lua_stringexpr(L   , "settings.serverpassword" , options.serverpassword) , MAX_PASSWD_LEN);

        options.botname[MAX_BOT_NAMELEN -1] = '\0';

        /*channels and channel passwords*/
        for (counter = 0; counter < MAX_CHANNELS; counter++)
        {
            char name_buff[strlen(basestr) + strlen(namestr) +2];
            char passwd_buff[strlen(basestr) + strlen(passwdstr) +2];

            (void) snprintf(name_buff, sizeof(name_buff) -1, basestr, counter +1, namestr);
            (void) snprintf(passwd_buff, sizeof(passwd_buff) -1, basestr, counter +1, passwdstr);

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

        debug("creating plugin paths\n");
        /* plugin paths */
        basestr = "settings.plugin_path[%d]";
        for (counter = options.no_pluginpaths; counter < MAX_CHANNELS; counter++)
        {
            char pluginpath[strlen(basestr) +10];
            (void) snprintf(pluginpath, sizeof(pluginpath) -1, basestr, counter +1);
            strncpy(options.pluginpaths[counter], lua_stringexpr(L, pluginpath, options.pluginpaths[counter]), MAX_PATH_LEN);

            if (strlen(options.pluginpaths[counter]) == 0) counter = MAX_CHANNELS;
            else
            {
                options.no_pluginpaths = counter +1;
                debug("fetching %s: %s\n", pluginpath, options.pluginpaths[counter]);
            }
        }

        debug("creating plugins\n");
        /* plugins */
        basestr = "settings.plugin[%d]";
        for (counter = options.no_plugins; counter < MAX_CHANNELS; counter++)
        {
            char plugin[strlen(basestr) +10];
            (void) snprintf(plugin, sizeof(plugin) -1, basestr, counter +1);
            strncpy(options.plugins[counter], lua_stringexpr(L, plugin, options.plugins[counter]), MAX_CHANNELS_NAMELEN);

            if (strlen(options.plugins[counter]) == 0) counter = MAX_CHANNELS;
            else
            {
                options.no_plugins = counter +1;
                debug("fetching %s: %s\n", plugin, options.plugins[counter]);
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

static char *execute_lua_string_plugin(const char *file, char *string)
{
    /* initialize Lua */
    lua_State *L = lua_open();
    /* load Lua base libraries */
    luaL_openlibs(L);
    
    /* load the script */
    (void)luaL_dofile(L, file);
    char *retstr = string;

    if (L != NULL)
    {
        /* the function name */
        lua_getglobal(L, "plugin_string_exec");
        /* the first argument */
        lua_pushstring(L, string);
        /* call the function with 2
           arguments, return 1 result */
        lua_call(L, 1, 1);
        /* get the result */

        if (lua_isstring(L, -1) == 1) retstr = lua_tostring(L, -1);
        lua_pop(L, 1);
        lua_close(L);
    }
    return retstr;
}

char *execute_str_plugins(char *string)
{
    int plugincounter;
    int pathcounter;
    char *retstr = string;

    if (options.enableplugins)
    {
        for (plugincounter = 0; plugincounter < options.no_plugins; plugincounter++)
        {
            char *basestr = "%s/%s.lua";
            for (pathcounter = 0; pathcounter < options.no_pluginpaths; pathcounter++)
            {
                char plugin[strlen(options.pluginpaths[pathcounter]) + strlen(options.plugins[plugincounter]) +strlen(basestr) +2];
                (void) snprintf(plugin, sizeof(plugin) -1, basestr, options.pluginpaths[pathcounter], options.plugins[plugincounter]);

                debug("testing: %s\n", plugin);

                struct stat sts;
                if (!(stat(plugin, &sts) == -1 && errno == ENOENT))
                {
                    /*plugin file exists*/
                    retstr = execute_lua_string_plugin(plugin, retstr);
                    pathcounter = options.no_pluginpaths;
                }
            }
        }
    }
    return string;
}

