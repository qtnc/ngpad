#include "TextView.hpp"
#include "TextDocument.hpp"
#include "TextEditor.hpp"
#include "TreeMarkerJumpList.hpp"
#include "../common/println.hpp"
#include "../app/App.hpp"
#include "FindReplaceDialog.hpp"
#include "../common/stringUtils.hpp"
#include <wx/numdlg.h>
#include <wx/cmdproc.h>
#include <wx/process.h>
#include "../common/println.hpp"
//#include "TestPopup.hpp"

wxIMPLEMENT_DYNAMIC_CLASS(TextView, wxView);

int GetEncodingIndex (int encoding);
wxMBConv& GetEncodingCodec (int encoding);

static inline void BindUpdateStatus (TextView* _this, wxControl* ctl) {
ctl->Bind(wxEVT_TEXT, [=](auto&e){ _this->GetDocument()->Modify(true); e.Skip(); });
ctl->Bind(wxEVT_KEY_UP, [=](auto&e){ _this->UpdateStatus(); e.Skip(); });
ctl->Bind(wxEVT_LEFT_UP, [=](auto&e){ _this->UpdateStatus(); e.Skip(); });
ctl->Bind(wxEVT_RIGHT_UP, [=](auto&e){ _this->UpdateStatus(); e.Skip(); });
ctl->Bind(wxEVT_MIDDLE_UP, [=](auto&e){ _this->UpdateStatus(); e.Skip(); });
}

bool TextView::OnCreate (wxDocument* wxdoc, long flags) {
if (!AbstractView::OnCreate(wxdoc, flags)) return false;
auto doc = static_cast<TextDocument*>(wxdoc);
auto& app = wxGetApp();
auto childWindow = GetFrame();
auto sizer = new wxBoxSizer(wxVERTICAL);
splitter = new wxSplitterWindow(childWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxSP_3D | wxSP_LIVE_UPDATE);
splitter->SetMinimumPaneSize(15);
splitter->SetSashGravity(0.3);

editor = TextEditor::Create(*this, splitter, doc->properties);
auto ctl = editor->GetControl();
BindUpdateStatus(this, ctl);
ctl->SetFocus();

splitter->Initialize(ctl);
sizer->Add(splitter, 1, wxEXPAND);
childWindow->SetSizerAndFit(sizer);

auto mainWindow = wxGetApp() .GetMainWindow();
auto notebook = wxGetApp() .GetNotebook();
#ifdef DEBUG
#define PS(O) { if (O) { wxSize size = O->GetSize(); wxPoint pt = O->GetPosition(); println("{} size = {}, {}, position = {}, {}, type = {}", #O, size.x, size.y, pt.x, pt.y, typeid(*O).name() ); }}
PS(mainWindow)
PS(notebook)
PS(childWindow)
PS(splitter)
PS(ctl)
#undef PS
#endif

return true;
}

bool TextView::FindReplaceDefined () {
return !findReplace.find.empty() && !(findReplace.flags&(FRI_REPLACE | FRI_MULTIPLE));
}

bool TextView::FindReplace (const FindReplaceInfo& info, const std::vector<FindResultInfo>& results) {
findReplace = info;
if (findReplace.flags&FRI_REPLACE) return DoReplace(results);
else if (findReplace.flags&FRI_UPWARD) return FindPrevious();
else return FindNext();
}

bool TextView::FindNext (const FindReplaceInfo& info, bool reverse, bool selectUntil) {
wxString text = editor->GetValue();

size_t start=0, end=0;
editor->GetSelection(reinterpret_cast<long*>(&start), reinterpret_cast<long*>(&end));
size_t initialStart = reverse? std::min(start,end) : std::max(start,end);

if (
(reverse && !info.FindPrevious(text, start, end))
|| (!reverse && !info.FindNext(text, start, end))
) return false;

if (start!=std::string::npos) {
if (selectUntil) start = initialStart;
editor->SetSelection(start, end);
return true;
}
return false;
}

