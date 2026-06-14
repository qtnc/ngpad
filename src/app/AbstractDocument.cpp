#include "AbstractDocument.hpp"
#include "App.hpp"
#include "../common/stringUtils.hpp"
#include "../common/println.hpp"

wxDEFINE_EVENT(wxEVT_DOC_CREATING, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_DOC_CREATED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_DOC_LOADING, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_DOC_LOADED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_DOC_SAVING, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_DOC_SAVED, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_DOC_CLOSING, wxCommandEvent);
wxDEFINE_EVENT(wxEVT_DOC_CLOSED, wxCommandEvent);

void LoadEditorConfig (Properties& properties, const wxString& filename);
void LoadPlugins (App& app, Properties& props);

bool AbstractDocument::OnSaveDocument (const wxString& filename) {
SendEvent(wxEVT_DOC_SAVING);
bool result = wxDocument::OnSaveDocument(filename);
if (result) {
lastModifiedTime = wxDateTime::Now();
SendEvent(wxEVT_DOC_SAVED);
}
return result;
}

bool AbstractDocument::OnOpenDocument (const wxString& filename) {
wxFileName fn(filename);
lastModifiedTime = fn.Exists()? fn.GetModificationTime() : wxDateTime::Now();
LoadEditorConfig(properties, filename);
LoadPlugins(wxGetApp(), properties);
SendEvent(wxEVT_DOC_LOADING);
bool result = wxDocument::OnOpenDocument(filename);
if (result) SendEvent(wxEVT_DOC_LOADED);
return result;
}

bool AbstractDocument::OnCloseDocument () {
if (closing) return true;
closing = true;
SendEvent(wxEVT_DOC_CLOSING);
if (!wxDocument::OnCloseDocument()) {
closing = false;
return false;
}
SendEvent(wxEVT_DOC_CLOSED);
return true;
}

void AbstractDocument::Modify (bool modified) {
bool changed = modified==IsModified();
wxDocument::Modify(modified);
if (changed) wxGetApp() .UpdateTitle();
}

bool AbstractDocument::OnCreate (const wxString& filename, long flags) {
LoadEditorConfig(properties, filename);
LoadPlugins(wxGetApp(), properties);
lastModifiedTime = wxDateTime::Now();
SendEvent(wxEVT_DOC_CREATING);
bool result = wxDocument::OnCreate(filename, flags);
if (result) SendEvent(wxEVT_DOC_CREATED);
return result;
}

bool AbstractDocument::OnNewDocument () {
if (!wxDocument::OnNewDocument()) return false;
auto& app = wxGetApp();
LoadEditorConfig(properties, GetFilename());
app.UpdateTitle();
return true;
}

bool AbstractDocument::Reload () {
return OnOpenDocument(GetFilename());
}

wxString AbstractDocument::GetWorkspaceRoot () {
std::string root = properties.get("workspace_root", "");
if (!root.empty()) return U(root);
auto& config = wxGetApp().GetConfig();
std::vector<std::string> files = split(config.get("workspace_root_indicators", ""), ",; ", true);
wxString sep = wxFileName::GetPathSeparator();
wxString filename = GetFilename(), finalPath, path, oldPath = filename;
do {
wxFileName::SplitPath(oldPath, &path, nullptr, nullptr);
if (path.empty() || path==oldPath) break;
oldPath = path;
if (path[path.size() -1]!=sep) path += sep;
if (finalPath.empty()) finalPath = path;
for (auto& file: files) {
wxString fullpath = path+file;
if (wxFileExists(fullpath) || wxDirExists(fullpath)) finalPath=path;
}
} while(true);
properties.put("workspace_root", U(finalPath));
return finalPath;
}

void AbstractDocument::CheckConcurrentModification () {
wxString filename = GetFilename();
if (filename.empty()) return;
wxFileName fn(filename);
if (!fn.Exists()) return;
wxDateTime modifiedTime = fn.GetModificationTime();
if (modifiedTime.IsLaterThan(lastModifiedTime)) {
if (!IsModified()) Reload();
else wxGetApp().CallAfter([=](){
if (wxYES== wxMessageBox(
U(format( GetTranslation("FileModifMsg"),  U(GetUserReadableName()) )), 
GetUserReadableName(), wxYES_NO | wxICON_ASTERISK)) 
{ Reload(); }
});
}
lastModifiedTime = modifiedTime;
}

bool AbstractDocument::SendEvent (wxEventType type) {
wxCommandEvent e(type);
e.SetEventObject(this);
return ProcessEvent(e);
}

