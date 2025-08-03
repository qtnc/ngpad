#include "base.hpp"
#include <wx/mstream.h>
#include <wx/txtstrm.h>

wxMBConv& GetEncodingCodecFromName (const std::string& name);

static int decode (lua_State* L) {
size_t len;
const char* input = lua_tolstring(L, 1, &len);
const char* encoding = lua_tostring(L, 2);
auto& mb = GetEncodingCodecFromName(encoding);
wxString output = wxString(input, mb, len);
lua_push(L, output);
return 1;
}

static int encode (lua_State* L) {
wxString input = lua_get<wxString>(L, 1);
const char* encoding = lua_tostring(L, 2);
auto& mb = GetEncodingCodecFromName(encoding);
wxMemoryOutputStream mOut;
wxTextOutputStream tOut(mOut, wxEOL_NATIVE, mb);
tOut.WriteString(input);
tOut.Flush();
auto buf = mOut.GetOutputStreamBuffer();
const char* cs = reinterpret_cast<const char*>( buf->GetBufferStart() );
const char* csEnd = reinterpret_cast<const char*>( buf->GetBufferPos() );
lua_pushlstring(L, cs, csEnd-cs);
return 1;
}

static wxString MakeLower (wxString s) {
return s.MakeLower();
}

static wxString MakeUpper (wxString s) {
return s.MakeUpper();
}

static wxString MakeCapitalized (wxString s) {
return s.MakeCapitalized();
}


export int luaopen_UTF8 (lua_State* L) {
//T Additions to the utf8 lua module
lua_getglobal(L, "utf8");

//F Turn a string into lowercase
//P string: string: nil: string to translate to lowercase
//R string: the transformed string
lua_pushfield(L, "lower", &MakeLower);

//F Turn a string into uppercase
//P string: string: nil: string to translate to uppercase
//R string: the transformed string
lua_pushfield(L, "upper", &MakeUpper);

//F Capitalize the string
//P string: string: nil: string to capitalize
//R string: the transformed string
lua_pushfield(L, "capitalize", &MakeCapitalized);

//F Decode a string from some encoding into UTF-8
//P string: string: nil: the string to decode
//P encoding: string: nil: the encoding in which the initial string is encoded
//R string: the decoded string in UTF-8
lua_pushfield(L, "decode", &decode);

//F Encode an UTF-8 string into some encoding
//P string: string: nil: the UTF-8 string to be encoded
//P encoding: string: nil: the encoding to encode to
//R string: the string encoded in the given encoding
lua_pushfield(L, "encode", &encode);

lua_pop(L, 1);
return 0;
}
