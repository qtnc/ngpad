#include "AgentSession.hpp"
#include "../app/App.hpp"
#include "../app/WebRequester.hpp"
#include "../common/println.hpp"

using json = nlohmann::json;

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
Cancel();
messages = json::array();
}

void AgentSession::Cancel () {
if (busy && request.IsOk()) request.Cancel();
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

bool AgentSession::Send (const wxString& prompt, const ResponseCallback& callback) {
if (busy) return false;
auto& requester = WebRequester::Get();
request = requester.Create(U(config.url + "chat/completions"), "POST");
if (!request.IsOk()) return false;
messages.push_back({ {"role", "user"}, {"content", U(prompt)} });
for (auto& [key, value]: config.headers.items()) {
if (value.is_string()) request.SetHeader(U(key), U(value.get<std::string>()));
}
request.SetTimeouts(30000, config.timeout * 1000L);
requester.SetBody(request, "application/json", BuildRequestBody().dump());
busy = true;
std::weak_ptr<bool> weakAlive = alive;
requester.Send(request, [this, weakAlive, callback](const wxWebRequestEvent& e){
auto lock = weakAlive.lock();
if (!lock || !*lock) return;
OnResponse(e, callback);
});
return true;
}

void AgentSession::OnResponse (const wxWebRequestEvent& e, const ResponseCallback& callback) {
busy = false;
request = wxWebRequest();
std::string error;
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
std::string body = ReadAll(response.GetStream());
int status = response.GetStatus();
if (status<200 || status>=300) {
error = format("HTTP {} {}: {}", status, U(response.GetStatusText()), ExtractErrorMessage(body));
break;
}
try {
json j = json::parse(body);
auto& message = j.at("choices").at(0).at("message");
std::string content = message.contains("content") && message["content"].is_string()? message["content"].get<std::string>() : "";
if (content.empty()) {
error = GetTranslation("AgentEmptyResponse");
break;
}
messages.push_back({ {"role", "assistant"}, {"content", content} });
callback(true, U(content));
return;
} catch (std::exception& ex) {
error = format("{}: {}", GetTranslation("AgentInvalidResponse"), ex.what());
}
}break;
default: return;
}
// Remove the unanswered prompt so that the history stays consistent
if (!messages.empty() && messages.back().value("role", "")=="user") messages.erase(messages.size() -1);
callback(false, U(error));
}
