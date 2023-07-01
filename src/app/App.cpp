#include "App.hpp"
#include "../common/ReadOnlyTextDialog.hpp"
#include "../common/println.hpp"
#include "../text/TextView.hpp"
#include "../text/TextDocument.hpp"
#include "../console/ConsoleView.hpp"
#include "../console/ConsoleDocument.hpp"
#include "../common/SingleChoiceDialog.hpp"
#include "../text/FindReplaceDialog.hpp"
#include "../text/TreeJumpListDialog.hpp"
#include "DocChildPanel.hpp"
#include "../common/ObjectClientData.hpp"
#include "../common/LiveRegion.hpp"
#include "../common/wxUtils.hpp"
#include "../common/stringUtils.hpp"

#include <wx/cmdline.h>
#include <wx/filename.h>
#include <wx/dir.h>
#include <wx/stdpaths.h>
#include <wx/textdlg.h>
#include <wx/filedlg.h>
#include <wx/dirdlg.h>
#include <wx/snglinst.h>
#include <wx/aboutdlg.h>
#include <wx/datetime.h>
#include <wx/translation.h>
#include <wx/cmdproc.h>
#include <wx/docmdi.h>
#include <wx/stream.h>
#include <wx/stdstream.h>
#include <wx/wfstream.h>
#include <wx/fontmap.h>
#include <wx/artprov.h>
#include <wx/sysopt.h>
#include <wx/fileconf.h>
#include<wx/dynlib.h>

#include<boost/core/demangle.hpp>
#include<string>
#include<vector>
#include<unordered_map>
using namespace std;

int GetEncodingFromIndex (int index);
bool CheckSingleInstance (const std::vector<wxString>& cmdArgs);
void CloseSingleInstance ();
void InitLua (lua_State*&);
void CloseLua (lua_State*&);
void LoadPlugins (App& app, Properties& props);
void test();

wxIMPLEMENT_APP(App);

struct CustomArtProvider: wxArtProvider {
wxBitmapBundle CreateBitmapBundle (const wxArtID& id, const wxArtClient& client, const wxSize& size) override {
return wxBitmapBundle::FromSVGFile("icons/" + id + ".svg", size);
}
};

struct CustomFileTranslationLoader: wxTranslationsLoader {
mutable wxArrayString langs;
virtual wxMsgCatalog* LoadCatalog (const wxString& domain, const wxString& lang) final override {
auto& stdPaths = wxStandardPaths::Get();
wxString basePath = wxFileName(stdPaths.GetExecutablePath()).GetPath();
wxString filename = basePath + "/lang/" + domain + "_" + lang + ".mo";
if (wxFileName::FileExists(filename)) return wxMsgCatalog::CreateFromFile( U(filename), domain );
else return nullptr;
}
     virtual wxArrayString GetAvailableTranslations(const wxString& domain) const final override {
if (!langs.empty()) return langs;
wxArrayString files;
auto& stdPaths = wxStandardPaths::Get();
wxString basePath = wxFileName(stdPaths.GetExecutablePath()).GetPath();
wxDir::GetAllFiles(basePath + "/lang/", &files);
for (auto& file: files) {
auto underscore = file.rfind('_');
auto dot = file.rfind('.');
if (dot-underscore!=3) continue;
wxString lang = file.substr(underscore+1, 2);
lang.MakeLower();
if (std::find(langs.begin(), langs.end(), lang)==langs.end()) langs.push_back(lang);
}
return langs;
}};

std::string GetTranslation (const std::string& key) {
return wxGetApp() .GetTranslations() .get(key, key);
}

int App::GetWindowMode () {
return indexOf({ "sdi", "mdi", "notebook", "auinotebook" }, config.get("window_mode", "auinotebook"));
}

int App::GetNotebookStyle (bool aui) {
std::string s = config.get("notebook_style", "bottom multiline");
int style = 0;
if (std::string::npos!=s.find("top")) style |= (aui? wxAUI_NB_TOP : wxNB_TOP);
else if (std::string::npos!=s.find("bottom")) style |= (aui? wxAUI_NB_BOTTOM : wxNB_BOTTOM);
else if (std::string::npos!=s.find("left")) style |= (aui? wxAUI_NB_LEFT : wxNB_LEFT);
else if (std::string::npos!=s.find("right")) style |= (aui? wxAUI_NB_RIGHT : wxNB_RIGHT);
if (std::string::npos!=s.find("multiline") && !aui) style |= wxNB_MULTILINE;
if (std::string::npos!=s.find("fixed with")) style |= (aui? wxAUI_NB_TAB_FIXED_WIDTH : wxNB_FIXEDWIDTH);
return style;
}

