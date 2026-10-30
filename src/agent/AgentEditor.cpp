#include "AgentEditor.hpp"
#include "../common/println.hpp"
#include "AgentDialog.hpp"
#include "../app/App.hpp"
#include "../common/stringUtils.hpp"

AgentEditor::AgentEditor (AgentDialog* dialog0, wxWindow* parent, int id, const wxString& text, const wxPoint& pos, const wxSize& size, long flags):
wxTextCtrl(parent, id, text, pos, size, flags | wxTE_MULTILINE | wxTE_NOHIDESEL | wxVSCROLL | wxTE_PROCESS_TAB | wxTE_PROCESS_ENTER),
dialog(dialog0)
{
Bind(wxEVT_TEXT_CUT, &AgentEditor::OnCut, this);
Bind(wxEVT_TEXT_PASTE, &AgentEditor::OnPaste, this);
Bind(wxEVT_CHAR_HOOK, &AgentEditor::OnCharHook, this);
Bind(wxEVT_CHAR, &AgentEditor::OnChar, this);
}

bool AgentEditor::IsEditableAtInsertionPoint (bool backspace) {
if (dialog && dialog->IsBusy()) return false;
long ss, se, ip = GetInputInsertionPoint();
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
if (dialog && dialog->IsBusy()) {
wxBell();
wxGetApp().SayText(MSG("AgentBusy"));
return;
}
if (!IsEditableAtInsertionPoint()) {
wxBell();
return;
}
int last = GetLastPosition();
int ip = GetInputInsertionPoint();
SetInsertionPoint(last);
if (last==ip) {
wxBell();
return;
}
wxString prompt = GetRange(ip, last);
WriteText("\n\n");
SetInputInsertionPoint(GetInsertionPoint());
dialog->Send(prompt);
 }

void AgentEditor::WriteHeader (const wxString& header) {
SetInsertionPoint(GetLastPosition());
long headerStart = GetLastPosition();
WriteText(header);
WriteText("\n");
messageMarks.push_back({ headerStart, GetLastPosition() });
}

void AgentEditor::WriteAnswer (const wxString& a) {
WriteHeader(MSG("AIResponse"));
WriteText(a);
WriteText("\n\n");
WritePromptInvite();
}

void AgentEditor::WriteError (const wxString& e) {
WriteHeader(MSG("AIError"));
WriteText(e);
WriteText("\n\n");
WritePromptInvite();
}

void AgentEditor::WriteInfo (const wxString& info) {
SetInsertionPoint(GetLastPosition());
WriteText(info);
WriteText("\n\n");
WritePromptInvite();
}

void AgentEditor::WriteToolCall (const wxString& text) {
SetInsertionPoint(GetLastPosition());
WriteText(text);
WriteText("\n");
}

void AgentEditor::WritePromptInvite () {
WriteHeader(MSG("PromptInvite"));
SetInputInsertionPoint(GetLastPosition());
}

void AgentEditor::Reset () {
Clear();
messageMarks.clear();
WritePromptInvite();
}

long AgentEditor::GetCurrentMessageStart () {
return messageMarks.empty()? 0 : messageMarks.back().headerStart;
}

void AgentEditor::TruncateFrom (long pos) {
Remove(pos, GetLastPosition());
while (!messageMarks.empty() && messageMarks.back().headerStart>=pos) messageMarks.pop_back();
WritePromptInvite();
}

long AgentEditor::GetMessageContentEnd (size_t index) {
if (index+1>=messageMarks.size()) return GetLastPosition();
long start = messageMarks[index].contentStart, end = messageMarks[index+1].headerStart;
while (end>start) {
wxString c = GetRange(end -1, end);
if (c!="\n" && c!="\r") break;
end--;
}
return end;
}

bool AgentEditor::MoveToPreviousMessage (bool select) {
long start, end;
GetSelection(&start, &end);
for (size_t i=messageMarks.size(); i-->0; ) {
long pos = messageMarks[i].contentStart;
if (pos>=start) continue;
if (select) SetSelection(pos, end);
else SetInsertionPoint(pos);
ShowPosition(pos);
return true;
}
return false;
}

bool AgentEditor::MoveToNextMessage (bool select) {
long start, end;
GetSelection(&start, &end);
for (size_t i=0, n=messageMarks.size(); i<n; i++) {
long pos = select? GetMessageContentEnd(i) : messageMarks[i].contentStart;
if (pos<=end) continue;
if (select) SetSelection(start, pos);
else SetInsertionPoint(pos);
ShowPosition(pos);
return true;
}
return false;
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
case WXK_UP:
if (mod==wxMOD_CONTROL) BellIfFalse(MoveToPreviousMessage(false));
else if (mod==(wxMOD_CONTROL | wxMOD_SHIFT)) BellIfFalse(MoveToPreviousMessage(true));
else e.Skip();
break;
case WXK_DOWN:
if (mod==wxMOD_CONTROL) BellIfFalse(MoveToNextMessage(false));
else if (mod==(wxMOD_CONTROL | wxMOD_SHIFT)) BellIfFalse(MoveToNextMessage(true));
else e.Skip();
break;
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
