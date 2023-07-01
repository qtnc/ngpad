#include "../common/Properties.hpp"
#include "../common/wxUtils.hpp"
#include "../app/App.hpp"
#include <wx/fontmap.h>
#include <wx/regex.h>
#include <wx/stream.h>
#include <wx/stdstream.h>
#include <wx/wfstream.h>
#include <wx/ffile.h>

#include<sstream>
#include<unordered_map>
#include<functional>
#include<numeric>
#include "../common/println.hpp"

#define LE_CRLF 0
#define LE_LF 1
#define LE_CR 2
#define LE_LS 3
#define LE_PS 4
#define LE_NEL 5
#define LE_RS 6
#define LE_NUL 7
#define LE_LAST 8

#define ENC_WITH_BOM 0x10000

wxFontEncoding GetLocalDefaultEncoding ();
wxFontEncoding GetLocalOemDefaultEncoding ();

static wxMBConvUTF16LE wxConvUTF16LE;
static wxMBConvUTF16BE wxConvUTF16BE;
static wxMBConvUTF32LE wxConvUTF32LE;
static wxMBConvUTF32BE wxConvUTF32BE;

static wxMBConv* CreateMBConv (int value);

struct Encoding {
wxMBConv* conv = nullptr;
int value;
std::string bom;
std::vector<std::string> alias;

wxMBConv& GetCodec () {
if (!conv) conv = CreateMBConv(value);
return *conv;
}
};

struct Enctable {
int cp;
char name[16];
char desc[64];
short data[256];
};

struct Enctable8MBConv: wxMBConv {
const Enctable& enc;
Enctable8MBConv (const Enctable& e): enc(e) {}
wxMBConv* Clone () const override { return new Enctable8MBConv(*this); }
size_t ToWChar (wchar_t* dest, size_t destlen, const char* src, size_t srclen) const override {
if (!src) return wxCONV_FAILED;
if (!dest || !destlen) return srclen;
if (destlen<srclen) return wxCONV_FAILED;
for (size_t i=0; i<srclen; i++) {
*dest++ = enc.data[*src++&0xFF];
}
return srclen;
}
size_t FromWChar (char* dest, size_t destlen, const wchar_t* src, size_t srclen) const override {
if (!src) return wxCONV_FAILED;
if (!dest || !destlen) return srclen;
if (destlen<srclen) return wxCONV_FAILED;
std::unordered_map<wchar_t, char> map;
for (int i=0; i<256; i++) map[enc.data[i]] = i;
for (size_t i=0; i<srclen; i++) {
*dest++ = map[*src++];
}
return srclen;
}
};

