#ifndef _____AGENT_DIALOG_HPP
#define _____AGENT_DIALOG_HPP
#include "../common/wxUtils.hpp"
#include<memory>
#include<vector>

class AgentDialog: public wxDialog {
private:
struct AgentEditor* editor = nullptr;
std::unique_ptr<struct AgentSession> session;
std::vector<long> exchangeStarts;

public:
AgentDialog (wxWindow* parent, const wxString& title, const struct AgentConfig& config, const wxString& root);
~AgentDialog ();
struct AgentEditor* GetEditor () { return editor; }
struct AgentSession* GetSession () { return session.get(); }
const wxString& GetRoot () const;

void ShowAgent ();
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
void SaveConversation ();
void CloseAgent ();

void OnCharHook (wxKeyEvent& e);
void OnClose (wxCloseEvent& e);
void OnContextMenu (wxContextMenuEvent& e);
void OnMenuAction (wxCommandEvent& e);

friend class AgentEditor;
};

#endif
