// run_lua tool: executed in the tool thread of the agent, with the Lua lock held.
// Bindings marked SYNC run on the main thread and release the lock meanwhile.
#include "AgentTools.hpp"
#include "../lua/app/base.hpp"
#include "../app/App.hpp"
#include "../common/println.hpp"

using json = nlohmann::json;

LuaEnvironment::~LuaEnvironment () {
if (ref==LUA_NOREF) return;
lua_State* L = wxGetApp().GetLuaState();
LockLuaCS(L);
luaL_unref(L, LUA_REGISTRYINDEX, ref);
}

// Values of the stack from index first to the top, converted to strings and separated by tabs
static std::string ValuesToString (lua_State* L, int first) {
std::string result;
for (int i=first, n=lua_gettop(L); i<=n; i++) {
size_t len = 0;
const char* s = luaL_tolstring(L, i, &len);
if (i>first) result += '\t';
result.append(s, len);
lua_pop(L, 1);
}
return result;
}

// print of the agent environment: appends a line to the output table given as upvalue
static int AgentPrint (lua_State* L) {
std::string line = ValuesToString(L, 1);
lua_pushlstring(L, line.data(), line.size());
lua_rawseti(L, lua_upvalueindex(1), lua_rawlen(L, lua_upvalueindex(1)) +1);
return 0;
}

static std::string GetOutput (lua_State* L, int outputIndex) {
std::string output;
for (lua_Integer i=1, n=lua_rawlen(L, outputIndex); i<=n; i++) {
lua_rawgeti(L, outputIndex, i);
size_t len = 0;
const char* s = lua_tolstring(L, -1, &len);
if (s) output.append(s, len);
output += '\n';
lua_pop(L, 1);
}
return output;
}

ToolResult ToolRunLua (const json& args, const ToolContext& context) {
auto it = args.find("code");
std::string code = it!=args.end() && it->is_string()? it->get<std::string>() : "";
if (code.find_first_not_of(" \t\r\n")==std::string::npos) return ToolResult::Error("code is required.");
if (!context.luaEnv) return ToolResult::Error("Lua isn't available.");
auto answer = context.Confirm(MSG("AgentConfirmLua"), true, U(code), GrantKind::None);
if (answer==ToolConfirmation::Stop) return ToolResult::Stop();
if (answer!=ToolConfirmation::Allow) return ToolResult::Error("The user denied running this code. Don't retry it: ask the user what to do instead.");
println("Agent Lua code: {}", code);

// The Lua state is created on the main thread on first use
lua_State* L = RunEDTSync([](){ return wxGetApp().GetLuaState(); });
LockLuaCS(L);
int top = lua_gettop(L);
auto& env = *context.luaEnv;
if (env.ref==LUA_NOREF) {
// Globals of the agent are kept in their own table; other globals are read from the global table
lua_newtable(L);
lua_newtable(L);
lua_pushglobaltable(L);
lua_setfield(L, -2, "__index");
lua_setmetatable(L, -2);
env.ref = luaL_ref(L, LUA_REGISTRYINDEX);
}
lua_rawgeti(L, LUA_REGISTRYINDEX, env.ref);
int envIndex = lua_gettop(L);
lua_newtable(L);
int outputIndex = lua_gettop(L);
lua_pushvalue(L, outputIndex);
lua_pushcclosure(L, AgentPrint, 1);
lua_setfield(L, envIndex, "print");

// As in the Lua console: first try as an expression, then as statements
std::string expr = "return " + code;
int status = luaL_loadbufferx(L, expr.data(), expr.size(), "=run_lua", "t");
if (status!=LUA_OK) {
lua_pop(L, 1);
status = luaL_loadbufferx(L, code.data(), code.size(), "=run_lua", "t");
}
ToolResult result;
if (status!=LUA_OK) result = ToolResult::Error(format("Lua syntax error: {}", lua_tostring(L, -1)));
else {
lua_pushvalue(L, envIndex);
lua_setupvalue(L, -2, 1); // _ENV
int base = lua_gettop(L);
status = luaL_call(L, 0, LUA_MULTRET);
std::string output = GetOutput(L, outputIndex);
if (status!=LUA_OK) {
const char* msg = lua_tostring(L, -1);
result = ToolResult::Error(format("Lua error: {}", msg? msg : "(no message)"));
if (!output.empty()) result.text += "\nOutput before the error:\n" + output;
}
else {
// luaL_call leaves its traceback handler at the former index of the function
std::string values = lua_gettop(L)>base? ValuesToString(L, base+1) : "";
std::string text = output;
if (!values.empty()) text += (output.empty()? "" : "Returned: ") + values + "\n";
if (text.empty()) text = "Executed successfully (no output, nothing returned).\n";
result = ToolResult::Ok(text);
}}
// print of the environment falls back to the global print once the script is finished
lua_pushnil(L);
lua_setfield(L, envIndex, "print");
lua_settop(L, top);
wxCSConv conv(wxFONTENCODING_ISO8859_1);
result.text = ToValidUTF8(result.text, conv);
return result;
}
