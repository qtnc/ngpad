#include "base.hpp"

int lua_iswxpoint (lua_State* L, int idx) {
if (!lua_istable(L, idx)) return false; 
bool result = false; 
lua_rawgeti(L, idx, 1);  
lua_rawgeti(L, idx, 2); 
result = lua_isinteger(L, -1) && lua_isinteger(L, -2); 
lua_pop(L, 2); 
if (result) return true; 
lua_getfield(L, idx, "x"); 
lua_getfield(L, idx, "y"); 
result = lua_isinteger(L, -1) && lua_isinteger(L, -2); 
lua_pop(L, 2); 
return result; 
}

wxPoint lua_towxpoint (lua_State* L, int idx) {
if (!lua_istable(L, idx)) return wxDefaultPosition;
lua_rawgeti(L, idx, 1); 
lua_rawgeti(L, idx, 2); 
if (lua_isinteger(L, -1) && lua_isinteger(L, -2)) return wxPoint(lua_tointeger(L, -2), lua_tointeger(L, -1)); 
lua_pop(L, 2); 
lua_getfield(L, idx, "x"); 
lua_getfield(L, idx, "y"); 
if (lua_isinteger(L, -1) && lua_isinteger(L, -2)) return wxPoint(lua_tointeger(L, -2), lua_tointeger(L, -1)); 
else return wxDefaultPosition;
}

int lua_pushwxpoint  (lua_State* L, const wxPoint& pt) {
lua_newtable(L);
lua_pushinteger(L, pt.x);
lua_setfield(L, -2, "x");
lua_pushinteger(L, pt.y);
lua_setfield(L, -2, "y");
lua_pushinteger(L, pt.x);
lua_rawseti(L, -2, 1);
lua_pushinteger(L, pt.y);
lua_rawseti(L, -2, 2);
return 1;
}

int lua_iswxsize (lua_State* L, int idx) {
if (!lua_istable(L, idx)) return false; 
bool result = false; 
lua_rawgeti(L, idx, 1);  
lua_rawgeti(L, idx, 2); 
result = lua_isinteger(L, -1) && lua_isinteger(L, -2); 
lua_pop(L, 2); 
if (result) return true; 
lua_getfield(L, idx, "width"); 
lua_getfield(L, idx, "height"); 
result = lua_isinteger(L, -1) && lua_isinteger(L, -2); 
lua_pop(L, 2); 
return result; 
}

wxSize lua_towxsize (lua_State* L, int idx) {
if (!lua_istable(L, idx)) return wxDefaultSize;
lua_rawgeti(L, idx, 1); 
lua_rawgeti(L, idx, 2); 
if (lua_isinteger(L, -1) && lua_isinteger(L, -2)) return wxSize(lua_tointeger(L, -2), lua_tointeger(L, -1)); 
lua_pop(L, 2); 
lua_getfield(L, idx, "width"); 
lua_getfield(L, idx, "height"); 
if (lua_isinteger(L, -1) && lua_isinteger(L, -2)) return wxSize(lua_tointeger(L, -2), lua_tointeger(L, -1)); 
else return wxDefaultSize;
}

int lua_pushwxsize  (lua_State* L, const wxSize& sz) {
lua_newtable(L);
lua_pushinteger(L, sz.x);
lua_setfield(L, -2, "width");
lua_pushinteger(L, sz.y);
lua_setfield(L, -2, "height");
lua_pushinteger(L, sz.x);
lua_rawseti(L, -2, 1);
lua_pushinteger(L, sz.y);
lua_rawseti(L, -2, 2);
return 1;
}

wxArrayString lua_towxarstr (lua_State* L, int idx) {
wxArrayString ar;
for (int i=1; lua_geti(L, idx, i); i++) {
ar.push_back( lua_get<wxString>(L, -1));
lua_pop(L, 1);
}
return ar;
}

int lua_pushwxarstr (lua_State* L, const wxArrayString& ar) {
lua_newtable(L);
for (int i=0, n=ar.size(); i<n; i++) {
lua_push(L, ar[i]);
lua_rawseti(L, -2, i+1);
}
return 1;
}

int wxDynamicLoad (lua_State* L) {
std::string name = lua_tostring(L, 2);
auto func = reinterpret_cast<lua_CFunction>(GetProcAddress(GetModuleHandle(NULL), ("luaopen_" + name).c_str()));
if (func) {
func(L);
lua_pushvalue(L, 2);
lua_pushvalue(L, -2);
lua_rawset(L, 1);
}
else lua_pushnil(L);
return 1;
}

export int luaopen_DynamicLoad (lua_State* L) {
lua_pushglobaltable(L);
lua_newtable(L);
lua_pushcfunction(L, wxDynamicLoad);
lua_setfield(L, -2, "__index");
lua_setmetatable(L, -2);
lua_settop(L, 0);
return 0;
}
