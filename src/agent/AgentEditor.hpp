#ifndef _____AGENT_EDITOR_HPP
#define _____AGENT_EDITOR_HPP
#include "../common/wxUtils.hpp"
#include<memory>
#include<vector>

class AgentEditor: public wxTextCtrl {
private:
struct MessageMark { long headerStart, contentStart; };

class AgentDialog* dialog;
int inputInsertionPoint;
std::vector<MessageMark> messageMarks;

void WriteHeader (const wxString& header);
long GetMessageContentEnd (size_t index);

public:
AgentEditor (AgentDialog* dialog, wxWindow* parent, int id, const wxString& text, const wxPoint& pos, const wxSize& size, long flags);

bool IsEditableAtInsertionPoint (bool bs = false);
int GetInputInsertionPoint () { return inputInsertionPoint; }
void SetInputInsertionPoint (int i) { inputInsertionPoint=i; }
void WritePromptInvite ();
void WriteAnswer (const wxString& answer);
void WriteError (const wxString& error);
void WriteInfo (const wxString& info);
void WriteToolCall (const wxString& text);
void Reset ();
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
