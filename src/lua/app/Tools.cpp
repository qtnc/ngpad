#include<optional>
#include<variant>
#include "base.hpp"
#include "../../common/ObjectClientData.hpp"
#include "../../app/App.hpp"
#include "../../app/AbstractDocument.hpp"

LuaRegisterReferenceType(wxToolBar);
LuaRegisterReferenceType(wxToolBarToolBase);
LuaRegisterTypeAlias(wxToolBarBase, wxToolBar);

struct LuaEventHandler* BindLuaHandler (lua_State* L, int idx, wxEvtHandler& obj, wxEventType type, int id1, int id2);
wxWindow* GetCurDocWindow ();

static wxMenuItem* MenuItemConstructor (wxMenu* parent, int id, const wxString& label, const wxString& help, wxItemKind kind, wxMenu* submenu) {
return new wxMenuItem(parent, id, label, help, kind, submenu);
}

static int ToolBind (lua_State* L) {
auto& item = *lua_get<wxToolBarToolBase*>(L, 1);
auto& app = wxGetApp();
auto eptr = BindLuaHandler(L, 2, *item.GetToolBar(), wxEVT_TOOL, item.GetId(), wxID_ANY);
lua_pushlightuserdata(L, eptr);
return 1;
}

static bool MenuBarEnable (wxMenuBar& mb, std::optional<bool> opt) {
return mb.Enable(opt.value_or(true));
}

static void MenuBarCheck (wxMenuBar& mb, int id, std::optional<bool> check) {
mb.Check(id, check.value_or(true));
}

static void MenuEnable (wxMenu& m, int id, std::optional<bool> opt) {
m.Enable(id, opt.value_or(true));
}

static void MenuCheck (wxMenu& m, int id, std::optional<bool> check) {
m.Check(id, check.value_or(true));
}

static wxString ToolGetName (wxToolBarToolBase& tool) {
auto cd = static_cast<StringWxObject*>( tool.GetClientData() );
return cd? cd->value : wxString(wxEmptyString);
}

static void ToolSetName (wxToolBarToolBase& tool, const wxString& name) {
tool.SetClientData(new StringWxObject(name));
}

static inline size_t TranslatePosition (wxToolBar& t, int pos) {
if (pos>=1) return static_cast<size_t>(pos -1);
else return static_cast<size_t>( t.GetToolsCount() +pos);
}

static wxToolBarToolBase* ToolBarGetTool  (wxToolBar& toolbar, std::variant<int, wxString> value) {
struct Visitor {
wxToolBar& tb;
Visitor (wxToolBar& t): tb(t) {}
wxToolBarToolBase* operator() (int pos) {
if (pos<=(int)tb.GetToolsCount()) return tb.GetToolByPos(TranslatePosition(tb, pos));
else return tb.FindById(pos);
}
wxToolBarToolBase* operator() (const wxString& name) {
for (int i=0, n=tb.GetToolsCount(); i<n; i++) {
auto tool = tb.GetToolByPos(i);
wxString nm = ToolGetName(*tool);
if (name==nm) return tool;
}
return nullptr;
}};
lua_getglobal(wxGetApp() .GetLuaState(), "Tool");
return std::visit(Visitor(toolbar), value);
}

static wxToolBarToolBase* ToolBarAdd (wxToolBar& tb, const wxString& label, const wxString& sIcon, const wxString& sDisabledIcon, const wxString& shortHelp, const wxString& longHelp, const wxString& name, int pos, int id, bool checkable, bool radio, bool checked) {
if (id<=0) id=wxID_ANY;
wxItemKind kind = radio? wxITEM_RADIO : (checkable? wxITEM_CHECK : wxITEM_NORMAL);
wxObject* objName = name.empty()? nullptr : new StringWxObject(name);
wxBitmapBundle icon = sIcon.empty()? wxNullBitmap : wxBitmapBundle::FromFiles(sIcon);
wxBitmapBundle disabledIcon = sDisabledIcon.empty()? wxNullBitmap : wxBitmapBundle::FromFiles(sDisabledIcon);
auto item = tb.InsertTool(TranslatePosition(tb, pos), id, label, icon, disabledIcon, kind, shortHelp, longHelp, objName);
if (checked) item->Toggle(checked);
tb.Realize();
lua_getglobal(wxGetApp() .GetLuaState(), "Tool");
return item;
}

