#include "AgentTools.hpp"
#include "../app/App.hpp"
#include "../text/TextDocument.hpp"
#include "../text/FindReplaceDialog.hpp"
#include "../common/stringUtils.hpp"
#include "../common/println.hpp"
#include <wx/filename.h>
#include <wx/dir.h>
#include <wx/file.h>
#include <wx/regex.h>
#include<algorithm>
#include<set>

using json = nlohmann::json;

void LoadEditorConfig (Properties& properties, const wxString& filename);
bool LoadFile (const wxString& filename, wxString& text, Properties& properties);
wxRegEx GlobToRegEx (const wxString& glob);

#define DEFAULT_AGENT_IGNORED_DIRS ".git, .svn, .hg, node_modules, obj, objd, __pycache__"

static std::vector<wxString> SplitConfigList (const std::string& value) {
std::vector<wxString> result;
for (auto& s: split(value, ",; ", true)) if (!s.empty()) result.push_back(U(s));
return result;
}

ToolContext::ToolContext (const wxString& root0, const AgentLimits& limits0):
root(root0), limits(limits0)
{
wxFileName fn = wxFileName::DirName(root);
fn.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE);
rootDir = fn.GetFullPath();
auto& config = wxGetApp().GetConfig();
ignoredDirs = SplitConfigList(config.get("agent_ignored_dirs", DEFAULT_AGENT_IGNORED_DIRS));
excludedExtensions = SplitConfigList(config.get("workspace_file_list_exclude_exts", ""));
}

bool ToolContext::ResolvePath (const std::string& input, wxString& absPath, std::string& error) const {
wxString p = U(input);
p.Trim(true).Trim(false);
p.Replace("\\", "/");
bool absolute = (p.size()>=2 && p[1]==':') || p.StartsWith("//");
wxString full;
if (absolute) full = p;
else {
while (p.StartsWith("/")) p.Remove(0, 1);
full = rootDir + p;
}
wxFileName fn(full);
fn.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE);
absPath = fn.GetFullPath();
while (absPath.size()>1 && absPath.EndsWith(wxFILE_SEP_PATH)) absPath.RemoveLast();
wxString lowerAbs = absPath.Lower(), lowerRoot = rootDir.Lower();
if (lowerAbs + wxFILE_SEP_PATH == lowerRoot || lowerAbs.StartsWith(lowerRoot)) return true;
error = format("Path is outside the workspace: {}. Use paths relative to the workspace root.", input);
return false;
}

std::string ToolContext::GetRelativePath (const wxString& absPath, bool isDir) const {
wxString rel = absPath;
if (rel.Lower().StartsWith(rootDir.Lower())) rel = rel.Mid(rootDir.size());
else if ((rel + wxFILE_SEP_PATH).Lower() == rootDir.Lower()) rel.clear();
rel.Replace("\\", "/");
if (isDir && !rel.empty()) rel += "/";
return U(rel);
}

bool ToolContext::IsIgnoredDir (const wxString& name) const {
for (auto& d: ignoredDirs) if (d.IsSameAs(name, false)) return true;
return false;
}

bool ToolContext::IsExcludedFile (const wxString& name) const {
wxString ext = wxFileName(name).GetExt();
for (auto& e: excludedExtensions) if (e.IsSameAs(ext, false)) return true;
return false;
}

static std::string GetStringArg (const json& args, const std::string& key, const std::string& def = "") {
auto it = args.find(key);
if (it==args.end() || it->is_null()) return def;
if (it->is_string()) return it->get<std::string>();
throw std::invalid_argument(format("parameter {} must be a string", key));
}

static bool GetBoolArg (const json& args, const std::string& key, bool def) {
auto it = args.find(key);
if (it==args.end() || it->is_null()) return def;
if (it->is_boolean()) return it->get<bool>();
if (it->is_string()) return it->get<std::string>()=="true";
throw std::invalid_argument(format("parameter {} must be a boolean", key));
}

static bool GetIntArg (const json& args, const std::string& key, int& value) {
auto it = args.find(key);
if (it==args.end() || it->is_null()) return false;
if (it->is_number_integer()) value = it->get<int>();
else if (it->is_string()) value = std::stoi(it->get<std::string>());
else throw std::invalid_argument(format("parameter {} must be an integer", key));
return true;
}

