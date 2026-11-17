#ifndef _____AGENT_CONFIG_HPP
#define _____AGENT_CONFIG_HPP
#include "../common/wxUtils.hpp"
#include "nlohmann/json.hpp"
#include<vector>

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

struct AgentConfig {
std::string name, api, url, model, systemPrompt, shell;
nlohmann::json headers, extraBody;
double temperature = -1;
int maxTokens = -1;
int timeout = 600;
int maxToolIterations = 15;
bool toolsSpecified = false;
std::vector<std::string> tools;
AgentLimits limits;

bool IsToolEnabled (const std::string& name) const;

static AgentConfig FromJson (const nlohmann::json& j);
static bool LoadAll (std::vector<AgentConfig>& configs, wxString& error);
};

#endif
