#ifndef _____AGENT_EDITOR_HPP
#define _____AGENT_EDITOR_HPP
#include "../common/wxUtils.hpp"
#include "wx/docview.h"
#include<memory>

class AgentEditor: public wxTextCtrl {
private:
struct AgentView* view;
int inputInsertionPoint;

public:
AgentEditor (AgentView* view, wxWindow* parent, int id, const wxString& text, const wxPoint& pos, const wxSize& size, long flags);

struct AgentDocument* GetDocument ();

bool IsEditableAtInsertionPoint (bool bs = false);
int GetInputInsertionPoint () { return inputInsertionPoint; }
void SetInputInsertionPoint (int i) { inputInsertionPoint=i; }
void WritePromptInvite ();
void WriteAnswer (const wxString& answer);
void WriteError (const wxString& error);

void OnChar  (wxKeyEvent& e);
void OnCharHook (wxKeyEvent& e);
void OnCut (wxClipboardTextEvent& e);
void OnPaste (wxClipboardTextEvent& e);
bool OnEnter ();
void OnShiftEnter ();
void OnCtrlEnter ();
bool OnBackspace ();
bool OnDelete ();

friend class AgentView;
friend class AgentDocument;
};

#endif
