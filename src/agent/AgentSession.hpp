#ifndef _____AGENT_SESSION_HPP
#define _____AGENT_SESSION_HPP
#include "AgentConfig.hpp"
#include <wx/webrequest.h>
#include<functional>
#include<memory>
#include<vector>

class AgentSession {
public:
typedef std::function<void(bool success, const wxString& text)> ResponseCallback;
typedef std::function<void(bool success, const std::vector<std::string>& models, const wxString& error)> ModelListCallback;

private:
typedef std::function<void(const std::string& body, const std::string& error)> RawCallback;

AgentConfig config;
std::string model;
nlohmann::json messages;
wxWebRequest request;
bool busy = false;
std::shared_ptr<bool> alive;

nlohmann::json BuildRequestBody ();
bool StartRequest (const std::string& path, const std::string& body, const RawCallback& callback);
void OnRequestDone (const wxWebRequestEvent& e, const RawCallback& callback);
void OnChatResponse (const std::string& body, std::string error, const ResponseCallback& callback);

public:
AgentSession (const AgentConfig& config);
~AgentSession ();

const AgentConfig& GetConfig () const { return config; }
const std::string& GetModel () const { return model; }
void SetModel (const std::string& m) { model = m; }
bool IsBusy () const { return busy; }

void Clear ();
bool UndoLastExchange (wxString* prompt = nullptr);
bool Send (const wxString& prompt, const ResponseCallback& callback);
bool ListModels (const ModelListCallback& callback, bool forceRefresh = false);
bool Cancel ();
};

#endif