int App::GetSessionMode () {
return indexOf({ "never", "when_empty", "always" }, config.get("session_mode", "never"));
}

bool App::OnInit () {
initDirs();
initConfig();
initLocale();
initTranslations();
initDocManager();

if (!wxApp::OnInit()) return false;
if (config.get("single_instance", true) && CheckSingleInstance(cmdLineArgs)) return false;

wxArtProvider::PushBack(new CustomArtProvider());
wxSystemOptions::SetOption("msw.remap", 2);

auto windowMode = GetWindowMode();
switch(windowMode){
case MODE_SDI:
mainWindow = new wxDocParentFrame(docManager, nullptr, wxID_ANY, GetAppDisplayName(), wxDefaultPosition, wxDefaultSize);
break;
case MODE_MDI:
mainWindow = new wxDocMDIParentFrame(docManager, nullptr, wxID_ANY, GetAppDisplayName(), wxDefaultPosition, wxSize(768, 576));
break;
case MODE_NB:
case MODE_AUINB:
{
mainWindow = new wxDocParentFrame(docManager, nullptr, wxID_ANY, GetAppDisplayName(), wxDefaultPosition, wxSize(768, 576));
auto mwSizer = new wxBoxSizer(wxVERTICAL);
if (windowMode==MODE_NB) {
notebook = new wxNotebook(mainWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, GetNotebookStyle(false) | wxNB_NOPAGETHEME);
notebook->Bind(wxEVT_NOTEBOOK_PAGE_CHANGED, &App::OnPageChanged, this);
} else {
notebook = new wxAuiNotebook(mainWindow, wxID_ANY, wxDefaultPosition, wxDefaultSize, GetNotebookStyle(true) | wxAUI_NB_TAB_SPLIT | wxAUI_NB_TAB_MOVE | wxAUI_NB_SCROLL_BUTTONS | wxAUI_NB_CLOSE_ON_ACTIVE_TAB | wxAUI_NB_MIDDLE_CLICK_CLOSE | wxAUI_NB_WINDOWLIST_BUTTON);
notebook->Bind(wxEVT_AUINOTEBOOK_PAGE_CHANGED, &App::OnPageChanged, this);
notebook->Bind(wxEVT_AUINOTEBOOK_PAGE_CLOSE, &App::OnPageClose, this);
}
mwSizer->Add(notebook, 1, wxEXPAND);
mainWindow->SetSizer(mwSizer);
}
break;
default:
return false;
}

wxMenuBar* menubar = CreateMenuBar();
mainWindow->SetMenuBar(menubar);

wxToolBar* toolbar = CreateToolBar(mainWindow);
mainWindow->SetToolBar(toolbar);

CreateStatusBarForWindow(mainWindow);

liveRegion = new wxStaticText(mainWindow, wxID_ANY, wxEmptyString, wxPoint(0, 0), wxSize(1, 1));
liveRegion->Hide();
SetLiveRegion(liveRegion, LIVE_REGION_POLITE);

accelerators = {
{ wxACCEL_NORMAL, WXK_F6, IDM_SWITCH_PANE }
};
wxAcceleratorTable acctable(accelerators.size(), &accelerators[0]);
mainWindow->SetAcceleratorTable(acctable);

Bind(wxEVT_MENU, &App::OnMenuAction, this, IDM_FIRST, IDM_LAST);
mainWindow->Bind(wxEVT_CLOSE_WINDOW, &App::OnClose, this);
Bind(wxEVT_DOC_CLOSED, &App::OnDocumentClosed, this);
LoadPlugins(*this, config);

int sessionMode = GetSessionMode();
if (cmdLineArgs.empty()) {
if (sessionMode==SESSION_RELOAD_WHEN_EMPTY) sessionMode = SESSION_RELOAD_ALWAYS;
}
else {
for (auto& arg: cmdLineArgs) DoQuickJump(arg);
}
if (sessionMode==SESSION_RELOAD_ALWAYS) ReloadSession();
if (docManager->GetDocuments().empty()) CreateNewEmptyDocument();
mainWindow->Show((GetWindowMode()!=MODE_SDI) );
return true;
}

