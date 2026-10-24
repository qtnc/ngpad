#include "AgentView.hpp"
#include "AgentDocument.hpp"
#include "AgentEditor.hpp"
#include "AgentSession.hpp"
#include "../app/App.hpp"
#include "../common/println.hpp"

wxIMPLEMENT_DYNAMIC_CLASS(AgentView, wxView);

AgentView::AgentView () {}

AgentView::~AgentView () {}

void AgentView::SetAgent (const AgentConfig& config) {
session = std::make_unique<AgentSession>(config);
}

bool AgentView::IsBusy () {
return session && session->IsBusy();
}

void AgentView::Send (const wxString& prompt) {
if (!session) return;
auto& app = wxGetApp();
wxString name = U(session->GetConfig().name);
bool sent = session->Send(prompt, [this, name](bool success, const wxString& text){
auto& app = wxGetApp();
if (success) {
editor->WriteAnswer(text);
app.SayText(U(format(GetTranslation("AgentAnswerReceived"), U(name))));
}
else {
editor->WriteError(text);
app.SayText(U(format(GetTranslation("AgentErrorReceived"), U(name))));
}});
if (sent) app.SayText(MSG("AgentThinking"));
else editor->WriteError(MSG("AgentSendFailed"));
}

bool AgentView::OnCreate (wxDocument* wxdoc, long flags) {
if (!AbstractView::OnCreate(wxdoc, flags)) return false;
auto doc = static_cast<AgentDocument*>(wxdoc);
auto& app = wxGetApp();
auto childWindow = GetFrame();
auto sizer = new wxBoxSizer(wxVERTICAL);

editor = new AgentEditor(this, childWindow, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_DONTWRAP | wxHSCROLL | wxVSCROLL | wxTE_RICH2 | wxTE_MULTILINE);
sizer->Add(editor, 1, wxEXPAND);
childWindow->SetSizerAndFit(sizer);
editor->SetFocus();
ClearAgent();
return true;
}

void AgentView::OnUpdate (wxView* sender, wxObject* param) {
auto frame = GetParentWindow<wxFrame>(GetFrame());
auto menubar = frame? frame->GetMenuBar() : nullptr;
if (!menubar) return;
menubar->Enable(wxID_REVERT_TO_SAVED, false);
menubar->Enable(wxID_FIND, false);
menubar->Enable(IDM_FINDNEXT, false);
menubar->Enable(IDM_FINDPREV, false);
menubar->Enable(wxID_REPLACE, false);
}

void AgentView::ClearAgent () {
if (session) session->Clear();
if (editor) {
editor->Clear();
editor->WritePromptInvite();
}}

bool AgentView::OnMenuAction (wxCommandEvent& e) {
if (AbstractView::OnMenuAction(e)) return true;
size_t id = e.GetId();
switch(id){
default: return false;
}
return true;
}


