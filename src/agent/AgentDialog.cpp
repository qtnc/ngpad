#include "AgentDialog.hpp"
#include "AgentEditor.hpp"
#include "AgentSession.hpp"
#include "AgentConversation.hpp"
#include "../app/App.hpp"
#include "../common/println.hpp"
#include "../common/SingleChoiceDialog.hpp"
#include <wx/filedlg.h>
#include <wx/file.h>
#include <wx/filename.h>
#include<algorithm>

// Confirmation of a tool call. A long text to check (command, Lua script, list of replacements...) is shown in a read-only field
// which has the focus, so that it can be read first. For a command or a path, the value allowed by "for the session" or "always" can be edited.
static ToolConfirmation ShowConfirmDialog (wxWindow* parent, const wxString& title, ConfirmRequest& request) {
enum { ID_SESSION = wxID_HIGHEST + 1, ID_ALWAYS };
wxDialog dlg(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
auto sizer = new wxBoxSizer(wxVERTICAL);
wxString message = request.message;
bool canGrant = request.grant!=GrantKind::None;
if (request.grant==GrantKind::Tool) message += "\n" + U(format(GetTranslation("AgentGrantTool"), U(request.grantValue)));
auto label = new wxStaticText(&dlg, wxID_ANY, message);
label->Wrap(dlg.FromDIP(600));
sizer->Add(label, 0, wxALL | wxEXPAND, 5);
wxTextCtrl* details = nullptr, *grantText = nullptr;
if (!request.details.empty()) {
details = new wxTextCtrl(&dlg, wxID_ANY, request.details, wxDefaultPosition, dlg.FromDIP(wxSize(600, 250)), wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2 | wxHSCROLL | wxTE_DONTWRAP);
sizer->Add(details, 1, wxALL | wxEXPAND, 5);
}
if (canGrant && request.grant!=GrantKind::Tool) {
sizer->Add(new wxStaticText(&dlg, wxID_ANY, MSG(request.grant==GrantKind::Command? "AgentGrantCommand" : "AgentGrantPath")), 0, wxLEFT | wxRIGHT | wxTOP, 5);
grantText = new wxTextCtrl(&dlg, wxID_ANY, request.grantValue);
sizer->Add(grantText, 0, wxALL | wxEXPAND, 5);
}
auto buttons = new wxBoxSizer(wxHORIZONTAL);
std::vector<wxButton*> choices;
auto allow = new wxButton(&dlg, wxID_YES, MSG("AgentAllow"));
choices.push_back(allow);
if (canGrant) {
choices.push_back(new wxButton(&dlg, ID_SESSION, MSG("AgentAllowSession")));
choices.push_back(new wxButton(&dlg, ID_ALWAYS, MSG("AgentAllowAlways")));
}
auto deny = new wxButton(&dlg, wxID_NO, MSG("AgentDeny"));
choices.push_back(deny);
auto stop = new wxButton(&dlg, wxID_CANCEL, MSG("AgentStop"));
for (auto btn: choices) buttons->Add(btn, 0, wxALL, 5);
buttons->Add(stop, 0, wxALL, 5);
sizer->Add(buttons, 0, wxALIGN_RIGHT);
dlg.SetSizerAndFit(sizer);
auto defaultButton = request.denyByDefault? deny : allow;
defaultButton->SetDefault();
dlg.SetEscapeId(wxID_CANCEL);
for (auto btn: choices) btn->Bind(wxEVT_BUTTON, [&dlg, grantText](wxCommandEvent& e){
int id = e.GetId();
if ((id==ID_SESSION || id==ID_ALWAYS) && grantText && wxString(grantText->GetValue()).Trim(true).Trim(false).empty()) {
wxBell();
grantText->SetFocus();
return;
}
dlg.EndModal(id);
});
dlg.CentreOnParent();
if (details) details->SetFocus();
else defaultButton->SetFocus();
int result = dlg.ShowModal();
if (grantText) request.grantValue = grantText->GetValue();
switch(result){
case wxID_YES: return ToolConfirmation::Allow;
case ID_SESSION: return ToolConfirmation::AllowSession;
case ID_ALWAYS: return ToolConfirmation::AllowAlways;
case wxID_NO: return ToolConfirmation::Deny;
default: return ToolConfirmation::Stop;
}}

AgentDialog::AgentDialog (wxWindow* parent, const wxString& title, const AgentConfig& config, const wxString& root):
wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER | wxMINIMIZE_BOX | wxMAXIMIZE_BOX | wxDIALOG_NO_PARENT),
defaultTitle(title)
{
session = std::make_unique<AgentSession>(config, root);
session->SetConfirmCallback([this](ConfirmRequest& request){
// The dialog may be hidden while the agent is working
ShowAgent();
return ShowConfirmDialog(this, U(session->GetConfig().name), request);
});

editor = new AgentEditor(this, this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_DONTWRAP | wxHSCROLL | wxVSCROLL | wxTE_RICH2 | wxTE_MULTILINE);
auto sizer = new wxBoxSizer(wxVERTICAL);
sizer->Add(editor, 1, wxEXPAND);
SetSizer(sizer);
SetSize(FromDIP(wxSize(800, 600)));
CentreOnScreen();

std::vector<wxAcceleratorEntry> accelerators = {
{ wxACCEL_CTRL | wxACCEL_SHIFT, 'L', IDM_AGENT_CLEAR },
{ wxACCEL_CTRL, 'M', IDM_AGENT_MODEL },
{ wxACCEL_CTRL, 'S', IDM_AGENT_SAVE },
{ wxACCEL_CTRL | wxACCEL_SHIFT, 'S', IDM_AGENT_SAVE_AS },
{ wxACCEL_CTRL, 'O', IDM_AGENT_OPEN }
};
wxAcceleratorTable acctable(accelerators.size(), &accelerators[0]);
SetAcceleratorTable(acctable);
// Escape does nothing, to avoid closing the conversation by accident
SetEscapeId(wxID_NONE);

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
if (IsIconized()) Iconize(false);
Show();
Raise();
editor->SetFocus();
}

