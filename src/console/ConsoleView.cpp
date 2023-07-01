#include "ConsoleView.hpp"
#include "ConsoleDocument.hpp"
#include "ConsoleEditor.hpp"
#include "../app/App.hpp"
#include "../common/println.hpp"

wxIMPLEMENT_DYNAMIC_CLASS(ConsoleView, wxView);


wxString LuaGetBanner ();

void ConsoleView::AddToHistory (const wxString& s) {
auto it = std::find(history.begin(), history.end(), s);
if (it!=history.end()) history.erase(it);
if (history.size()>=100) history.erase(history.begin());
history.push_back(s);
}

bool ConsoleView::OnCreate (wxDocument* wxdoc, long flags) {
if (!AbstractView::OnCreate(wxdoc, flags)) return false;
auto doc = static_cast<ConsoleDocument*>(wxdoc);
auto& app = wxGetApp();
auto childWindow = GetFrame();
auto sizer = new wxBoxSizer(wxVERTICAL);

editor = new ConsoleEditor(this, childWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_DONTWRAP | wxHSCROLL | wxVSCROLL | wxTE_RICH2 | wxTE_MULTILINE);
sizer->Add(editor, 1, wxEXPAND);
childWindow->SetSizerAndFit(sizer);
editor->SetFocus();
return true;
}

void ConsoleView::OnUpdate (wxView* sender, wxObject* param) {
auto frame = GetParentWindow<wxFrame>(GetFrame());
auto menubar = frame? frame->GetMenuBar() : nullptr;
if (!menubar) return;
menubar->Enable(wxID_REVERT_TO_SAVED, false);
menubar->Enable(wxID_FIND, false);
menubar->Enable(IDM_FINDNEXT, false);
menubar->Enable(IDM_FINDPREV, false);
menubar->Enable(wxID_REPLACE, false);
}

bool ConsoleView::ExecuteCommand (const wxString& cmd) {
println("Executing command: {}", U(cmd));
auto proc = wxProcess::Open(cmd, wxEXEC_ASYNC | wxEXEC_HIDE_CONSOLE);
if (!proc) return false;
auto doc = static_cast<ConsoleDocument*>(GetDocument());
wxMBConv& conv = wxConvLocal; //wxConvUTF8;
liveAppender1 = std::make_unique<CommandLiveTextAppender>( *proc->GetInputStream(), proc, editor, conv);
liveAppender2 = std::make_unique<CommandLiveTextAppender>( *proc->GetErrorStream(), proc, editor, conv, liveAppender1.get());
liveSubmitter = std::make_unique<CommandLiveTextSubmitter>( *proc->GetOutputStream(), proc, editor, conv, liveAppender1.get());
proc->Bind(wxEVT_END_PROCESS, [&](auto&e){ 
println("Process exited with code {}", e.GetExitCode());
liveAppender1.reset();
liveAppender2.reset();
liveSubmitter.reset();
e.Skip(); 
});
return true;
}

void ConsoleView::ClearConsole () {
if (liveAppender1) liveAppender1->GetInsertionPoint() = 0;
if (editor) editor->Clear();
}

void ConsoleView::MakeLuaConsole () {
liveAppender2.reset();
liveAppender1 = std::make_unique<LuaConsoleTextAppender>(this, editor);
liveSubmitter = std::make_unique<LuaConsoleTextSubmitter>(editor, liveAppender1.get());
wxString initialText = U(
APP_DISPLAY_NAME " " APP_VERSION_STRING "\n"
APP_COPYRIGHT_INFO " " APP_VENDOR " (" APP_WEBSITE_URL ")\n"
) + LuaGetBanner() + "\n>>>";
liveAppender1->Append(initialText);
}

bool ConsoleView::OnMenuAction (wxCommandEvent& e) {
if (AbstractView::OnMenuAction(e)) return true;
size_t id = e.GetId();
switch(id){
default: return false;
}
return true;
}


