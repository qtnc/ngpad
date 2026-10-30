#include "AgentDialog.hpp"
#include "AgentEditor.hpp"
#include "AgentSession.hpp"
#include "../app/App.hpp"
#include "../common/println.hpp"
#include "../common/SingleChoiceDialog.hpp"
#include <wx/filedlg.h>
#include <wx/file.h>
#include<algorithm>

AgentDialog::AgentDialog (wxWindow* parent, const wxString& title, const AgentConfig& config, const wxString& root):
wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMAXIMIZE_BOX | wxDIALOG_NO_PARENT)
{
session = std::make_unique<AgentSession>(config, root);
session->SetConfirmCallback([this](const wxString& message){
// The dialog may be hidden while the agent is working
ShowAgent();
wxMessageDialog dlg(this, message, U(session->GetConfig().name), wxYES_NO | wxCANCEL | wxICON_QUESTION);
dlg.SetYesNoCancelLabels(MSG("AgentAllow"), MSG("AgentDeny"), MSG("AgentStop"));
switch(dlg.ShowModal()){
case wxID_YES: return ToolConfirmation::Allow;
case wxID_NO: return ToolConfirmation::Deny;
default: return ToolConfirmation::Stop;
}});

editor = new AgentEditor(this, this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_DONTWRAP | wxHSCROLL | wxVSCROLL | wxTE_RICH2 | wxTE_MULTILINE);
auto sizer = new wxBoxSizer(wxVERTICAL);
sizer->Add(editor, 1, wxEXPAND);
SetSizer(sizer);
SetSize(FromDIP(wxSize(800, 600)));
CentreOnScreen();

std::vector<wxAcceleratorEntry> accelerators = {
{ wxACCEL_CTRL | wxACCEL_SHIFT, 'L', IDM_AGENT_CLEAR },
{ wxACCEL_CTRL, 'M', IDM_AGENT_MODEL },
{ wxACCEL_CTRL, 'S', IDM_AGENT_SAVE }
};
wxAcceleratorTable acctable(accelerators.size(), &accelerators[0]);
SetAcceleratorTable(acctable);

Bind(wxEVT_CHAR_HOOK, &AgentDialog::OnCharHook, this);
Bind(wxEVT_CLOSE_WINDOW, &AgentDialog::OnClose, this);
Bind(wxEVT_MENU, &AgentDialog::OnMenuAction, this);
editor->Bind(wxEVT_CONTEXT_MENU, &AgentDialog::OnContextMenu, this);
ClearAgent();
}

AgentDialog::~AgentDialog () {}

const wxString& AgentDialog::GetRoot () const {
return session->GetRoot();
}

void AgentDialog::ShowAgent () {
Show();
Raise();
editor->SetFocus();
}

static wxString FormatToolCall (const std::string& name, const nlohmann::json& args, const ToolResult& result) {
wxString argText;
// For tools modifying files, other arguments can be the whole content of a file: only the path is shown
auto tool = AgentTools::Find(name);
bool pathOnly = tool && tool->modifiesFiles;
if (args.is_object()) for (auto& [key, value]: args.items()) {
if (pathOnly && key!="path") continue;
if (!argText.empty()) argText << ", ";
argText << U(key) << ": " << U(value.is_string()? value.get<std::string>() : value.dump());
}
else if (args.is_string()) argText = U(args.get<std::string>());
wxString summary = U(result.text.substr(0, result.text.find('\n')));
if (summary.size()>150) summary = summary.Left(150) + wxString::FromUTF8("…");
return U(format(GetTranslation("AgentToolCall"), name, U(argText), U(summary)));
}

