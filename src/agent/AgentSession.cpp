#include "AgentSession.hpp"
#include "../app/App.hpp"
#include "../app/WebRequester.hpp"
#include "../common/println.hpp"
#include<unordered_map>
#include<thread>
#include<optional>
#include <wx/filename.h>

using json = nlohmann::json;

// Model lists by base URL, shared by all sessions to avoid useless requests
static std::unordered_map<std::string, std::vector<std::string>> modelListCache;

static std::string ReadAll (wxInputStream* in) {
std::string data;
if (!in) return data;
char buf[4096];
while (in->CanRead()) {
in->Read(buf, sizeof(buf));
size_t n = in->LastRead();
if (n<=0) break;
data.append(buf, n);
}
return data;
}

static std::string ExtractErrorMessage (const std::string& body) {
try {
json j = json::parse(body);
if (j.contains("error")) {
auto& err = j["error"];
if (err.is_string()) return err.get<std::string>();
if (err.is_object() && err.contains("message") && err["message"].is_string()) return err["message"].get<std::string>();
}
} catch (...) {}
return body.substr(0, 500);
}

wxString GetWorkspaceName (const wxString& root) {
auto dirs = wxFileName::DirName(root).GetDirs();
return dirs.empty()? root : dirs.Last();
}

static long long GetCount (const json& obj, const char* key, bool* found = nullptr) {
auto it = obj.find(key);
if (it==obj.end() || !it->is_number()) return 0;
if (found) *found = true;
return it->get<long long>();
}

void TokenUsage::Add (const json& u) {
requests++;
if (!u.is_object()) return;
hasUsage = true;
// OpenAI chat completions format; input_tokens / output_tokens (Responses API) are also accepted
lastInput = GetCount(u, "prompt_tokens") + GetCount(u, "input_tokens");
input += lastInput;
output += GetCount(u, "completion_tokens") + GetCount(u, "output_tokens");
bool found = false;
auto details = u.find("prompt_tokens_details");
if (details!=u.end() && details->is_object()) cached += GetCount(*details, "cached_tokens", &found);
// DeepSeek
cached += GetCount(u, "prompt_cache_hit_tokens", &found);
if (found) hasCached = true;
}

AgentSession::AgentSession (const AgentConfig& cfg, const wxString& root0):
config(cfg), root(root0), model(cfg.model), messages(json::array()), alive(std::make_shared<bool>(true)),
cancelled(std::make_shared<std::atomic<bool>>(false)), luaEnv(std::make_shared<LuaEnvironment>())
{}

AgentSession::~AgentSession () {
*alive = false;
*cancelled = true;
Cancel();
}

void AgentSession::Clear () {
if (IsBusy()) {
// Silently drop the pending request or tool calls: their callbacks would write into the cleared conversation
*alive = false;
alive = std::make_shared<bool>(true);
*cancelled = true;
if (busy && request.IsOk()) request.Cancel();
busy = false;
toolsRunning = false;
request = wxWebRequest();
}
messages = json::array();
// Globals defined by Lua scripts are forgotten
luaEnv = std::make_shared<LuaEnvironment>();
totalUsage = TokenUsage();
}

bool AgentSession::UndoLastExchange (wxString* prompt) {
if (IsBusy()) return false;
for (size_t i=messages.size(); i-->0; ) {
if (messages[i].value("role", "")!="user") continue;
if (prompt) *prompt = U(messages[i].value("content", ""));
messages.erase(messages.begin() +i, messages.end());
return true;
}
return false;
}

bool AgentSession::Cancel () {
if (toolsRunning) {
// The tool thread stops after the current tool; a running command is killed
*cancelled = true;
return true;
}
if (!busy || !request.IsOk()) return false;
request.Cancel();
return true;
}

json AgentSession::BuildRequestBody () {
json body = json::object();
for (auto& [key, value]: config.extraBody.items()) body[key] = value;
if (!model.empty()) body["model"] = model;
json tools = AgentTools::GetDefinitions(config);
std::string systemPrompt = config.systemPrompt;
if (!tools.empty()) {
if (!systemPrompt.empty()) systemPrompt += "\n\n";
systemPrompt += format("The workspace root is the folder '{}'. All file paths are relative to it and use '/' as separator.", U(GetWorkspaceName(root)));
// Places outside the workspace the agent may use without asking, resolved as absolute paths
ToolContext paths(root, config.limits);
paths.SetAllowedPaths(config.readablePaths, config.writablePaths);
std::string outside;
for (auto& p: paths.readablePaths) outside += (outside.empty()? "" : ", ") + U(p.path) + " (read only)";
for (auto& p: paths.writablePaths) outside += (outside.empty()? "" : ", ") + U(p.path);
if (!outside.empty()) systemPrompt += " Outside of the workspace, you can also access these files and directories, with absolute paths: " + outside + ".";
if (AgentTools::IsEnabled("get_editor_context", config)) systemPrompt += " The user works in a text editor: when they refer to the current file, the selection, the cursor or 'here', call get_editor_context.";
body["tools"] = tools;
}
json msgs = json::array();
if (!systemPrompt.empty()) msgs.push_back({ {"role", "system"}, {"content", systemPrompt} });
for (auto& m: messages) msgs.push_back(m);
body["messages"] = msgs;
if (config.temperature>=0) body["temperature"] = config.temperature;
if (config.maxTokens>0) body["max_tokens"] = config.maxTokens;
body["stream"] = false;
return body;
}