static wxString FormatToolCall (const std::string& name, const nlohmann::json& args, const ToolResult& result) {
wxString argText;
// For tools modifying files, other arguments can be the whole content of a file: only the paths are shown
auto tool = AgentTools::Find(name);
bool pathOnly = tool && tool->modifiesFiles;
if (args.is_object()) for (auto& [key, value]: args.items()) {
if (pathOnly && key!="path" && key!="new_path") continue;
if (!argText.empty()) argText << ", ";
argText << U(key) << ": " << U(value.is_string()? value.get<std::string>() : value.dump());
}
else if (args.is_string()) argText = U(args.get<std::string>());
wxString summary = U(result.text.substr(0, result.text.find('\n')));
if (summary.size()>150) summary = summary.Left(150) + wxString::FromUTF8("…");
return U(format(GetTranslation("AgentToolCall"), name, U(argText), U(summary)));
}

static std::string FormatTokenCounts (const TokenUsage& usage) {
std::string text = format(GetTranslation("AgentTokensInput"), usage.input);
if (usage.hasCached) text += " " + format(GetTranslation("AgentTokensCached"), usage.cached);
text += ", " + format(GetTranslation("AgentTokensOutput"), usage.output);
if (usage.requests>1) text += ", " + format(GetTranslation("AgentTokensRequests"), usage.requests);
return text;
}

static wxString FormatTokenUsage (const TokenUsage& usage) {
if (!usage.hasUsage) return wxEmptyString;
return U(format(GetTranslation("AgentTokens"), FormatTokenCounts(usage)));
}

