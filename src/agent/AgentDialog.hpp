#ifndef _____AGENT_DIALOG_HPP
#define _____AGENT_DIALOG_HPP
#include "../common/wxUtils.hpp"
#include "nlohmann/json.hpp"
#include<memory>
#include<vector>

class AgentDialog: public wxDialog {
private:
struct AgentEditor* editor = nullptr;
std::unique_ptr<struct AgentSession> session;
std::vector<long> exchangeStarts;
long pendingStart = -1; // Start of the exchange in progress in the editor, -1 if none
wxString defaultTitle, customTitle;
wxString filePath; // File where the conversation is saved, empty if not saved yet
bool autoFile = false; // filePath was created by the automatic save, in the conversations directory
bool autoSaveErrorShown = false;

nlohmann::ordered_json BuildConversationData ();
bool WriteConversation (const wxString& path, bool automatic, wxString& error);
bool ExportText (const wxString& path);

public:
AgentDialog (wxWindow* parent, const wxString& title, const struct AgentConfig& config, const wxString& root);
~AgentDialog ();
struct AgentEditor* GetEditor () { return editor; }
struct AgentSession* GetSession () { return session.get(); }
const wxString& GetRoot () const;
const wxString& GetFilePath () const { return filePath; }

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
void SetCustomTitle (const wxString& title);
// Ctrl+S: in the current file if it was chosen by the user, otherwise as Save as
void SaveConversation ();
void SaveConversationAs ();
// After each change: saved in the current file, or in a new file of the conversations directory once there is a successful exchange
void AutoSave ();
// Ctrl+O: choose a saved conversation; it replaces this one if it has the same agent and root, otherwise it opens in a new window
void OpenConversation ();
// Replace the conversation by a saved one, already read and checked by ReadConversationFile
bool LoadConversation (const nlohmann::json& data, const wxString& path);
void CloseAgent ();

void OnCharHook (wxKeyEvent& e);
void OnClose (wxCloseEvent& e);
void OnContextMenu (wxContextMenuEvent& e);
void OnMenuAction (wxCommandEvent& e);

friend class AgentEditor;
};

#endif