bool AgentSession::StartRequest (const std::string& path, const std::string& body, const RawCallback& callback) {
if (busy) return false;
auto& requester = WebRequester::Get();
request = requester.Create(U(config.url + path), body.empty()? "GET" : "POST");
if (!request.IsOk()) return false;
for (auto& [key, value]: config.headers.items()) {
if (value.is_string()) request.SetHeader(U(key), U(value.get<std::string>()));
}
request.SetTimeouts(30000, config.timeout * 1000L);
if (!body.empty()) requester.SetBody(request, "application/json", body);
busy = true;
std::weak_ptr<bool> weakAlive = alive;
requester.Send(request, [this, weakAlive, callback](const wxWebRequestEvent& e){
auto lock = weakAlive.lock();
if (!lock || !*lock) return;
OnRequestDone(e, callback);
});
return true;
}

void AgentSession::OnRequestDone (const wxWebRequestEvent& e, const RawCallback& callback) {
busy = false;
request = wxWebRequest();
std::string body, error;
switch(e.GetState()) {
case wxWebRequest::State_Cancelled:
error = GetTranslation("AgentRequestCancelled");
break;
case wxWebRequest::State_Failed:
error = U(e.GetErrorDescription());
if (e.GetResponse().IsOk() && e.GetResponse().GetStatus()>0) {
error = format("HTTP {} {}: {}", e.GetResponse().GetStatus(), U(e.GetResponse().GetStatusText()), ExtractErrorMessage(ReadAll(e.GetResponse().GetStream())));
}
break;
case wxWebRequest::State_Completed: {
auto response = e.GetResponse();
body = ReadAll(response.GetStream());
int status = response.GetStatus();
if (status<200 || status>=300) error = format("HTTP {} {}: {}", status, U(response.GetStatusText()), ExtractErrorMessage(body));
}break;
default: return;
}
callback(body, error);
}

bool AgentSession::Send (const wxString& prompt, const ResponseCallback& callback, const ToolCallCallback& toolCallback) {
if (IsBusy()) return false;
cancelled = std::make_shared<std::atomic<bool>>(false);
exchangeStart = messages.size();
toolIterations = 0;
usage = TokenUsage();
pendingCallback = callback;
pendingToolCallback = toolCallback;
messages.push_back({ {"role", "user"}, {"content", U(prompt)} });
bool started = SendChatRequest();
if (!started) messages.erase(messages.begin() +exchangeStart, messages.end());
return started;
}

bool AgentSession::SendChatRequest () {
return StartRequest("chat/completions", BuildRequestBody().dump(), [this](const std::string& body, const std::string& error){
OnChatResponse(body, error);
});
}

void AgentSession::FailExchange (const std::string& error) {
// Remove the unanswered exchange so that the history stays consistent
if (exchangeStart<messages.size()) messages.erase(messages.begin() +exchangeStart, messages.end());
pendingCallback(false, U(error));
}

void AgentSession::OnChatResponse (const std::string& body, std::string error) {
if (error.empty()) try {
json j = json::parse(body);
// Tokens are counted in the conversation total even if the exchange fails later
json u = j.contains("usage")? j["usage"] : json();
usage.Add(u);
totalUsage.Add(u);
auto& message = j.at("choices").at(0).at("message");
auto toolCalls = message.find("tool_calls");
if (toolCalls!=message.end() && toolCalls->is_array() && !toolCalls->empty()) {
HandleToolCalls(message);
return;
}
std::string content = message.contains("content") && message["content"].is_string()? message["content"].get<std::string>() : "";
if (!content.empty()) {
messages.push_back({ {"role", "assistant"}, {"content", content} });
pendingCallback(true, U(content));
return;
}
error = GetTranslation("AgentEmptyResponse");
} catch (std::exception& ex) {
error = format("{}: {}", GetTranslation("AgentInvalidResponse"), ex.what());
}
FailExchange(error);
}