bool TextView::FindNext () {
if (FindReplaceDefined()) return FindNext(findReplace, false, false);
else return OpenFindReplaceDialog(FRI_FIND);
}

bool TextView::FindPrevious () {
if (FindReplaceDefined())  return FindNext(findReplace, true, false);
else return OpenFindReplaceDialog(FRI_FIND | FRI_UPWARD);
}

bool TextView::DoReplace (const std::vector<FindResultInfo>& results) {
std::vector<size_t> acceptedReplacements;
for (auto& result: results) if (result.enabled) acceptedReplacements.push_back(result.pos);
int count = findReplace.ReplaceAllInEditor(GetEditor(), acceptedReplacements);
return count>0;
}

bool TextView::OpenFindReplaceDialog (int flags) {
if (findReplaceDialog && flags==findReplace.flags) {
findReplaceDialog->SetFocus();
return true;
}
if (findReplaceDialog) findReplaceDialog->Destroy();
findReplace.flags &= ~(FRI_REPLACE | FRI_MULTIPLE);
findReplace.flags |= flags;
findReplaceDialog = new FindReplaceDialog(GetFrame(), findReplace, this);
findReplaceDialog->Bind(wxEVT_DESTROY, [this](auto&e){ findReplaceDialog=nullptr; e.Skip(); });
findReplaceDialog->Show();
return true;
}

void TextView::OnUpdate (wxView* sender, wxObject* param) {
UpdateStatus();
UpdateMenus();
}

void TextView::UpdateMenus () {
auto doc = static_cast<TextDocument*>(GetDocument());
auto frame = GetParentWindow<wxFrame>(GetFrame());
auto menubar = frame? frame->GetMenuBar() : nullptr;
if (!menubar) return;
if (!menubar->FindItem(IDM_ENCODING)) return; 

int encoding = GetEncodingIndex(doc->GetEncoding());
int lineEnding = doc->GetLineEnding();
int indentType = doc->GetIndentType();
menubar->Check( encoding<0? IDM_ENCODING_OTHER : IDM_ENCODING + encoding, true);
menubar->Check( lineEnding<0? IDM_LINE_ENDING_OTHER : IDM_LINE_ENDING + lineEnding, true);
menubar->Check(IDM_INDENT + std::max(indentType, 0), true);
menubar->Check(IDM_READONLY, !editor->IsEditable());
}

static int countNonBlankChars (const wxString& text) {
int count = 0;
for (wxChar c: text) if (c>32) count++;
return count;
}

static int countWords (const wxString& text) {
int count = 0;
bool inWord = false;
wxString nonWordChars = " \r\n\t.,;:?!()[]{}/\\";
for (wxChar c: text) {
bool nonWordChar = nonWordChars.find(c)!=std::string::npos;
if (nonWordChar && inWord) {
count++;
inWord = false;
}
else if (!nonWordChar && !inWord) inWord=true;
}
if (inWord) count++;
return count;
}

void TextView::UpdateStatus () {
if (!wxGetApp().IsValidView(this)) return;
auto status = GetStatusBar();
if (!editor || !status) return;

auto last = editor->GetLastPosition();
auto [start, end] = editor->GetSelection();
auto [sx, sy] = editor->PositionToXY(start);
auto [ex, ey] = editor->PositionToXY(end);
auto [lx, ly] = editor->PositionToXY(last);
auto prc = last? 100 * std::max(start, end) / last :0;
wxString s[3];

switch(statusBarDisplayModes[0]){
case 0: // line / column selection
s[0] = U(format(
start==end? GetTranslation("StatusNoSel") : GetTranslation("StatusWithSel"),
sy+1, sx+1, ey+1, ex+1));
break;
}
switch (statusBarDisplayModes[1]) {
case 0: // number of lines
s[1] = U(format(
ly>1? GetTranslation("n-lines") : GetTranslation("n-line"), ly+1));
break;
case 1: // number of characters
s[1] = U(format(GetTranslation("n-chars"), last));
break;
case 2: // number of non-blank characters
s[1] = U(format(GetTranslation("n-nb-chars"), countNonBlankChars(editor->GetValue()) ));
break;
case 3: // number of words
s[1] = U(format(GetTranslation("n-words"), countWords(editor->GetValue()) ));
break;
}
switch(statusBarDisplayModes[2]) {
case 0: // percentage over the whole document
s[2] = U(format("{}%", prc));
break;
}
for (int i=0; i<3; i++) status->SetStatusText(s[i], i);
}

