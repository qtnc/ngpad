#include "AgentTools.hpp"
#include "../app/App.hpp"
#include "../text/TextDocument.hpp"
#include "../text/FindReplaceDialog.hpp"
#include "../text/TextEditor.hpp"
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
bool SaveFile (const wxString& filename, wxString& text, const Properties& properties);
int GetEncodingFromName (const std::string& name);
int GetLineEndingFromName (const std::string& name);
int GetEncoding (const Properties& properties);
int GetLineEnding (const Properties& properties);
void SetEncoding (Properties& properties, int encoding);
void SetLineEnding (Properties& properties, int lineEnding);
wxRegEx GlobToRegEx (const wxString& glob);
bool MatchGlob (const wxString& pattern, const wxString& filename);

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

// Absolute normalized path (native separators, no trailing separator) from a path relative to the root, or absolute.
// '/' and '\' are both accepted; a single leading '/' designates the root, while 'X:/' and '//server/share' are absolute.
static wxString MakeAbsolutePath (const wxString& input, const wxString& rootDir) {
wxString p = input;
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
wxString absPath = fn.GetFullPath();
while (absPath.size()>1 && absPath.EndsWith(wxFILE_SEP_PATH)) absPath.RemoveLast();
return absPath;
}

static wxString ToSlashes (wxString path) {
path.Replace("\\", "/");
return path;
}

void ToolContext::SetAllowedPaths (const std::vector<std::string>& readable, const std::vector<std::string>& writable) {
auto convert = [&](const std::vector<std::string>& entries, std::vector<AllowedPath>& result){
result.clear();
for (auto& entry: entries) {
wxString e = U(entry);
bool isPattern = e.find_first_of("*?[{")!=wxString::npos;
result.push_back({ ToSlashes(MakeAbsolutePath(e, rootDir)), isPattern });
}};
convert(readable, readablePaths);
convert(writable, writablePaths);
}

bool ToolContext::IsInWorkspace (const wxString& absPath) const {
wxString lowerAbs = absPath.Lower(), lowerRoot = rootDir.Lower();
return lowerAbs + wxFILE_SEP_PATH == lowerRoot || lowerAbs.StartsWith(lowerRoot);
}

static bool IsUnder (const wxString& path, const wxString& dir) {
wxString p = path.Lower(), d = dir.Lower();
return p==d || p.StartsWith(d.EndsWith("/")? d : d + "/");
}

static bool MatchesAllowedPath (const wxString& slashPath, const std::vector<AllowedPath>& entries) {
for (auto& e: entries) {
if (e.isPattern? MatchGlob(e.path, slashPath) : IsUnder(slashPath, e.path)) return true;
}
return false;
}

bool ToolContext::IsAllowedOutside (const wxString& absPath, PathAccess access) const {
if (access==PathAccess::RootOnly) return false;
wxString p = ToSlashes(absPath);
if (MatchesAllowedPath(p, writablePaths)) return true;
if (grants) {
std::lock_guard<std::mutex> lock(grants->mutex);
if (MatchesAllowedPath(p, grants->writePaths)) return true;
if (access==PathAccess::Read && MatchesAllowedPath(p, grants->readPaths)) return true;
}
if (access!=PathAccess::Read) return false;
if (MatchesAllowedPath(p, readablePaths)) return true;
for (auto& a: approvedReads) if (IsUnder(p, ToSlashes(a))) return true;
return false;
}

// Directory proposed to allow access to a path for the session: the path itself if it is a directory, else its parent
static wxString GetGrantDirectory (const wxString& absPath) {
if (wxDirExists(absPath)) return absPath;
return wxFileName(absPath).GetPath();
}

