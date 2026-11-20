// run_command tool: executed in the tool thread of the agent
#include "AgentTools.hpp"
#include "../app/App.hpp"
#include "../common/println.hpp"
#include <wx/filename.h>
#include <wx/process.h>
#include <wx/utils.h>
#include<atomic>
#include<chrono>
#include<thread>

using json = nlohmann::json;

wxFontEncoding GetLocalOemDefaultEncoding ();

// Remove a truncated multibyte sequence at the end of UTF-8 text, so that a cut text can still be valid
static void TrimIncompleteUTF8 (std::string& s) {
size_t n = s.size(), i = n;
while (i>0 && n-i<4 && (static_cast<unsigned char>(s[i-1]) & 0xC0)==0x80) i--;
if (i==0) return;
unsigned char lead = s[i-1];
size_t expected = lead>=0xF0? 4 : lead>=0xE0? 3 : lead>=0xC0? 2 : 1;
if (n-(i-1) < expected) s.resize(i-1);
}

std::string ToValidUTF8 (const std::string& bytes, wxMBConv& fallback) {
std::string s = bytes;
TrimIncompleteUTF8(s);
wxString text;
// wxConvUTF8 is strict: it fails on invalid UTF-8
if (wxConvUTF8.ToWChar(nullptr, 0, s.data(), s.size())!=wxCONV_FAILED) text = wxString(s.data(), wxConvUTF8, s.size());
else text = wxString(bytes.data(), fallback, bytes.size());
text.Replace("\r\n", "\n");
return U(text);
}

// Keeps the beginning and the end of a long output, and counts what is dropped in the middle
struct OutputBuffer {
std::string head, tail;
size_t half, dropped = 0;
OutputBuffer (size_t max): half(max/2) {}
void Append (const char* data, size_t len) {
if (head.size()<half) {
size_t n = std::min(len, half - head.size());
head.append(data, n);
data += n;
len -= n;
}
if (len==0) return;
tail.append(data, len);
if (tail.size()>2*half) {
size_t cut = tail.size() - half;
dropped += cut;
tail.erase(0, cut);
}}
std::string GetText (wxMBConv& conv) {
if (dropped==0) return ToValidUTF8(head + tail, conv);
if (tail.size()>half) {
dropped += tail.size() - half;
tail.erase(0, tail.size() - half);
}
return ToValidUTF8(head, conv) + format("\n[... {} bytes of output not shown ...]\n", dropped) + ToValidUTF8(tail, conv);
}
};

static char AsciiLower (char c) {
return c>='A' && c<='Z'? c + ('a' - 'A') : c;
}

// Whether the glob pattern matches the beginning of text, up to its end or a whitespace (word boundary).
// '*' matches any sequence of characters, spaces included, and '?' a single (UTF-8) character. Case-insensitive for ASCII letters.
static bool GlobMatchesCommandStart (const std::string& pattern, const std::string& text) {
size_t n = text.size();
// reach[i]: the part of the pattern processed so far matches text[0..i)
std::vector<char> reach(n+1, 0), next(n+1);
reach[0] = 1;
for (char c: pattern) {
std::fill(next.begin(), next.end(), 0);
if (c=='*') {
bool on = false;
for (size_t i=0; i<=n; i++) next[i] = on = on || reach[i];
}
else if (c=='?') {
for (size_t i=0; i<n; i++) {
if (!reach[i]) continue;
size_t j = i+1;
while (j<n && (static_cast<unsigned char>(text[j]) & 0xC0)==0x80) j++;
next[j] = 1;
}}
else {
for (size_t i=0; i<n; i++) if (reach[i] && AsciiLower(text[i])==AsciiLower(c)) next[i+1] = 1;
}
reach.swap(next);
}
for (size_t i=0; i<=n; i++) if (reach[i] && (i==n || text[i]==' ' || text[i]=='\t')) return true;
return false;
}

bool IsAllowedCommand (const std::string& command, const std::vector<std::string>& patterns) {
size_t start = command.find_first_not_of(" \t"), end = command.find_last_not_of(" \t");
if (start==std::string::npos) return false;
std::string cmd = command.substr(start, end - start + 1);
// Chaining, pipes, redirections, escapes and variable expansions of cmd, PowerShell and Unix shells: always ask
if (cmd.find_first_of("&|<>^%;`$\r\n")!=std::string::npos) return false;
for (auto& pattern: patterns) if (!pattern.empty() && GlobMatchesCommandStart(pattern, cmd)) return true;
return false;
}

static std::string BuildCommandLine (const std::string& shell, const std::string& command) {
std::string cmd = shell.empty()? DEFAULT_AGENT_SHELL : shell;
size_t pos = cmd.find("{command}");
if (pos==std::string::npos) return cmd + " " + command;
return cmd.replace(pos, 9, command);
}