// Split "src/**/*.cpp" into the directory part "src" and the file mask "**/*.cpp"
static void SplitPattern (const std::string& input, std::string& dir, std::string& mask) {
wxString p = U(input);
p.Replace("\\", "/");
auto parts = wxSplit(p, '/', 0);
size_t i = 0;
for (; i<parts.size(); i++) if (parts[i].find_first_of("*?[{")!=wxString::npos) break;
wxString d, m;
for (size_t j=0; j<parts.size(); j++) {
if (j<i) d += (j>0? "/" : "") + parts[j];
else m += (j>i? "/" : "") + parts[j];
}
dir = U(d);
mask = U(m);
}

static void ListDir (wxString dir, bool recursive, const ToolContext& context, std::vector<wxString>& dirs, std::vector<wxString>& files) {
wxLogNull noLog;
wxDir d(dir);
if (!d.IsOpened()) return;
if (!dir.EndsWith(wxFILE_SEP_PATH)) dir += wxFILE_SEP_PATH;
wxString name;
bool cont = d.GetFirst(&name, wxEmptyString, wxDIR_FILES | wxDIR_DIRS | wxDIR_HIDDEN);
while (cont) {
wxString path = dir + name;
if (wxDirExists(path)) {
if (!context.IsIgnoredDir(name)) {
dirs.push_back(path);
if (recursive) ListDir(path, true, context, dirs, files);
}}
else files.push_back(path);
cont = d.GetNext(&name);
}}

static bool IsBinaryFile (const wxString& filename) {
wxFile file(filename, wxFile::OpenMode::read);
if (!file.IsOpened()) return false;
unsigned char buf[8192];
ssize_t n = file.Read(buf, sizeof(buf));
if (n<=0) return false;
if (n>=2 && ((buf[0]==0xFF && buf[1]==0xFE) || (buf[0]==0xFE && buf[1]==0xFF))) return false;
if (n>=3 && buf[0]==0xEF && buf[1]==0xBB && buf[2]==0xBF) return false;
return std::find(buf, buf+n, 0)!=buf+n;
}

static wxTextCtrlIface* FindOpenEditor (const wxString& filename) {
auto doc = dynamic_cast<TextDocument*>(wxGetApp().FindDocument(filename));
return doc? doc->GetEditor() : nullptr;
}

static bool ReadTextFile (const wxString& filename, wxString& text) {
auto editor = FindOpenEditor(filename);
if (editor) {
text = editor->GetValue();
text.Replace("\r\n", "\n");
return true;
}
Properties properties;
LoadEditorConfig(properties, filename);
return LoadFile(filename, text, properties);
}

static bool CompareCaseInsensitive (const wxString& a, const wxString& b) {
return a.CmpNoCase(b)<0;
}

// Collect the files and directories designated by a path with an optional mask, relative to the base directory
struct FileSelection {
wxString base, singleFile;
std::vector<wxString> dirs, files;
};

static bool SelectFiles (const std::string& path, bool recursive, bool includeDirs, const ToolContext& context, FileSelection& sel, std::string& error) {
std::string dirPart, mask;
SplitPattern(path, dirPart, mask);
if (!context.ResolvePath(dirPart, sel.base, error)) return false;
if (mask.empty()) {
if (wxFileExists(sel.base)) {
sel.singleFile = sel.base;
return true;
}
mask = "*";
}
if (!wxDirExists(sel.base)) {
error = format("Directory not found: {}. Use list_files with an empty path to list the workspace root.", dirPart);
return false;
}
if (mask.find("**")!=std::string::npos) recursive = true;
std::vector<wxString> dirs, files;
ListDir(sel.base, recursive, context, dirs, files);
wxRegEx reg = GlobToRegEx(U(mask));
wxString base = sel.base;
if (!base.EndsWith(wxFILE_SEP_PATH)) base += wxFILE_SEP_PATH;
auto matches = [&](const wxString& p){
wxString rel = p.Mid(base.size());
rel.Replace("\\", "/");
return reg.Matches(rel);
};
if (includeDirs) for (auto& d: dirs) if (matches(d)) sel.dirs.push_back(d);
for (auto& f: files) if (matches(f)) sel.files.push_back(f);
std::sort(sel.dirs.begin(), sel.dirs.end(), CompareCaseInsensitive);
std::sort(sel.files.begin(), sel.files.end(), CompareCaseInsensitive);
return true;
}

