#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"

extern int luaopen_nav_core(lua_State *L);
extern int luaopen_lfs(lua_State *L);

//#define DTEST
int main(int argc, char* argv[])
{
#ifndef DTEST
	assert(argc == 3);
#endif
	lua_State* L = luaL_newstate();
	luaL_openlibs(L);
	luaL_requiref(L, "nav.core", luaopen_nav_core, 0);
	luaL_requiref(L, "lfs", luaopen_lfs, 0);
#ifndef DTEST
	int status = luaL_loadfile(L, "preprocess.lua");
#else
	int status = luaL_loadfile(L, "loader.lua");
#endif
	if (status != LUA_OK)  {
		fprintf(stderr, "%s\n", lua_tostring(L, -1));
		exit(1);
	}

#ifndef DTEST
	lua_pushstring(L, argv[1]);
	lua_pushstring(L, argv[2]);
	
	status = lua_pcall(L, 2, 0, 0);
#else
	status = lua_pcall(L, 0, 0, 0);
#endif
	if (status != LUA_OK)  {
		fprintf(stderr, "%s\n", lua_tostring(L, -1));
		exit(1);
	}
	lua_close(L);

	return 0;
}