void TextView::OnStatusBarClick (wxMouseEvent& e, int field) {
unsigned char sbmmax[3] = { 1, 4, 1 };
statusBarDisplayModes[field] = (statusBarDisplayModes[field] +1) % sbmmax[field];
UpdateStatus();
e.Skip();
}

void TextView::OnSwitchPane () {
auto ctl = editor->GetControl();
if ((tree && tree->HasFocus()) || (list && list->HasFocus())) ctl->SetFocus();
else if (ctl->HasFocus() && tree) tree->SetFocus();
else if (ctl->HasFocus() && list) list->SetFocus();
else if (!splitter->IsSplit()) OnShowLateralPane();
else ctl->SetFocus();
}

void TextView::OnChangeLineWrap (bool wrap) {
auto doc = static_cast<TextDocument*>(GetDocument());
doc->properties.put("line_wrap", wrap);
auto oldEditor = editor;
auto newEditor = oldEditor->SetAutoWrap(wrap);
if (oldEditor==newEditor) return;
auto oldCtl = oldEditor->GetControl();
auto newCtl = newEditor->GetControl();
splitter->ReplaceWindow(oldCtl, newCtl);
editor = newEditor;
BindUpdateStatus(this, newCtl);
newCtl->SetFocus();
delete oldEditor;
}

void TextView::OnShowLateralPane () {
if (splitter->IsSplit()) {
splitter->Unsplit(tree? (wxWindow*)tree : (wxWindow*)list );
editor->GetControl()->SetFocus();
delete tree;
delete list;
tree = nullptr;
list = nullptr;
}
else {
tree = new TreeMarkerJumpList(splitter, editor);
splitter->SplitVertically(tree, editor->GetControl(), GetFrame()->GetClientSize().x /3);
tree->SetFocus();
}
menubar->Check(IDM_SHOW_PANE, splitter->IsSplit());
}

bool TextView::OnMenuAction (wxCommandEvent& e) {
if (AbstractView::OnMenuAction(e)) return true;
size_t id = e.GetId();
switch(id){
case wxID_FIND: OpenFindReplaceDialog(FRI_FIND); break;
case wxID_REPLACE: OpenFindReplaceDialog(FRI_REPLACE); break;
case IDM_FINDNEXT: BellIfFalse(FindNext()); break;
case IDM_FINDPREV: BellIfFalse(FindPrevious()); break;
case IDM_READONLY: editor->SetEditable( !e.IsChecked() ); break;
case IDM_LINEWRAP: OnChangeLineWrap(e.IsChecked()); break;
case IDM_SHOW_PANE: OnShowLateralPane(); break;
case IDM_SWITCH_PANE: OnSwitchPane(); break;
#ifdef DEBUG
case IDM_TEST: {
//auto tp = new TestPopup(GetFrame());
auto e = dynamic_cast<wxTextCtrl*>(editor);
long start, end;
e->GetSelection(&start, &end);
wxString sel1 = e->GetRange(start, end);
wxString sel2 = e->GetStringSelection();
Beep(800, 120);
Beep(600, 120);
wxMessageBox(sel1, sel2, wxICON_INFORMATION);
}break;
#endif
default: return false;
}
return true;
}


