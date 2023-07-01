#include "base.hpp"
#include "../../app/App.hpp"
#include "../../text/TextDocument.hpp"
#include "../../text/TextView.hpp"
#include "../../text/TextEditor.hpp"

LuaRegisterReferenceType(wxMenuBar);
LuaRegisterReferenceType(wxToolBar);
LuaRegisterReferenceType(AbstractDocument)
LuaRegisterTypeAlias(wxDocument, AbstractDocument)
LuaRegisterReferenceType(wxTextCtrlIface)

struct LuaEventHandler* BindLuaHandler (lua_State* L, int idx, wxEvtHandler& obj, wxEventType type, int id1, int id2);


static wxMenuBar* DocGetMenus (AbstractDocument& doc) {
lua_State* L = wxGetApp() .GetLuaState();
lua_getglobal(L, "MenuBar");
lua_getglobal(L, "Menu");
lua_getglobal(L, "MenuItem");
return static_cast<AbstractView*>(doc.GetFirstView()) ->GetMenuBar();
}

static wxTextCtrlIface* DocGetEditor (AbstractDocument& doc) {
lua_State* L = wxGetApp() .GetLuaState();
lua_getglobal(L, "TextEditor");
return dynamic_cast<TextEditor*>(doc.GetEditor());
}

static wxToolBar* DocGetTools (AbstractDocument& doc) {
lua_State* L = wxGetApp() .GetLuaState();
lua_getglobal(L, "ToolBar");
return static_cast<AbstractView*>(doc.GetFirstView()) ->GetToolBar();
}

static int DocGetProperty (lua_State* L) {
auto doc = lua_get<AbstractDocument*>(L, 1);
auto& props = doc->GetProperties();
const char* key = lua_tostring(L, 2);
if (lua_isnoneornil(L, 3)) {
if (!props.contains(key)) lua_pushnil(L);
else {
std::string value = props.get(key, "");
if (value=="true") lua_pushboolean(L, true);
else if (value=="false") lua_pushboolean(L, false);
else lua_push(L, value);
}}
#define P(T) else if (lua_is##T(L, 3)) lua_push(L, props.get(key, lua_to##T(L, 3)));
P(string)
P(integer)
P(boolean)
P(number)
#undef P
else luaL_typeerror(L, 3, "string, number or boolean");
return 1;
}

int DocSetProperty (lua_State* L) {
auto doc = lua_get<AbstractDocument*>(L, 1);
auto& props = doc->GetProperties();
const char* key = lua_tostring(L, 2);
bool replace = !lua_isboolean(L, 4) || lua_toboolean(L, 4);
if (false) {}
#define P(T) else if (lua_is##T(L, 3)) lua_pushboolean(L, props.put(key, lua_to##T(L, 3), replace));
P(string)
P(integer)
P(boolean)
P(number)
#undef P
else luaL_typeerror(L, 3, "string, number or boolean");
return 1;
}

static int DocGetAllProperties (lua_State* L) {
auto doc = lua_get<AbstractDocument*>(L, 1);
auto& props = doc->GetProperties();
lua_newtable(L);
propertiesToTable(L, -1, props);
return 1;
}

static int DocSetAllProperties (lua_State* L) {
auto doc = lua_get<AbstractDocument*>(L, 1);
auto& props = doc->GetProperties();
tableToProperties(L, -1, props);
return 1;
}

static wxString DocGetStatus (AbstractDocument& doc, int i) {
auto aview = doc.GetFirstView();
auto tview = aview? static_cast<TextView*>(aview) : nullptr;
auto status = tview? tview->GetStatusBar() : nullptr;
return status? status->GetStatusText(i) : wxString();
}

static bool DocSetStatus (AbstractDocument& doc, const wxString& text, int i) {
auto aview = doc.GetFirstView();
auto tview = aview? static_cast<TextView*>(aview) : nullptr;
auto status = tview? tview->GetStatusBar() : nullptr;
if (status) status->SetStatusText(text, i);
return !!status;
}

static int DocBindAccelerator (lua_State* L) {
auto doc = lua_get<AbstractDocument*>(L, 1);
wxString key = lua_get<wxString>(L, 2);
wxAcceleratorEntry entry(0, 0, 0);
entry.FromString(key);
bool result = static_cast<AbstractView*>(doc->GetFirstView())->AddAccelerator(entry);
if (!result) return 0;
auto re = BindLuaHandler(L, 3, *doc, wxEVT_MENU, entry.GetCommand(), wxID_ANY);
lua_pushlightuserdata(L, re);
lua_pushinteger(L, entry.GetCommand());
return 2;
}

export int luaopen_Document (lua_State* L) {
lua_pushglobaltable(L);

//T Type representing a document currently opened in the application
Binding::LuaClass<AbstractDocument>(L, "Document")
.parent<wxEvtHandler>()
.referenceEquals()

//M Activate the document and make it visible
.method("activate", &AbstractDocument::Activate)
//M Close the document
.method("close", &AbstractDocument::Close)
//M Save the document
.method("save", &AbstractDocument::Save)
//M Save the document under another name. This opens the "Save as" dialog box.
.method("saveAs", &AbstractDocument::SaveAs)
//M Revert the document to the state of its last save.
.method("revert", &AbstractDocument::Revert)
//M Reload the document from disk and update the display.
.method("reload", &AbstractDocument::Reload)

//M Bind an accelerator to an action
//P accelerator: string: nil: accelerator to bind
//P action: function: nil: action to execute when the accelerator is triggered
//R handler: an object that can be passed to unbind() in order to cancel the event
.method("bindAccelerator", &DocBindAccelerator)

//G boolean: tells if the document hasn't been modified since the last save
.boolGetter("alreadySaved", &AbstractDocument::AlreadySaved)
//A boolean: tells if the document has already been saved
.boolProperty("saved", &AbstractDocument::GetDocumentSaved, &AbstractDocument::SetDocumentSaved)
//A boolean: tells if the document has been modified since its last save.
.boolProperty("modified", &AbstractDocument::IsModified, &AbstractDocument::Modify)
//G string: type of document
.getter("type", &AbstractDocument::GetDocumentName)

//G MenuBar: Menu bar of the document
.getter("menus", &DocGetMenus)
//G ToolBar: Toolbar of the document
.getter("tools", &DocGetTools)
//G TextEditor: Text edition zone of the document
.getter("editor", &DocGetEditor)
//A string: content of the status bar
.property("status", &DocGetStatus, &DocSetStatus)

//M Get a property of this document
//P key: string: nil: name of the property to get
//R any: value of the property or nil if not found
.method("getProperty", &DocGetProperty)
//M Sets a property for this document
//P key: string: nil: name of the property to set
//P value: any: nil: value to set
.method("setProperty", &DocSetProperty)
//A table: all properties of the document
.property("properties", &DocGetAllProperties, &DocSetAllProperties)
//A string: File name from and into which the document is loaded and saved on disk
.property("filename", &AbstractDocument::GetFilename, &AbstractDocument::SetFilename)
//A string: Title of the window or tab for this document
.property("title", &AbstractDocument::GetUserReadableName, &AbstractDocument::SetTitle)

.pop();
lua_getglobal(L, "Document");
return 1;
}

