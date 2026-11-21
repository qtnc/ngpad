#ifndef _____AGENT_CONFIG_HPP
#define _____AGENT_CONFIG_HPP
#include "../common/wxUtils.hpp"
#include "nlohmann/json.hpp"
#include<vector>
#include<map>
#include<functional>

#define AGENTS_CONFIG_FILENAME "config/agents.json"
#define DEFAULT_AGENT_SHELL "cmd /s /c \"${command}\""
// Reasoning levels accepted by /effort, substituted to ${effort} in extraBody
#define AGENT_EFFORT_LEVELS { "none", "minimal", "low", "medium", "high", "xhigh" }

struct AgentLimits {
int readMaxLines = 500;
int listMaxEntries = 500;
int searchMaxResults = 100;
int searchMaxLineLength = 200;
int maxResultChars = 50000;
int commandTimeout = 60; // Seconds
int commandMaxTimeout = 600;
};

// Default: the built-in permission of the tool (allow for tools without side effects, ask for the others)
enum class ToolPermission { Default, Allow, Ask, Deny };

struct AgentConfig {
std::string name, api, url, model, systemPrompt, shell;
std::string effort; // Initial reasoning level, empty if none; only used through ${effort} in extraBody
std::string conversationsDir; // As given in the configuration: empty = default directory, relative = from the workspace root
nlohmann::json headers, extraBody;
double temperature = -1;
int maxTokens = -1;
int timeout = 600;
int maxToolIterations = 15;
// "tools" key: value given to each listed tool, and to the other tools ("*"). Without "*", tools not listed are absent.
// The effective permission of a tool is computed by AgentTools::GetPermission.
std::map<std::string, ToolPermission> toolSettings;
bool toolsListClosed = false;
ToolPermission otherToolsSetting = ToolPermission::Default;
std::vector<std::string> allowedCommands; // Glob patterns of commands run_command can run without confirmation
std::vector<std::string> readablePaths, writablePaths; // Files and directories outside the workspace the tools can access, as given in the configuration
AgentLimits limits;

static AgentConfig FromJson (const nlohmann::json& j);
// Replace ${env:NAME} and ${file:path} in all the string values of an entry (not in keys). The substituted text is never parsed again.
// Relative file paths are resolved from configDir. Other ${...} are kept as is, as ${command} and ${effort} which are substituted later.
static void ExpandVariables (nlohmann::json& j, const wxString& configDir, const std::string& agent);
static bool LoadAll (std::vector<AgentConfig>& configs, wxString& error);
// Modify the entry of an agent in agents.json, found by its name. The order of keys is kept, but comments are lost.
// The previous file is kept as agents.json.bak. update receives the entry, and the defaults (empty object if none).
typedef std::function<void(nlohmann::ordered_json& agent, const nlohmann::ordered_json& defaults)> EntryUpdater;
static bool UpdateEntry (const std::string& agentName, const EntryUpdater& update, wxString& error);
};

#endif