void AgentSession::HandleToolCalls (const json& message) {
if (++toolIterations > config.maxToolIterations) {
FailExchange(format(GetTranslation("AgentTooManyToolCalls"), config.maxToolIterations));
return;
}
json calls = message["tool_calls"];
for (size_t i=0; i<calls.size(); i++) {
if (!calls[i].contains("id") || !calls[i]["id"].is_string()) calls[i]["id"] = format("call_{}_{}", toolIterations, i);
}
std::string content = message.contains("content") && message["content"].is_string()? message["content"].get<std::string>() : "";
messages.push_back({ {"role", "assistant"}, {"content", content}, {"tool_calls", calls} });
ToolContext context(root, config.limits);
// Called on the main thread: the dialog may have been closed, or the conversation cleared, in the meantime
std::weak_ptr<bool> confirmAlive = alive;
auto confirm = confirmCallback;
if (confirm) context.confirm = [confirmAlive, confirm](const wxString& message, const wxString& details, bool denyByDefault){
auto lock = confirmAlive.lock();
if (!lock || !*lock) return ToolConfirmation::Stop;
return confirm(message, details, denyByDefault);
};
context.shell = config.shell;
context.allowedCommands = config.allowedCommands;
context.SetAllowedPaths(config.readablePaths, config.writablePaths);
context.cancelled = cancelled;
context.luaEnv = luaEnv;
// Permissions are computed here, on the main thread, from the configuration; nullopt: unknown tool
std::vector<std::optional<ToolPermission>> permissions;
for (auto& call: calls) {
std::string name = call.contains("function") && call["function"].is_object()? call["function"].value("name", "") : "";
auto tool = AgentTools::Find(name);
permissions.push_back(tool? std::optional<ToolPermission>(AgentTools::GetPermission(*tool, config)) : std::nullopt);
}
// Tools run in their own thread, so that the interface stays responsive during long commands or searches.
// Results come back to the main thread with CallAfter, only if the session still exists.
toolsRunning = true;
std::weak_ptr<bool> weakAlive = alive;
std::thread([this, weakAlive, calls, permissions, context](){
std::vector<ToolResult> results;
bool stopped = false;
for (size_t i=0; i<calls.size() && !context.IsCancelled(); i++) {
auto& call = calls[i];
std::string name, arguments;
if (call.contains("function") && call["function"].is_object()) {
auto& function = call["function"];
name = function.value("name", "");
if (function.contains("arguments")) {
auto& a = function["arguments"];
arguments = a.is_string()? a.get<std::string>() : a.dump();
}}
// The model may call a tool that wasn't offered to it: Execute never runs a denied tool
ToolResult result = permissions[i]? AgentTools::Execute(name, arguments, context, *permissions[i]) : ToolResult::Error(format("Unknown tool: {}. Only use the tools you were given.", name));
results.push_back(result);
json args;
try { args = json::parse(arguments); } catch (...) { args = arguments; }
wxGetApp().CallAfter([this, weakAlive, name, args, result](){
auto lock = weakAlive.lock();
if (lock && *lock && pendingToolCallback) pendingToolCallback(name, args, result);
});
if (result.stop) {
stopped = true;
break;
}}
wxGetApp().CallAfter([this, weakAlive, calls, results, stopped](){
auto lock = weakAlive.lock();
if (lock && *lock) OnToolCallsDone(calls, results, stopped);
});
}).detach();
}

void AgentSession::OnToolCallsDone (const json& calls, const std::vector<ToolResult>& results, bool stopped) {
toolsRunning = false;
if (*cancelled) {
FailExchange(GetTranslation("AgentRequestCancelled"));
return;
}
if (stopped) {
// Remaining tool calls are abandoned along with the whole exchange
FailExchange(GetTranslation("AgentStoppedByUser"));
return;
}
for (size_t i=0; i<results.size(); i++) {
std::string name = calls[i].contains("function") && calls[i]["function"].is_object()? calls[i]["function"].value("name", "") : "";
messages.push_back({ {"role", "tool"}, {"tool_call_id", calls[i]["id"]}, {"name", name}, {"content", results[i].text} });
}
if (!SendChatRequest()) FailExchange(GetTranslation("AgentSendFailed"));
}

bool AgentSession::ListModels (const ModelListCallback& callback, bool forceRefresh) {
auto it = modelListCache.find(config.url);
if (!forceRefresh && it!=modelListCache.end()) {
callback(true, it->second, wxEmptyString);
return true;
}
std::string url = config.url;
return StartRequest("models", "", [url, callback](const std::string& body, std::string error){
std::vector<std::string> models;
if (error.empty()) try {
json j = json::parse(body);
for (auto& m: j.at("data")) {
if (m.contains("id") && m["id"].is_string()) models.push_back(m["id"].get<std::string>());
}
modelListCache[url] = models;
callback(true, models, wxEmptyString);
return;
} catch (std::exception& ex) {
error = format("{}: {}", GetTranslation("AgentInvalidResponse"), ex.what());
}
callback(false, models, U(error));
});
}
