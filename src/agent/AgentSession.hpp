#ifndef _____AGENT_SESSION_HPP
#define _____AGENT_SESSION_HPP
#include "AgentConfig.hpp"
#include <wx/webrequest.h>
#include<functional>
#include<memory>

class AgentSession {
public:
typedef std::function<void(bool success, const wxString& text)> ResponseCallback;

private:
AgentConfig config;
std::string model;
nlohmann::json messages;
wxWebRequest request;
bool busy = false;
std::shared_ptr<bool> alive;

nlohmann::json BuildRequestBody ();
void OnResponse (const wxWebRequestEvent& e, const ResponseCallback& callback);

public:
AgentSession (const AgentConfig& config);
~AgentSession ();

const AgentConfig& GetConfig () const { return config; }
const std::string& GetModel () const { return model; }
void SetModel (const std::string& m) { model = m; }
bool IsBusy () const { return busy; }

void Clear ();
bool Send (const wxString& prompt, const ResponseCallback& callback);
void Cancel ();
};

#endif
