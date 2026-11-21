#ifndef _____AGENT_EDITOR_HPP
#define _____AGENT_EDITOR_HPP
#include "../common/wxUtils.hpp"
#include "nlohmann/json.hpp"
#include<memory>
#include<vector>

class AgentEditor: public wxTextCtrl {
private:
struct MessageMark { long headerStart, contentStart; };

class AgentDialog* dialog;
int inputInsertionPoint;
std::vector<MessageMark> messageMarks;
// Display journal: what is written in the conversation, to restore it identically when a saved conversation is opened.
// Each entry is {"kind": "prompt"|"answer"|"error"|"info"|"tool", "text": ..., "stats": ...}; transcriptStarts gives the position where each one starts.
nlohmann::json transcript = nlohmann::json::array();
std::vector<long> transcriptStarts;

void WriteHeader (const wxString& header);
void Record (const char* kind, long start, const wxString& text, const wxString& stats = wxEmptyString);
long GetMessageContentEnd (size_t index);

public:
AgentEditor (AgentDialog* dialog, wxWindow* parent, int id, const wxString& text, const wxPoint& pos, const wxSize& size, long flags);

bool IsEditableAtInsertionPoint (bool bs = false);
int GetInputInsertionPoint () { return inputInsertionPoint; }
void SetInputInsertionPoint (int i) { inputInsertionPoint=i; }
void WritePromptInvite ();
void WriteAnswer (const wxString& answer, const wxString& stats = wxEmptyString);
void WriteError (const wxString& error);
void WriteInfo (const wxString& info);
void WriteToolCall (const wxString& text);
void Reset ();
// Journal of the entries written before the position pos (-1 = all)
nlohmann::json GetTranscript (long before = -1) const;
// Rewrite a saved journal; exchangeStarts receives the start of each successful exchange, as recorded by AgentDialog
void Restore (const nlohmann::json& transcript, std::vector<long>& exchangeStarts);
long GetCurrentMessageStart ();
void TruncateFrom (long pos);
bool MoveToPreviousMessage (bool select);
bool MoveToNextMessage (bool select);

void OnChar  (wxKeyEvent& e);
void OnCharHook (wxKeyEvent& e);
void OnCut (wxClipboardTextEvent& e);
void OnPaste (wxClipboardTextEvent& e);
bool OnEnter ();
void OnShiftEnter ();
void OnCtrlEnter ();
bool OnBackspace ();
bool OnDelete ();

friend class AgentDialog;
};

#endif