static ToolResult ToolListFiles (const json& args, const ToolContext& context) {
std::string path = GetStringArg(args, "path");
bool recursive = GetBoolArg(args, "recursive", false);
FileSelection sel;
std::string error;
if (!SelectFiles(path, recursive, true, context, sel, error)) return ToolResult::Error(error);
std::vector<std::string> entries;
if (!sel.singleFile.empty()) entries.push_back(context.GetRelativePath(sel.singleFile));
bool recursiveListing = recursive || path.find("**")!=std::string::npos;
if (!recursiveListing) for (auto& d: sel.dirs) entries.push_back(context.GetRelativePath(d, true));
for (auto& f: sel.files) entries.push_back(context.GetRelativePath(f));
if (entries.empty()) return ToolResult::Ok(format("No files match: {}", path.empty()? "/" : path));
std::string text;
int max = context.limits.listMaxEntries;
for (int i=0, n=entries.size(); i<n && i<max; i++) text += entries[i] + "\n";
if (entries.size()>static_cast<size_t>(max)) text += format("[Truncated: {} more entries not shown. Use a more specific path or pattern.]\n", entries.size() -max);
return ToolResult::Ok(text);
}

static ToolResult ToolReadFile (const json& args, const ToolContext& context) {
std::string path = GetStringArg(args, "path"), error;
if (path.empty()) return ToolResult::Error("path is required.");
wxString file;
if (!context.ResolvePath(path, file, error)) return ToolResult::Error(error);
std::string rel = context.GetRelativePath(file);
if (wxDirExists(file)) return ToolResult::Error(format("{} is a directory. Use list_files to see its content.", rel));
if (!wxFileExists(file)) return ToolResult::Error(format("File not found: {}. Use list_files to see available files.", rel));
if (!FindOpenEditor(file) && IsBinaryFile(file)) return ToolResult::Error(format("{} is a binary file and can't be read as text.", rel));
wxString text;
if (!ReadTextFile(file, text)) return ToolResult::Error(format("Unable to read {}.", rel));
auto lines = wxSplit(text, '\n', 0);
if (!lines.empty() && lines.back().empty()) lines.pop_back();
int total = lines.size();
if (total==0) return ToolResult::Ok(format("File: {} (empty)\n", rel));
int start = 1, end = total;
GetIntArg(args, "start_line", start);
GetIntArg(args, "end_line", end);
if (start<0) start = total + start +1;
if (end<0) end = total + end +1;
if (start<1) start = 1;
if (end>total) end = total;
if (start>total) return ToolResult::Error(format("start_line {} is beyond the end of the file ({} lines).", start, total));
if (end<start) return ToolResult::Error(format("end_line ({}) must be greater than or equal to start_line ({}).", end, start));
int shownEnd = std::min(end, start + context.limits.readMaxLines -1);
std::string result = format("File: {} (lines {}-{} of {})\n", rel, start, shownEnd, total);
for (int i=start; i<=shownEnd; i++) {
wxString& line = lines[i -1];
if (line.EndsWith("\r")) line.RemoveLast();
result += format("{}\t{}\n", i, U(line));
}
if (shownEnd<end) result += format("[Truncated at {} lines. Use start_line={} to continue.]\n", context.limits.readMaxLines, shownEnd+1);
return ToolResult::Ok(result);
}

static ToolResult ToolSearchFiles (const json& args, const ToolContext& context) {
std::string pattern = GetStringArg(args, "pattern"), path = GetStringArg(args, "path"), error;
bool regex = GetBoolArg(args, "regex", false);
if (pattern.empty()) return ToolResult::Error("pattern is required.");
FindReplaceInfo info(FRI_FIND | (regex? FRI_REGEX : 0), U(pattern));
try {
info.CreateRegEx();
} catch (std::exception& e) {
return ToolResult::Error(format("Invalid regular expression: {}", e.what()));
}
FileSelection sel;
if (!SelectFiles(path, true, false, context, sel, error)) return ToolResult::Error(error);
std::vector<wxString> files = sel.files;
if (!sel.singleFile.empty()) files.push_back(sel.singleFile);
std::vector<std::string> lines;
size_t nFiles = 0;
for (auto& file: files) {
bool open = FindOpenEditor(file);
if (!open && sel.singleFile.empty() && (context.IsExcludedFile(file) || IsBinaryFile(file))) continue;
wxString text;
if (!ReadTextFile(file, text)) continue;
std::vector<FindResultInfo> results;
info.FindAllInText(results, text, file, 0, text.size());
if (results.empty()) continue;
nFiles++;
std::string rel = context.GetRelativePath(file);
size_t lastLine = std::string::npos;
for (auto& r: results) {
if (r.y==lastLine) continue;
lastLine = r.y;
wxString lineText = r.lineText;
lineText.Trim(false).Trim(true);
if (static_cast<int>(lineText.size())>context.limits.searchMaxLineLength) lineText = lineText.Left(context.limits.searchMaxLineLength) + wxString::FromUTF8("…");
lines.push_back(format("{}:{}: {}", rel, r.y+1, U(lineText)));
}}
if (lines.empty()) return ToolResult::Ok(format("No matches for '{}' in {}", pattern, path.empty()? "/" : path));
std::string text;
int max = context.limits.searchMaxResults;
for (int i=0, n=lines.size(); i<n && i<max; i++) text += lines[i] + "\n";
if (lines.size()>static_cast<size_t>(max)) text += format("[{} more matches not shown. Use a more specific path or pattern.]\n", lines.size() -max);
text += format("[{} matches in {} files]\n", lines.size(), nFiles);
return ToolResult::Ok(text);
}