static wxString FormatTotalTokenUsage (const TokenUsage& usage) {
if (usage.requests==0) return MSG("AgentTokensNoRequest");
if (!usage.hasUsage) return MSG("AgentTokensUnavailable");
return U(format(GetTranslation("AgentTokensTotal"), FormatTokenCounts(usage)) + "\n" + format(GetTranslation("AgentTokensContext"), usage.lastInput));
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
pendingStart = exchangeStart;
auto onToolCall = [this](const std::string& toolName, const nlohmann::json& args, const ToolResult& result){
editor->WriteToolCall(FormatToolCall(toolName, args, result));
wxGetApp().SayText(U(format(GetTranslation("AgentToolCallSay"), toolName)));
};
bool sent = session->Send(prompt, [this, name, exchangeStart](bool success, const wxString& text){
auto& app = wxGetApp();
pendingStart = -1;
if (success) {
exchangeStarts.push_back(exchangeStart);
editor->WriteAnswer(text, FormatTokenUsage(session->GetLastUsage()));
app.SayText(U(format(GetTranslation("AgentAnswerReceived"), U(name))));
}
else {
editor->WriteError(text);
app.SayText(name + ": " + text);
}
AutoSave();
}, onToolCall);
if (sent) app.SayText(MSG("AgentThinking"));
else {
pendingStart = -1;
editor->WriteError(MSG("AgentSendFailed"));
}}

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
else if (name=="effort") ChangeEffort(arg);
else if (name=="undo") UndoLastExchange(false);
else if (name=="retry") UndoLastExchange(true);
else if (name=="save") SaveConversation();
else if (name=="title") {
SetCustomTitle(arg);
wxString msg = U(format(GetTranslation("AgentTitleChanged"), U(GetTitle())));
editor->WriteInfo(msg);
app.SayText(msg);
}
else if (name=="tokens") {
wxString text = FormatTotalTokenUsage(session->GetTotalUsage());
editor->WriteInfo(text);
app.SayText(text.BeforeFirst('\n'));
}
else if (name=="help") editor->WriteInfo(MSG("AgentHelp"));
else {
wxString msg = U(format(GetTranslation("AgentUnknownCommand"), U(name)));
editor->WriteInfo(msg);
app.SayText(msg);
}
// The output of the command is part of the conversation; after /clear, nothing is saved until the next successful exchange
AutoSave();
}

// Without argument: current level and list of levels; "default": level of the configuration
void AgentDialog::ChangeEffort (const wxString& arg) {
static const std::vector<std::string> levels = AGENT_EFFORT_LEVELS;
std::string wanted = U(wxString(arg).Lower());
if (wanted=="default") wanted = session->GetConfig().effort;
bool valid = wanted.empty() || std::find(levels.begin(), levels.end(), wanted)!=levels.end();
wxString msg;
if (!arg.empty() && !valid) msg = U(format(GetTranslation("AgentEffortUnknown"), U(arg))) + "\n";
else if (!arg.empty()) session->SetEffort(wanted);
std::string current = session->GetEffort();
msg += U(format(GetTranslation("AgentEffortCurrent"), current.empty()? GetTranslation("AgentEffortNone") : current));
if (arg.empty() || !valid) {
std::string list;
for (auto& l: levels) list += (list.empty()? "" : ", ") + l;
msg += "\n" + U(format(GetTranslation("AgentEffortLevels"), list));
}
editor->WriteInfo(msg);
wxGetApp().SayText(msg.BeforeFirst('\n'));
}

void AgentDialog::SetCustomTitle (const wxString& title) {
customTitle = title;
customTitle.Trim(true).Trim(false);
SetTitle(customTitle.empty()? defaultTitle : customTitle);
}

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
AutoSave();
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
AutoSave();
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

nlohmann::ordered_json AgentDialog::BuildConversationData () {
using ordered_json = nlohmann::ordered_json;
auto& config = session->GetConfig();
// While an exchange is in progress, only the completed exchanges are saved
ordered_json data = ordered_json::object();
data["format"] = AGENT_CONVERSATION_FORMAT;
data["version"] = AGENT_CONVERSATION_VERSION;
data["title"] = U(customTitle);
data["agent"] = config.name;
data["api"] = config.api;
data["root"] = U(GetRoot());
data["model"] = session->GetModel();
data["effort"] = session->GetEffort();
data["autoSaved"] = autoFile;
// For information only: recomputed from the configuration when the conversation is opened again
data["systemPrompt"] = session->GetSystemPrompt();
data["usage"] = ordered_json(session->GetTotalUsage().ToJson());
data["messages"] = ordered_json(session->GetMessages(true));
data["transcript"] = ordered_json(editor->GetTranscript(pendingStart));
return data;
}

