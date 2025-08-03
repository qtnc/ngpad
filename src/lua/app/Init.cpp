#include "base.hpp"
#include "../../app/App.hpp"
#include "../../common/stringUtils.hpp"
#include<sstream>

export int luaopen_wx (lua_State* L);
export int luaopen_AbstractDocument (lua_State* L);
export int luaopen_UTF8 (lua_State* L);
export int luaopen_IO (lua_State* L);
export int luaopen_OS (lua_State* L);
export int luaopen_DynamicLoad(lua_State* L);

static wxString warnmsg;
static void luaWarn (void* unused, const char* msg, int cont) {
if (!warnmsg.empty()) warnmsg += ' ';
warnmsg += U(msg);
if (!cont) {
wxGetApp() .OnLuaMessage("Warning: " + warnmsg);
warnmsg.clear();
}}

static int luaPrint (lua_State* L) {
std::ostringstream out;
lua_getglobal(L, "tostring");
for (int i=1, n=lua_gettop(L); i<n; i++) {
if (lua_isstring(L, i)) {
size_t l; const char* s = lua_tolstring(L, i, &l);
out.write(s,l);
}
else {
lua_pushvalue(L, -1);
lua_pushvalue(L, i);
lua_call(L, 1, 1);
size_t l; const char* s = lua_tolstring(L, -1, &l);
out.write(s,l);
lua_pop(L, 1);
}
if (i<n) out << '\t';
}
wxGetApp() .OnLuaMessage( U(out.str()) );
return 0;
}

static int dbgdump (lua_State* L) {
const void* p = lua_touserdata(L, 1);
size_t len = lua_rawlen(L, 1);
lua_pushlstring(L, reinterpret_cast<const char*>(p), len);
return 1;
}

wxString LuaGetBanner () {
return U(LUA_COPYRIGHT) + "\n" + U(LUA_AUTHORS);
}

bool IsIncompleteStatement (lua_State* L) {
std::string msg = lua_tostring(L, -1);
return iends_with(msg, "<eof>");
}

int LuaSimpleEval (const wxString& s, wxString* re) {
lua_State* L = wxGetApp() .GetLuaState();
lua_settop(L, 0);
int result = luaL_loadbufferx(L, U("return " + s).c_str(), s.size()+7, "eval", "bt");
if (result && IsIncompleteStatement(L)) return 10;
if (result) {
lua_pop(L, 1);
result = luaL_loadbufferx(L, U(s).c_str(), s.size(), "eval", "bt");
}
if (result && IsIncompleteStatement(L)) return 10;
if (result) {
if (re) *re = U(lua_tostring(L, -1));
return result;
}
result = luaL_call(L, 0, LUA_MULTRET);
lua_remove(L, 1);
std::ostringstream out;
for (int i= 1, n=lua_gettop(L); i<=n; i++) {
if (!lua_isstring(L, i)) {
lua_getglobal(L, "tostring");
lua_pushvalue(L, i);
lua_pcall(L, 1, 1, 0);
lua_replace(L, i);
}
const char* cs; size_t l;
cs = lua_tolstring(L, i, &l);
out.write(cs, l);
if (i<n) out << '\t';
}
if (re) *re = U(out.str());
return result;
}

void LoadPlugins (App& app, Properties& props) {
lua_State* L = nullptr;
std::vector<std::string> pluginList = split(props.get("plugin", ""), ",;\t\r\n", true);
for (auto& name: pluginList) {
trim(name);
if (name.empty()) continue;
if (app.LoadPlugin(name)) continue;
if (!L) {
L = app.GetLuaState();
lua_getglobal(L, "require");
}
lua_pushvalue(L, -1);
lua_push(L, name);
if (luaL_call(L, 1, 1)) throw std::runtime_error(lua_tostring(L,-1));
lua_pop(L, 1);
}
if (L) lua_pop(L, 1);
}

void CloseLua (lua_State*& L) {
if (!L) return;
lua_close(L);
L = nullptr;
}

void InitLua (lua_State*& L) {
CloseLua(L);
auto& app = wxGetApp();
L = luaL_newstate();
lua_setwarnf(L, &luaWarn, nullptr);
luaL_openlibs(L);
luaopen_DynamicLoad(L);
luaopen_UTF8(L);
luaopen_IO(L);
luaopen_OS(L);

lua_pushglobal(L, "print", &luaPrint);
lua_pushglobal(L, "beep", &Beep);

#ifdef DEBUG
lua_getglobal(L, "debug");
lua_pushcfunction(L, &dbgdump);
lua_setfield(L, -2, "dump");
lua_pop(L, 1);

wxString initscriptfn = wxGetApp() .FindAppFile("init.lua");
if (!initscriptfn.empty()) {
if (luaL_loadfilex(L, U(initscriptfn).c_str(), nullptr) || luaL_call(L, 0, 0)) throw std::runtime_error(lua_tostring(L, -1));
}
#endif

lua_settop(L, 0);
}