static void RegisterBuiltinTools (std::vector<AgentTool>& tools) {
tools.push_back({ "list_files",
"List files and directories in the workspace. Directories end with '/'. Paths are relative to the workspace root and use '/' as separator. Not recursive unless recursive is true or the pattern contains '**'.",
json::parse(R"({
"type": "object",
"properties": {
"path": { "type": "string", "description": "Directory to list, optionally followed by a file name pattern with wildcards * and ?, for example 'src/', 'src/*.cpp' or 'src/**/*.hpp'. Empty for the workspace root." },
"recursive": { "type": "boolean", "description": "List files in subdirectories too. Default: false." }
}
})"),
ToolListFiles
});
tools.push_back({ "read_file",
"Read a text file from the workspace. Returns a header line followed by the requested lines, each prefixed by its line number and a tab. Line numbers are not part of the file content. Long files are truncated: use start_line and end_line to read other parts.",
json::parse(R"({
"type": "object",
"properties": {
"path": { "type": "string", "description": "Path of the file, relative to the workspace root." },
"start_line": { "type": "integer", "description": "First line to read, starting at 1. Negative values count from the end: -1 is the last line. Default: 1." },
"end_line": { "type": "integer", "description": "Last line to read, inclusive. Negative values count from the end. Default: end of the file." }
},
"required": ["path"]
})"),
ToolReadFile
});
tools.push_back({ "search_files",
"Search for text in workspace files, case-insensitively. The search is always recursive. Returns one line per match in the form path:line: text.",
json::parse(R"({
"type": "object",
"properties": {
"pattern": { "type": "string", "description": "Text to search for, or a regular expression (PCRE syntax) if regex is true." },
"path": { "type": "string", "description": "Directory or file to search in, optionally followed by a file name pattern, for example 'src/' or 'src/*.cpp'. Empty for the whole workspace." },
"regex": { "type": "boolean", "description": "Whether pattern is a regular expression. Default: false." }
},
"required": ["pattern"]
})"),
ToolSearchFiles
});
}

std::vector<AgentTool>& AgentTools::GetAll () {
static std::vector<AgentTool> tools;
static bool initialized = false;
if (!initialized) {
initialized = true;
RegisterBuiltinTools(tools);
}
return tools;
}

void AgentTools::Register (const AgentTool& tool) {
auto& tools = GetAll();
for (auto& t: tools) {
if (t.name==tool.name) {
t = tool;
return;
}}
tools.push_back(tool);
}

const AgentTool* AgentTools::Find (const std::string& name) {
for (auto& t: GetAll()) if (t.name==name) return &t;
return nullptr;
}

json AgentTools::GetDefinitions (const AgentConfig& config) {
json defs = json::array();
for (auto& t: GetAll()) {
if (!config.IsToolEnabled(t.name)) continue;
defs.push_back({ {"type", "function"}, {"function", { {"name", t.name}, {"description", t.description}, {"parameters", t.parameters} }} });
}
return defs;
}

ToolResult AgentTools::Execute (const std::string& name, const std::string& arguments, const ToolContext& context) {
ToolResult result;
auto tool = Find(name);
if (!tool) {
std::string names;
for (auto& t: GetAll()) names += (names.empty()? "" : ", ") + t.name;
return ToolResult::Error(format("Unknown tool: {}. Available tools: {}", name, names));
}
json args;
try {
args = arguments.empty()? json::object() : json::parse(arguments);
} catch (std::exception& e) {
return ToolResult::Error(format("Invalid JSON arguments: {}", e.what()));
}
if (args.is_null()) args = json::object();
if (!args.is_object()) return ToolResult::Error("Arguments must be a JSON object.");
try {
result = tool->handler(args, context);
} catch (std::exception& e) {
result = ToolResult::Error(e.what());
}
size_t max = context.limits.maxResultChars;
if (result.text.size()>max) {
size_t cut = max;
while (cut>0 && (static_cast<unsigned char>(result.text[cut]) & 0xC0)==0x80) cut--;
result.text = result.text.substr(0, cut) + "\n[Truncated: result too long.]\n";
}
return result;
}