std::vector<Encoding> encodings = {
{ &wxConvUTF8, wxFONTENCODING_UTF8, "", { "utf-8", "utf8" }},
{ &wxConvUTF8, wxFONTENCODING_UTF8 | ENC_WITH_BOM, "\xEF\xBB\xBF", { "utf-8-bom", }},
{ &wxConvUTF16LE, wxFONTENCODING_UTF16LE, "", { "utf-16le", "utf-16", "utf16" }},
{ &wxConvUTF16LE, wxFONTENCODING_UTF16LE | ENC_WITH_BOM, "\xFF\xFE", { "utf-16le-bom" }},
{ &wxConvUTF16BE, wxFONTENCODING_UTF16BE, "", { "utf-16be" }},
{ &wxConvUTF16BE, wxFONTENCODING_UTF16BE | ENC_WITH_BOM, "\xFE\xFF", { "utf-16be-bom" }},

#define WIN(E) { nullptr, wxFONTENCODING_CP##E, "", { "windows-" #E, "windows" #E, "cp" #E  }}
#define ISO(E) { nullptr, wxFONTENCODING_ISO8859_##E, "", { "iso-8859-" #E, "iso8859-" #E, "latin" #E, "latin-" #E }}
#define IBM(E) { nullptr, wxFONTENCODING_CP##E,  "", { "cp" #E, "ibm" #E, "oem" #E  }}
#define CP(E,...) { nullptr, wxFONTENCODING_CP##E,  "", { "cp" #E, __VA_ARGS__  }}
#define MAC(E,L) { nullptr, wxFONTENCODING_MAC##E, "", { "mac-" #L, "mac" #L }}

ISO(1),
{ nullptr, GetLocalDefaultEncoding(), "", { "local" }},
{ nullptr, GetLocalOemDefaultEncoding(), "", { "oem-local", "oem" }},

ISO(2), ISO(3), ISO(4), ISO(5), ISO(6), ISO(7), ISO(8), 
ISO(9), ISO(10), ISO(11), ISO(12), ISO(13), ISO(14), ISO(15),
WIN(1250), WIN(1251), WIN(1252), WIN(1253), WIN(1254), WIN(1255), WIN(1256), WIN(1257), WIN(1258),
IBM(437), IBM(850), IBM(852), IBM(855), IBM(866), 
CP(874), 
CP(932, "shift-gis"), 
CP(936, "gb-2312", "gb2312"), 
CP(949), 
CP(950, "big-5", "big5"), 
CP(1361, "johab"),

MAC(ROMAN, roman),
MAC(JAPANESE, japanese),
MAC(CHINESETRAD, chinese-traditional),
MAC(CHINESESIMP, chinese-simplified),
MAC(KOREAN, korean),
MAC(ARABIC, arabic),
MAC(ARABICEXT, arabic-extended),
MAC(HEBREW, hebrew),
MAC(CENTRALEUR, central-european),
MAC(GREEK, greek),
MAC(CYRILLIC, cyrillic),
MAC(TURKISH, turkish),
MAC(CROATIAN, croatian),
MAC(ROMANIAN, romanian),
MAC(ICELANDIC, icelandic),
MAC(CELTIC, celtic),
MAC(GAELIC, gaelic),
MAC(THAI, thai),
MAC(LAOTIAN, laotian),
MAC(GEORGIAN, georgian),
MAC(ARMENIAN, armenian),
MAC(VIATNAMESE, viatnamese),

MAC(DEVANAGARI, devanagari),
MAC(GURMUKHI, gurmukhi),
MAC(GUJARATI, gujarati),
MAC(ORIYA, oriya),
MAC(BENGALI, bengali),
MAC(TAMIL, tamil),
MAC(TELUGU, telugu),
MAC(KANNADA, kannada),
MAC(MALAJALAM, malajalam),
MAC(SINHALESE, sinhalese),
MAC(BURMESE, burmese),
MAC(KHMER, khmer),
MAC(TIBETAN, tibetan),
MAC(MONGOLIAN, mongolian),
MAC(ETHIOPIC, ethiopic),

#undef WIN
#undef ISO
#undef CP
#undef MAC

{ &wxConvUTF32LE, wxFONTENCODING_UTF32LE, "", { "utf-32le", "utf-32", "utf32" }},
{ &wxConvUTF32LE, wxFONTENCODING_UTF32LE | ENC_WITH_BOM, "\x00\x00\xFF\xFE", { "utf-32le-bom" }},
{ &wxConvUTF32BE, wxFONTENCODING_UTF32BE, "", { "utf-32be" }},
{ &wxConvUTF32BE, wxFONTENCODING_UTF32BE | ENC_WITH_BOM, "\xFE\xFF\x00\x00", { "utf-32be-bom" }},
{ &wxConvUTF7, wxFONTENCODING_UTF7, "", { "utf-7"  }},

{ nullptr, wxFONTENCODING_KOI8, "", { "koi8", "koi8r", "koi8-r" }},
{ nullptr, wxFONTENCODING_KOI8_U, "", { "koi8u", "koi8-u" }},
{ nullptr, wxFONTENCODING_EUC_JP, "", { "euc-jp" }},
{ nullptr, wxFONTENCODING_ISO2022_JP, "", { "iso-2022-jp", "iso2022-jp" }}
};