static void ToolBarAddSeparator (wxToolBar& tb, int pos) {
tb.InsertSeparator(TranslatePosition(tb, pos));
}

static void ToolBarAddStretchableSeparator (wxToolBar& tb, int pos) {
tb.InsertStretchableSpace(TranslatePosition(tb, pos));
}

static void ToolBarDeleteTool (wxToolBar& tb, std::variant<wxToolBarToolBase*, int> value) {
struct Visitor {
wxToolBar& t;
Visitor (wxToolBar& t1): t(t1) {}
void operator() (wxToolBarToolBase* it) { t.DeleteTool(it->GetId()); }
void operator() (int id) { 
if (id<=(int)t.GetToolsCount()) t.DeleteToolByPos(TranslatePosition(t, id));
else t.DeleteTool(id);
}};
std::visit(Visitor(tb), value);
tb.Realize();
}

static wxToolBar* ToolGetToolBar (wxToolBarToolBase& tool) {
return static_cast<wxToolBar*>(tool.GetToolBar());
}

static bool ToolBarIsEnabled (wxToolBar& tb, int i) {
auto it = ToolBarGetTool(tb, i);
return it && it->IsEnabled();
}

static void ToolCheck (wxToolBarToolBase& item, std::optional<bool> check) {
item.Toggle(check.value_or(true));
}

static void ToolEnable (wxToolBarToolBase& item, std::optional<bool> enable) {
item.Enable(enable.value_or(true));
}

export int luaopen_Tool (lua_State* L) {
lua_getglobal(L, "Event");
lua_pop(L,1);
lua_pushglobaltable(L);

//T Type representing a tool
Binding::LuaClass<wxToolBarToolBase>(L, "Tool")
//.constructor(SYNC(&MenuItemConstructor), { "parent", "id", "label", "help", "kind", "subMenu" })
.referenceEquals()

//M Checks or unchecks the checkbox of this item
//P checked: boolean: true: The state checked or unchecked to apply
.method("check", SYNC(&ToolCheck))
//M Enable or disable this item
//P enabled: boolean: true: the enabled or disabled state to apply
.method("enable", SYNC(&ToolEnable))
//A ToolBar: the parent toolbar of this item
.getter("toolbar", SYNC(&ToolGetToolBar))
//A string: the help string associated with this item. The help string is displayed as a tooltip when the mouse is over the tool
.property("shortHelpString", SYNC(&wxToolBarToolBase::GetShortHelp), SYNC(&wxToolBarToolBase::SetShortHelp))
//A string: the help string associated with this item. The help string is displayed in the status bar while the item is highlighted.
.property("longHelpString", SYNC(&wxToolBarToolBase::GetLongHelp), SYNC(&wxToolBarToolBase::SetLongHelp))
//A string: the text label of this item
.property("label", SYNC(&wxToolBarToolBase::GetLabel), SYNC(&wxToolBarToolBase::SetLabel))
//A boolean: Tells if this item is currently checked
.boolProperty("checked", SYNC(&wxToolBarToolBase::IsToggled), SYNC(&ToolCheck))
//G boolean: Tells if this item is checkable, i.e. if it has a checkbox
.getter("checkable", SYNC(&wxToolBarToolBase::CanBeToggled))
//G boolean: Tells if this item is checkable and if it's a radio button
//.getter("radio", SYNC(&wxMenuItem::IsRadio))
//A boolean:  Tells if this item is currently enabled
.boolProperty("enabled", SYNC(&wxToolBarToolBase::IsEnabled), SYNC(&ToolEnable))
//G boolean: Tells if this item is a separator
.getter("separator", SYNC(&wxToolBarToolBase::IsSeparator))
//G integer: unique identifier of this item
.getter("id", SYNC(&wxToolBarToolBase::GetId))
//M Bind an event handler to this item
//P handler: function: nil: the event handler to bind to this item
//R handler: an object allowing to unbind the event handler later on
.method("bind", SYNC(&ToolBind))
.pop();

lua_pop(L, 1);
lua_getglobal(L, "Tool");
return 1;
}

