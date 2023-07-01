#include<optional>
#include<variant>
#include "base.hpp"
#include "../../common/ObjectClientData.hpp"
#include "../../app/App.hpp"
#include "../../app/AbstractDocument.hpp"

LuaRegisterReferenceType(wxMenu);
LuaRegisterReferenceType(wxMenuItem);
LuaRegisterReferenceType(wxMenuBar);
LuaRegisterTypeAlias(wxMenuBarBase, wxMenuBar);
LuaRegisterTypeAlias(wxMenuBase, wxMenu);
LuaRegisterTypeAlias(wxMenuItemBase, wxMenuItem);

struct LuaEventHandler* BindLuaHandler (lua_State* L, int idx, wxEvtHandler& obj, wxEventType type, int id1, int id2);
wxWindow* GetCurDocWindow ();

static int MenuItemBind (lua_State* L) {
auto& item = *lua_get<wxMenuItem*>(L, 1);
auto& app = wxGetApp();
auto eptr = BindLuaHandler(L, 2, *item.GetMenu(), wxEVT_MENU, item.GetId(), wxID_ANY);
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

static wxString MenuGetName (wxMenu& menu) {
auto cd = static_cast<StringClientData*>( menu.GetClientObject() );
return cd? cd->value : wxString(wxEmptyString);
}

static void MenuSetName (wxMenu& menu, const wxString& name) {
menu.SetClientObject(new StringClientData(name));
}

static inline size_t TranslatePosition (wxMenu& m, int pos) {
if (pos>=1) return static_cast<size_t>(pos -1);
else return static_cast<size_t>( m.GetMenuItemCount() +pos);
}

static inline size_t TranslatePosition (wxMenuBar& m, int pos) {
if (pos>=1) return static_cast<size_t>(pos -1);
else return static_cast<size_t>( m.GetMenuCount() +pos);
}

static wxMenuItem* MenuAdd (wxMenu& menu, const wxString& label, wxMenu* subMenu, const wxString& help, int pos, int id, bool checkable, bool radio, bool checked) {
if (id<=0) id=wxID_ANY;
wxItemKind kind = radio? wxITEM_RADIO : (checkable? wxITEM_CHECK : wxITEM_NORMAL);
wxMenuItem* item = subMenu? 
menu.Insert(TranslatePosition(menu, pos), id, label, subMenu, help):
menu.Insert(TranslatePosition(menu, pos), id, label, help, kind);
if (checked) item->Check();
lua_getglobal(wxGetApp() .GetLuaState(), "MenuItem");
return item;
}

static void MenuAddSeparator (wxMenu& menu, int pos) {
menu.InsertSeparator(TranslatePosition(menu, pos));
}

static void MenuRemove (wxMenu& menu, std::variant<wxMenuItem*, int> value) {
struct Visitor {
wxMenu& m;
Visitor (wxMenu& m1): m(m1) {}
void operator() (wxMenuItem* it) { m.Remove(it); }
void operator() (int id) { 
if (id<=(int)m.GetMenuItemCount()) m.Remove(m.FindItemByPosition(TranslatePosition(m, id)));
else m.Remove(id);
}};
std::visit(Visitor(menu), value);
}

static void MenuBarAdd (wxMenuBar& menubar, const wxString& label, wxMenu* menu, int pos) {
menubar.Insert(TranslatePosition(menubar, pos), menu, label);
}

static wxMenu* MenuBarGetMenu (wxMenuBar& menubar, std::variant<int, wxString> value) {
struct Visitor {
wxMenuBar& menubar;
Visitor (wxMenuBar& menubar): menubar(menubar) {}
wxMenu* operator() (int pos) {
return menubar.GetMenu(TranslatePosition(menubar, pos));
}
wxMenu* operator() (const wxString& name) {
for (int i=0, n=menubar.GetMenuCount(); i<n; i++) {
auto menu = menubar.GetMenu(i);
wxString nm = MenuGetName(*menu);
if (name==nm) return menu;
}
return nullptr;
}};
lua_getglobal(wxGetApp() .GetLuaState(), "Menu");
return std::visit(Visitor(menubar), value);
}

static wxMenuItem* MenuGetMenuItem (wxMenu& menu, int pos) {
lua_getglobal(wxGetApp() .GetLuaState(), "MenuItem");
return menu.FindItemByPosition(TranslatePosition(menu, pos));
}

static void MenuItemCheck (wxMenuItem& item, std::optional<bool> check) {
item.Check(check.value_or(true));
}

static void MenuItemEnable (wxMenuItem& item, std::optional<bool> enable) {
item.Enable(enable.value_or(true));
}

static void MenuShowPopup (wxMenu* menu) {
auto win = GetCurDocWindow();
win->PopupMenu(menu);
}

static int showPopupMenu (const wxArrayString& options) {
wxMenu menu;
int id = 0;
for (auto& label: options) menu.Append(++id, label);
auto win = GetCurDocWindow();
return win->GetPopupMenuSelectionFromUser(menu);
}

export int luaopen_MenuItem (lua_State* L) {
lua_getglobal(L, "Event");
lua_getglobal(L, "Menu");
lua_pop(L,2);
lua_pushglobaltable(L);

//T Type representing a single menu item
Binding::LuaClass<wxMenuItem>(L, "MenuItem")
.constructor< wxMenu*, int, const wxString&, const wxString&, wxItemKind, wxMenu* >({ "parent", "id", "label", "help", "kind", "subMenu" })
.referenceEquals()
//M Checks or unchecks the checkbox of this item
//P checked: boolean: true: The state checked or unchecked to apply
.method("check", &MenuItemCheck)
//M Enable or disable this item
//P enabled: boolean: true: the enabled or disabled state to apply
.method("enable", &MenuItemEnable)
//A Menu: the submenu included in this item
.property("subMenu", &wxMenuItem::GetSubMenu, &wxMenuItem::SetSubMenu)
//A Menu: the parent menu of this item
.property("menu", &wxMenuItem::GetMenu, &wxMenuItem::SetMenu)
//A string: the help string associated with this item. The help string is displayed in the status bar while the item is highlighted.
.property("helpString", &wxMenuItem::GetHelp, &wxMenuItem::SetHelp)
//A string: the text label of this item
.property("label", &wxMenuItem::GetItemLabel, &wxMenuItem::SetItemLabel)
//G string: the text label of this item, without including accelerators
.getter("labelText", &wxMenuItem::GetItemLabelText)
//A boolean: Tells if this item is currently checked
.boolProperty("checked", &wxMenuItem::IsChecked, &MenuItemCheck)
//G boolean: Tells if this item is checkable, i.e. if it has a checkbox
.getter("checkable", &wxMenuItem::IsCheckable)
//G boolean: Tells if this item is checkable and if it's a radio button
.getter("radio", &wxMenuItem::IsRadio)
//A boolean:  Tells if this item is currently enabled
.boolProperty("enabled", &wxMenuItem::IsEnabled, &MenuItemEnable)
//G boolean: Tells if this item is a separator
.getter("separator", &wxMenuItem::IsSeparator)
//G integer: unique identifier of this item
.getter("id", &wxMenuItem::GetId)
//M Bind an event handler to this item
//P handler: function: nil: the event handler to bind to this item
//R handler: an object allowing to unbind the event handler later on
.method("bind", &MenuItemBind)
.pop();

lua_pop(L, 1);
lua_getglobal(L, "MenuItem");
return 1;
}

export int luaopen_Menu (lua_State* L) {
lua_getglobal(L, "Event");
lua_pop(L,1);
lua_pushglobaltable(L);

//T Type representing a menu, whether a menu in the menubar, or a popup menu
Binding::LuaClass<wxMenu>(L, "Menu")
//C Constructor without arguments
.constructor<>()
.referenceEquals()
//M Add a new menu item to this menu
//P label: string: '': text label of the item
//P subMenu: Menu: nil: a submenu to attach to this item
//P helpString: string: '': the help string displayed on the status bar when the item is highlighted
//P position: integer: 0: position of the item inside the menu (1-based)
//P id: integer: 0: unique identifier of the item (0 = automatically allocate any ID)
//P checkable: boolean: false: tells if the menu item is checkable, i.e. has a checkbox that can be checked
//P radio: boolean: false: Tells if this menu item is a radio button item
//P checked: boolean: false: Tells if this menu item is checked
//R MenuItem: the item created and added in this menu
.method("add", &MenuAdd, { "label", "subMenu", "helpString", "position", "id", "checkable", "radio", "checked" })
//M add a separator to this menu
//P position: integer: 0: position of the item in the menu (1-based)
//R MenuItem: item created and added in this menu
.method("addSeparator", &MenuAddSeparator)
//M Remove an item from this menu
//P itemOrId: integer | MenuItem: nil: the item to remove from this menu, whether a MenuItem object, or an integer denothing the 1-based position or the unique identifier of the item
.method("remove", &MenuRemove)
//M Check a menu item by its ID
//P id: integer: nil: ID of the item to check or uncheck
//P checked: boolean: true: state checked or unchecked to apply
.method("check", &MenuCheck)
//M Tells if a given menu item is checked
//P id: integer: nil: Id of the menu item to check
//R boolean: true if the given menu item exists and is checked
.method("isChecked", &wxMenu::IsChecked)
//M Enable a menu item by its ID
//P id: integer: nil: ID of the item to enable or disable
//P enabled: boolean: true: state enabled or disabled to apply
.method("enable", &MenuEnable)
//M Tells if a menu item given by its ID is enabled
//P id: integer: nil: ID of the menu item to check
//R boolean: true if the given item exists and is enabled
.method("isEnabled", &wxMenu::IsEnabled)
.property("helpString", &wxMenu::GetHelpString, &wxMenu::SetHelpString)
.property("label", &wxMenu::GetLabel, &wxMenu::SetLabel)
.getter("labelText", &wxMenu::GetLabelText)
//M Return the number of items present in this menu
//R integer: the number of items present in this menu
.method("getMenuItemCount", &wxMenu::GetMenuItemCount)
//M Return the number of items present in this menu
//R integer: the number of items present in this menu
.method("__len", &wxMenu::GetMenuItemCount)
//M 1-based index access to items of this menu
//P index: integer: nil: 1-based index
//R MenuItem: corresponding item
.method("__altindex", &MenuGetMenuItem)
//G Menu: parent menu of this menu
.getter("parent", &wxMenu::GetParent)
//A string: internal name of this menu
.property("name", &MenuGetName, &MenuSetName)
//M Displays this menu as a popup/context menu
.method("show", &MenuShowPopup)
//F Show a popup/context menu with simple text options to choose from
//P options: table: nil: a table of options to choose from
//R integer: the 1-based index of the options selected by the user, 0 or -1 if the menu has been closed without choosing any option
.method("chooseOne", &showPopupMenu)
.pop();

lua_pop(L, 1);
lua_getglobal(L, "Menu");
return 1;
}

export int luaopen_MenuBar (lua_State* L) {
lua_getglobal(L, "Event");
lua_getglobal(L, "Menu");
lua_pop(L,2);
lua_pushglobaltable(L);

//T Type representing the menu bar of a document window or tab
Binding::LuaClass<wxMenuBar>(L, "MenuBar")
.referenceEquals()
//M Add a new menu in this menu bar
//P label: string: '': the text label of the menu, displayed in the menu bar
//P menu: Menu: nil: the menu to add
//P position: integer: 0: the 1-based position where to add the new menu
.method("add", &MenuBarAdd, {"label", "menu", "position"})
//M Remove a menu from this menu bar
//P index: integer: nil: 1-based index of the menu to remove
.method("remove", &wxMenuBar::Remove)
.method("replace", &wxMenuBar::Replace)
//M Check or uncheck a menu item by its ID
//P id: integer: nil: menu item ID
//P checked: boolean: true: state checked or unchecked to apply
.method("check", &MenuBarCheck)
//M Checks the checked state of a menu item by its ID
//P id: integer: nil: ID of the menu item
//R boolean: checked state
.method("isChecked", &wxMenuBar::IsChecked)
//M Enable or disable a menu item by its ID
//P id: integer: nil: menu item ID
//P enable: boolean: true: enabled or disabled state to apply
.method("enable", &MenuBarEnable)
//M Checks if a menu item is enabled or disabled by its ID
//P id: integer: nil: menu item ID
//R boolean: enabled or disabled state
.method("isEnabled", (bool(wxMenuBar::*)(int)const) &wxMenuBar::IsEnabled)
.method("enableTop", &wxMenuBar::EnableTop)
.method("isEnabledTop", &wxMenuBar::IsEnabledTop)
.property("helpString", &wxMenuBar::GetHelpString, &wxMenuBar::SetHelpString)
//M fetch a menu from this menu bar
//P indexOrName: integer | string: nil: 1-based index or name of the menu to fetch
//R Menu: the requested menu 
.method("getMenu", &MenuBarGetMenu)
//M Return the number of menus present in this menu bar
//R integer: the number of menus in this menu bar
.method("getMenuCount", &wxMenuBar::GetMenuCount)
//M Return the number of menus present in this menu bar
//R integer: the number of menus in this menu bar
.method("__len", &wxMenuBar::GetMenuCount)
//M 1-based index or by name access to menus of the menu bar
//P indexOrName: integer|string: menu name or 1-based index of the menu to get
//R Menu: requested menu
.method("__altindex", &MenuBarGetMenu)
.property("menuLabel", &wxMenuBar::GetMenuLabel, &wxMenuBar::SetMenuLabel)
.method("getMenuLabelText", &wxMenuBar::GetMenuLabelText)
.pop();

lua_pop(L, 1);
lua_getglobal(L, "MenuBar");
return 1;
}

