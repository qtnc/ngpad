#include "base.hpp"

unsigned long long mtime ();

export int luaopen_OS (lua_State* L) {
lua_getglobal(L, "os");
lua_pushfield(L, "clock", &mtime);
lua_pushfield(L, "execute", nullptr);
lua_pushfield(L, "exit", nullptr);
lua_pushfield(L, "setlocale", nullptr);
lua_pop(L, 1);
return 0;
}
