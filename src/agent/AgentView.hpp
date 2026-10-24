#ifndef _____AGENT_VIEW_HPP
#define _____AGENT_VIEW_HPP
#include "../app/AbstractView.hpp"
#include<memory>

class AgentView: public AbstractView {
private:
struct AgentEditor* editor = nullptr;
std::unique_ptr<struct AgentSession> session;

public:
AgentView ();
~AgentView ();
struct AgentEditor* GetEditor () { return editor; }
struct AgentSession* GetSession () { return session.get(); }

void SetAgent (const struct AgentConfig& config);
bool IsBusy ();
void ClearAgent ();
void Send (const wxString& prompt);

bool OnCreate (wxDocument* doc, long flags) override;
void OnUpdate (wxView* sender, wxObject* param) override;
bool OnMenuAction (wxCommandEvent& e) override;

friend class AgentDocument;
friend class AgentEditor;

wxDECLARE_DYNAMIC_CLASS(AgentView);
};

#endif