export int luaopen_ToolBar (lua_State* L) {
lua_getglobal(L, "Event");
lua_pop(L,1);
lua_pushglobaltable(L);

//T Type representing the tool bar of a document window or tab
Binding::LuaClass<wxToolBar>(L, "ToolBar")
.referenceEquals()

//M Remove a menu from this tool bar
//P index: integer: nil: ID or 1-based index of the tool to remove
.method("remove", SYNC(&ToolBarDeleteTool))
//M Check or uncheck a menu item by its ID
//P id: integer: nil: menu item ID
//P checked: boolean: true: state checked or unchecked to apply
//.method("check", SYNC(&MenuBarCheck))
//M Checks the checked state of a menu item by its ID
//P id: integer: nil: ID of the menu item
//R boolean: checked state
//.method("isChecked", SYNC(&wxMenuBar::IsChecked))
//M Enable or disable a menu item by its ID
//P id: integer: nil: menu item ID
//P enable: boolean: true: enabled or disabled state to apply
//.method("enable", SYNC(&MenuBarEnable))
//M Checks if a menu item is enabled or disabled by its ID
//P id: integer: nil: menu item ID
//R boolean: enabled or disabled state
//.method("isEnabled", SYNC(&MenuBarIsEnabled))
//M fetch a tool from this tool bar
//P indexOrID: integer: nil: ID or 1-based index of the tool to fetch
//R Tool: the requested tool 
.method("getTool", SYNC(&ToolBarGetTool))
//M Return the number of menus present in this tool bar
//R integer: the number of menus in this tool bar
.method("getToolCount", SYNC(&wxToolBar::GetToolsCount))
//M Return the number of tools present in this tool bar
//R integer: the number of tools in this tool bar
.method("__len", SYNC(&wxToolBar::GetToolsCount))
//M 1-based index or by ID access to menus of the tool bar
//P indexOrID: integer: 1-based index or ID of the tool to get
//R Tool: requested tool
.method("__altindex", SYNC(&ToolBarGetTool))
//M Add a new tool to this tool bar
//P label: string: '': text label of the item
//P icon: string: '': icon file
//P disabledIcon: string: '': icon file for disabled state
//P shortHelpString: string: '': the help string displayed in a tooltip when the item is overed
//P longHelpString: string: '': the help string displayed on the status bar when the item is highlighted
//P name: string: '': internal tool name
//P position: integer: 0: position of the item inside the toolbar (1-based)
//P id: integer: 0: unique identifier of the item (0 = automatically allocate any ID)
//P checkable: boolean: false: tells if the menu item is checkable, i.e. has a checkbox that can be checked
//P radio: boolean: false: Tells if this menu item is a radio button item
//P checked: boolean: false: Tells if this menu item is checked
//R Tool: the item created and added in this tool bar
.method("add", SYNC(&ToolBarAdd), { "label", "icon", "disabledIcon", "shortHelpString", "longHelpString", "name", "position", "id", "checkable", "radio", "checked" })
//M add a separator to this tool bar
//P position: integer: 0: position of the item in the tool bar (1-based)
//R Tool: item created and added in this tool bar
.method("addSeparator", SYNC(&ToolBarAddSeparator))
//M add a stretchable separator to this tool bar
//P position: integer: 0: position of the item in the tool bar (1-based)
//R Tool: item created and added in this tool bar
.method("addStretchableSeparator", SYNC(&ToolBarAddStretchableSeparator))
//A string: internal name of this menu
//.property("name", SYNC(&MenuGetName), SYNC(&MenuSetName))
.pop();

lua_pop(L, 1);
lua_getglobal(L, "ToolBar");
return 1;
}