void App::OnInitCmdLine (wxCmdLineParser& cmd) {
wxApp::OnInitCmdLine(cmd);
cmd.AddParam(wxEmptyString, wxCMD_LINE_VAL_STRING, wxCMD_LINE_PARAM_MULTIPLE | wxCMD_LINE_PARAM_OPTIONAL);
}

bool App::OnCmdLineParsed (wxCmdLineParser& cmd) {
for (int i=0, n=cmd.GetParamCount(); i<n; i++) cmdLineArgs.push_back(cmd.GetParam(i));
return true;
}

void App::OnClose (wxCloseEvent& e) {
if (GetSessionMode()!=SESSION_RELOAD_NEVER) SaveSession();
closing = true;
e.Skip();
}

int App::OnExit () {
CloseSingleInstance();
//CloseLua(L);

wxFileConfig fileConfig(GetAppName());
docManager->FileHistorySave(fileConfig);

return wxApp::OnExit();
}

bool App::OnExceptionInMainLoop () {
try { throw; } catch (std::exception& e) {
std::string tp = boost::core::demangle(typeid(e).name()), what = e.what(), msg = tp + ": " + what;
OnLuaMessage(U(msg));
wxWindow* parent = nullptr;
auto doc = GetCurrentDocument();
if (doc) parent = doc->GetDocumentWindow();
if (parent) parent = mainWindow;
ReadOnlyTextDialog rtd(parent, MSG("ErrorOccurred"), MSG("Error"), msg, wxICON_ERROR | wxOK | RTD_DETAILS_OPEN);
rtd.ShowModal();
CloseSingleInstance();
}
return true;
}

void App::OnUnhandledException () {
OnExceptionInMainLoop();
}

bool App::initDocManager () {
docManager = new wxDocManager();
auto consoleTpl = new wxDocTemplate(docManager, "Console", "*.console", wxEmptyString, "console", "consoleDoc", "consoleView", wxCLASSINFO(ConsoleDocument), wxCLASSINFO(ConsoleView), wxTEMPLATE_INVISIBLE);
auto textTpl = new wxDocTemplate(docManager, "Text files", "*.*", wxEmptyString, "txt", "textDoc", "textView", wxCLASSINFO(TextDocument), wxCLASSINFO(TextView), wxTEMPLATE_VISIBLE);

wxFileConfig fileConfig(GetAppName());
docManager->FileHistoryLoad(fileConfig);

return true;
}

bool App::initDirs () {
SetAppName(APP_NAME);
SetClassName(APP_NAME);
SetVendorName(APP_VENDOR);
SetAppDisplayName(APP_DISPLAY_NAME);

auto& stdPaths = wxStandardPaths::Get();
appDir = wxFileName(stdPaths.GetExecutablePath()).GetPath();
userDir = stdPaths.GetUserDataDir();
userLocalDir = stdPaths.GetUserLocalDataDir();

cout << "userDir = " << userDir << endl;
cout << "userLocalDir = " << userLocalDir << endl;
cout << "appDir = " << appDir << endl;

auto userDirFn = wxFileName::DirName(userDir);
auto userLocalDirFn = wxFileName::DirName(userLocalDir);

pathList.Add(userDir);
pathList.Add(userLocalDir);
pathList.Add(appDir);

return 
userDirFn .Mkdir(wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)
&& userLocalDirFn .Mkdir(wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL)
&& userDirFn.IsDirReadable()
&& userLocalDirFn.IsDirReadable();
}

wxString App::FindAppFile (const wxString& filename) {
return pathList.FindAbsoluteValidPath(filename);
}

bool App::initConfig () {
wxString configIniPath = FindAppFile(CONFIG_FILENAME);
if (!configIniPath.empty()) {
wxFileInputStream fIn(configIniPath);
wxBufferedInputStream bIn(fIn);
wxStdInputStream in(bIn);
config.load(in);
}
return true;
}