bool AgentDialog::IsBusy () {
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

void AgentDialog::Send (const wxString& prompt) {
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

void AgentDialog::SendPrompt (const wxString& prompt) {
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

void AgentDialog::ExecuteCommand (const wxString& name, const wxString& arg) {
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
else if (name=="save") SaveConversation();
else if (name=="help") editor->WriteInfo(MSG("AgentHelp"));
else {
wxString msg = U(format(GetTranslation("AgentUnknownCommand"), U(name)));
editor->WriteInfo(msg);
app.SayText(msg);
}}

void AgentDialog::ShowModelList () {
bool started = session->ListModels([this](bool success, const std::vector<std::string>& models, const wxString& error){
wxString text = success? FormatModelList(models, session->GetModel()) : MSG("AIError") + " " + error;
editor->WriteInfo(text);
wxGetApp().SayText(success? U(format(GetTranslation("AgentModelCount"), models.size())) : error);
});
if (!started) editor->WriteInfo(MSG("AgentSendFailed"));
}

void AgentDialog::ChangeModel (const wxString& name) {
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

void AgentDialog::ShowModelDialog () {
if (!session || IsBusy()) {
wxBell();
return;
}
session->ListModels([this](bool success, const std::vector<std::string>& models, const wxString& error){
if (!success) {
wxMessageBox(error, MSG("Error"), wxICON_ERROR | wxOK, this);
return;
}
if (models.empty()) {
wxMessageBox(MSG("AgentNoModel"), MSG("Error"), wxICON_ERROR | wxOK, this);
return;
}
wxArrayString names;
int selection = 0;
for (size_t i=0; i<models.size(); i++) {
names.push_back(U(models[i]));
if (models[i]==session->GetModel()) selection = i;
}
SingleChoiceDialog dlg(this, MSG("AgentChooseModel"), MSG("AgentChooseModelMsg"), names, selection);
if (wxID_OK!=dlg.ShowModal()) return;
int i = dlg.GetSelection();
if (i<0) return;
session->SetModel(models[i]);
wxGetApp().SayText(U(format(GetTranslation("AgentModelChanged"), models[i])));
editor->SetFocus();
});
}

void AgentDialog::UndoLastExchange (bool retry) {
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

void AgentDialog::SaveConversation () {
wxString name = U(session->GetConfig().name) + ".md";
wxFileDialog fd(this, MSG("AgentSave"), GetRoot(), name, MSG("AgentSaveFilter"), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
if (wxID_OK!=fd.ShowModal()) return;
std::string text = U(editor->GetValue());
wxFile file(fd.GetPath(), wxFile::write);
if (!file.IsOpened() || !file.Write(text.data(), text.size())) {
wxMessageBox(MSG("AgentSaveError"), MSG("Error"), wxICON_ERROR | wxOK, this);
return;
}
wxGetApp().SayText(MSG("AgentSaved"));
}

void AgentDialog::CancelRequest () {
if (!session || !session->Cancel()) wxBell();
}

void AgentDialog::CloseAgent () {
// The conversation is lost; a pending request is abandoned by the session destructor
Destroy();
}

void AgentDialog::ClearAgent () {
if (session) session->Clear();
exchangeStarts.clear();
if (editor) editor->Reset();
}

void AgentDialog::OnCharHook (wxKeyEvent& e) {
if (e.GetKeyCode()!=WXK_ESCAPE) e.Skip();
else if (e.GetModifiers()==wxMOD_SHIFT) CancelRequest();
// Escape only hides the dialog: the conversation and a pending request are kept
else if (e.GetModifiers()==wxMOD_NONE) Hide();
else e.Skip();
}

void AgentDialog::OnClose (wxCloseEvent& e) {
if (e.CanVeto()) {
e.Veto();
Hide();
}
else Destroy();
}

void AgentDialog::OnContextMenu (wxContextMenuEvent& e) {
wxMenu menu;
menu.Append(wxID_COPY);
menu.Append(wxID_SELECTALL);
menu.AppendSeparator();
menu.Append(IDM_AGENT_SAVE, MSG("AgentSave") + "...\tCtrl+S");
menu.Append(IDM_AGENT_CLEAR, MSG("AgentClear") + "\tCtrl+Shift+L");
menu.Append(IDM_AGENT_MODEL, MSG("AgentChooseModel") + "...\tCtrl+M");
menu.Append(IDM_AGENT_CANCEL, MSG("AgentCancel") + "\tShift+Esc");
menu.AppendSeparator();
menu.Append(IDM_AGENT_CLOSE, MSG("AgentClose"));
menu.Enable(IDM_AGENT_CANCEL, IsBusy());
editor->PopupMenu(&menu);
}

void AgentDialog::OnMenuAction (wxCommandEvent& e) {
switch(e.GetId()){
case IDM_AGENT_CLEAR:
ClearAgent();
wxGetApp().SayText(MSG("AgentCleared"));
break;
case IDM_AGENT_MODEL: ShowModelDialog(); break;
case IDM_AGENT_CANCEL: CancelRequest(); break;
case IDM_AGENT_SAVE: SaveConversation(); break;
case IDM_AGENT_CLOSE: CloseAgent(); break;
default: e.Skip(); break;
}}
