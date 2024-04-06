#include "base.hpp"
#include "nlohmann/json.hpp"


using json = nlohmann::json;

struct ToLuaJsonSax {
lua_State* L;
std::vector<int> arrayIndices;

ToLuaJsonSax (lua_State* L1):
L(L1) {}

void store () {
if (arrayIndices.empty()) return;
if (arrayIndices.back() < 0) lua_settable(L, -3);
else lua_rawseti(L, -2, ++arrayIndices.back());
}

bool null () { 
lua_pushnil(L);
store();
return true; 
}

bool boolean (bool val) { 
lua_pushboolean(L, val);
store();
return true; 
}

bool number_integer (long long val) {
lua_pushinteger(L, val);
store(); 
return true; 
}

bool number_unsigned (unsigned long long val) { 
lua_pushinteger(L, val);
store();
return true; 
}

bool number_float (double val, const std::string& s) { 
lua_pushnumber(L, val);
store();
return true; 
}

bool string (const std::string& s) { 
lua_pushstring(L, s.c_str());
store();
return true; 
}

bool binary (const json::binary_t& val) { 
lua_pushlstring(L, reinterpret_cast<const char*>(val.data()), val.size());
store();
return true; 
}

bool key (const std::string& s) { 
lua_pushstring(L, s.c_str());
return true;
}

bool start_array (size_t n) { 
lua_newtable(L);
arrayIndices.push_back(0);
return true; 
}

bool end_array () { 
arrayIndices.pop_back();
if (!arrayIndices.empty()) store();
return true; 
}

bool start_object (size_t n) { 
lua_newtable(L);
arrayIndices.push_back(-1);
return true; 
}

bool end_object () { 
arrayIndices.pop_back();
if (!arrayIndices.empty()) store();
return true; 
}

bool parse_error (size_t pos, const std::string& token, const std::exception& ex) { 
luaL_error(L, "%s: pos=%d, token=%s", ex.what(), pos, token.c_str());
return false; 
}

};//JsonSax

static json ltojson (lua_State* L, int i) {
if (lua_isnoneornil(L, i)) return nullptr;
else if (lua_isinteger(L, i)) return lua_tointeger(L, i);
else if (lua_isnumber(L, i)) return lua_tonumber(L, i);
else if (lua_isboolean(L, i)) return static_cast<bool>(lua_toboolean(L, i));
else if (lua_isstring(L, i)) return lua_get<std::string>(L, i);
else if (lua_istable(L, i)) {
int top = lua_gettop(L) +1;
int rawlen = lua_rawlen(L, i);
json j;
if (rawlen>0) for (int k=1; k<=rawlen; k++) {
lua_rawgeti(L, i, k);
j.push_back(ltojson(L, top));
lua_pop(L, 1);
}
else {
lua_pushnil(L);
while(lua_next(L, i)) {
std::string key = lua_get<std::string>(L, top);
j[key] = ltojson(L, top+1);
lua_pop(L, 1);
}}
return j;
}
else luaL_argerror(L, i, "value is not serializable to JSON");
return nullptr;
}


static int ljsonload (lua_State* L) {
int n = lua_gettop(L);
for (int i=1; i<=n; i++) {
ToLuaJsonSax sax(L);
std::string s = lua_get<std::string>(L, i);
json::sax_parse(s, &sax);
}
return n;
}

static int ljsondump (lua_State* L) {
int n = lua_gettop(L);
for (int i=1; i<=n; i++) {
json j = ltojson(L, i);
auto out = j.dump();
lua_pushstring(L, out.c_str());
}
return n;
}


export int luaopen_json (lua_State* L) {
lua_newtable(L);

//T JSON parsing library
//lua_setfield(L, -2, "json");

lua_pushcclosure(L, ljsondump, 0);
//F Dump a lua value into JSON
//P values...: any: nil: any lua values to convert to JSON
//R string: the resulting JSON string
lua_setfield(L, -2, "dump");

lua_pushcclosure(L, ljsonload, 0);
//F Load JSON data
//P jsonString...: string: nil: JSON strings to parse
//R string: the resulting lua object parsed from JSON
lua_setfield(L, -2, "load");

return 1;
}
