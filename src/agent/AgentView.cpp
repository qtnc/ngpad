#include "AgentView.hpp"
#include "AgentDocument.hpp"
#include "AgentEditor.hpp"
#include "AgentSession.hpp"
#include "../app/App.hpp"
#include "../common/println.hpp"
#include "../common/SingleChoiceDialog.hpp"
#include<algorithm>

wxIMPLEMENT_DYNAMIC_CLASS(AgentView, wxView);

AgentView::AgentView () {}

AgentView::~AgentView () {}

void AgentView::SetAgent (const AgentConfig& config, const wxString& root) {
session = std::make_unique<AgentSession>(config, root);
}

static wxString FormatToolCall (const std::string& name, const nlohmann::json& args, const ToolResult& result) {
wxString argText;
if (args.is_object()) for (auto& [key, value]: args.items()) {
if (!argText.empty()) argText << ", ";
argText << U(key) << ": " << U(value.is_string()? value.get<std::string>() : value.dump());
}
else if (args.is_string()) argText = U(args.get<std::string>());
wxString summary = U(result.text.substr(0, result.text.find('\n')));
if (summary.size()>150) summary = summary.Left(150) + wxString::FromUTF8("…");
return U(format(GetTranslation("AgentToolCall"), name, U(argText), U(summary)));
}

bool AgentView::IsBusy () {
return session && session->IsBusy();
}

static wxString FormatModelList (const std::vector<std::string>& models, const std::string& current) {
if (models.empty()) return MSG("AgentNoModel");
wxString text = MSG("AgentModelList");
for (auto& m: models) {
text << "\n" << U(m);
if (m==current) text << " " << MSG("AgentCurrentModel");
}
return text;
}

void AgentView::Send (const wxString& prompt) {
if (!session) return;
wxString text = prompt;
text.Trim(true).Trim(false);
if (text.StartsWith("/")) {
wxString name = text.Mid(1).BeforeFirst(' '), arg = text.Mid(1).AfterFirst(' ');
arg.Trim(false);
ExecuteCommand(name.Lower(), arg);
}
else SendPrompt(prompt);
}

void AgentView::SendPrompt (const wxString& prompt) {
auto& app = wxGetApp();
wxString name = U(session->GetConfig().name);
long exchangeStart = editor->GetCurrentMessageStart();
auto onToolCall = [this](const std::string& toolName, const nlohmann::json& args, const ToolResult& result){
editor->WriteToolCall(FormatToolCall(toolName, args, result));
wxGetApp().SayText(U(format(GetTranslation("AgentToolCallSay"), toolName)));
};
bool sent = session->Send(prompt, [this, name, exchangeStart](bool success, const wxString& text){
auto& app = wxGetApp();
if (success) {
exchangeStarts.push_back(exchangeStart);
editor->WriteAnswer(text);
app.SayText(U(format(GetTranslation("AgentAnswerReceived"), U(name))));
}
else {
editor->WriteError(text);
app.SayText(name + ": " + text);
}}, onToolCall);
if (sent) app.SayText(MSG("AgentThinking"));
else editor->WriteError(MSG("AgentSendFailed"));
}

void AgentView::ExecuteCommand (const wxString& name, const wxString& arg) {
auto& app = wxGetApp();
if (name=="clear") {
ClearAgent();
app.SayText(MSG("AgentCleared"));
}
else if (name=="model") {
if (arg.empty()) ShowModelList();
else ChangeModel(arg);
}
else if (name=="undo") UndoLastExchange(false);
else if (name=="retry") UndoLastExchange(true);
else if (name=="help") editor->WriteInfo(MSG("AgentHelp"));
else {
wxString msg = U(format(GetTranslation("AgentUnknownCommand"), U(name)));
editor->WriteInfo(msg);
app.SayText(msg);
}}

void AgentView::ShowModelList () {
bool started = session->ListModels([this](bool success, const std::vector<std::string>& models, const wxString& error){
wxString text = success? FormatModelList(models, session->GetModel()) : MSG("AIError") + " " + error;
editor->WriteInfo(text);
wxGetApp().SayText(success? U(format(GetTranslation("AgentModelCount"), models.size())) : error);
});
if (!started) editor->WriteInfo(MSG("AgentSendFailed"));
}

void AgentView::ChangeModel (const wxString& name) {
std::string wanted = U(name);
bool started = session->ListModels([this, wanted](bool success, const std::vector<std::string>& models, const wxString& error){
wxString text;
if (!success) text = MSG("AIError") + " " + error;
else if (std::find(models.begin(), models.end(), wanted)!=models.end()) {
session->SetModel(wanted);
text = MSG("AgentModelOK");
}
else text = MSG("AgentModelNotFound") + "\n" + FormatModelList(models, session->GetModel());
editor->WriteInfo(text);
wxGetApp().SayText(text.BeforeFirst('\n'));
});
if (!started) editor->WriteInfo(MSG("AgentSendFailed"));
}

void AgentView::ShowModelDialog () {
if (!session || IsBusy()) {
wxBell();
return;
}
session->ListModels([this](bool success, const std::vector<std::string>& models, const wxString& error){
if (!success) {
wxMessageBox(error, MSG("Error"), wxICON_ERROR | wxOK, GetFrame());
return;
}
if (models.empty()) {
wxMessageBox(MSG("AgentNoModel"), MSG("Error"), wxICON_ERROR | wxOK, GetFrame());
return;
}
wxArrayString names;
int selection = 0;
for (size_t i=0; i<models.size(); i++) {
names.push_back(U(models[i]));
if (models[i]==session->GetModel()) selection = i;
}
SingleChoiceDialog dlg(GetFrame(), MSG("AgentChooseModel"), MSG("AgentChooseModelMsg"), names, selection);
if (wxID_OK!=dlg.ShowModal()) return;
int i = dlg.GetSelection();
if (i<0) return;
session->SetModel(models[i]);
wxGetApp().SayText(U(format(GetTranslation("AgentModelChanged"), models[i])));
editor->SetFocus();
});
}

void AgentView::UndoLastExchange (bool retry) {
wxString prompt;
if (exchangeStarts.empty() || !session->UndoLastExchange(&prompt)) {
editor->WriteInfo(MSG("AgentNothingToUndo"));
wxGetApp().SayText(MSG("AgentNothingToUndo"));
return;
}
// Removes the last exchange and everything written after it, including this command
editor->TruncateFrom(exchangeStarts.back());
exchangeStarts.pop_back();
if (retry) {
editor->WriteText(prompt);
editor->OnShiftEnter();
}
else wxGetApp().SayText(MSG("AgentUndone"));
}

void AgentView::CancelRequest () {
if (!session || !session->Cancel()) wxBell();
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
exchangeStarts.clear();
if (editor) editor->Reset();
}

bool AgentView::OnMenuAction (wxCommandEvent& e) {
if (AbstractView::OnMenuAction(e)) return true;
size_t id = e.GetId();
switch(id){
case IDM_AGENT_CLEAR:
ClearAgent();
wxGetApp().SayText(MSG("AgentCleared"));
break;
case IDM_AGENT_MODEL: ShowModelDialog(); break;
case IDM_AGENT_CANCEL: CancelRequest(); break;
default: return false;
}
return true;
}


