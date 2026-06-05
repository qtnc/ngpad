#ifndef ____APP_HPP
#define ____APP_HPP
#include "constants.hpp"
#include "../common/Properties.hpp"
#include "../common/wxUtils.hpp"
#include "PluginInterface.hpp"
#include "Worker.hpp"
#include <wx/thread.h>
#include <wx/accel.h>
#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/notebook.h>
#include <wx/aui/auibook.h>
#include "wx/docview.h"
#include<string>
#include<vector>
#include<unordered_map>
#include<fstream>
#include<functional>
#include<memory>
#include<future>

class AbstractDocument;

class App;
wxDECLARE_APP(App);

class App: public wxApp, public PluginInterface  {
private:
wxDocManager* docManager = nullptr;
wxFrame* mainWindow = nullptr;
wxBookCtrlBase* notebook = nullptr;
wxStaticText* liveRegion = nullptr;
wxPathList pathList;
struct lua_State* L = nullptr;
Properties config, translations;

wxLocale* wxlocale = nullptr;
std::string locale;
wxString appDir, userDir, userLocalDir;

std::vector<wxAcceleratorEntry> accelerators;
std::vector<wxString> cmdLineArgs;
bool closing = false;

Worker worker;

friend void test();

bool initDirs ();
bool initConfig ();
bool initLocale ();
bool initTranslations ();
bool initDocManager ();

public:
int GetWindowMode ();
int GetSessionMode ();
wxString FindAppFile (const wxString& filename);
Properties& GetConfig () override { return config; }
Properties& GetTranslations () override { return translations; }
const std::string& GetLocale () override { return locale; }
const wxString& GetAppDir () override { return appDir; }
const wxString& GetUserDir () override { return userDir; }
const wxString& GetUserLocalDir () override { return userLocalDir; }
wxFrame* GetMainWindow () { return mainWindow; }
wxBookCtrlBase* GetNotebook () { return notebook; }
int GetNotebookStyle (bool aui);
wxWindow* CreateChildWindow (wxView* view);
wxMenuBar* CreateMenuBar ();
wxToolBar* CreateToolBar (wxFrame* parentFrame);
wxStatusBar* CreateStatusBarForWindow (wxFrame* frame);
lua_State* GetLuaState ();
wxDocManager* GetDocManager () override { return docManager; }
std::vector<wxAcceleratorEntry>& GetAccelerators () { return accelerators; }

template <class F> inline void Submit (const F& f) { worker.submit(f); }

void UpdateTitle ();
void SayText (const wxString& text) override;
bool IsClosing () { return closing; }
bool IsValidDocument (wxDocument* doc);
bool IsValidView (wxView* view);
bool LoadPlugin (const std::string& name);
bool ReloadSession ();
bool SaveSession ();

AbstractDocument* FindDocument (const wxString& filename);
AbstractDocument* OpenOrCreateDocument (const wxString& filename) override;
AbstractDocument* CreateNewEmptyDocument (const wxString& baseFileName = DEFAULT_NEW_DOC_BASE_FILENAME, const wxString& title = wxEmptyString);
AbstractDocument* GetCurrentDocument () override;
AbstractDocument* CreateLuaConsole ();
void ShowOrCreateLuaConsole ();
void ShowFileTree ();
bool ExecuteCommand (const wxString& cmd);

void OnDocumentClosed (wxCommandEvent& e);
void OnPageChanged (wxBookCtrlEvent& e);
void OnPageClose (wxAuiNotebookEvent& e);
void OnAbout ();
void OnMultiFindReplace (bool replace);
void OnExecCommandDialog ();
void OnMenuAction (wxCommandEvent& e);
void HandleLuaBoundEvent (wxEvent& e);
void OnLuaMessage (const wxString& msg);

bool OnInit () override;
void OnInitCmdLine (wxCmdLineParser& cmd) override;
bool OnCmdLineParsed (wxCmdLineParser& cmd) override;
bool OnExceptionInMainLoop () override;
void OnUnhandledException () override;
int OnExit () override;
void OnClose (wxCloseEvent& e);

void OnQuickJump ();
bool DoQuickJump (const wxString& cmd);
wxString FindQuickJumpMatchingFilename (const wxString& pattern);
};

template <class F> inline void RunEDT (F&& f) {
if (wxIsMainThread()) std::forward<F>(f)();
else wxGetApp().CallAfter(f);
}

template <class F> inline decltype(auto) RunEDTSync (F&& f) {
using Fn = std::decay_t<F>;
using R = std::invoke_result_t<Fn&>;
if (wxIsMainThread()) return std::forward<F>(f)(); 
auto& app = wxGetApp();
    auto fn = std::make_shared<Fn>(std::forward<F>(f));
    auto promise = std::make_shared<std::promise<R>>();
    auto future = promise->get_future();
    wxGetApp().CallAfter([fn, promise]() mutable {
        try {
            if constexpr (std::is_void_v<R>) {
                (*fn)();
                promise->set_value();
            } else {
                promise->set_value((*fn)());
            }
        } catch (...) {
            promise->set_exception(std::current_exception());
        }
    });
    if constexpr (std::is_void_v<R>)         future.get();
else {
        return future.get();
    }
}

template <class F> inline decltype(auto) RunEDTSync (wxCriticalSection& cs, F&& f) {
using Fn = std::decay_t<F>;
using R = std::invoke_result_t<Fn&>;
if (wxIsMainThread()) return std::forward<F>(f)(); 
auto& app = wxGetApp();
    auto fn = std::make_shared<Fn>(std::forward<F>(f));
    auto promise = std::make_shared<std::promise<R>>();
    auto future = promise->get_future();
cs.Leave();
    wxGetApp().CallAfter([fn, promise, &cs]() mutable {
wxCriticalSectionLocker csl(cs);
        try {
            if constexpr (std::is_void_v<R>) {
                (*fn)();
                promise->set_value();
            } else {
                promise->set_value((*fn)());
            }
        } catch (...) {
            promise->set_exception(std::current_exception());
        }
    });
    if constexpr (std::is_void_v<R>)         {
future.get();
cs.Enter();
}
else {
auto re = future.get();
cs.Enter();
return re;
    }
}

#endif

