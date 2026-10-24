#include "AgentView.hpp"
#include "AgentDocument.hpp"
#include "AgentEditor.hpp"
#include "../app/App.hpp"
#include "../common/println.hpp"

wxIMPLEMENT_DYNAMIC_CLASS(AgentView, wxView);

void AgentView::Send (const wxString& prompt) {
//todo: make the real request
editor->WriteAnswer("It works !");
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