bool App::initLocale () {
cout << "Initializing locale..." << endl;
locale = "default"; //config.get("locale", "default");
wxlocale = new wxLocale();
if (locale=="default") {
cout << "No locale set in the configuration, retrieving system default" << endl;
wxlocale->Init();
}
else {
auto info = wxLocale::FindLanguageInfo(U(locale));
if (info) wxlocale->Init(info->Language);
else cout << "Couldn't find locale information for " << locale << endl;
}
this->locale = U(wxlocale->GetCanonicalName());
auto& translations = *wxTranslations::Get();
translations.SetLoader(new CustomFileTranslationLoader());
translations.AddStdCatalog();
cout << "Locale configured to " << locale << endl;
return true;
}

bool App::initTranslations () {
vector<string> locales = {
locale,
locale.substr(0, 5),
locale.substr(0, 2),
"en"
};
for (string& l: locales) {
wxString filename = pathList.FindAbsoluteValidPath(format("lang/main_{}.properties", l));
if (!filename.empty()) {
wxFileInputStream fIn(filename);
wxBufferedInputStream bIn(fIn);
wxStdInputStream in(bIn);
translations.load(in);
break;
}}
return true;
}

lua_State* App::GetLuaState () { 
if (!L) InitLua(L);
return L;
}

bool App::ReloadSession () {
wxString sessionPath = FindAppFile(SESSION_FILENAME);
if (sessionPath.empty()) return false;
wxFileInputStream fIn(sessionPath);
wxBufferedInputStream bIn(fIn);
wxStdInputStream in(bIn);
Properties session;
session.load(in);
for (int i=1; ; i++) {
std::string file = session.get("document" + std::to_string(i));
if (file.empty()) break;
DoQuickJump(U(file));
}
return true;
}

bool App::SaveSession () {
wxLogNull logNull;
wxString sessionPath = appDir + "/" SESSION_FILENAME;
wxFileOutputStream fOut(sessionPath);
if (!fOut.IsOk()) return false;
wxBufferedOutputStream bOut(fOut);
wxStdOutputStream out(bOut);
Properties session;
auto& wxdocs = docManager->GetDocuments();
for (size_t i=0; i<wxdocs.size(); i++) {
auto doc = static_cast<AbstractDocument*>(wxdocs[i]);
session.put("document" + std::to_string(i+1), doc->GetFilename());
}
session.save(out);
return true;
}

bool App::LoadPlugin (const std::string& name) {
typedef bool(*PluginFunc)(PluginInterface&);
typedef std::unordered_map<std::string, std::unique_ptr<wxDynamicLibrary>> PluginMap;
static PluginMap* pluginmap = nullptr;
if (!pluginmap) pluginmap = new PluginMap();
auto& plugins = *pluginmap;
if (plugins.find(name)!=plugins.end()) return true;
wxLogNull logNull;
auto dll = std::make_unique<wxDynamicLibrary>( U(name) );
if (!dll || !dll->IsLoaded()) return false;
PluginFunc func = (PluginFunc) dll->GetSymbol("LoadPlugin");
if (!func || !func(*this)) return false;
plugins[name] .swap(dll);
return true;
}

void App::SayText (const wxString& text) {
if (!liveRegion) return;
liveRegion->SetLabel(text);
LiveRegionUpdated(liveRegion);
}

void App::OnMenuAction (wxCommandEvent& e) {
size_t id = e.GetId();
switch(id){
case wxID_JUMP_TO: OnQuickJump(); break;
case wxID_ABOUT: OnAbout(); break;
case IDM_MULTIFIND: OnMultiFindReplace(false); break;
case IDM_MULTIREPLACE: OnMultiFindReplace(true); break;
case IDM_EXEC_COMMAND: OnExecCommandDialog(); break;
case IDM_LUA_CONSOLE: ShowOrCreateLuaConsole(); break;
case IDM_FILE_TREE: ShowFileTree(); break;
default:  
auto view = static_cast<AbstractView*>(docManager->GetCurrentView());
auto doc = static_cast<AbstractDocument*>(docManager->GetCurrentDocument());
if (
(!view || !view->OnMenuAction(e))
&& (!doc || !doc->OnMenuAction(e))
) e.Skip(); 
break;
}}

AbstractDocument* App::CreateNewEmptyDocument (const wxString& baseFileName, const wxString& title) {
auto doc = docManager->CreateDocument(baseFileName, wxDOC_SILENT | wxDOC_NEW);
if (!title.empty()) doc->SetTitle(title);
doc->SetDocumentSaved(false);
UpdateTitle();
return static_cast<AbstractDocument*>( doc );
}

