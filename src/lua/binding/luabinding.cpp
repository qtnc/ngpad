#include "luabinding.hpp"
#include<iostream>
using namespace std;

void* luaL_testutype  (lua_State* L, int idx, const char* tname) {
if (lua_type(L, idx)!=LUA_TUSERDATA || !luaL_getmetatable(L, tname) || !lua_getmetatable(L, idx)) return nullptr;
void* p = const_cast<void*>(lua_topointer(L, idx));
while (p && !lua_rawequal(L, -1, -2)) {
if (luaL_getmetafield(L, -1, "__index")) lua_remove(L, -2);
else p=nullptr;
}
lua_pop(L, 2);
return p;
}

void* luaL_checkutype  (lua_State* L, int idx, const char* tname) {
if (lua_isnoneornil(L, idx)) return nullptr;
void* ptr = luaL_testutype(L, idx, tname);
if (ptr) return ptr;
luaL_newmetatable(L, tname);
lua_getfield(L, -1, "__name");
luaL_typeerror(L, idx, lua_tostring(L, -1));
return nullptr;
}

namespace Binding {

int luaB_index (lua_State* L) {
if (!lua_getmetatable(L, 1)) return 0;
const char* name = lua_tostring(L, 2);
if (lua_getfield(L, -1, name)) return 1;
if (LUA_TFUNCTION==lua_getfield(L, -2, computeGetterName(name).c_str())) {
lua_pushvalue(L, 1);
lua_call(L, 1, 1);
return 1;
}
if (LUA_TFUNCTION==lua_getfield(L, -3, computeBoolGetterName(name).c_str())) {
lua_pushvalue(L, 1);
lua_call(L, 1, 1);
return 1;
}
if (LUA_TFUNCTION==lua_getfield(L, -4, "__altindex")) {
lua_pushvalue(L, 1);
lua_pushvalue(L, 2);
lua_call(L, 2, 1);
return 1;
}
return 0;
}

int luaB_newindex (lua_State* L) {
if (!lua_getmetatable(L, 1)) return 0;
const char* name = lua_tostring(L, 2);
if (LUA_TFUNCTION==lua_getfield(L, -1, computeSetterName(name).c_str())) {
lua_pushvalue(L, 1);
lua_pushvalue(L, 3);
lua_call(L, 2, 0);
return 0;
}
if (LUA_TFUNCTION==lua_getfield(L, -2, "__altnewindex")) {
lua_pushvalue(L, 1);
lua_pushvalue(L, 2);
lua_pushvalue(L, 3);
lua_call(L, 3, 0);
return 0;
}
return 0;
}

}//Namespace Binding

int retraceback (lua_State* L) {
luaL_traceback(L, L, lua_tostring(L, -1), 0);
return 1;
}

int luaL_call (lua_State* L, int nArgs, int nRets) {
lua_pushcclosure(L, retraceback, 0);
lua_insert(L, -nArgs -2);
return lua_pcall(L, nArgs, nRets, -nArgs -2);
}

