#include "base.hpp"
#include "../../app/App.hpp"
#include "../../app/AbstractDocument.hpp"

LuaRegisterReferenceType(App);
LuaRegisterReferenceType(wxApp);
LuaRegisterReferenceType(AbstractDocument)

static void AppSayText (const wxString& text) {
wxGetApp() .SayText(text);
}

static int AppGetDocuments (lua_State* L) {
lua_getglobal(L, "Document");
auto& app = lua_get<App&>(L, 1);
auto docManager = app.GetDocManager();
lua_newtable(L);
int i=0;
for (auto doc: docManager->GetDocumentsVector()) {
lua_push(L,  static_cast<AbstractDocument*>(doc) );
lua_rawseti(L, -2, ++i);
}
return 1;
}

static AbstractDocument* AppCreateNewDoc (App& app) {
return app.CreateNewEmptyDocument();
}

static std::string AppGetConfigKey (App& app, const std::string& key) {
return app.GetConfig().get(key, "");
}

static std::string AppGetTranslationKey (App& app, const std::string& key) {
return app.GetTranslations().get(key, key);
}

export int luaopen_App (lua_State* L) {
lua_getglobal(L, "Event");
lua_pop(L, 1);
lua_pushglobaltable(L);

//T Type representing the application. A single instance of this type exists, under the global variable `app`.
Binding::LuaClass<App>(L, "App")
.parent<wxEvtHandler>()

//G string: current language of the application
.getter("locale", &App::GetLocale)
//G string: directory where the executable of the application is found
.getter("applicationDirectory", &App::GetAppDir)
//G string: directory where user settings are stored
.getter("userDataDirectory", &App::GetUserDir)
//G string: directory where local user settings are stored
.getter("userLocalDataDirectory", &App::GetUserLocalDir)
//M Get a configuration key
//P key: string: nil: the configuration key to get
//R string: the configuration value, or an empty string if nothing is defined for that key
.method("getConfig", &AppGetConfigKey)
//M Get a translation key
//P key: string: nil: the translation key to get
//R string: the translation corresponding to that key, or an empty string if it was not found
.method("getTranslation", &AppGetTranslationKey)

//G table: Table of all documents currently opened in the application
.getter("documents", &AppGetDocuments)
//G Document: The currently active document
.getter("currentDocument", &App::GetCurrentDocument)
//M Open the specified filename, or reactivate (put its tab/window in front) if it's already open
//P filename: string: nil: file to open or reactivate
//R Document: the document opened or reactivated
.method("openDocument", &App::OpenOrCreateDocument)
//M Open a new tab/window with an empty document
.method("openNewDocument", &AppCreateNewDoc)
//M Execute a command and put the result in a new tab/window. This is equivalent to Tools>Run... or F10
//P command: string: nil: command to execute
//R boolean: true if the execution succeeded. 
.method("execute", &App::ExecuteCommand)
//M Execute a quick jump command. This is equivalent to Ctrl+J
//P command: string: nil: quick jump command to execute
//R boolean: true if the quick jump command succeeded
.method("quickJump", &App::DoQuickJump)

//F send some text to the live region, to be spoken by screen readers
//P text: string: nil: text to be spoken by screen readers
.method("sayText", &AppSayText)

//F Set the text of the clipboard
//P text: string: nil: text to put into the clipboard
.method("setClipboardText", &SetClipboardText)

//F Get the current text of the clipboard
//R string: text of the clipboard
.method("getClipboardText", &GetClipboardText)

.pop();
lua_getglobal(L, "App");
return 1;
}

export int luaopen_app (lua_State* L) {
lua_getglobal(L, "App");
lua_pop(L, 1);
lua_push(L, &wxGetApp());
return 1;
}