AbstractDocument* App::CreateLuaConsole () {
auto doc = CreateNewEmptyDocument("lua.console", MSG("LuaConsole"));
static_cast<ConsoleView*>( doc->GetFirstView() ) ->MakeLuaConsole();
return doc;
}

void App::ShowOrCreateLuaConsole () {
auto lc = LuaConsoleTextAppender::GetInstance();
if (lc) lc->GetView()->Activate(true);
else CreateLuaConsole();
}

void App::OnLuaMessage (const wxString& error) {
std::cout << error << std::endl;
auto lc = LuaConsoleTextAppender::GetInstance();
if (lc) {
lc->Append("\n" + error);
lc->Append("\n>>>", false);
lc->GetView()->Activate(true);
}
}

void App::ShowFileTree () {
auto doc = GetCurrentDocument();
if (!doc) return;
wxString rootDir = doc->GetWorkspaceRoot();
if (rootDir.empty()) return;
auto tjld = new TreeJumpListDialog(doc->GetDocumentWindow(), MSG("WorkspaceFileTree"), MSG("WorkspaceFileTree"));
AddFileTree(tjld->GetTreeJumpList(), rootDir, "*.*");
tjld->Show();
}

wxString App::FindQuickJumpMatchingFilename (const wxString& name) {
auto doc = GetCurrentDocument();
if (doc && name.find_first_of("/\\.")==std::string::npos) {
wxString glob("*"), rootDir = doc->GetWorkspaceRoot();
for (size_t i=0, n=name.size(); i<n; i++) {
glob += name[i];
if (isupper(name[i])) glob += '*';
}
if (glob[glob.size() -1]!='*') glob += '*';
wxArrayString files;
wxDir::GetAllFiles(rootDir, &files, glob, wxDIR_FILES | wxDIR_DIRS);
if (files.size()==1) return files[0];
else if (files.size()>1) {
wxString ext;
wxFileName::SplitPath(doc->GetFilename(), nullptr, nullptr, &ext);
ext = '.' + ext;
if (1==std::count_if(files.begin(), files.end(), [&](auto&f){ return iends_with(U(f), U(ext)); })) {
return *std::find_if(files.begin(), files.end(), [&](auto&f){ return iends_with(U(f), U(ext)); });
}
wxArrayString fileList;
for (auto& file: files) {
auto i = file.find_last_of("/\\");
fileList.push_back(file.substr(i+1) + " (" + file + ')');
}
SingleChoiceDialog scd(doc->GetFirstView()->GetFrame(), MSG("QuickJumpDlg"), MSG("SelectFileToOpen"), fileList, -1, true);
if (wxID_OK==scd.ShowModal() && scd.GetSelection()>=0) {
return files[scd.GetSelection()];
}
}
}
wxFileName fn(name);
return fn.GetAbsolutePath();
}

AbstractDocument* App::GetCurrentDocument () {
return static_cast<AbstractDocument*>( docManager->GetCurrentDocument() );
}

AbstractDocument* App::FindDocument (const wxString& filename) {
auto wxdoc = docManager->FindDocumentByPath(filename);
return wxdoc? static_cast<AbstractDocument*>(wxdoc) : nullptr;
}

AbstractDocument* App::OpenOrCreateDocument (const wxString& name) {
if (name.empty()) return nullptr;
wxString filename;
wxFileName fn(name);
if (fn.IsAbsolute()) filename = name;
else if (fn.Exists()) filename = fn.GetAbsolutePath();
else filename = FindQuickJumpMatchingFilename(name);
auto doc = docManager->FindDocumentByPath(filename);
if (!doc) doc = docManager->CreateDocument(filename, wxDOC_SILENT);
return static_cast<AbstractDocument*>( doc );
}

wxWindow* App::CreateChildWindow (wxView* view) {
auto doc = view->GetDocument();
wxWindow* win = nullptr;

switch(GetWindowMode()){
case MODE_SDI:
win = new wxDocChildFrame(doc, view, wxStaticCast(mainWindow, wxDocParentFrame), wxID_ANY, doc->GetUserReadableName(), wxDefaultPosition, wxDefaultSize);
break;
case MODE_MDI:
win = new wxDocMDIChildFrame(doc, view, wxStaticCast(mainWindow, wxDocMDIParentFrame), wxID_ANY, doc->GetUserReadableName(), wxDefaultPosition, wxDefaultSize);
break;
case MODE_NB:
case MODE_AUINB:
win = new DocChildPanel(view, notebook);
notebook->AddPage(win, doc->GetUserReadableName(), true);
break;
default:
return nullptr;
}

if (auto frame = wxDynamicCast(win, wxFrame)) {
CreateStatusBarForWindow(frame);
frame->Show(true);
}

return win;
}

