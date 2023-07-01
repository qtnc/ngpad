#include "base.hpp"
#include "../../common/Properties.hpp"

bool LoadFile (const wxString& filename, wxString& text, Properties& properties);
bool SaveFile (const wxString& filename, wxString& text, const Properties& properties);

void tableToProperties (lua_State* L, int idx, Properties& props) {
if (!lua_istable(L, idx)) return;
idx = lua_absindex(L, idx);
lua_pushnil(L);
while(lua_next(L, idx)) {
if (!lua_isstring(L, -1)) {
lua_getglobal(L, "tostring");
lua_insert(L, -2);
lua_call(L, 1, 1);
}
props.put(lua_tostring(L, -2), lua_tostring(L, -1));
lua_pop(L, 1);
}
lua_pop(L, 1);
}

void propertiesToTable (lua_State* L, int idx, Properties& props) {
idx = lua_absindex(L, idx);
for (auto& e: props.entries()) {
if (e.second=="true") lua_pushboolean(L, true);
else if (e.second=="false") lua_pushboolean(L, false);
else lua_push(L, e.second);
lua_setfield(L, idx, e.first.c_str());
}
}

static int ioWriteFile (lua_State* L) {
wxString filename = lua_get<wxString>(L, 1);
wxString text = lua_get<wxString>(L, 2);
Properties props;
tableToProperties(L, 3, props);
lua_pushboolean(L, SaveFile(filename, text, props));
if (lua_istable(L, 3)) lua_pushvalue(L, 3);
else lua_newtable(L);
propertiesToTable(L, -1, props);
return 2;
}

static int ioReadFile (lua_State* L) {
wxString filename = lua_get<wxString>(L, 1);
Properties props;
wxString text;
tableToProperties(L, 2, props);
LoadFile(filename, text, props);
lua_push(L, text);
if (lua_istable(L, 2)) lua_pushvalue(L, 2);
else lua_newtable(L);
propertiesToTable(L, -1, props);
return 2;
}

export int luaopen_IO (lua_State* L) {
//T Additions to the standard io lua module
lua_getglobal(L, "io");

//F Reads a file from disk
//P filename: string: nil: name of the file to read
//P properties: table: nil: table of properties specifying encoding, line ending, etc.
//R string: content of the file, read and decoded as requested
lua_pushfield(L, "readfile", &ioReadFile);

//F Write a file to disk
//P filename: string: nil: name of the file to write to
//P text: string: nil: text content of the file
//P properties: table: nil: table of properties specifying encoding, line ending, etc.
lua_pushfield(L, "writefile", &ioWriteFile);

lua_pushfield(L, "input", nullptr);
lua_pushfield(L, "output", nullptr);
lua_pushfield(L, "flush", nullptr);
lua_pushfield(L, "read", nullptr);
lua_pushfield(L, "write", nullptr);
lua_pushfield(L, "popen", nullptr);
lua_pop(L, 1);
return 0;
}
