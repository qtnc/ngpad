#ifndef _____AGENT_SESSION_HPP
#define _____AGENT_SESSION_HPP
#include "AgentConfig.hpp"
#include "AgentTools.hpp"
#include <wx/webrequest.h>
#include<functional>
#include<memory>
#include<vector>

wxString GetWorkspaceName (const wxString& root);

class AgentSession {
public:
typedef std::function<void(bool success, const wxString& text)> ResponseCallback;
typedef std::function<void(bool success, const std::vector<std::string>& models, const wxString& error)> ModelListCallback;
typedef std::function<void(const std::string& name, const nlohmann::json& args, const ToolResult& result)> ToolCallCallback;

private:
typedef std::function<void(const std::string& body, const std::string& error)> RawCallback;

AgentConfig config;
wxString root;
std::string model;
nlohmann::json messages;
wxWebRequest request;
bool busy = false;
std::shared_ptr<bool> alive;
ResponseCallback pendingCallback;
ToolCallCallback pendingToolCallback;
ToolContext::ConfirmCallback confirmCallback;
size_t exchangeStart = 0;
int toolIterations = 0;

nlohmann::json BuildRequestBody ();
bool StartRequest (const std::string& path, const std::string& body, const RawCallback& callback);
void OnRequestDone (const wxWebRequestEvent& e, const RawCallback& callback);
bool SendChatRequest ();
void OnChatResponse (const std::string& body, std::string error);
void HandleToolCalls (const nlohmann::json& message);
void FailExchange (const std::string& error);

public:
AgentSession (const AgentConfig& config, const wxString& root);
~AgentSession ();

const AgentConfig& GetConfig () const { return config; }
const wxString& GetRoot () const { return root; }
const std::string& GetModel () const { return model; }
void SetModel (const std::string& m) { model = m; }
bool IsBusy () const { return busy; }
void SetConfirmCallback (const ToolContext::ConfirmCallback& callback) { confirmCallback = callback; }

void Clear ();
bool UndoLastExchange (wxString* prompt = nullptr);
bool Send (const wxString& prompt, const ResponseCallback& callback, const ToolCallCallback& toolCallback = nullptr);
bool ListModels (const ModelListCallback& callback, bool forceRefresh = false);
bool Cancel ();
};

#endif
