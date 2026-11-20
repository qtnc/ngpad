#ifndef _____AGENT_SESSION_HPP
#define _____AGENT_SESSION_HPP
#include "AgentConfig.hpp"
#include "AgentTools.hpp"
#include <wx/webrequest.h>
#include<functional>
#include<memory>
#include<vector>

wxString GetWorkspaceName (const wxString& root);

// Token counts reported by the server (usage field), summed over all the requests of an exchange
struct TokenUsage {
long long input = 0, output = 0, cached = 0;
long long lastInput = 0; // Input tokens of the last request, i.e. the current size of the context
int requests = 0;
bool hasUsage = false, hasCached = false;
void Add (const nlohmann::json& usage);
};

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
bool busy = false; // An HTTP request is pending
bool toolsRunning = false; // Tool calls are being executed in their thread
std::shared_ptr<bool> alive;
std::shared_ptr<std::atomic<bool>> cancelled; // Cancellation of the current exchange, shared with the tool thread
std::shared_ptr<LuaEnvironment> luaEnv;
std::shared_ptr<SessionGrants> grants; // Kept until the agent is closed, even by Clear
ResponseCallback pendingCallback;
ToolCallCallback pendingToolCallback;
ToolContext::ConfirmCallback confirmCallback;
size_t exchangeStart = 0;
int toolIterations = 0;
TokenUsage usage, totalUsage;

nlohmann::json BuildRequestBody ();
bool StartRequest (const std::string& path, const std::string& body, const RawCallback& callback);
void OnRequestDone (const wxWebRequestEvent& e, const RawCallback& callback);
bool SendChatRequest ();
void OnChatResponse (const std::string& body, std::string error);
void HandleToolCalls (const nlohmann::json& message);
void OnToolCallsDone (const nlohmann::json& calls, const std::vector<ToolResult>& results, bool stopped);
void FailExchange (const std::string& error);

public:
AgentSession (const AgentConfig& config, const wxString& root);
~AgentSession ();

const AgentConfig& GetConfig () const { return config; }
const wxString& GetRoot () const { return root; }
const std::string& GetModel () const { return model; }
void SetModel (const std::string& m) { model = m; }
bool IsBusy () const { return busy || toolsRunning; }
const TokenUsage& GetLastUsage () const { return usage; }
const TokenUsage& GetTotalUsage () const { return totalUsage; }
void SetConfirmCallback (const ToolContext::ConfirmCallback& callback) { confirmCallback = callback; }

void Clear ();
bool UndoLastExchange (wxString* prompt = nullptr);
bool Send (const wxString& prompt, const ResponseCallback& callback, const ToolCallCallback& toolCallback = nullptr);
bool ListModels (const ModelListCallback& callback, bool forceRefresh = false);
bool Cancel ();
};

#endif
