#ifndef _____AGENT_TOOLS_HPP
#define _____AGENT_TOOLS_HPP
#include "AgentConfig.hpp"
#include<functional>
#include<vector>

enum class ToolConfirmation { Allow, Deny, Stop };

struct ToolContext {
typedef std::function<ToolConfirmation(const wxString& message)> ConfirmCallback;
wxString root, rootDir; // rootDir: normalized, with trailing separator
const AgentLimits& limits;
std::vector<wxString> ignoredDirs, excludedExtensions;
ConfirmCallback confirm;

ToolContext (const wxString& root, const AgentLimits& limits);
bool ResolvePath (const std::string& input, wxString& absPath, std::string& error) const;
std::string GetRelativePath (const wxString& absPath, bool isDir = false) const;
bool IsIgnoredDir (const wxString& name) const;
bool IsExcludedFile (const wxString& name) const;
ToolConfirmation Confirm (const wxString& message) const;
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
};

// Registry of the tools an agent can use.
// Built-in tools are registered on first access; other tools (e.g. from Lua scripts) can be added with Register.
class AgentTools {
public:
static std::vector<AgentTool>& GetAll ();
static void Register (const AgentTool& tool);
static const AgentTool* Find (const std::string& name);
static nlohmann::json GetDefinitions (const AgentConfig& config);
static ToolResult Execute (const std::string& name, const std::string& arguments, const ToolContext& context);
};

#endif