bool AgentDialog::WriteConversation (const wxString& path, bool automatic, wxString& error) {
bool previous = autoFile;
autoFile = automatic;
auto data = BuildConversationData();
autoFile = previous;
return WriteConversationFile(path, data, error);
}

bool AgentDialog::ExportText (const wxString& path) {
std::string text = U(editor->GetValue());
wxFile file(path, wxFile::write);
return file.IsOpened() && file.Write(text.data(), text.size());
}

void AgentDialog::AutoSave () {
if (!session) return;
if (filePath.empty()) {
// Nothing is saved before the first successful exchange
if (session->GetMessages(true).empty()) return;
filePath = MakeAutoConversationPath(GetConversationsDir(session->GetConfig(), GetRoot()), GetRoot());
autoFile = true;
}
wxString error;
if (WriteConversation(filePath, autoFile, error)) return;
// Shown only once, so that a directory which can't be written doesn't make the agent unusable
println("Automatic save failed: {}", U(error));
if (autoSaveErrorShown) return;
autoSaveErrorShown = true;
wxMessageBox(error, MSG("Error"), wxICON_ERROR | wxOK, this);
}

void AgentDialog::SaveConversation () {
if (filePath.empty() || autoFile) {
SaveConversationAs();
return;
}
wxString error;
if (!WriteConversation(filePath, false, error)) {
wxMessageBox(error, MSG("Error"), wxICON_ERROR | wxOK, this);
return;
}
wxGetApp().SayText(MSG("AgentSaved"));
}

