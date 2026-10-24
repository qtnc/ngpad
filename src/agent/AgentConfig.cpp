#include "AgentConfig.hpp"
#include "../app/App.hpp"
#include "../common/println.hpp"
#include<fstream>

using json = nlohmann::json;

AgentConfig AgentConfig::FromJson (const json& j) {
AgentConfig c;
c.name = j.value("name", "");
c.api = j.value("api", "openai-chat");
c.url = j.value("url", "");
c.model = j.value("model", "");
c.systemPrompt = j.value("systemPrompt", "");
c.temperature = j.value("temperature", -1.0);
c.maxTokens = j.value("maxTokens", -1);
c.timeout = j.value("timeout", 600);
if (j.contains("headers") && j["headers"].is_object()) c.headers = j["headers"];
else c.headers = json::object();
if (j.contains("extraBody") && j["extraBody"].is_object()) c.extraBody = j["extraBody"];
else c.extraBody = json::object();
if (!c.url.empty() && c.url.back()!='/') c.url += '/';
return c;
}

bool AgentConfig::LoadAll (std::vector<AgentConfig>& configs, wxString& error) {
wxString path = wxGetApp().FindAppFile(AGENTS_CONFIG_FILENAME);
if (path.empty()) {
error = U(format(GetTranslation("AgentConfigNotFound"), AGENTS_CONFIG_FILENAME));
return false;
}
try {
std::ifstream in(UFN(path));
json j = json::parse(in, nullptr, true, true);
if (!j.is_array()) throw std::runtime_error("root element must be an array");
for (auto& item: j) {
if (!item.is_object()) continue;
auto c = FromJson(item);
if (c.name.empty() || c.url.empty()) continue;
configs.push_back(c);
}
} catch (std::exception& e) {
error = U(format(GetTranslation("AgentConfigError"), U(path), e.what()));
return false;
}
if (configs.empty()) {
error = U(format(GetTranslation("AgentConfigEmpty"), U(path)));
return false;
}
return true;
}
