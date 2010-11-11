#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "config.h"
#include "configdefaults.h"

/** 
Evaluates a Lua expression and returns the string result. 
If an error occurs or the result is not string, def is returned. 
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

static lua_State *conf_open(const char *file)
{
    int err = 0;
    lua_State *L = luaL_newstate();
    if (L == NULL)
    {
        debug_printf("Bailing out! No memory.");
        return L;
    }

    luaL_openlibs(L);

    if ( (err = luaL_loadfile(L, file) ) != 0) 
    {
        if (err == LUA_ERRFILE)
        {
            nsilent_printf("WARNING: cannot access config file: %s\n", file);
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
    }

    return L;
}

int read_config_file()
{
    lua_State *L = conf_open(options.configfile);
    int errorcode = 0;
    
    if (L != NULL)
    {
        options.silent   = lua_boolexpr(L   , "settings.silent"   , CONFIG_SILENT);
        options.verbose  = lua_boolexpr(L   , "settings.verbose"  , CONFIG_VERBOSE);
        options.debug    = lua_boolexpr(L   , "settings.debug"    , CONFIG_DEBUG);
        const char *mode = lua_stringexpr(L , "settings.mode"     , NULL );

        lua_intexpr(L                       , "settings.port"     , &options.port);
        options.server   = lua_stringexpr(L , "settings.server"   , CONFIG_SERVER);
        options.channel  = lua_stringexpr(L , "settings.channel"  , CONFIG_CHANNEL);
        options.botname  = lua_stringexpr(L , "settings.name"     , CONFIG_BOTNAME);
//        options.password = lua_stringexpr(L , "settings.password" , CONFIG_PASSWORD);

        if (mode != NULL)
        {
            if (strncmp(mode, "in", 2) == 0)
            {
                options.mode = input;
            }
            else if (strncmp(mode, "out", 3) == 0)
            {
                options.mode = output;
            }
            else if (strncmp(mode, "both", 4) == 0)
            {
                options.mode = both;
            }
            else if (strncmp(mode, "auto", 4) == 0)
            {
                options.mode = autodetect;
            }
        }

        lua_close(L);
    }
    else errorcode = 1;

    return  errorcode;
}