void AgentDialog::SaveConversationAs () {
wxString dir = !filePath.empty() && !autoFile? wxFileName(filePath).GetPath() : GetConversationsDir(session->GetConfig(), GetRoot());
wxString name = customTitle.empty()? wxString(U(session->GetConfig().name)) : customTitle;
for (auto c: wxString("\\/:*?\"<>|")) name.Replace(wxString(c), "_");
wxFileDialog fd(this, MSG("AgentSave"), dir, name + AGENT_CONVERSATION_EXT, GetConversationFileFilter() + "|" + MSG("AgentSaveFilter"), wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
if (wxID_OK!=fd.ShowModal()) return;
wxString path = fd.GetPath();
// Markdown or text: only an export of what is displayed, which can't be opened again
if (fd.GetFilterIndex()!=0 && !HasConversationExtension(path)) {
if (!ExportText(path)) wxMessageBox(MSG("AgentSaveError"), MSG("Error"), wxICON_ERROR | wxOK, this);
else wxGetApp().SayText(MSG("AgentSaved"));
return;
}
wxString fullPath = AddConversationExtension(path);
if (fullPath!=path && wxFileExists(fullPath) && wxYES!=wxMessageBox(U(format(GetTranslation("AgentFileExists"), U(fullPath))), MSG("AgentSave"), wxYES_NO | wxICON_QUESTION, this)) return;
wxString error;
if (!WriteConversation(fullPath, false, error)) {
wxMessageBox(error, MSG("Error"), wxICON_ERROR | wxOK, this);
return;
}
// The automatic file is useless once the conversation is saved elsewhere
if (autoFile && !filePath.empty() && !wxFileName(filePath).SameAs(wxFileName(fullPath))) wxRemoveFile(filePath);
filePath = fullPath;
autoFile = false;
wxGetApp().SayText(MSG("AgentSaved"));
}

void AgentDialog::OpenConversation () {
wxString dir = !filePath.empty() && !autoFile? wxFileName(filePath).GetPath() : GetConversationsDir(session->GetConfig(), GetRoot());
if (!wxDirExists(dir)) dir = wxEmptyString;
wxFileDialog fd(this, MSG("AgentOpenConversation"), dir, wxEmptyString, GetConversationFileFilter(), wxFD_OPEN | wxFD_FILE_MUST_EXIST);
if (wxID_OK!=fd.ShowModal()) return;
wxGetApp().OpenAgentConversation(fd.GetPath(), this);
}

static nlohmann::json BuildTranscriptFromMessages (const nlohmann::json& messages) {
// For a file without journal, for example written by hand: only messages and answers
nlohmann::json t = nlohmann::json::array();
for (auto& m: messages) {
std::string role = m.value("role", "");
std::string content = m.contains("content") && m["content"].is_string()? m["content"].get<std::string>() : "";
bool hasToolCalls = m.contains("tool_calls") && m["tool_calls"].is_array() && !m["tool_calls"].empty();
if (role=="user") t.push_back({ {"kind", "prompt"}, {"text", content} });
else if (role=="assistant" && !hasToolCalls && !content.empty()) t.push_back({ {"kind", "answer"}, {"text", content} });
}
return t;
}

bool AgentDialog::LoadConversation (const nlohmann::json& data, const wxString& path) {
if (!session || IsBusy()) return false;
// The current conversation stays in its file
AutoSave();
if (!session->Restore(data["messages"], TokenUsage::FromJson(data.value("usage", nlohmann::json::object())))) return false;
std::string model = data.contains("model") && data["model"].is_string()? data["model"].get<std::string>() : "";
if (!model.empty()) session->SetModel(model);
if (data.contains("effort") && data["effort"].is_string()) session->SetEffort(data["effort"].get<std::string>());
auto transcript = data.contains("transcript") && data["transcript"].is_array()? data["transcript"] : BuildTranscriptFromMessages(data["messages"]);
editor->Restore(transcript, exchangeStarts);
pendingStart = -1;
SetCustomTitle(U(data.contains("title") && data["title"].is_string()? data["title"].get<std::string>() : ""));
filePath = path;
autoFile = data.value("autoSaved", false);
autoSaveErrorShown = false;
wxGetApp().SayText(MSG("AgentConversationOpened"));
return true;
}

void AgentDialog::CancelRequest () {
if (!session || !session->Cancel()) wxBell();
}

void AgentDialog::CloseAgent () {
Close();
}

void AgentDialog::ClearAgent () {
// A new conversation, saved in a new file; the previous one stays in its file
if (session && editor) AutoSave();
if (session) session->Clear();
exchangeStarts.clear();
pendingStart = -1;
filePath.clear();
autoFile = false;
autoSaveErrorShown = false;
if (editor) editor->Reset();
}

void AgentDialog::OnCharHook (wxKeyEvent& e) {
if (e.GetKeyCode()!=WXK_ESCAPE) e.Skip();
else if (e.GetModifiers()==wxMOD_SHIFT) CancelRequest();
// Escape alone does nothing, to avoid closing the conversation by accident
else if (e.GetModifiers()!=wxMOD_NONE) e.Skip();
}

void AgentDialog::OnClose (wxCloseEvent& e) {
// The conversation is already saved after each exchange; a pending request is abandoned by the session destructor
if (e.CanVeto() && IsBusy() && wxYES!=wxMessageBox(MSG("AgentCloseBusy"), GetTitle(), wxYES_NO | wxICON_QUESTION, this)) {
e.Veto();
return;
}
AutoSave();
Destroy();
}

void AgentDialog::OnContextMenu (wxContextMenuEvent& e) {
wxMenu menu;
menu.Append(wxID_COPY);
menu.Append(wxID_SELECTALL);
menu.AppendSeparator();
menu.Append(IDM_AGENT_SAVE, MSG("AgentSave") + "\tCtrl+S");
menu.Append(IDM_AGENT_SAVE_AS, MSG("AgentSaveAs") + "...\tCtrl+Shift+S");
menu.Append(IDM_AGENT_OPEN, MSG("AgentOpenConversation") + "...\tCtrl+O");
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
case IDM_AGENT_SAVE_AS: SaveConversationAs(); break;
case IDM_AGENT_OPEN:
if (IsBusy()) wxBell();
else OpenConversation();
break;
case IDM_AGENT_CLOSE: CloseAgent(); break;
default: e.Skip(); break;
}}
