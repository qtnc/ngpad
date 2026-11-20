#include "AgentConfig.hpp"
#include "../app/App.hpp"
#include "../common/println.hpp"
#include <wx/filefn.h>
#include <wx/log.h>
#include<fstream>
#include<iterator>
#include<algorithm>

using json = nlohmann::json;

static ToolPermission ParseToolPermission (const json& value, const std::string& agent, const std::string& tool) {
if (value.is_string()) {
auto s = value.get<std::string>();
if (s=="allow") return ToolPermission::Allow;
if (s=="ask") return ToolPermission::Ask;
if (s=="deny") return ToolPermission::Deny;
if (s=="default") return ToolPermission::Default;
}
throw std::runtime_error(format("agent \"{}\", tool \"{}\": invalid permission {}, expected \"allow\", \"ask\", \"deny\" or \"default\"", agent, tool, value.dump()));
}

// Array of strings, trimmed, empty strings ignored
static void ReadStringList (const json& j, const std::string& key, const std::string& agent, std::vector<std::string>& list) {
if (!j.contains(key)) return;
auto& a = j[key];
if (!a.is_array()) throw std::runtime_error(format("agent \"{}\": \"{}\" must be an array of strings", agent, key));
for (auto& item: a) {
if (!item.is_string()) throw std::runtime_error(format("agent \"{}\": invalid item {} in \"{}\", expected a string", agent, item.dump(), key));
std::string s = item.get<std::string>();
size_t start = s.find_first_not_of(" \t"), end = s.find_last_not_of(" \t");
if (start!=std::string::npos) list.push_back(s.substr(start, end - start + 1));
}}

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
c.shell = j.value("shell", DEFAULT_AGENT_SHELL);
if (j.contains("headers") && j["headers"].is_object()) c.headers = j["headers"];
else c.headers = json::object();
if (j.contains("extraBody") && j["extraBody"].is_object()) c.extraBody = j["extraBody"];
else c.extraBody = json::object();
if (!c.url.empty() && c.url.back()!='/') c.url += '/';
c.maxToolIterations = j.value("maxToolIterations", 15);
// "tools": absent = all tools; array = only these tools; object = permission of each listed tool, "*" for the other ones
if (j.contains("tools")) {
auto& t = j["tools"];
if (t.is_array()) {
c.toolsListClosed = true;
for (auto& name: t) if (name.is_string()) c.toolSettings[name.get<std::string>()] = ToolPermission::Default;
}
else if (t.is_object()) {
c.toolsListClosed = !t.contains("*");
for (auto& [name, value]: t.items()) {
auto permission = ParseToolPermission(value, c.name, name);
if (name=="*") c.otherToolsSetting = permission;
else c.toolSettings[name] = permission;
}}
else throw std::runtime_error(format("agent \"{}\": \"tools\" must be an array of tool names, or an object giving the permission of each tool", c.name));
}
ReadStringList(j, "allowedCommands", c.name, c.allowedCommands);
ReadStringList(j, "readablePaths", c.name, c.readablePaths);
ReadStringList(j, "writablePaths", c.name, c.writablePaths);
if (j.contains("limits") && j["limits"].is_object()) {
auto& l = j["limits"];
c.limits.readMaxLines = l.value("readMaxLines", c.limits.readMaxLines);
c.limits.listMaxEntries = l.value("listMaxEntries", c.limits.listMaxEntries);
c.limits.searchMaxResults = l.value("searchMaxResults", c.limits.searchMaxResults);
c.limits.searchMaxLineLength = l.value("searchMaxLineLength", c.limits.searchMaxLineLength);
c.limits.maxResultChars = l.value("maxResultChars", c.limits.maxResultChars);
c.limits.commandTimeout = l.value("commandTimeout", c.limits.commandTimeout);
c.limits.commandMaxTimeout = l.value("commandMaxTimeout", c.limits.commandMaxTimeout);
}
return c;
}

bool AgentConfig::UpdateEntry (const std::string& agentName, const EntryUpdater& update, wxString& error) {
using ordered_json = nlohmann::ordered_json;
wxString path = wxGetApp().FindAppFile(AGENTS_CONFIG_FILENAME);
if (path.empty()) {
error = U(format(GetTranslation("AgentConfigNotFound"), AGENTS_CONFIG_FILENAME));
return false;
}
try {
std::string content;
{
std::ifstream in(UFN(path), std::ios::binary);
if (!in) throw std::runtime_error("unable to read the file");
content.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}
bool crlf = content.find("\r\n")!=std::string::npos;
ordered_json j = ordered_json::parse(content, nullptr, true, true);
ordered_json* agents = nullptr;
ordered_json defaults = ordered_json::object();
if (j.is_array()) agents = &j;
else if (j.is_object() && j.contains("agents") && j["agents"].is_array()) {
agents = &j["agents"];
if (j.contains("defaults") && j["defaults"].is_object()) defaults = j["defaults"];
}
else throw std::runtime_error("root element must be an array of agents, or an object with an \"agents\" array");
ordered_json* entry = nullptr;
for (auto& a: *agents) {
if (a.is_object() && a.contains("name") && a["name"].is_string() && a["name"].get<std::string>()==agentName) {
entry = &a;
break;
}}
if (!entry) throw std::runtime_error(format("agent \"{}\" not found", agentName));
update(*entry, defaults);
// No indentation, as the files written by hand
std::string out = j.dump(0) + "\n";
if (crlf) {
std::string converted;
for (char c: out) {
if (c=='\n') converted += '\r';
converted += c;
}
out = std::move(converted);
}
wxString tmp = path + ".tmp", backup = path + ".bak";
{
std::ofstream o(UFN(tmp), std::ios::binary | std::ios::trunc);
o << out;
if (!o) throw std::runtime_error("unable to write the file");
}
wxLogNull noLog;
wxCopyFile(path, backup, true);
if (!wxRenameFile(tmp, path, true)) throw std::runtime_error("unable to replace the file");
} catch (std::exception& e) {
error = U(format(GetTranslation("AgentConfigSaveError"), U(path), e.what()));
return false;
}
return true;
}

// Keys whose array values from defaults are concatenated with those of the agent, instead of being replaced
static bool IsConcatenatedKey (const std::string& key) {
return key=="allowedCommands" || key=="readablePaths" || key=="writablePaths";
}

// Merge defaults into an agent entry: missing keys are copied, objects are merged recursively, the whitelists above are concatenated (defaults first), and any other value of the agent wins
static void MergeDefaults (json& target, const json& defaults, bool topLevel) {
for (auto& [key, value]: defaults.items()) {
if (!target.contains(key)) target[key] = value;
else {
json& t = target[key];
if (t.is_object() && value.is_object()) MergeDefaults(t, value, false);
else if (topLevel && IsConcatenatedKey(key) && t.is_array() && value.is_array()) {
json merged = value;
for (auto& item: t) merged.push_back(item);
t = std::move(merged);
}}}
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
// Root: either an array of agents, or an object {"defaults": {...}, "agents": [...]}
json defaults = json::object(), agents;
if (j.is_array()) agents = std::move(j);
else if (j.is_object() && j.contains("agents") && j["agents"].is_array()) {
agents = std::move(j["agents"]);
if (j.contains("defaults")) {
if (!j["defaults"].is_object()) throw std::runtime_error("\"defaults\" must be an object");
defaults = std::move(j["defaults"]);
}}
else throw std::runtime_error("root element must be an array of agents, or an object with an \"agents\" array");
for (auto& item: agents) {
if (!item.is_object()) continue;
MergeDefaults(item, defaults, true);
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