bool ToolContext::ResolvePath (const std::string& input, wxString& absPath, std::string& error, PathAccess access) const {
absPath = MakeAbsolutePath(U(input), rootDir);
if (IsInWorkspace(absPath)) return true;
if (access==PathAccess::RootOnly) {
error = format("Path is outside the workspace: {}. Use paths relative to the workspace root.", input);
return false;
}
if (IsAllowedOutside(absPath, access)) return true;
// Writing: the tool asks for a confirmation anyway, which becomes mandatory
if (access==PathAccess::Write) {
if (!outsideWorkspace) outsidePath = absPath;
outsideWorkspace = true;
return true;
}
std::string shown = GetRelativePath(absPath);
ConfirmRequest request;
request.message = U(format(GetTranslation("AgentConfirmReadOutside"), shown));
request.grant = GrantKind::ReadPath;
request.grantValue = U(GetRelativePath(GetGrantDirectory(absPath), true));
auto answer = Ask(request);
if (answer==ToolConfirmation::Allow) {
approvedReads.push_back(absPath);
return true;
}
if (answer==ToolConfirmation::Stop) {
stopped = true;
error = "The user stopped the agent.";
}
else error = format("The user denied access to {}, which is outside the workspace. Don't retry: work inside the workspace, or ask the user what to do.", shown);
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

ToolConfirmation ToolContext::Confirm (const wxString& message, bool denyByDefault, const wxString& details, GrantKind grant, const wxString& grantValue) const {
ConfirmRequest request { message, details, denyByDefault, grant, grantValue };
if (outsideWorkspace) {
// Outside the workspace, the user can allow the directory, not the tool or the command
request.message += "\n" + MSG("AgentOutsideWorkspace");
request.denyByDefault = true;
if (grant!=GrantKind::None) {
request.grant = GrantKind::WritePath;
request.grantValue = U(GetRelativePath(GetGrantDirectory(outsidePath), true));
}
return Ask(request);
}
if (permission==ToolPermission::Allow) return ToolConfirmation::Allow;
if (grant==GrantKind::Tool) {
request.grantValue = U(toolName);
if (grants) {
std::lock_guard<std::mutex> lock(grants->mutex);
if (grants->tools.count(toolName)) return ToolConfirmation::Allow;
}}
return Ask(request);
}

ToolConfirmation ToolContext::Ask (ConfirmRequest& request) const {
if (!confirm) return ToolConfirmation::Deny;
auto answer = RunEDTSync([&](){ return confirm(request); });
if (answer==ToolConfirmation::AllowSession || answer==ToolConfirmation::AllowAlways) {
if (request.grant!=GrantKind::None) AddGrant(request);
return ToolConfirmation::Allow;
}
return answer;
}

void ToolContext::AddGrant (const ConfirmRequest& request) const {
if (!grants) return;
wxString value = request.grantValue;
value.Trim(true).Trim(false);
if (value.empty()) return;
std::lock_guard<std::mutex> lock(grants->mutex);
switch(request.grant){
case GrantKind::Tool: grants->tools.insert(U(value)); break;
case GrantKind::Command: grants->commands.push_back(U(value)); break;
case GrantKind::ReadPath:
case GrantKind::WritePath: {
AllowedPath p { ToSlashes(MakeAbsolutePath(value, rootDir)), value.find_first_of("*?[{")!=wxString::npos };
(request.grant==GrantKind::ReadPath? grants->readPaths : grants->writePaths).push_back(p);
break;
}
default: break;
}
println("Agent grant for the session: {}", U(value));
}

bool ToolContext::IsCommandAllowed (const std::string& command) const {
if (IsAllowedCommand(command, allowedCommands)) return true;
if (!grants) return false;
std::lock_guard<std::mutex> lock(grants->mutex);
return IsAllowedCommand(command, grants->commands);
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

static wxString NormalizePath (const wxString& path) {
wxFileName fn(path);
fn.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE);
return fn.GetFullPath();
}

// Main thread only
static wxTextCtrlIface* FindOpenEditor (const wxString& filename) {
auto doc = dynamic_cast<TextDocument*>(wxGetApp().FindDocument(filename));
return doc? doc->GetEditor() : nullptr;
}

// Functions below can be called from any thread: documents and editors are only accessed on the main thread

static bool IsOpenInEditor (const wxString& filename) {
return RunEDTSync([&](){ return FindOpenEditor(filename)!=nullptr; });
}

// Normalized lower case paths of the documents open in the editor
static std::set<wxString> GetOpenFiles () {
return RunEDTSync([](){
std::set<wxString> files;
for (auto wxdoc: wxGetApp().GetDocManager()->GetDocumentsVector()) {
if (!wxdoc->GetFilename().empty()) files.insert(NormalizePath(wxdoc->GetFilename()).Lower());
}
return files;
});
}

// checkEditor: false when the file is known not to be open, to avoid a round trip to the main thread
static bool ReadTextFile (const wxString& filename, wxString& text, bool checkEditor = true) {
bool open = checkEditor && RunEDTSync([&](){
auto editor = FindOpenEditor(filename);
if (editor) text = editor->GetValue();
return editor!=nullptr;
});
if (open) {
text.Replace("\r\n", "\n");
return true;
}
Properties properties;
LoadEditorConfig(properties, filename);
return LoadFile(filename, text, properties);
}

// Write the new content of a file: in the editor if the file is open (without saving), otherwise on disk
static bool WriteTextFile (const wxString& filename, wxString text) {
int written = RunEDTSync([&](){
auto iface = FindOpenEditor(filename);
if (!iface) return -1;
auto editor = dynamic_cast<TextEditor*>(iface);
if (!editor) return 0;
auto [start, end] = editor->GetSelection();
editor->ReplaceAndPushUndoState(0, editor->GetLastPosition(), text);
long last = editor->GetLastPosition();
editor->SetSelection(std::min(start, last), std::min(end, last));
return 1;
});
if (written>=0) return written>0;
Properties properties;
LoadEditorConfig(properties, filename);
if (wxFileExists(filename)) {
// Keep the encoding and line ending of the existing file
wxString oldText;
if (!LoadFile(filename, oldText, properties)) return false;
}
else {
auto& config = wxGetApp().GetConfig();
if (GetEncoding(properties)==wxFONTENCODING_DEFAULT) SetEncoding(properties, GetEncodingFromName(config.get("default_charset", "utf-8")));
if (GetLineEnding(properties)<0) SetLineEnding(properties, GetLineEndingFromName(config.get("default_line_ending", "crlf")));
wxLogNull noLog;
wxFileName::Mkdir(wxFileName(filename).GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
}
return SaveFile(filename, text, properties);
}

static wxString GetWriteLocation (const wxString& filename) {
return IsOpenInEditor(filename)? MSG("AgentInEditor") : MSG("AgentOnDisk");
}

static ToolResult RefusedResult (ToolConfirmation answer) {
if (answer==ToolConfirmation::Stop) return ToolResult::Stop();
return ToolResult::Error("The user denied this change. Don't retry it: ask the user what to do instead.");
}

static size_t CountOccurrences (const wxString& text, const wxString& sub) {
size_t count = 0;
for (size_t pos = text.find(sub); pos!=wxString::npos; pos = text.find(sub, pos + sub.size())) count++;
return count;
}

static bool CompareCaseInsensitive (const wxString& a, const wxString& b) {
return a.CmpNoCase(b)<0;
}

// Collect the files and directories designated by a path with an optional mask, relative to the base directory
struct FileSelection {
wxString base, singleFile;
std::vector<wxString> dirs, files;
};

static bool SelectFiles (const std::string& path, bool recursive, bool includeDirs, PathAccess access, const ToolContext& context, FileSelection& sel, std::string& error) {
std::string dirPart, mask;
SplitPattern(path, dirPart, mask);
if (!context.ResolvePath(dirPart, sel.base, error, access)) return false;
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
if (!SelectFiles(path, recursive, true, PathAccess::Read, context, sel, error)) return ToolResult::Error(error);
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
if (!context.ResolvePath(path, file, error, PathAccess::Read)) return ToolResult::Error(error);
std::string rel = context.GetRelativePath(file);
if (wxDirExists(file)) return ToolResult::Error(format("{} is a directory. Use list_files to see its content.", rel));
if (!wxFileExists(file)) return ToolResult::Error(format("File not found: {}. Use list_files to see available files.", rel));
if (!IsOpenInEditor(file) && IsBinaryFile(file)) return ToolResult::Error(format("{} is a binary file and can't be read as text.", rel));
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
bool regex = GetBoolArg(args, "regex", false), ignoreCase = GetBoolArg(args, "ignore_case", true);
if (pattern.empty()) return ToolResult::Error("pattern is required.");
// FRI_ICASE actually means case-sensitive
FindReplaceInfo info(FRI_FIND | (regex? FRI_REGEX : 0) | (ignoreCase? 0 : FRI_ICASE), U(pattern));
try {
info.CreateRegEx();
} catch (std::exception& e) {
return ToolResult::Error(format("Invalid regular expression: {}", e.what()));
}
FileSelection sel;
if (!SelectFiles(path, true, false, PathAccess::Read, context, sel, error)) return ToolResult::Error(error);
std::vector<wxString> files = sel.files;
if (!sel.singleFile.empty()) files.push_back(sel.singleFile);
std::vector<std::string> lines;
size_t nFiles = 0;
auto openFiles = GetOpenFiles();
for (auto& file: files) {
if (context.IsCancelled()) return ToolResult::Error("Cancelled.");
bool open = openFiles.count(file.Lower());
if (!open && sel.singleFile.empty() && (context.IsExcludedFile(file) || IsBinaryFile(file))) continue;
wxString text;
if (!ReadTextFile(file, text, open)) continue;
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

// In a replacement of a non-regex search, $ and \ have no special meaning
static wxString EscapeReplacement (const wxString& s) {
wxString result;
for (auto c: s) {
if (c=='$' || c=='\\') result += c;
result += c;
}
return result;
}

static ToolResult ToolReplaceInFiles (const json& args, const ToolContext& context) {
std::string pattern = GetStringArg(args, "pattern"), replacement = GetStringArg(args, "replacement"), path = GetStringArg(args, "path"), error;
bool regex = GetBoolArg(args, "regex", false), ignoreCase = GetBoolArg(args, "ignore_case", false);
if (pattern.empty()) return ToolResult::Error("pattern is required.");
if (args.find("replacement")==args.end()) return ToolResult::Error("replacement is required. Use an empty string to delete the matches.");
wxString repl = U(replacement);
repl.Replace("\r\n", "\n");
FindReplaceInfo info(FRI_FIND | FRI_REPLACE | (regex? FRI_REGEX : 0) | (ignoreCase? 0 : FRI_ICASE), U(pattern), regex? repl : EscapeReplacement(repl));
try {
info.CreateRegEx();
} catch (std::exception& e) {
return ToolResult::Error(format("Invalid regular expression: {}", e.what()));
}
FileSelection sel;
if (!SelectFiles(path, true, false, PathAccess::Write, context, sel, error)) return ToolResult::Error(error);
std::vector<wxString> files = sel.files;
if (!sel.singleFile.empty()) files.push_back(sel.singleFile);
struct FileChange { wxString file; std::string rel; bool open; wxString newText; int count; };
std::vector<FileChange> changes;
std::vector<std::string> samples;
int total = 0;
auto openFiles = GetOpenFiles();
try {
for (auto& file: files) {
if (context.IsCancelled()) return ToolResult::Error("Cancelled.");
bool open = openFiles.count(file.Lower());
if (!open && sel.singleFile.empty() && (context.IsExcludedFile(file) || IsBinaryFile(file))) continue;
wxString text;
if (!ReadTextFile(file, text, open)) continue;
std::vector<FindResultInfo> results;
info.FindAllInText(results, text, file, 0, text.size());
if (results.empty()) continue;
std::string rel = context.GetRelativePath(file);
for (size_t i=0; i<results.size() && samples.size()<5; i++) {
auto& r = results[i];
samples.push_back(format("{}:{}: {} -> {}", rel, r.y+1, U(r.foundText), U(r.replacementText)));
}
wxString newText = text;
int count = info.ReplaceAllInText(newText, 0);
if (count<=0 || newText==text) continue;
changes.push_back({ file, rel, open, newText, count });
total += count;
}
} catch (std::exception& e) {
return ToolResult::Error(format("Invalid replacement: {}", e.what()));
}
if (changes.empty()) return ToolResult::Ok(format("No matches for '{}' in {}: nothing was replaced.", pattern, path.empty()? "/" : path));
std::string details = GetTranslation("AgentReplaceFiles") + "\n";
for (auto& c: changes) details += format("{} ({}, {})\n", c.rel, c.count, U(c.open? MSG("AgentInEditor") : MSG("AgentOnDisk")));
details += "\n" + GetTranslation("AgentReplaceExamples") + "\n";
for (auto& s: samples) details += s + "\n";
auto answer = context.Confirm(U(format(GetTranslation("AgentConfirmReplace"), total, changes.size(), pattern, replacement)), false, U(details));
if (answer!=ToolConfirmation::Allow) return RefusedResult(answer);
std::string result, failed;
int replaced = 0, nFiles = 0;
for (auto& c: changes) {
if (WriteTextFile(c.file, c.newText)) {
result += format("{}: {}\n", c.rel, c.count);
replaced += c.count;
nFiles++;
}
else failed += c.rel + "\n";
}
result = format("Replaced {} occurrence{} in {} file{}:\n", replaced, replaced>1? "s" : "", nFiles, nFiles>1? "s" : "") + result;
if (!failed.empty()) result += "Unable to write these files:\n" + failed;
return ToolResult::Ok(result);
}

static ToolResult ToolEditFile (const json& args, const ToolContext& context) {
std::string path = GetStringArg(args, "path"), error;
wxString oldText = U(GetStringArg(args, "old_text")), newText = U(GetStringArg(args, "new_text"));
bool replaceAll = GetBoolArg(args, "replace_all", false);
if (path.empty()) return ToolResult::Error("path is required.");
if (oldText.empty()) return ToolResult::Error("old_text is required. Use write_file to create a file or to replace its whole content.");
wxString file;
if (!context.ResolvePath(path, file, error, PathAccess::Write)) return ToolResult::Error(error);
std::string rel = context.GetRelativePath(file);
if (wxDirExists(file)) return ToolResult::Error(format("{} is a directory. Give the path of a file.", rel));
bool open = IsOpenInEditor(file);
if (!open && !wxFileExists(file)) return ToolResult::Error(format("File not found: {}. Use write_file to create a new file.", rel));
if (!open && IsBinaryFile(file)) return ToolResult::Error(format("{} is a binary file and can't be edited as text.", rel));
wxString text;
if (!ReadTextFile(file, text)) return ToolResult::Error(format("Unable to read {}.", rel));
oldText.Replace("\r\n", "\n");
newText.Replace("\r\n", "\n");
if (oldText==newText) return ToolResult::Error("old_text and new_text are identical: nothing to change.");
size_t count = CountOccurrences(text, oldText);
if (count==0) return ToolResult::Error(format("old_text not found in {}. It must match the file content exactly, including whitespace and indentation, without line numbers. Read the file again with read_file: it may have changed.", rel));
if (count>1 && !replaceAll) return ToolResult::Error(format("old_text found {} times in {}. Include more surrounding lines to make it unique, or set replace_all to true to replace all occurrences.", count, rel));
auto answer = context.Confirm(U(format(GetTranslation("AgentConfirmEdit"), rel, U(GetWriteLocation(file)))));
if (answer!=ToolConfirmation::Allow) return RefusedResult(answer);
text.Replace(oldText, newText, replaceAll);
if (!WriteTextFile(file, text)) return ToolResult::Error(format("Unable to write {}.", rel));
return ToolResult::Ok(format("Edited {}: {} replacement{}.", rel, count, count>1? "s" : ""));
}

static ToolResult ToolWriteFile (const json& args, const ToolContext& context) {
std::string path = GetStringArg(args, "path"), error;
if (path.empty()) return ToolResult::Error("path is required.");
if (args.find("content")==args.end()) return ToolResult::Error("content is required.");
wxString content = U(GetStringArg(args, "content"));
wxString file;
if (!context.ResolvePath(path, file, error, PathAccess::Write)) return ToolResult::Error(error);
std::string rel = context.GetRelativePath(file);
if (rel.empty() || wxDirExists(file)) return ToolResult::Error(format("{} is a directory. Give the path of a file.", rel.empty()? "/" : rel));
content.Replace("\r\n", "\n");
bool exists = IsOpenInEditor(file) || wxFileExists(file);
std::string message = exists? format(GetTranslation("AgentConfirmOverwrite"), rel, U(GetWriteLocation(file))) : format(GetTranslation("AgentConfirmCreate"), rel);
auto answer = context.Confirm(U(message));
if (answer!=ToolConfirmation::Allow) return RefusedResult(answer);
if (!WriteTextFile(file, content)) return ToolResult::Error(format("Unable to write {}.", rel));
size_t lines = std::count(content.begin(), content.end(), '\n');
if (!content.empty() && content.Last()!='\n') lines++;
return ToolResult::Ok(format("Wrote {}: {} line{}.", rel, lines, lines>1? "s" : ""));
}

// Open documents designating the given file, or located in the given directory
static std::vector<AbstractDocument*> FindOpenDocuments (const wxString& path, bool isDir) {
std::vector<AbstractDocument*> docs;
wxString lowerPath = path.Lower(), lowerDir = lowerPath + wxFILE_SEP_PATH;
for (auto wxdoc: wxGetApp().GetDocManager()->GetDocumentsVector()) {
if (wxdoc->GetFilename().empty()) continue;
wxString filename = NormalizePath(wxdoc->GetFilename()).Lower();
if (filename==lowerPath || (isDir && filename.StartsWith(lowerDir))) docs.push_back(static_cast<AbstractDocument*>(wxdoc));
}
return docs;
}

// Close documents without asking to save them
static void CloseDocuments (const std::vector<AbstractDocument*>& docs) {
if (docs.empty()) return;
auto& app = wxGetApp();
auto manager = app.GetDocManager();
// Closing the last document may exit the application
if (manager->GetDocuments().GetCount()<=docs.size()) app.CreateNewEmptyDocument();
for (auto doc: docs) {
doc->Modify(false);
manager->CloseDocument(doc, true);
}
app.UpdateTitle();
}

static std::string DescribeCursorAndSelection (wxTextCtrlIface* editor, int maxLines) {
long x = 0, y = 0, start = 0, end = 0;
editor->PositionToXY(editor->GetInsertionPoint(), &x, &y);
std::string result = format("Cursor: line {}, column {}\n", y+1, x+1);
editor->GetSelection(&start, &end);
if (start==end) return result + "Selection: none\n";
long sx = 0, sy = 0, ex = 0, ey = 0;
editor->PositionToXY(start, &sx, &sy);
editor->PositionToXY(end, &ex, &ey);
result += format("Selection: line {}, column {} to line {}, column {}:\n", sy+1, sx+1, ey+1, ex+1);
wxString text = editor->GetRange(start, end);
text.Replace("\r\n", "\n");
auto lines = wxSplit(text, '\n', 0);
for (int i=0, n=lines.size(); i<n && i<maxLines; i++) result += U(lines[i]) + "\n";
if (static_cast<int>(lines.size())>maxLines) result += format("[Selection truncated at {} lines. Use read_file to read the rest.]\n", maxLines);
return result;
}

static ToolResult ToolGetEditorContext (const json&, const ToolContext& context) {
auto doc = wxGetApp().GetCurrentDocument();
if (!doc) return ToolResult::Ok("No active document found in the editor.");
wxString filename = doc->GetFilename(), file;
std::string error;
if (filename.empty() || !doc->GetDocumentSaved()) return ToolResult::Ok("The active document is untitled: it isn't saved in a file of the workspace.");
if (!context.ResolvePath(U(filename), file, error, PathAccess::RootOnly)) return ToolResult::Ok("The active document is outside the workspace.");
std::string result = format("Active file: {}\n", context.GetRelativePath(file));
result += format("Unsaved changes: {}\n", doc->IsModified()? "yes" : "no");
if (auto editor = doc->GetEditor()) result += DescribeCursorAndSelection(editor, context.limits.readMaxLines);
std::vector<std::string> others;
for (auto wxdoc: wxGetApp().GetDocManager()->GetDocumentsVector()) {
if (wxdoc==doc || wxdoc->GetFilename().empty() || !wxdoc->GetDocumentSaved()) continue;
wxString other;
if (!context.ResolvePath(U(wxdoc->GetFilename()), other, error, PathAccess::RootOnly)) continue;
others.push_back(context.GetRelativePath(other) + (wxdoc->IsModified()? " (unsaved changes)" : ""));
}
std::sort(others.begin(), others.end());
result += others.empty()? "Other open files: none\n" : "Other open files:\n";
for (auto& o: others) result += o + "\n";
return ToolResult::Ok(result);
}

// Subset of the quick jump syntax: Lua evaluation, commands and relative moves are excluded
static bool IsAllowedPosition (const wxString& position) {
static wxRegEx reg("^(\\[?\\d+([,:]\\d+)?\\]?|\\d+([,:]\\d+)?-\\d+([,:]\\d+)?|/[^/]+/[is]*|#.+)$");
return reg.Matches(position);
}

static ToolResult ToolOpenInEditor (const json& args, const ToolContext& context) {
std::string path = GetStringArg(args, "path"), position = GetStringArg(args, "position"), error;
if (path.empty()) return ToolResult::Error("path is required.");
wxString file;
if (!context.ResolvePath(path, file, error, PathAccess::Read)) return ToolResult::Error(error);
std::string rel = context.GetRelativePath(file);
if (rel.empty() || wxDirExists(file)) return ToolResult::Error(format("{} is a directory. Give the path of a file.", rel.empty()? "/" : rel));
if (!FindOpenEditor(file) && !wxFileExists(file)) return ToolResult::Error(format("File not found: {}. Use list_files to see available files.", rel));
wxString pos = U(position);
pos.Trim(true).Trim(false);
if (!pos.empty() && !IsAllowedPosition(pos)) return ToolResult::Error(format("Invalid position: {}. Use a line such as '12', a line and column such as '12:5', a range such as '12-15' or '12:5-15:20', '/regex/' to find a text, or '#name' to go to a marker.", position));
auto& app = wxGetApp();
auto doc = app.OpenOrCreateDocument(file);
if (!doc) return ToolResult::Error(format("Unable to open {}.", rel));
doc->Activate();
if (!pos.empty()) doc->DoQuickJump(pos);
app.UpdateTitle();
std::string result = format("Opened {} in the editor.\n", rel);
if (auto editor = doc->GetEditor()) result += DescribeCursorAndSelection(editor, context.limits.readMaxLines);
return ToolResult::Ok(result);
}

static ToolResult ToolMoveFile (const json& args, const ToolContext& context) {
std::string path = GetStringArg(args, "path"), newPath = GetStringArg(args, "new_path"), error;
if (path.empty()) return ToolResult::Error("path is required.");
if (newPath.empty()) return ToolResult::Error("new_path is required.");
wxString source, dest;
if (!context.ResolvePath(path, source, error, PathAccess::Write) || !context.ResolvePath(newPath, dest, error, PathAccess::Write)) return ToolResult::Error(error);
std::string rel = context.GetRelativePath(source), newRel = context.GetRelativePath(dest);
if (rel.empty()) return ToolResult::Error("The workspace root can't be moved.");
if (newRel.empty()) return ToolResult::Error("new_path must be the path of a file or directory, not the workspace root.");
bool isDir = wxDirExists(source);
if (!isDir && !wxFileExists(source)) return ToolResult::Error(format("File or directory not found: {}. Use list_files to see available files.", rel));
// Only the case changes: the destination exists because it is the source itself
bool sameName = source.IsSameAs(dest, false);
if (source==dest) return ToolResult::Error("path and new_path designate the same file.");
if (!sameName && (wxFileExists(dest) || wxDirExists(dest))) return ToolResult::Error(format("{} already exists. Choose another destination, or delete it first with delete_file.", newRel));
if (isDir && dest.Lower().StartsWith(source.Lower() + wxFILE_SEP_PATH)) return ToolResult::Error("A directory can't be moved into itself.");
auto answer = context.Confirm(U(format(GetTranslation(isDir? "AgentConfirmMoveDir" : "AgentConfirmMoveFile"), rel, newRel)));
if (answer!=ToolConfirmation::Allow) return RefusedResult(answer);
auto docs = FindOpenDocuments(source, isDir);
{
wxLogNull noLog;
wxFileName::Mkdir(wxFileName(dest).GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
if (!wxRenameFile(source, dest, sameName)) return ToolResult::Error(format("Unable to move {} to {}.", rel, newRel));
}
// Open documents follow the moved files, with their unsaved changes
for (auto doc: docs) doc->SetFilename(dest + NormalizePath(doc->GetFilename()).Mid(source.size()), true);
if (!docs.empty()) wxGetApp().UpdateTitle();
return ToolResult::Ok(format("Moved {} to {}.", rel, newRel));
}

static ToolResult ToolDeleteFile (const json& args, const ToolContext& context) {
std::string path = GetStringArg(args, "path"), error;
if (path.empty()) return ToolResult::Error("path is required.");
wxString file;
if (!context.ResolvePath(path, file, error, PathAccess::Write)) return ToolResult::Error(error);
std::string rel = context.GetRelativePath(file);
if (rel.empty()) return ToolResult::Error("The workspace root can't be deleted.");
bool isDir = wxDirExists(file);
if (!isDir && !wxFileExists(file)) return ToolResult::Error(format("File or directory not found: {}. Use list_files to see available files.", rel));
#ifdef wxHAS_MOVE_TO_TRASH
std::string message;
if (isDir) {
wxArrayString files;
wxLogNull noLog;
wxDir::GetAllFiles(file, &files, wxEmptyString, wxDIR_DEFAULT);
message = format(GetTranslation("AgentConfirmDeleteDir"), rel, files.size());
}
else message = format(GetTranslation("AgentConfirmDeleteFile"), rel);
auto docs = FindOpenDocuments(file, isDir);
size_t unsaved = std::count_if(docs.begin(), docs.end(), [](AbstractDocument* d){ return d->IsModified(); });
if (unsaved) message += " " + format(GetTranslation("AgentConfirmDeleteUnsaved"), docs.size(), unsaved);
else if (!docs.empty()) message += " " + format(GetTranslation("AgentConfirmDeleteOpen"), docs.size());
auto answer = context.Confirm(U(message), true, wxEmptyString, GrantKind::None);
if (answer!=ToolConfirmation::Allow) return RefusedResult(answer);
if (!wxMoveToTrash(file)) return ToolResult::Error(format("Unable to delete {}.", rel));
CloseDocuments(docs);
return ToolResult::Ok(format("Deleted {} (moved to the trash).", rel));
#else
return ToolResult::Error("Deleting files isn't supported on this platform.");
#endif
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
"Search for text in workspace files. The search is case-insensitive unless ignore_case is false, and always recursive. Returns one line per match in the form path:line: text.",
json::parse(R"({
"type": "object",
"properties": {
"pattern": { "type": "string", "description": "Text to search for, or a regular expression (PCRE syntax) if regex is true." },
"path": { "type": "string", "description": "Directory or file to search in, optionally followed by a file name pattern, for example 'src/' or 'src/*.cpp'. Empty for the whole workspace." },
"regex": { "type": "boolean", "description": "Whether pattern is a regular expression. Default: false." },
"ignore_case": { "type": "boolean", "description": "Ignore case when searching. Default: true." }
},
"required": ["pattern"]
})"),
ToolSearchFiles
});
tools.push_back({ "replace_in_files",
"Replace a text or a regular expression in several files of the workspace at once. The search is case-sensitive unless ignore_case is true, and always recursive. To change a single passage of a file, prefer edit_file. Use search_files first to check what will be replaced.",
json::parse(R"({
"type": "object",
"properties": {
"pattern": { "type": "string", "description": "Text to replace, or a regular expression (PCRE syntax) if regex is true." },
"replacement": { "type": "string", "description": "Replacement text. With regex, $1 to $9 insert captured groups and $0 the whole match. Empty to delete the matches." },
"path": { "type": "string", "description": "Directory or file to search in, optionally followed by a file name pattern, for example 'src/' or 'src/*.cpp'. Empty for the whole workspace." },
"regex": { "type": "boolean", "description": "Whether pattern is a regular expression. Default: false." },
"ignore_case": { "type": "boolean", "description": "Ignore case when searching. Default: false." }
},
"required": ["pattern", "replacement"]
})"),
ToolReplaceInFiles, true
});
tools.push_back({ "edit_file",
"Edit a text file of the workspace by replacing an exact text with a new one. old_text must match the file content exactly, including whitespace and indentation, and must appear only once unless replace_all is true. Read the file with read_file before editing it, and don't include line numbers in old_text.",
json::parse(R"({
"type": "object",
"properties": {
"path": { "type": "string", "description": "Path of the file, relative to the workspace root." },
"old_text": { "type": "string", "description": "Exact text to replace. Include enough surrounding lines to make it unique in the file." },
"new_text": { "type": "string", "description": "Text replacing old_text. Empty to delete old_text." },
"replace_all": { "type": "boolean", "description": "Replace all occurrences of old_text instead of requiring a unique one. Default: false." }
},
"required": ["path", "old_text", "new_text"]
})"),
ToolEditFile, true
});
tools.push_back({ "write_file",
"Create a text file in the workspace, or replace the whole content of an existing file. Missing directories are created. To change only a part of an existing file, prefer edit_file.",
json::parse(R"({
"type": "object",
"properties": {
"path": { "type": "string", "description": "Path of the file, relative to the workspace root." },
"content": { "type": "string", "description": "Complete content of the file." }
},
"required": ["path", "content"]
})"),
ToolWriteFile, true
});
tools.push_back({ "get_editor_context",
"Get what the user is looking at in the text editor: the active file, the cursor position, the selected text and the other open files. Call it when the user refers to the current file, the selection, the cursor or 'here'.",
json::parse(R"({
"type": "object",
"properties": {}
})"),
ToolGetEditorContext, false, true
});
tools.push_back({ "open_in_editor",
"Open a file of the workspace in the text editor so that the user can see it, optionally moving the cursor or selecting a passage. Returns the resulting cursor position and selection.",
json::parse(R"({
"type": "object",
"properties": {
"path": { "type": "string", "description": "Path of the file, relative to the workspace root." },
"position": { "type": "string", "description": "Where to move the cursor: a line '12', a line and column '12:5', a range of lines to select '12-15', a precise passage to select '12:5-15:20', '/regex/' to move to a match of a regular expression (without '/' inside; '/regex/i' to ignore case), or '#name' to move to a marker such as a function. Default: don't move the cursor." }
},
"required": ["path"]
})"),
ToolOpenInEditor, false, true
});
tools.push_back({ "move_file",
"Move or rename a file or directory of the workspace. Missing directories of the destination are created. Fails if the destination already exists.",
json::parse(R"({
"type": "object",
"properties": {
"path": { "type": "string", "description": "Current path of the file or directory, relative to the workspace root." },
"new_path": { "type": "string", "description": "New path, relative to the workspace root." }
},
"required": ["path", "new_path"]
})"),
ToolMoveFile, true, true
});
tools.push_back({ "delete_file",
"Delete a file or directory of the workspace, with all its content. It is moved to the trash.",
json::parse(R"({
"type": "object",
"properties": {
"path": { "type": "string", "description": "Path of the file or directory, relative to the workspace root." }
},
"required": ["path"]
})"),
ToolDeleteFile, true, true
});
tools.push_back({ "run_command",
"Run a command line in a shell, in the workspace or one of its subdirectories, and return its exit code and output (standard output and errors). The command can't read any input and is killed when it takes too long.",
json::parse(R"({
"type": "object",
"properties": {
"command": { "type": "string", "description": "Command line to run." },
"cwd": { "type": "string", "description": "Working directory, relative to the workspace root. Default: the workspace root." },
"timeout": { "type": "integer", "description": "Maximum time in seconds before the command is killed." }
},
"required": ["command"]
})"),
ToolRunCommand
});
tools.push_back({ "run_lua",
"Run Lua code inside the text editor, with access to its whole scripting API (documents, editors, menus...). As in an interactive console, an expression is evaluated and its value returned, and the output of print is captured. Global variables persist between calls.",
json::parse(R"({
"type": "object",
"properties": {
"code": { "type": "string", "description": "Lua code: a single expression, or statements optionally ending with return." }
},
"required": ["code"]
})"),
ToolRunLua, true
});
for (auto& t: tools) {
t.readOnly = t.name=="list_files" || t.name=="read_file" || t.name=="search_files" || t.name=="get_editor_context" || t.name=="open_in_editor";
t.requiresExplicitAllow = t.name=="run_command" || t.name=="run_lua";
}
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
if (GetPermission(t, config)==ToolPermission::Deny) continue;
defs.push_back({ {"type", "function"}, {"function", { {"name", t.name}, {"description", t.description}, {"parameters", t.parameters} }} });
}
return defs;
}

