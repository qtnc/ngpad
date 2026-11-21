#include "AgentConversation.hpp"
#include "AgentConfig.hpp"
#include "AgentSession.hpp"
#include "../app/App.hpp"
#include "../common/println.hpp"
#include <wx/filename.h>
#include <wx/datetime.h>
#include <wx/log.h>
#include<fstream>
#include<iterator>

using json = nlohmann::json;

wxString GetConversationFileFilter () {
return U(format(GetTranslation("AgentConversationFilter"), AGENT_CONVERSATION_EXT));
}

bool HasConversationExtension (const wxString& path) {
wxString ext = AGENT_CONVERSATION_EXT;
return path.Lower().EndsWith(ext.Lower());
}

wxString AddConversationExtension (const wxString& path) {
return HasConversationExtension(path)? path : path + AGENT_CONVERSATION_EXT;
}

wxString GetConversationsDir (const AgentConfig& config, const wxString& root) {
wxString dir = U(config.conversationsDir);
dir.Trim(true).Trim(false);
if (dir.empty()) {
// Next to the config directory where agents.json was found: user directory, or directory of the application
wxString base;
wxString configPath = wxGetApp().FindAppFile(AGENTS_CONFIG_FILENAME);
if (!configPath.empty()) base = wxFileName(configPath).GetPath(wxPATH_GET_VOLUME | wxPATH_NO_SEPARATOR);
if (!base.empty()) base = wxFileName(base).GetPath(wxPATH_GET_VOLUME | wxPATH_NO_SEPARATOR);
if (base.empty()) base = wxGetApp().GetUserDir();
return base + wxFILE_SEP_PATH + AGENT_CONVERSATIONS_DEFAULT_DIR;
}
// Relative paths, including ".", are relative to the workspace root
wxFileName fn = wxFileName::DirName(dir);
if (fn.IsRelative() && !root.empty()) fn.MakeAbsolute(root);
fn.Normalize(wxPATH_NORM_DOTS | wxPATH_NORM_ABSOLUTE);
return fn.GetPath(wxPATH_GET_VOLUME | wxPATH_NO_SEPARATOR);
}

wxString MakeAutoConversationPath (const wxString& dir, const wxString& root) {
wxString name = wxDateTime::Now().Format("%Y-%m-%d_%H-%M-%S");
wxString workspace = GetWorkspaceName(root);
for (auto c: wxString("\\/:*?\"<>|")) workspace.Replace(wxString(c), "_");
if (!workspace.empty()) name << "_" << workspace;
wxString path = dir + wxFILE_SEP_PATH + name + AGENT_CONVERSATION_EXT;
for (int i=2; wxFileExists(path); i++) path = dir + wxFILE_SEP_PATH + name + "-" + wxString::Format("%d", i) + AGENT_CONVERSATION_EXT;
return path;
}

bool ReadConversationFile (const wxString& path, json& data, wxString& error) {
try {
std::ifstream in(UFN(path), std::ios::binary);
if (!in) throw std::runtime_error(GetTranslation("AgentConversationUnreadable"));
data = json::parse(in);
if (!data.is_object() || data.value("format", "")!=AGENT_CONVERSATION_FORMAT) throw std::runtime_error(GetTranslation("AgentConversationNotConversation"));
int version = data.value("version", 0);
if (version<1 || version>AGENT_CONVERSATION_VERSION) throw std::runtime_error(format(GetTranslation("AgentConversationBadVersion"), version));
if (!data.contains("messages") || !data["messages"].is_array()) throw std::runtime_error("\"messages\" must be an array");
for (auto& m: data["messages"]) if (!m.is_object() || !m.contains("role")) throw std::runtime_error("invalid item in \"messages\"");
} catch (std::exception& e) {
error = U(format(GetTranslation("AgentConversationLoadError"), U(path), e.what()));
return false;
}
return true;
}

bool WriteConversationFile (const wxString& path, const nlohmann::ordered_json& data, wxString& error) {
try {
wxFileName fn(path);
if (!fn.DirExists() && !fn.Mkdir(wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)) throw std::runtime_error("unable to create the directory");
// Indented for readability; invalid UTF-8, which should never happen, is replaced instead of failing
std::string out = data.dump(1, '\t', false, nlohmann::ordered_json::error_handler_t::replace) + "\n";
wxString tmp = path + ".tmp";
{
std::ofstream o(UFN(tmp), std::ios::binary | std::ios::trunc);
o << out;
o.close();
if (!o) throw std::runtime_error("unable to write the file");
}
wxLogNull noLog;
if (!wxRenameFile(tmp, path, true)) {
wxRemoveFile(tmp);
throw std::runtime_error("unable to replace the file");
}
} catch (std::exception& e) {
error = U(format(GetTranslation("AgentConversationSaveError"), U(path), e.what()));
return false;
}
return true;
}