wxStatusBar* App::CreateStatusBarForWindow (wxFrame* frame) {
auto status = frame->CreateStatusBar();
int widths[] = { -1, 100, 50 };
status->SetFieldsCount(3, widths);

auto handler = [=](wxMouseEvent& e){
auto pos = e.GetPosition();
auto view = static_cast<AbstractView*>(docManager->GetCurrentView());
for (int i=0, n=status->GetFieldsCount(); i<n; i++) {
wxRect rect;
if (!status->GetFieldRect(i, rect)) continue;
if (rect.Contains(pos)) view->OnStatusBarClick(e, i);
}
e.Skip();
};
#define H(N) status->Bind(wxEVT_##N, handler);
H(LEFT_DOWN) H(RIGHT_DOWN) H(MIDDLE_DOWN)
#undef H

return status;
}

void App::UpdateTitle () {
switch(GetWindowMode()) {
case MODE_SDI:
break;
case MODE_MDI:
mainWindow->SetTitle(GetAppDisplayName());
break;
case MODE_NB:
case MODE_AUINB:

auto doc = static_cast<AbstractDocument*>(docManager->GetCurrentDocument());
auto view = static_cast<AbstractView*>(docManager->GetCurrentView());
wxString title;
if (doc) title = doc->GetUserReadableName();
if (doc && doc->IsModified()) title += '*';

mainWindow->SetTitle(title + " - " + GetAppDisplayName());

int pageIndex = view? view->GetPageIndex() :-8;
if (pageIndex>=0) notebook->SetPageText(pageIndex, title);
break;
}}

void App::OnPageChanged (wxBookCtrlEvent& e) {
int i = e.GetSelection();
if (i<0) return;
auto page = dynamic_cast<DocChildPanel*>( notebook->GetPage(i) );
if (!page) return;
auto view = page->GetView();
if (!view || !IsValidView(view)) return;
view->Activate(true);
UpdateTitle();
}

void App::OnPageClose (wxAuiNotebookEvent& e) {
int i = e.GetSelection();
if (i<0) return;
auto page = dynamic_cast<DocChildPanel*>( notebook->GetPage(i) );
if (!page) return;
auto view = page->GetView();
if (!view->Close()) e.Veto();
}

void App::OnDocumentClosed (wxCommandEvent& e) {
e.Skip();
if (closing) return;
CallAfter([=](){
if (docManager->GetDocuments().empty()
&& (GetWindowMode()==MODE_SDI || config.get("exit_on_last_close", true))
) mainWindow->Close();
});
}

bool App::IsValidDocument (wxDocument* doc) {
auto docs = docManager->GetDocumentsVector();
for (auto& d: docs) if (d==doc) return true;
return false;
}

bool App::IsValidView (wxView* view) {
auto docs = docManager->GetDocumentsVector();
for (auto& d: docs) 
for (auto& v: d->GetViewsVector()) 
if (v==view) return true;
return false;
}

void App::OnAbout () {
wxAboutDialogInfo info;
info.SetName(GetAppDisplayName());
info.SetVersion(U(APP_VERSION_STRING));
info.SetCopyright(U(APP_COPYRIGHT_INFO));
info.SetWebSite(APP_WEBSITE_URL);
wxAboutBox(info);
}

void splitJumpCommand (const wxString& s, wxString& file, wxString& cmd, wxString& kv) {
if (!s.empty()) {
wxChar firstChar = s[0];
if ((firstChar>='A' && firstChar<='Z') || (firstChar>='a' && firstChar<='z') || firstChar=='/' || firstChar=='\\' || firstChar=='.' || firstChar=='*' || firstChar=='?' || firstChar>=128) {
auto colon = s.find(':', 2);
file = colon==std::string::npos? s : s.substr(0, colon);
cmd = colon==std::string::npos? wxString(wxEmptyString) : s.substr(colon+1);
}
else if (firstChar==':') cmd = s.substr(1);
else  cmd = s;
}
auto i = cmd.find('?');
if (i!=std::string::npos) {
kv = cmd.substr(i+1);
cmd = cmd.substr(0, i);
}
}

