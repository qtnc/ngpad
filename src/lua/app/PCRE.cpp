#include<optional>
#include "base.hpp"
#include "../../common/stringUtils.hpp"
#include "../../common/pcre2cpp.hpp"

struct LPCRE {
PCRE reg;
std::string subject;
size_t pos;

LPCRE (lua_State* L, int& idx, bool unused) {
subject = lua_get<std::string>(L, idx++);
std::string pattern = lua_get<std::string>(L, idx++);
int start =  lua_isinteger(L, idx)? lua_tointeger(L, idx++) : 1;
pos = start>0? start -1 : subject.size() + start;
uint32_t opts = PCRE2_UTF | PCRE2_ALT_BSUX;
if (pattern[0]=='/') {
size_t endopt = pattern.rfind('/');
if (endopt!=std::string::npos && endopt>0) {
for (size_t i=endopt+1, n=pattern.size(); i<n; i++) switch(pattern[i]) {
#define O(C,V) case C: opts |= PCRE2_##V; break;
O('i', CASELESS)
O('m', MULTILINE)
O('s', DOTALL)
O('u', UCP)
O('U', UNGREEDY)
O('A', ANCHORED)
O('E', ENDANCHORED)
O('D', DOLLAR_ENDONLY)
O('J', DUPNAMES)
O('x', EXTENDED)
#undef O
}
pattern = pattern.substr(1, endopt -1);
}}
reg = PCRE(pattern, opts);
}
LPCRE (lua_State* L, const int idx): LPCRE(L, const_cast<int&>(idx), true) {}
};

LuaRegisterValueType(LPCRE);

static int lspushmatches (lua_State* L, PCRE& reg) {
int n = reg.groupCount();
if (n<=1) {
lua_push(L, reg.group(0));
lua_pushinteger(L, reg.start(0));
lua_pushinteger(L, reg.end(0));
return 3;
}
else {
for (int i=1; i<n; i++) lua_push(L, reg.group(i));
for (int i=1; i<n; i++) {
lua_pushinteger(L, reg.start(i));
lua_pushinteger(L, reg.end(i));
}
return 3 * n -3;
}
}

static int lspfind (lua_State* L) {
LPCRE l(L, 1);
if (!l.reg.match(l.subject, l.pos)) return 0;
lua_pushinteger(L, 1+l.reg.start());
lua_pushinteger(L, l.reg.end());
return 2 + lspushmatches(L,l.reg);
}

static int lspmatch (lua_State* L) {
LPCRE l(L, 1);
if (!l.reg.match(l.subject, l.pos)) return 0;
return lspushmatches(L,l.reg);
}

static int lspgmatchnext  (lua_State* L) {
LPCRE& l = lua_get<LPCRE&>(L, 1);
if (!l.reg.match(l.subject, l.pos)) return 0;
l.pos = l.reg.end();
return lspushmatches(L,l.reg);
}

static int lspgmatch (lua_State* L) {
LPCRE l(L, 1);
lua_pushcfunction(L, &lspgmatchnext);
lua_push(L, l);
return 2;
}

static int lspgsub (lua_State* L) {
int idx = 1;
LPCRE l(L, idx, true);
int count = 0, max=0;
std::function<std::string(PCRE&)> replacer;
if (lua_isstring(L, idx)) {
std::string replacement = lua_get<std::string>(L, idx++);
replacer = [&](auto&r)mutable{
if (max>0 && count>=max) return r.group(0);
count++;
return r.getReplacement(replacement); 
};
}
else if (lua_isfunction(L, idx)) {
int fidx = idx;
replacer = [&](auto&r)mutable{
if (max>0 && count>=max) return r.group(0);
lua_pushvalue(L, fidx);
int n = lspushmatches(L, r);
lua_pushinteger(L, ++count);
luaL_call(L, n+1, 1);
std::string re = lua_get<std::string>(L, -1);
lua_pop(L, 1);
return re;
};
}
else if (lua_istable(L, idx)) {
int tidx = idx;
size_t g = std::min(1U, l.reg.groupCount() -1);
replacer = [&](auto&r)mutable{
if (max>0 && count>=max) return r.group(0);
count++;
lua_push(L, r.group(g));
lua_gettable(L, tidx);
std::string re;
if (lua_isnoneornil(L, -1) || lua_isboolean(L, -1)) re = r.group(0);
else if (lua_isstring(L, -1)) re = r.getReplacement(lua_get<std::string>(L, -1));
else {
lua_getglobal(L, "tostring");
lua_pushvalue(L, -2);
luaL_call(L, 1, 1);
re = lua_get<std::string>(L, -1);
lua_pop(L, 1);
}
lua_pop(L, 1);
return re;
};
}
max = lua_isinteger(L, idx)? lua_tointeger(L, idx++) : -1;
std::string result = l.reg.replace(l.subject, l.pos, replacer);
lua_push(L, result);
lua_pushinteger(L, count);
return 2;
}

export int luaopen_pcre (lua_State* L) {
//T PCRE2 regular expression support
lua_newtable(L);

//F Works almost like string.find, using PCRE2 regular expression instead of lua pattern syntax: search for the first match and return the position of where it has been found
//P subject: string: nil: subject string
//P pattern: string: nil: PCRE2 regular expression pattern. By starting the pattern with /, you can specify options with the traditional /pattern/options syntax as known in PHP PCRE or perl. Available options: i=ignore case, m=multiline, s=dot all, x=extended, u=unicode UCP extended support, U=ungreedy, A=anchored, D=dollar end only, E=end ancohred, J=allow duplicate names.
//P start: integer: 1: starting position where to start the search
//R integer: starting position of the first match
//R integer: ending position of the first match
//R string...: matched subgroups
lua_pushfield(L, "find", &lspfind);

//F Works almost like string.match, using PCRE2 regular expression instead of lua pattern syntax: search for the first match and return the matched groups
//P subject: string: nil: subject string
//P pattern: string: nil: PCRE2 regular expression pattern (see string.find)
//P start: integer: 1: starting position where to start the search
//R string...: matched subgroups
lua_pushfield(L, "match", &lspmatch);

//F Works almost like string.gmatch, using PCRE2 regular expression instead of lua pattern syntax: return an interator explist to iterate over each matches of the string.
//P subject: string: nil: subject string
//P pattern: string: nil: PCRE2 regular expression pattern (see string.find)
//P start: integer: 1: starting position where to start the search
//R function: iterator function
//R userdata: iterator state data
lua_pushfield(L, "gmatch", &lspgmatch);

//F Works almost like string.gsub, using PCRE2 regular expression instead of lua pattern syntax: perform a global substitution, i.e. replace each match by a replacement
//P subject: string: nil: subject string
//P pattern: string: nil: PCRE2 regular expression pattern (see string.find)
//P start: integer: 1: optional: starting position where to starting search for matches
//P replacement: string|function|table: nil: replacement to apply: whether a replacement string with PCRE2 substitution syntax, a function receiving matched groups as parameters and returning the replacement string, or a table which will be looked up with the first matched group as a key. In a replacement string, Use $0 to $9 or $g{name} to refer to matched groups by index or by name.
//P max: integer: 0: maximum number of replacements to make, <=0 indicating no limit
//R string: new string with all replacements performed
//R integer: number of replacements made 
lua_pushfield(L, "gsub", &lspgsub);

return 1;
}
