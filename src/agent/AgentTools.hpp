#ifndef _____AGENT_TOOLS_HPP
#define _____AGENT_TOOLS_HPP
#include "AgentConfig.hpp"
#include<atomic>
#include<functional>
#include<memory>
#include<vector>

enum class ToolConfirmation { Allow, Deny, Stop };

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
std::vector<wxString> ignoredDirs, excludedExtensions;
ConfirmCallback confirm;
ToolPermission permission = ToolPermission::Ask; // Effective permission of the tool being executed: Allow skips confirmations
std::shared_ptr<std::atomic<bool>> cancelled;
std::shared_ptr<LuaEnvironment> luaEnv;

ToolContext (const wxString& root, const AgentLimits& limits);
bool ResolvePath (const std::string& input, wxString& absPath, std::string& error) const;
std::string GetRelativePath (const wxString& absPath, bool isDir = false) const;
bool IsIgnoredDir (const wxString& name) const;
bool IsExcludedFile (const wxString& name) const;
bool IsCancelled () const { return cancelled && *cancelled; }
// Ask the user on the main thread, unless the tool is allowed. details: long text (command, script...) shown in a read-only text field
ToolConfirmation Confirm (const wxString& message, bool denyByDefault = false, const wxString& details = wxEmptyString) const;
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