std::vector<std::string> LINE_ENDINGS = { "crlf", "lf", "cr", "ls", "ps", "nel", "rs", "nul" };
wxString LINE_ENDING_CHARS = wxString(L"\n\r\x2028\x2029\x85\x1E\x00", 8);

std::unordered_map<int, Encoding*> encodingsByValue;
std::unordered_map<std::string, Encoding*> encodingsByName;

std::vector<Enctable> enctables;

static std::vector<Enctable>& getEnctables () {
if (!enctables.empty()) return enctables;
wxLogNull logNull;
enctables.clear();
wxString enctableFile = wxGetApp() .FindAppFile("enctable.tbl");
if (!enctableFile.empty()) {
wxFileInputStream in(enctableFile);
if (in.IsOk()) {
Enctable enctable;
while(in.ReadAll(&enctable, sizeof(enctable))) {
enctables.emplace_back(enctable);
}
}}
return enctables;
}

static wxMBConv* CreateMBConv (int value) {
if (value<0x20000)
return new wxCSConv( static_cast<wxFontEncoding>(value & 0xFFFF ));
else if (value<0x40000) {
for (auto& enctable: getEnctables()) {
if (enctable.cp == (value&0xFFFF)) return new Enctable8MBConv(enctable);
}}
return nullptr;
}

void initEncodings () {
if (encodingsByValue.empty() || encodingsByName.empty()) {
for (auto& enctable: getEnctables()) {
Encoding enc;
enc.value = enctable.cp | 0x20000;
if (*enctable.name>='0' && *enctable.name<='9') {
enc.alias.push_back(format("cp{}", enctable.name));
enc.alias.push_back(format("oem{}", enctable.name));
enc.alias.push_back(format("ibm{}", enctable.name));
}
else enc.alias.push_back(enctable.name);
encodings.emplace_back(enc);
}

for (auto& enc: encodings) {
encodingsByValue[enc.value] = &enc;
for (auto& alias: enc.alias) encodingsByName[alias] = &enc;
}}}

int GetEncodingFromName (const std::string& name) {
initEncodings();
auto e = encodingsByName[name];
return e? e->value : wxFONTENCODING_DEFAULT;
}

std::string GetEncodingName (int value) {
initEncodings();
auto e = encodingsByValue[value];
return e? e->alias[0] : "default";
}

wxString GetEncodingDescription (int value) {
switch(value){
case wxFONTENCODING_CP850:
return MSG("CP850");
case wxFONTENCODING_CP852:
return MSG("CP852");
case wxFONTENCODING_CP855:
return MSG("CP855");
default:
if (value>=0x10000 && value<0x20000)
return wxFontMapper::GetEncodingDescription( static_cast<wxFontEncoding>( value &0xFFFF )) + " + " + MSG("EncodingBOM");
else if (value<0x10000) 
return wxFontMapper::GetEncodingDescription( static_cast<wxFontEncoding>( value &0xFFFF ));
else if (value>=0x20000 && value<0x40000) {
for (auto& enctable: getEnctables()) {
if (enctable.cp==(value&0xFFFF)) return U(format("{} (CP {})", enctable.desc, enctable.name));
}}
return U(format("Undefined encoding CP{}", value));
}}

int GetEncodingFromIndex (int index) {
return encodings[index].value;
}

int GetEncodingIndex (int value) {
for (int i=0, n=encodings.size(); i<n; i++) if (encodings[i].value==value) return i;
return -1;
}

wxMBConv& GetEncodingCodec (int index) {
return encodings[index].GetCodec();
}

wxMBConv& GetEncodingCodecFromName (const std::string& name) {
initEncodings();
auto e = encodingsByName[name];
return e? e->GetCodec() : wxConvUTF8;
}

int GetEncodingsCount (bool forMenu) {
int n = encodings.size();
if (forMenu) n = std::min(n, 9);
return n;
}

