#include "AgentEditor.hpp"
#include "../common/println.hpp"
#include "AgentView.hpp"
#include "AgentDocument.hpp"
#include "../app/App.hpp"
#include "../common/stringUtils.hpp"

AgentEditor::AgentEditor (AgentView* view0, wxWindow* parent, int id, const wxString& text, const wxPoint& pos, const wxSize& size, long flags):
wxTextCtrl(parent, id, text, pos, size, flags | wxTE_MULTILINE | wxTE_NOHIDESEL | wxVSCROLL | wxTE_PROCESS_TAB | wxTE_PROCESS_ENTER),
view(view0) 
{
Bind(wxEVT_TEXT_CUT, &AgentEditor::OnCut, this);
Bind(wxEVT_TEXT_PASTE, &AgentEditor::OnPaste, this);
Bind(wxEVT_CHAR_HOOK, &AgentEditor::OnCharHook, this);
Bind(wxEVT_CHAR, &AgentEditor::OnChar, this);
}

AgentDocument* AgentEditor::GetDocument () {
return view? static_cast<AgentDocument*>(view->GetDocument()) : nullptr;
}

bool AgentEditor::IsEditableAtInsertionPoint (bool backspace) {
long ss, se, ip = 0; //view->GetExecutingCommandInsertionPoint();
GetSelection(&ss, &se);
if (backspace) return ss>ip && se>ip && ip>=0;
else  return ss>=ip && se>=ip && ip>=0;
}

void AgentEditor::OnCut (wxClipboardTextEvent& e) {
if (IsEditableAtInsertionPoint()) e.Skip();
else wxBell();
}

void AgentEditor::OnPaste (wxClipboardTextEvent& e) {
if (IsEditableAtInsertionPoint()) e.Skip();
else wxBell();
}

void AgentEditor::OnShiftEnter () {
if (!IsEditableAtInsertionPoint()) {
wxBell();
return;
}
int last = GetLastPosition();
int ip = 0; //view && view->liveAppender1? view->liveAppender1->GetInsertionPoint() : last;
SetInsertionPoint(last);
if (last==ip) {
wxBell();
return;
}
WriteText("\n");
//todo: send request
 }

void AgentEditor::OnCtrlEnter () {
size_t x, y;
PositionToXY(GetInsertionPoint(), reinterpret_cast<long*>(&x), reinterpret_cast<long*>(&y));
wxString text = GetLineText(y);
size_t start=x, end=x;
while (start>0) {
char c = text[start -1];
if (c==' ' || c=='"' || c=='\'' || c=='<' || c=='(' || c=='[' || c=='{' || c==',' || c==';') break;
else start--;
}
while(++end<text.size()) {
char c = text[end];
if (c==' ' || c=='"' || c=='\'' || c=='>' || c==')' || c==']' || c=='}' || c==',' || c==';') break;
else if (c==':' && end<text.size() -1 && text[end+1]==' ') break;
}
wxString cmd = text.substr(start, end-start);
wxGetApp() .DoQuickJump(cmd);
}

bool AgentEditor::OnEnter () {
if (!IsEditableAtInsertionPoint()) return true;
return false;
}

bool AgentEditor::OnBackspace () {
if (!IsEditableAtInsertionPoint(true)) return true;
return false;
}

bool AgentEditor::OnDelete () {
if (!IsEditableAtInsertionPoint()) return true;
return false;
}

void AgentEditor::OnChar (wxKeyEvent& e) {
auto ch = e.GetUnicodeKey();
bool editable = IsEditable();
if (ch!=WXK_NONE && !e.HasAnyModifiers() && (!editable || !IsEditableAtInsertionPoint())) wxBell();
else e.Skip();
}

void AgentEditor::OnCharHook (wxKeyEvent& e) {
int key = e.GetKeyCode(), mod = e.GetModifiers();
switch(key){
case WXK_RETURN:
if (mod==wxMOD_SHIFT) OnShiftEnter();
else if (mod==wxMOD_CONTROL) OnCtrlEnter();
else if (!OnEnter()) e.Skip();
break;
case WXK_BACK:
if (mod!=wxMOD_NONE || !OnBackspace()) e.Skip();
break;
case WXK_DELETE:
if (mod!=wxMOD_NONE || !OnDelete()) e.Skip();
break;
default:
e.Skip();
break;
}}
