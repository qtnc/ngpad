#include "AgentSession.hpp"
#include "../app/App.hpp"
#include "../app/WebRequester.hpp"
#include "../common/println.hpp"
#include<unordered_map>

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

AgentSession::AgentSession (const AgentConfig& cfg):
config(cfg), model(cfg.model), messages(json::array()), alive(std::make_shared<bool>(true))
{}

AgentSession::~AgentSession () {
*alive = false;
Cancel();
}

void AgentSession::Clear () {
if (busy) {
// Silently drop the pending request: its callback would write into the cleared conversation
*alive = false;
alive = std::make_shared<bool>(true);
Cancel();
busy = false;
request = wxWebRequest();
}
messages = json::array();
}

bool AgentSession::UndoLastExchange (wxString* prompt) {
if (busy) return false;
for (size_t i=messages.size(); i-->0; ) {
if (messages[i].value("role", "")!="user") continue;
if (prompt) *prompt = U(messages[i].value("content", ""));
messages.erase(messages.begin() +i, messages.end());
return true;
}
return false;
}

bool AgentSession::Cancel () {
if (!busy || !request.IsOk()) return false;
request.Cancel();
return true;
}

json AgentSession::BuildRequestBody () {
json body = json::object();
for (auto& [key, value]: config.extraBody.items()) body[key] = value;
if (!model.empty()) body["model"] = model;
json msgs = json::array();
if (!config.systemPrompt.empty()) msgs.push_back({ {"role", "system"}, {"content", config.systemPrompt} });
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

bool AgentSession::Send (const wxString& prompt, const ResponseCallback& callback) {
messages.push_back({ {"role", "user"}, {"content", U(prompt)} });
bool started = StartRequest("chat/completions", BuildRequestBody().dump(), [this, callback](const std::string& body, const std::string& error){
OnChatResponse(body, error, callback);
});
if (!started) messages.erase(messages.size() -1);
return started;
}

void AgentSession::OnChatResponse (const std::string& body, std::string error, const ResponseCallback& callback) {
if (error.empty()) try {
json j = json::parse(body);
auto& message = j.at("choices").at(0).at("message");
std::string content = message.contains("content") && message["content"].is_string()? message["content"].get<std::string>() : "";
if (!content.empty()) {
messages.push_back({ {"role", "assistant"}, {"content", content} });
callback(true, U(content));
return;
}
error = GetTranslation("AgentEmptyResponse");
} catch (std::exception& ex) {
error = format("{}: {}", GetTranslation("AgentInvalidResponse"), ex.what());
}
// Remove the unanswered prompt so that the history stays consistent
if (!messages.empty() && messages.back().value("role", "")=="user") messages.erase(messages.size() -1);
callback(false, U(error));
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
