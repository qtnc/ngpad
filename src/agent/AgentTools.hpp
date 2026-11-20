#ifndef _____AGENT_TOOLS_HPP
#define _____AGENT_TOOLS_HPP
#include "AgentConfig.hpp"
#include<atomic>
#include<functional>
#include<memory>
#include<vector>

enum class ToolConfirmation { Allow, Deny, Stop };

// How a tool uses a path, for paths outside the workspace root:
// RootOnly: never accepted outside the root, without asking (e.g. listing open documents);
// Read: accepted if covered by readablePaths or writablePaths, else the user is asked immediately;
// Write: accepted if covered by writablePaths, else accepted but the confirmation of the tool is always asked (outsideWorkspace).
enum class PathAccess { RootOnly, Read, Write };

// Entry of readablePaths / writablePaths: absolute, normalized with '/' and without trailing '/'
struct AllowedPath {
wxString path;
bool isPattern; // Contains wildcards: matched with MatchGlob; otherwise the file or directory and all its content
};

// Lua environment of an agent: globals defined by run_lua scripts persist in it during the conversation
struct LuaEnvironment {
int ref = -2; // LUA_NOREF until first use
~LuaEnvironment ();
};

// Tools are executed in a separate thread: the context is copied into it and must not refer to the session
struct ToolContext {
typedef std::function<ToolConfirmation(const wxString& message, const wxString& details, bool denyByDefault)> ConfirmCallback;
wxString root, rootDir; // rootDir: normalized, with trailing separator
AgentLimits limits;
std::string shell;
std::vector<std::string> allowedCommands;
std::vector<AllowedPath> readablePaths, writablePaths;
std::vector<wxString> ignoredDirs, excludedExtensions;
ConfirmCallback confirm;
ToolPermission permission = ToolPermission::Ask; // Effective permission of the tool being executed: Allow skips confirmations
std::shared_ptr<std::atomic<bool>> cancelled;
std::shared_ptr<LuaEnvironment> luaEnv;
// State of the tool call being executed (the context is copied for each call)
mutable bool outsideWorkspace = false; // A path to write is outside the workspace and not covered by writablePaths: always confirm
mutable bool stopped = false; // The user chose to stop the agent in a confirmation asked by ResolvePath
mutable std::vector<wxString> approvedReads; // Paths outside the workspace the user allowed to read during this call

ToolContext (const wxString& root, const AgentLimits& limits);
void SetAllowedPaths (const std::vector<std::string>& readable, const std::vector<std::string>& writable);
// Resolve a path given by the model (relative to the root, or absolute), and check it is accessible (see PathAccess)
bool ResolvePath (const std::string& input, wxString& absPath, std::string& error, PathAccess access) const;
bool IsInWorkspace (const wxString& absPath) const;
// Path shown to the model: relative to the root, or absolute outside of it; always with '/'
std::string GetRelativePath (const wxString& absPath, bool isDir = false) const;
bool IsIgnoredDir (const wxString& name) const;
bool IsExcludedFile (const wxString& name) const;
bool IsCancelled () const { return cancelled && *cancelled; }
// Ask the user on the main thread, unless the tool is allowed. details: long text (command, script...) shown in a read-only text field
ToolConfirmation Confirm (const wxString& message, bool denyByDefault = false, const wxString& details = wxEmptyString) const;
// Ask the user on the main thread, whatever the permission of the tool
ToolConfirmation Ask (const wxString& message, bool denyByDefault = false, const wxString& details = wxEmptyString) const;
private:
bool IsAllowedOutside (const wxString& absPath, PathAccess access) const;
};

struct ToolResult {
bool error;
std::string text;
bool stop = false; // The user asked to stop the agent

static ToolResult Ok (const std::string& text) { return { false, text }; }
static ToolResult Error (const std::string& text) { return { true, "Error: " + text }; }
static ToolResult Stop () { return { true, "Error: The user stopped the agent.", true }; }
};

struct AgentTool {
typedef std::function<ToolResult(const nlohmann::json& args, const ToolContext& context)> Handler;
std::string name, description;
nlohmann::json parameters;
Handler handler;
bool modifiesFiles = false;
bool mainThread = false; // The handler works mostly with documents and the GUI: run it entirely on the main thread
bool readOnly = false; // No side effect: allowed by default, and not affected by "*" in the tools setting
bool requiresExplicitAllow = false; // Can do anything (commands, Lua): allowed without confirmation only by "allow" on its own key, never by "*"
};

// Registry of the tools an agent can use.
// Built-in tools are registered on first access; other tools (e.g. from Lua scripts) can be added with Register.
class AgentTools {
public:
static std::vector<AgentTool>& GetAll ();
static void Register (const AgentTool& tool);
static const AgentTool* Find (const std::string& name);
static nlohmann::json GetDefinitions (const AgentConfig& config);
// Effective permission of a tool for an agent: Allow, Ask or Deny (never Default). Deny means the tool isn't available at all
static ToolPermission GetPermission (const AgentTool& tool, const AgentConfig& config);
static bool IsEnabled (const std::string& name, const AgentConfig& config);
// Can be called from any thread. permission: result of GetPermission
static ToolResult Execute (const std::string& name, const std::string& arguments, const ToolContext& context, ToolPermission permission);
};

// Valid UTF-8 from bytes in UTF-8 or else in the given encoding; CRLF are converted to LF
std::string ToValidUTF8 (const std::string& bytes, wxMBConv& fallback);

// Whether a command matches one of the glob patterns of allowedCommands (see doc/agents.md). Commands containing characters with a special meaning for shells never match.
bool IsAllowedCommand (const std::string& command, const std::vector<std::string>& patterns);

ToolResult ToolRunCommand (const nlohmann::json& args, const ToolContext& context);
ToolResult ToolRunLua (const nlohmann::json& args, const ToolContext& context);

#endif
