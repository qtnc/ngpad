#include "App.hpp"
#include "../common/Properties.hpp"
#include "../common/wxUtils.hpp"
#include <wx/regex.h>
#include <wx/filename.h>
#include <wx/stream.h>
#include <wx/stdstream.h>
#include <wx/wfstream.h>
#include<sstream>
#include "../common/stringUtils.hpp"

const std::vector<std::string> STANDARD_EDITORCONFIG_KEYS = {
"charset",
"indent_style", "indent_size",
"line_ending", "insert_final_newline", "trim_trailing_whitespace"
};

wxRegEx GlobToRegEx (const wxString& glob) {
wxString r("^(?:.*[/\\\\])?"); 
int braceLevel = 0;
for (size_t i=0, n=glob.size(); i<n; i++) {
auto c = glob[i];
switch(static_cast<char>(c)) {
case '*':
if (i+1<n && glob[i+1]=='*') {
r += ".*";
i++;
}
else r += "[^/\\\\]*";
break;
case '?':
r += '.';
break;
case '{':
r += "(?:";
braceLevel++;
break;
case '}':
r += ')';
braceLevel--;
break;
case ',':
r += (braceLevel>0? '|' : ',');
break;
case ';':
r += '|';
break;
case '[':
r += '[';
if (i+1<n && glob[i]=='!') {
r += '^';
i++;
}
break;
case '.':  case '+': 
case '^': case '$':
case '(': case ')':
case '\\': case '|':
r += '\\'; 
[[fallthrough]];
default: 
r+=c; 
break;
}}
r += '$';
return wxRegEx(r, wxRE_ICASE);
}

bool MatchGlob (const wxString& pattern, const wxString& filename) {
wxRegEx reg = GlobToRegEx(pattern);
return reg.Matches(filename);
}


void includeEditorconfigFile (Properties& properties, const wxString& documentFile, const wxString& configFile, const wxString& fileToInclude, bool replace);

void readEditorconfigFile (Properties& properties, std::istream& in, const wxString& documentFile, const wxString& configFile, bool replace, bool skip) {
std::string line;
while (std::getline(in, line)) {
trim(line); to_lower(line);
if (line.empty() || line[0]=='#' || line[0]==';') continue;
if (line[0]=='[' && line[line.size() -1]==']') {
line.erase(line.begin()); line.erase(line.end() -1); trim(line);
skip = !MatchGlob(U(line), documentFile);
continue;
}
if (skip) continue;

auto i = line.find('=');
if (i==std::string::npos) continue;
std::string key = line.substr(0, i), value = line.substr(i+1);
while (ends_with(value, "\\") && std::getline(in, line)) {
trim(line);
value.pop_back();
value += '\n';
value += line;
}
unescape(value);
trim(key); trim(value);
to_lower(key); to_lower(value);
if (starts_with(value, "\\@")) value = value.substr(2);

if (key=="root" && value=="true") {
for (auto& k: STANDARD_EDITORCONFIG_KEYS) properties.remove(k);
continue;
}
if (key=="include") {
includeEditorconfigFile(properties, documentFile, configFile, U(value), replace);
continue;
}
if (key=="plugin") {
std::string oldValue = properties.get(key, "");
if (!oldValue.empty()) value = oldValue + ';' + value;
}

properties.put(key, value, replace);
}}

void includeEditorconfigFile (Properties& properties, const wxString& documentFile, const wxString& configFile, const wxString& fileToInclude, bool replace) {
wxFileName file(fileToInclude);
if (file.IsRelative()) {
wxString path;
wxFileName::SplitPath(configFile, &path, nullptr, nullptr);
file.MakeAbsolute(path);
}
if (!file.Exists()) return;
wxFileInputStream fIn(file.GetFullPath());
wxBufferedInputStream bIn(fIn);
wxStdInputStream in(bIn);
readEditorconfigFile(properties, in, documentFile, file.GetFullPath(), replace, false);
}

void LoadEditorConfig (Properties& properties, const wxString& filename) {
wxString sep = wxFileName::GetPathSeparator();
wxString path, oldPath = filename.Lower();
std::vector<wxString> files;

do {
wxFileName::SplitPath(oldPath, &path, nullptr, nullptr);
if (path.empty() || path==oldPath) break;
oldPath = path;
if (path[path.size() -1]!=sep) path += sep;
path += EDITORCONFIG_FILENAME;
if (wxFileExists(path)) files.push_back(path);
} while(true);
auto configFile = wxGetApp() .FindAppFile(EDITORCONFIG_ROOT_FILENAME);
if (!configFile.empty()) files.push_back(configFile);
for (int i=files.size() -1; i>=0; i--) {
auto& file = files[i];
wxFileInputStream fIn(file);
wxBufferedInputStream bIn(fIn);
wxStdInputStream in(bIn);
readEditorconfigFile(properties, in, filename, file, true, true);
}
}