bool App::DoQuickJump (const wxString& s) {
if (starts_with(s, "http://") || starts_with(s, "https://")) {
wxLaunchDefaultBrowser(s, wxBROWSER_NEW_WINDOW);
return true;
}
bool succeeded = false;
wxString file, cmd, kv;
splitJumpCommand(s, file, cmd, kv);
auto doc = file.empty()? GetCurrentDocument()  : OpenOrCreateDocument(file);
if (doc) {
doc->Activate();
if (!cmd.empty()) succeeded = doc->DoQuickJump(cmd);
UpdateTitle();
}
return succeeded;
}

void App::OnQuickJump () {
wxTextEntryDialog ted(GetTopWindow(), MSG("EnterQuickJumpCommand") +":", MSG("QuickJumpDlg"), wxEmptyString, wxOK | wxCANCEL);
if (wxID_OK==ted.ShowModal()) {
wxString cmd = ted.GetValue();
if (!cmd.empty()) BellIfFalse(DoQuickJump(cmd));
}}

bool App::ExecuteCommand (const wxString& cmd) {
auto doc = static_cast<ConsoleDocument*>( CreateNewEmptyDocument("command.console", cmd));
bool succeeded = doc->ExecuteCommand(cmd);
if (!succeeded) doc->GetFirstView()->Close();
return succeeded;
}

void App::OnExecCommandDialog () {
wxTextEntryDialog ted(GetTopWindow(), MSG("EnterExecCommand") +":", MSG("ExecCommandDlg"), wxEmptyString, wxOK | wxCANCEL);
if (wxID_OK==ted.ShowModal()) {
ExecuteCommand(ted.GetValue());
}}

void App::OnMultiFindReplace (bool replace) {
FindReplaceInfo info;
auto view = docManager->GetCurrentView();
auto wxdoc = view? view->GetDocument() :nullptr;
auto doc = dynamic_cast<TextDocument*>(wxdoc);
info.flags = FRI_MULTIPLE | (replace? FRI_REPLACE : 0);
if (wxdoc) {
wxString filename = wxdoc->GetFilename();
if (!filename.empty()) {
wxFileName::SplitPath(filename, &info.rootDir, nullptr, &info.glob);
if (!info.glob.empty()) info.glob = "*." + info.glob;
if (doc) info.rootDir = doc->GetWorkspaceRoot();
}}
auto frd = new FindReplaceDialog(view->GetFrame(), info);
frd->Show();
}

wxMenuBar* App::CreateMenuBar () {
auto menubar = new wxMenuBar();
auto file = new wxMenu();
auto history = new wxMenu();
file->Append(wxID_NEW);
file->Append(wxID_OPEN);
file->Append(wxID_SAVE);
file->Append(wxID_SAVEAS, wxGetStockLabel(wxID_SAVEAS) + "\tCtrl+Shift+S");
file->AppendSeparator();
file->Append(wxID_REVERT_TO_SAVED, MSG("Reload") + "\tF5");
file->Append(wxID_CLOSE, wxGetStockLabel(wxID_CLOSE) + "\tCtrl+F4");
file->AppendSeparator();
file->Append(wxID_ANY, MSG("RecentFiles"), history);
file->AppendSeparator();
file->Append(wxID_EXIT);
file->SetClientObject(new StringClientData("file"));
menubar->Append(file, wxGetStockLabel(wxID_FILE));

auto edit = new wxMenu();
edit->Append(wxID_UNDO);
edit->Append(wxID_REDO);
edit->AppendSeparator();
edit->Append(wxID_COPY);
edit->Append(wxID_CUT);
edit->Append(wxID_PASTE);
edit->AppendSeparator();
edit->Append(wxID_JUMP_TO, MSG("QuickJump") + "...\tCtrl+J");
edit->Append(wxID_FIND, MSG("Find") + "...\tCtrl+F");
edit->Append(IDM_FINDNEXT, MSG("FindNext") + "\tF3");
edit->Append(IDM_FINDPREV, MSG("FindPrev") + "\tShift+F3");
edit->Append(wxID_REPLACE, MSG("Replace") + "...\tCtrl+H");
edit->AppendSeparator();
edit->Append(wxID_SELECTALL);
edit->SetClientObject(new StringClientData("edit"));
menubar->Append(edit, wxGetStockLabel(wxID_EDIT));

auto tools = new wxMenu();
tools->Append(IDM_MULTIFIND, MSG("Multifind") + "...\tCtrl+Shift+F");
tools->Append(IDM_MULTIREPLACE, MSG("Multireplace") + "...\tCtrl+Shift+H");
tools->Append(IDM_FILE_TREE, MSG("WorkspaceFileTree") + "...");
tools->Append(IDM_EXEC_COMMAND, MSG("ExecCommand") + "...\tF10");
tools->Append(IDM_LUA_CONSOLE, MSG("LuaConsoleMI") + "\tF12");
tools->SetClientObject(new StringClientData("tools"));
menubar->Append(tools, MSG("ToolsMenu"));

auto help = new wxMenu();
help->Append(wxID_ABOUT);
#ifdef DEBUG
help->Append(IDM_TEST, "Test item\tF11");
#endif
help->SetClientObject(new StringClientData("help"));
menubar->Append(help, "?");

docManager->FileHistoryAddFilesToMenu(history);
docManager->FileHistoryUseMenu(history);

return menubar;
}