int GetLineEndingsCount (bool forMenu) {
int n = LINE_ENDINGS.size();
if (forMenu) n = std::min(n, 3);
return n;
}

int GetLineEndingFromName (const std::string& name) {
auto it = std::find(LINE_ENDINGS.begin(), LINE_ENDINGS.end(), name);
return it==LINE_ENDINGS.end()? -1 : it-LINE_ENDINGS.begin();
}

std::string GetLineEndingName (int lineEnding) {
return LINE_ENDINGS[lineEnding];
}

int GetEncoding (const Properties& properties) {
return GetEncodingFromName(properties.get("charset", "auto"));
}

void SetEncoding (Properties& properties, int encoding) {
properties.put("charset", GetEncodingName(encoding));
}

int GetLineEnding (const Properties& properties) {
return GetLineEndingFromName(properties.get("line_ending", "auto"));
}

void SetLineEnding (Properties& properties, int lineEnding) {
properties.put("line_ending", GetLineEndingName(lineEnding));
}

int GetIndentType (const Properties& properties) {
int size = properties.get("indent_size", 0);
return properties.get("indent_style", "auto")=="tab"? -size : size;
}

void SetIndentType (Properties& properties, int size) {
properties.put("indent_size", abs(size));
properties.put("indent_style", size<=0? "tab" : "space");
}

#ifdef __WIN32
static inline wxFontEncoding GetEncodingFromCP (int cp) {
// https://docs.microsoft.com/en-us/windows/win32/intl/code-page-identifiers
switch(cp){
#define CP(X) case X: return wxFONTENCODING_CP##X;
CP(1250) CP(1251) CP(1252)
CP(1253) CP(1254) CP(1255)
CP(1256) CP(1257) CP(1258)
CP(437) CP(850) CP(852) CP(855)
#undef CP
default: return wxFONTENCODING_SYSTEM;
}}
#endif

wxFontEncoding GetLocalDefaultEncoding () {
#ifdef __WIN32
return GetEncodingFromCP(GetACP());
#else
return wxFONTENCODING_ISO8859_1;
#endif
}

wxFontEncoding GetLocalOemDefaultEncoding () {
#ifdef __WIN32
return GetEncodingFromCP(GetOEMCP());
#else
return wxFONTENCODING_CP437;
#endif
}

void AdjustLineEnding (wxString& text, int lineEnding) {
switch(lineEnding){
case LE_CRLF:
text.Replace("\r\n", "\x7F\x7F");
text.Replace("\n", "\x7F\x7F");
for (int i=1, n=LINE_ENDING_CHARS.size(); i<n; i++) text.Replace(LINE_ENDING_CHARS.substr(i,1), "\r\n");
text.Replace("\x7F\x7F", "\r\n");
break;
default:
auto nl = LINE_ENDING_CHARS.substr(lineEnding -1, 1);
text.Replace("\r\n", nl);
for (int i=0, n=LINE_ENDING_CHARS.size(); i<n; i++) {
if (i+1==lineEnding) continue;
text.Replace(LINE_ENDING_CHARS.substr(i,1), nl);
}
}}

static inline bool checkUTF8AndAdvance (const unsigned char*& c) {
if (*c<0x80) return true;
else if (*c<0xC2) return false;
else if (*c<0xE0 && *++c>=0x80 && *c<0xC0) return true;
else if (*c<0xF0 && *++c>=0x80 && *c<0xC0 && *++c>=0x80 && *c<0xC0) return true;
else if (*c<0xF5 && *++c>=0x80 && *c<0xC0 && *++c>=0x80 && *c<0xC0 && *++c>=0x80 && *c<0xC0) return true;
else return false;
}

