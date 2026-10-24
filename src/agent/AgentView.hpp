#ifndef _____AGENT_VIEW_HPP
#define _____AGENT_VIEW_HPP
#include "../app/AbstractView.hpp"
#include<memory>
#include<vector>

class AgentView: public AbstractView {
private:
struct AgentEditor* editor = nullptr;
std::unique_ptr<struct AgentSession> session;
std::vector<long> exchangeStarts;

public:
AgentView ();
~AgentView ();
struct AgentEditor* GetEditor () { return editor; }
struct AgentSession* GetSession () { return session.get(); }

void SetAgent (const struct AgentConfig& config, const wxString& root);
bool IsBusy ();
void ClearAgent ();
void Send (const wxString& prompt);
void SendPrompt (const wxString& prompt);
void ExecuteCommand (const wxString& name, const wxString& arg);
void ChangeModel (const wxString& name);
void ShowModelList ();
void ShowModelDialog ();
void CancelRequest ();
void UndoLastExchange (bool retry);

bool OnCreate (wxDocument* doc, long flags) override;
void OnUpdate (wxView* sender, wxObject* param) override;
bool OnMenuAction (wxCommandEvent& e) override;

friend class AgentDocument;
friend class AgentEditor;

wxDECLARE_DYNAMIC_CLASS(AgentView);
};

#endif
