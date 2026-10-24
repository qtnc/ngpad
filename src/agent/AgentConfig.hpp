#ifndef _____AGENT_CONFIG_HPP
#define _____AGENT_CONFIG_HPP
#include "../common/wxUtils.hpp"
#include "nlohmann/json.hpp"
#include<vector>

#define AGENTS_CONFIG_FILENAME "config/agents.json"

struct AgentConfig {
std::string name, api, url, model, systemPrompt;
nlohmann::json headers, extraBody;
double temperature = -1;
int maxTokens = -1;
int timeout = 600;

static AgentConfig FromJson (const nlohmann::json& j);
static bool LoadAll (std::vector<AgentConfig>& configs, wxString& error);
};

#endif