ToolPermission AgentTools::GetPermission (const AgentTool& tool, const AgentConfig& config) {
ToolPermission builtin = tool.readOnly? ToolPermission::Allow : ToolPermission::Ask;
auto it = config.toolSettings.find(tool.name);
if (it!=config.toolSettings.end()) return it->second==ToolPermission::Default? builtin : it->second;
if (config.toolsListClosed) return ToolPermission::Deny;
// Value of "*": never applies to tools without side effects, and doesn't allow commands and Lua without confirmation
ToolPermission p = config.otherToolsSetting;
if (tool.readOnly || p==ToolPermission::Default) return builtin;
if (p==ToolPermission::Allow && tool.requiresExplicitAllow) return ToolPermission::Ask;
return p;
}

bool AgentTools::IsEnabled (const std::string& name, const AgentConfig& config) {
auto tool = Find(name);
return tool && GetPermission(*tool, config)!=ToolPermission::Deny;
}

ToolResult AgentTools::Execute (const std::string& name, const std::string& arguments, const ToolContext& baseContext, ToolPermission permission) {
ToolResult result;
auto tool = Find(name);
if (!tool) {
std::string names;
for (auto& t: GetAll()) names += (names.empty()? "" : ", ") + t.name;
return ToolResult::Error(format("Unknown tool: {}. Available tools: {}", name, names));
}
if (permission==ToolPermission::Deny) return ToolResult::Error(format("The tool {} is disabled by the configuration. Only use the tools you were given.", name));
ToolContext context = baseContext;
context.permission = permission;
context.toolName = name;
println("Agent tool call: {} {}", name, arguments);
json args;
try {
args = arguments.empty()? json::object() : json::parse(arguments);
} catch (std::exception& e) {
return ToolResult::Error(format("Invalid JSON arguments: {}", e.what()));
}
if (args.is_null()) args = json::object();
if (!args.is_object()) return ToolResult::Error("Arguments must be a JSON object.");
try {
if (tool->mainThread) result = RunEDTSync([&](){ return tool->handler(args, context); });
else result = tool->handler(args, context);
} catch (std::exception& e) {
result = ToolResult::Error(e.what());
}
if (context.stopped) result = ToolResult::Stop();
size_t max = context.limits.maxResultChars;
if (result.text.size()>max) {
size_t cut = max;
while (cut>0 && (static_cast<unsigned char>(result.text[cut]) & 0xC0)==0x80) cut--;
result.text = result.text.substr(0, cut) + "\n[Truncated: result too long.]\n";
}
println("Agent tool result: {}", result.text.substr(0, result.text.find('\n')));
return result;
}