wxToolBar* App::CreateToolBar (wxFrame* parentFrame) {
auto sTbStyle = config.get("toolbar_style", "icon+text");
int tbStyle = 0;
if (std::string::npos==sTbStyle.find("icon")) tbStyle |= wxTB_NOICONS;
if (std::string::npos!=sTbStyle.find("text")) tbStyle |= wxTB_TEXT;
if (std::string::npos!=sTbStyle.find("one line")) tbStyle |= wxTB_HORZ_TEXT;
if (std::string::npos!=sTbStyle.find("flat")) tbStyle |= wxTB_FLAT;
if (std::string::npos!=sTbStyle.find("dockable")) tbStyle |= wxTB_DOCKABLE;
if (std::string::npos!=sTbStyle.find("no divider")) tbStyle |= wxTB_NODIVIDER;
if (std::string::npos!=sTbStyle.find("vertical")) tbStyle |= wxTB_VERTICAL;
if (std::string::npos!=sTbStyle.find("right")) tbStyle |= wxTB_RIGHT;
if (std::string::npos!=sTbStyle.find("bottom")) tbStyle |= wxTB_BOTTOM;
auto toolbar = new wxToolBar(parentFrame, wxID_ANY, wxDefaultPosition, wxDefaultSize, tbStyle);
toolbar->AddTool(wxID_NEW, wxGetStockLabel(wxID_NEW), wxArtProvider::GetBitmapBundle(wxART_NEW));
toolbar->AddTool(wxID_OPEN, wxGetStockLabel(wxID_OPEN), wxArtProvider::GetBitmapBundle(wxART_FILE_OPEN));
toolbar->AddTool(wxID_SAVE, wxGetStockLabel(wxID_SAVE), wxArtProvider::GetBitmapBundle(wxART_FILE_SAVE));
toolbar->AddTool(wxID_SAVEAS, wxGetStockLabel(wxID_SAVEAS), wxArtProvider::GetBitmapBundle(wxART_FILE_SAVE_AS));
toolbar->AddSeparator();
toolbar->AddTool(wxID_COPY, wxGetStockLabel(wxID_COPY), wxArtProvider::GetBitmapBundle(wxART_COPY));
toolbar->AddTool(wxID_CUT, wxGetStockLabel(wxID_CUT), wxArtProvider::GetBitmapBundle(wxART_CUT));
toolbar->AddTool(wxID_PASTE, wxGetStockLabel(wxID_PASTE), wxArtProvider::GetBitmapBundle(wxART_PASTE));
toolbar->AddSeparator();
toolbar->AddTool(wxID_UNDO, wxGetStockLabel(wxID_UNDO), wxArtProvider::GetBitmapBundle(wxART_UNDO));
toolbar->AddTool(wxID_REDO, wxGetStockLabel(wxID_REDO), wxArtProvider::GetBitmapBundle(wxART_REDO));
toolbar->AddSeparator();
toolbar->AddTool(wxID_FIND, MSG("Find"), wxArtProvider::GetBitmapBundle(wxART_FIND));
toolbar->AddTool(wxID_REPLACE, MSG("Replace"), wxArtProvider::GetBitmapBundle(wxART_FIND_AND_REPLACE));
return toolbar;
}

