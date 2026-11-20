#ifndef _____AGENT_CONFIG_HPP
#define _____AGENT_CONFIG_HPP
#include "../common/wxUtils.hpp"
#include "nlohmann/json.hpp"
#include<vector>
#include<map>

#define AGENTS_CONFIG_FILENAME "config/agents.json"
#define DEFAULT_AGENT_SHELL "cmd /s /c \"{command}\""

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
AgentLimits limits;

static AgentConfig FromJson (const nlohmann::json& j);
static bool LoadAll (std::vector<AgentConfig>& configs, wxString& error);
};

#endif
