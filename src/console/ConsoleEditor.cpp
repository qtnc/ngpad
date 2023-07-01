#include "ConsoleEditor.hpp"
#include "../common/println.hpp"
#include "ConsoleView.hpp"
#include "ConsoleDocument.hpp"
#include "../app/App.hpp"
#include "../common/stringUtils.hpp"

ConsoleEditor::ConsoleEditor (ConsoleView* view0, wxWindow* parent, int id, const wxString& text, const wxPoint& pos, const wxSize& size, long flags):
wxTextCtrl(parent, id, text, pos, size, flags | wxTE_MULTILINE | wxTE_NOHIDESEL | wxVSCROLL | wxTE_PROCESS_TAB | wxTE_PROCESS_ENTER),
view(view0) 
{
Bind(wxEVT_TEXT_CUT, &ConsoleEditor::OnCut, this);
Bind(wxEVT_TEXT_PASTE, &ConsoleEditor::OnPaste, this);
Bind(wxEVT_CHAR_HOOK, &ConsoleEditor::OnCharHook, this);
Bind(wxEVT_CHAR, &ConsoleEditor::OnChar, this);
}

ConsoleDocument* ConsoleEditor::GetDocument () {
return view? static_cast<ConsoleDocument*>(view->GetDocument()) : nullptr;
}

bool ConsoleEditor::IsEditableAtInsertionPoint (bool backspace) {
long ss, se, ip = view->GetExecutingCommandInsertionPoint();
GetSelection(&ss, &se);
if (backspace) return ss>ip && se>ip && ip>=0;
else  return ss>=ip && se>=ip && ip>=0;
}

void ConsoleEditor::OnHistoryNext () {
if (++historyIndex > view->history.size()) historyIndex = 0;
RecallHistory();
}

void ConsoleEditor::OnHistoryPrevious () {
if (--historyIndex > view->history.size()) historyIndex = view->history.size();
RecallHistory();
}

void ConsoleEditor::RecallHistory () {
int last = GetLastPosition();
int ip = view && view->liveAppender1? view->liveAppender1->GetInsertionPoint() : last;
if (historyIndex>=view->history.size()) Remove(ip, last);
else {
const wxString& s = view->history[historyIndex];
Replace(ip, last, s);
SetSelection(ip, ip+s.size());
}
}

void ConsoleEditor::OnCut (wxClipboardTextEvent& e) {
if (IsEditableAtInsertionPoint()) e.Skip();
else wxBell();
}

void ConsoleEditor::OnPaste (wxClipboardTextEvent& e) {
if (IsEditableAtInsertionPoint()) e.Skip();
else wxBell();
}

void ConsoleEditor::OnEnter () {
if (!IsEditableAtInsertionPoint()) {
wxBell();
return;
}
int last = GetLastPosition();
int ip = view && view->liveAppender1? view->liveAppender1->GetInsertionPoint() : last;
SetInsertionPoint(last);
if (last==ip) {
wxBell();
return;
}
WriteText("\n");
view->AddToHistory(GetRange(ip, last));
view->ExecutingCommandSubmit();
historyIndex = view->history.size();
 }

void ConsoleEditor::OnCtrlEnter () {
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

bool ConsoleEditor::OnBackspace () {
if (!IsEditableAtInsertionPoint(true)) return true;
return false;
}

bool ConsoleEditor::OnDelete () {
if (!IsEditableAtInsertionPoint()) return true;
return false;
}

void ConsoleEditor::OnChar (wxKeyEvent& e) {
auto ch = e.GetUnicodeKey();
bool editable = IsEditable();
if (ch!=WXK_NONE && !e.HasAnyModifiers() && (!editable || !IsEditableAtInsertionPoint())) wxBell();
else e.Skip();
}

void ConsoleEditor::OnCharHook (wxKeyEvent& e) {
int key = e.GetKeyCode(), mod = e.GetModifiers();
switch(key){
case WXK_RETURN:
if (mod==wxMOD_NONE) OnEnter();
else if (mod==wxMOD_CONTROL) OnCtrlEnter();
else e.Skip();
break;
case WXK_BACK:
if (mod!=wxMOD_NONE || !OnBackspace()) e.Skip();
break;
case WXK_DELETE:
if (mod!=wxMOD_NONE || !OnDelete()) e.Skip();
break;
case WXK_UP:
if (mod==wxMOD_ALT) OnHistoryPrevious();
else e.Skip();
break;
case WXK_DOWN:
if (mod==wxMOD_ALT) OnHistoryNext();
else e.Skip();
break;
default:
e.Skip();
break;
}}