ToolResult ToolRunCommand (const json& args, const ToolContext& context) {
auto it = args.find("command");
std::string command = it!=args.end() && it->is_string()? it->get<std::string>() : "";
if (command.find_first_not_of(" \t\r\n")==std::string::npos) return ToolResult::Error("command is required.");
std::string cwdArg = args.contains("cwd") && args["cwd"].is_string()? args["cwd"].get<std::string>() : "", error;
int timeout = context.limits.commandTimeout;
if (args.contains("timeout") && args["timeout"].is_number()) timeout = args["timeout"].get<int>();
if (timeout<=0) timeout = context.limits.commandTimeout;
if (timeout>context.limits.commandMaxTimeout) timeout = context.limits.commandMaxTimeout;
wxString cwd;
if (!context.ResolvePath(cwdArg, cwd, error)) return ToolResult::Error(error);
if (!wxDirExists(cwd)) return ToolResult::Error(format("Directory not found: {}. cwd must be a directory of the workspace.", cwdArg));
std::string relCwd = context.GetRelativePath(cwd, true);
if (!IsAllowedCommand(command, context.allowedCommands)) {
wxString message = U(format(GetTranslation("AgentConfirmCommand"), relCwd.empty()? "/" : relCwd, timeout));
auto answer = context.Confirm(message, true, U(command));
if (answer==ToolConfirmation::Stop) return ToolResult::Stop();
if (answer!=ToolConfirmation::Allow) return ToolResult::Error("The user denied running this command. Don't retry it: ask the user what to do instead.");
}

std::string cmdline = BuildCommandLine(context.shell, command);
println("Agent command: {} (in {})", cmdline, U(cwd));

// wxExecute must be called on the main thread in asynchronous mode, and the end of the process is notified there.
// The streams of the process are read here, in the tool thread, as the console does with LiveTextAppender.
struct ProcessState {
std::atomic<bool> ended { false };
std::atomic<int> exitCode { 0 };
};
auto state = std::make_shared<ProcessState>();
wxProcess* process = RunEDTSync([&]() -> wxProcess* {
auto proc = new wxProcess();
proc->Redirect();
proc->Bind(wxEVT_END_PROCESS, [state](wxProcessEvent& e){
state->exitCode = e.GetExitCode();
state->ended = true;
});
wxExecuteEnv env;
env.cwd = cwd;
if (wxExecute(U(cmdline), wxEXEC_ASYNC | wxEXEC_HIDE_CONSOLE, proc, &env)<=0) {
delete proc;
return nullptr;
}
// No input: the command can't wait for the user
proc->CloseOutput();
return proc;
});
if (!process) return ToolResult::Error(format("Unable to start the command. Check the shell setting of the agent: {}", context.shell));
long pid = process->GetPid();
wxInputStream* streams[] = { process->GetInputStream(), process->GetErrorStream() };

OutputBuffer output(std::max(1000, context.limits.maxResultChars - 200));
char buf[4096];
// Read what is available in the standard output and error, without blocking
auto readAvailable = [&](){
bool any = false;
for (auto in: streams) {
while (in && in->CanRead()) {
in->Read(buf, sizeof(buf));
size_t n = in->LastRead();
if (n==0) break;
output.Append(buf, n);
any = true;
}}
return any;
};
auto start = std::chrono::steady_clock::now();
bool timedOut = false, cancelled = false;
while (true) {
if (context.IsCancelled()) cancelled = true;
else if (std::chrono::steady_clock::now() - start > std::chrono::seconds(timeout)) timedOut = true;
if (cancelled || timedOut) {
// Kill the command with all the processes it started
RunEDTSync([pid](){ wxProcess::Kill(pid, wxSIGKILL, wxKILL_CHILDREN); });
break;
}
if (readAvailable()) continue;
// A program started in the background by the command may keep the streams open: it isn't waited for
if (state->ended) break;
std::this_thread::sleep_for(std::chrono::milliseconds(20));
}
// After a kill, wait a little for the end notification
for (int i=0; i<100 && !state->ended; i++) std::this_thread::sleep_for(std::chrono::milliseconds(20));
readAvailable();
bool ended = state->ended;
RunEDTSync([process, ended](){
// Without the end notification, the process object must survive until the process ends: it then deletes itself
if (ended) delete process;
else process->Detach();
});

if (cancelled) return ToolResult::Error("Cancelled by the user: the command was killed.");
// Console programs write in the OEM code page, unless they use UTF-8
wxCSConv oemConv(GetLocalOemDefaultEncoding());
std::string text = output.GetText(oemConv);
std::string header = timedOut? format("Exit code: none (killed after {} seconds: timeout)\n", timeout) : format("Exit code: {}\n", static_cast<int>(state->exitCode));
if (text.find_first_not_of(" \t\n")==std::string::npos) return ToolResult::Ok(header + "(no output)\n");
if (text.back()!='\n') text += '\n';
return ToolResult::Ok(header + text);
}