int DetectEncoding (wxFile& file) {
unsigned char buf[4096];
int len = file.Read(buf, 4096);
file.Seek(0);
if (len>=3 && buf[0]==0xEF && buf[1]==0xBB && buf[2]==0xBF) return wxFONTENCODING_UTF8 | ENC_WITH_BOM;
else if (len>=2 && buf[0]==0xFF && buf[1]==0xFE) return wxFONTENCODING_UTF16LE | ENC_WITH_BOM;
else if (len>=2 && buf[0]==0xFE && buf[1]==0xFF) return wxFONTENCODING_UTF16BE | ENC_WITH_BOM;
for (const unsigned char *c = buf, *end = buf + std::min(len, 4000); c<end; c++) {
if (*c==0) return (c-buf)%2? wxFONTENCODING_UTF16BE : wxFONTENCODING_UTF16LE;
else if (*c<0x80) continue;
else if (*c<0xA0) return wxFONTENCODING_CP850;
else if (!checkUTF8AndAdvance(c)) return wxFONTENCODING_CP1252;
}
return wxFONTENCODING_UTF8;
}

int DetectLineEnding (const wxString& text) {
auto i = text.find_first_of(LINE_ENDING_CHARS);
if (text[i]=='\r' && i<text.size() -1 && text[i+1]=='\n') return LE_CRLF;
else return LINE_ENDING_CHARS.find(text[i]) +1;
}

int DetectIndentType (wxString& text) {
if (text.empty() || text[0]=='\t') return -4;
int indentSize = 2520;
for (size_t pos=0, len=std::min<size_t>(text.size(), 4096); pos<len && indentSize>1; ) {
size_t rbol = text.find_first_not_of(" \t", pos);
if (rbol==std::string::npos) break;
rbol -= pos;
if (rbol>0) indentSize = std::gcd(rbol, indentSize);
size_t eol = text.find_first_of("\r\n", pos);
if (eol==std::string::npos) break;
pos = text.find_first_not_of("\r\n", eol);
}
return indentSize > 10? 4 : indentSize;
}

bool ReadAndDecode (wxFile& file, wxString& text, int& encoding) {
initEncodings();
auto& enc = *encodingsByValue[encoding];
if (!enc.bom.empty()) {
char bom[enc.bom.size()];
file.Read(bom, enc.bom.size());
}
bool succeeded = file.ReadAll(&text, enc.GetCodec()) && (!text.empty() || file.Length()<=0);
if (!succeeded) {
encoding = GetLocalDefaultEncoding();
auto& defenc = *encodingsByValue[encoding];
file.Seek(0);
succeeded = file.ReadAll(&text, defenc.GetCodec());
}
return succeeded;
}

void EncodeAndWrite (wxFile& file, const wxString& text, int encoding) {
initEncodings();
auto& enc = *encodingsByValue[encoding];
if (!enc.bom.empty()) file.Write(enc.bom.data(), enc.bom.size());
file.Write(text, enc.GetCodec());
}

bool LoadFile (const wxString& filename, wxString& text, Properties& properties) {
wxFile file(filename, wxFile::OpenMode::read);
if (!file.IsOpened()) return false;
int encoding = GetEncoding(properties);
if (encoding==wxFONTENCODING_DEFAULT)  encoding = DetectEncoding(file);
if (!ReadAndDecode(file, text, encoding)) return false;
SetEncoding(properties, encoding);
if (GetLineEnding(properties)<0) SetLineEnding(properties, DetectLineEnding(text));
if (GetIndentType(properties)==0) SetIndentType(properties, DetectIndentType(text));
AdjustLineEnding(text, LE_LF);
return true;
}

bool SaveFile (const wxString& filename, wxString& text, const Properties& properties) {
if (properties.get("trim_trailing_whitespace", false)) wxRegEx("\\s*$", wxRE_NEWLINE) .Replace(&text, "");
if (properties.get("insert_final_newline", false) && text[text.size() -1]!='\n') text += '\n';
AdjustLineEnding(text, GetLineEnding(properties));
wxFile file(filename, wxFile::OpenMode::write);
if (!file.IsOpened()) return false;
EncodeAndWrite(file, text, GetEncoding(properties));
return true;
}
