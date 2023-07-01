#include<optional>
#include "base.hpp"
#include "../../app/App.hpp"
#include "../../app/AbstractDocument.hpp"
#include<unordered_map>

LuaRegisterReferenceType(AbstractDocument);
LuaRegisterReferenceType(wxCommandEvent)
LuaRegisterReferenceType(wxKeyEvent)
LuaRegisterReferenceType(wxMouseEvent)
LuaRegisterTypeAlias(wxKeyboardState, wxKeyEvent);
LuaRegisterTypeAlias(wxMouseState, wxMouseEvent);

struct LuaEventHandler: wxObject {
lua_State* L;
int ref;
wxEventType type;
int id1, id2;

LuaEventHandler (lua_State* L0, int ref0, wxEventType type0, int id01, int id02): L(L0), ref(ref0), type(type0), id1(id01), id2(id02)  {}
virtual ~LuaEventHandler () {
luaL_unref(L, LUA_REGISTRYINDEX, ref);
}
bool operator() (wxEvent& e) {
lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
lua_push(L, e);
if (LUA_OK != luaL_call(L, 1, 1)) throw std::runtime_error(lua_tostring(L, -1));
bool re = !lua_isboolean(L, -1) || lua_toboolean(L, -1);
lua_pop(L, 1);
return re;
}
};

void App::HandleLuaBoundEvent (wxEvent& e) {
auto& handler = *static_cast<LuaEventHandler*>(e.GetEventUserData());
if (handler(e)) e.Skip();
}

int EvtHandlerBind (lua_State* L) {
auto& obj = lua_get<wxEvtHandler&>(L, 1);
wxEventType type = wxEVT_NULL;
int id1=wxID_ANY, id2=wxID_ANY;
int idx = 2;
if (lua_isinteger(L, idx)) type = lua_get<wxEventType>(L, idx++);
if (lua_isinteger(L, idx)) id1 = lua_tointeger(L, idx++);
if (lua_isinteger(L, idx)) id2 = lua_tointeger(L, idx++);
auto eptr = BindLuaHandler(L, -1, obj, type, id1, id2);
lua_pushlightuserdata(L, eptr);
return 1;
}

int EvtHandlerUnbind (lua_State* L) {
auto& obj = lua_get<wxEvtHandler&>(L, 1);
auto eptr = static_cast<LuaEventHandler*>( lua_touserdata(L, 2));
lua_pushboolean(L, obj.Disconnect(eptr->id1, eptr->id2, eptr->type, (wxObjectEventFunction)&App::HandleLuaBoundEvent, eptr, &wxGetApp()));
return 1;
}

LuaEventHandler* BindLuaHandler (lua_State* L, int idx, wxEvtHandler& obj, wxEventType type, int id1, int id2) {
lua_pushvalue(L, idx);
auto func = new LuaEventHandler(L, luaL_ref(L, LUA_REGISTRYINDEX), type, id1, id2);
obj.Connect(id1, id2, type, (wxObjectEventFunction)&App::HandleLuaBoundEvent, func, &wxGetApp());
return func;
}

static wxString KeyEventGetUnicodeKey (wxKeyEvent& e) {
wxChar c = e.GetUnicodeKey();
return c==WXK_NONE? wxString(wxEmptyString) : wxString(c, 1);
}

static int EventGetDoc (lua_State* L) {
auto& e = lua_get<wxEvent&>(L, 1);
auto obj = e.GetEventObject();
auto doc = dynamic_cast<AbstractDocument*>(obj);
if (!doc) doc = wxGetApp().GetCurrentDocument();
lua_getglobal(L, "Document");
lua_push<AbstractDocument*>(L, doc);
return 1;
}

static void EventSkip (wxEvent& e, std::optional<bool> skip) {
e.Skip(skip.value_or(true));
}

static std::pair<int,int> MouseEventGetPosition (wxMouseEvent& e) {
int x, y;
e.GetPosition(&x, &y);
return { x, y };
}

export int luaopen_Event (lua_State* L) {
lua_pushglobaltable(L);

//T Parent class of all events
Binding::LuaClass<wxEvent>(L, "Event")
//G Document document in which the event took place
.getter("document", &EventGetDoc)
//G integer: ID of the element or control where the event took place
.property("id", &wxEvent::GetId, &wxEvent::SetId)
//A integer: type of the event
.property("type", &wxEvent::GetEventType, &wxEvent::SetEventType)
//A integer: timestamp of the event
.property("timestamp", &wxEvent::GetTimestamp, &wxEvent::SetTimestamp)
//M Set the skip state of the event
//P skip: boolean: true: skip state
.method("skip", &EventSkip)
//A boolean: skipped state of the event
.property("skipped", &wxEvent::GetSkipped, &EventSkip)
.pop();

//T Base class of all elements and controls that can react to events
Binding::LuaClass<wxEvtHandler>(L, "EventHandler")
//M Bind an event
//P type: integer: nil: type of event
//P fromId: integer: nil: minimum ID for which the event must be processed. This parameter can be totally omited.
//P toId: integer: nil: maximum ID for which the event must be processed. This parameter can be totally omited.
//P handler: function: event function to call when the event occurrs. The function must always be the last parameter.
//R handler: an object that can be passed to unbind() in order to cancel the event
.method("bind", &EvtHandlerBind)
//M unbind an event previously bount with bind()
//P handler: handler: nil: event handler to unbind
.method("unbind", &EvtHandlerUnbind)
.pop();

lua_pop(L, 1);
lua_getglobal(L, "Event");

#define C(N) lua_pushinteger(L, wxEVT_##N);; lua_setfield(L, -2, #N);
//K integer: type identifier for document creating event, triggered when a document is about to be created
C(DOC_CREATING) 
//K integer: type identifier for document created event, triggered when a document has been created
C(DOC_CREATED)
//K integer: type identifier for document loading event, triggered when a document is about to be laoded from a file
C(DOC_LOADING) 
//K integer: type identifier for document loaded event, triggered when a document has been loaded from file
C(DOC_LOADED)
//K integer: type identifier for document saving event, triggered when a document is about to be saved to file
C(DOC_SAVING) 
//K integer: type identifier for document saved event, triggered when a document has been saved to file
C(DOC_SAVED)
//K integer: type identifier for document closing event, triggered when a document is about to be closed
C(DOC_CLOSING) 
//K integer: type identifier for document closed event, triggered when a document has been closed
C(DOC_CLOSED)
#undef C

return 1;
}

export int luaopen_CommandEvent (lua_State* L) {
lua_pushglobaltable(L);
//T Event for all command-based simple events
Binding::LuaClass<wxCommandEvent>(L, "CommandEvent")
.parent<wxEvent>()
//A string: additional information of type string. The kind of information depends on the event type.
.property("string", &wxCommandEvent::GetString, (void(wxCommandEvent::*)(const wxString&))&wxCommandEvent::SetString)
//A integer: additional information of type integer. The kind of information depends on the event type.
.property("int", (int(wxCommandEvent::*)()) &wxCommandEvent::GetInt, (void(wxCommandEvent::*)(int)) &wxCommandEvent::SetInt)
//A integer: second additional information of type integer. The kind of information depends on the event type.
.property("extraLong", (long(wxCommandEvent::*)()) &wxCommandEvent::GetExtraLong, (void(wxCommandEvent::*)(long)) &wxCommandEvent::SetExtraLong)
//A integer: the 0-based index of the element or item just selected
.getter("selection", &wxCommandEvent::GetSelection)
//G boolean: tells if the element or item was just checked or unchecked
.getter("checked", &wxCommandEvent::IsChecked)
.pop();

lua_pop(L, 1);
lua_getglobal(L, "CommandEvent");

#define C(N) lua_pushinteger(L, wxEVT_##N);; lua_setfield(L, -2, #N);
//K integer: type identifier for menu item selection event
C(MENU) 
//K integer: event identifier for tool selection event
C(TOOL) 
//K integer: event type identifier for right clicking on a tool event
C(TOOL_RCLICKED) 
//K integer: event type identifier for activating a tool event
C(TOOL_ENTER)
//K integer: event type identifier for calling the context menu (right click, Shift+F10 or application key)
C(CONTEXT_MENU)
//K integer: event type identifier for pressing enter in the edition area
C(TEXT_ENTER)
#undef C

return 1;
}

export int luaopen_KeyEvent (lua_State* L) {
lua_pushglobaltable(L);
//T Event class for all keyboard-related events
Binding::LuaClass<wxKeyEvent>(L, "KeyEvent")
.parent<wxEvent>()
//G string: unicode character corresponding to the key
.getter("unicodeKey", &KeyEventGetUnicodeKey)
//G integer: key code of the key, see VK_XXX constants
.getter("keyCode", &wxKeyEvent::GetKeyCode)
//G integer: modifiers of the key, see MOD_XXX constants
.getter("modifiers", &wxKeyEvent::GetModifiers)
//G boolean: is Alt key down?
.boolGetter("altDown", &wxKeyEvent::AltDown)
//G boolean: is Shift key down?
.boolGetter("shiftDown", &wxKeyEvent::ShiftDown)
//G boolean: is Ctrl key down?
.boolGetter("controlDown", &wxKeyEvent::ControlDown)
//G boolean: is Ctrl key down?
.boolGetter("ctrlDown", &wxKeyEvent::ControlDown)
//G boolean: is command key down?
.boolGetter("cmdDown", &wxKeyEvent::CmdDown)
//G boolean: is Ctrl key down?
.boolGetter("rawControlDown", &wxKeyEvent::RawControlDown)
//G boolean: is meta key down?
.boolGetter("metaDown", &wxKeyEvent::MetaDown)
.pop();

lua_pop(L, 1);
lua_getglobal(L, "KeyEvent");

#define C(N) lua_pushinteger(L, wxEVT_##N);; lua_setfield(L, -2, #N);
//K integer: event type identifier for key down event
C(KEY_DOWN) 
//K integer: event type identifier for key up event
C(KEY_UP)
//K integer: event type identifier for character key press event
C(CHAR) 
//K integer: event type identifier for character key press event
C(CHAR_HOOK)
#undef C

#define C(N) lua_pushinteger(L, WXK_##N);; lua_setfield(L, -2, "VK_" #N);
C(RETURN) C(SPACE) C(ESCAPE) C(TAB) C(BACK)
C(CONTROL) C(ALT) C(SHIFT)
C(LEFT) C(RIGHT) C(UP) C(DOWN)
C(HOME) C(END) C(PAGEUP) C(PAGEDOWN) C(DELETE) C(INSERT)
C(F1) C(F2) C(F3) C(F4) C(F5) C(F6) C(F7) C(F8) C(F9) C(F10) C(F11) C(F12)
#undef C
for (auto c: std::string("ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789")) {
lua_pushlstring(L, &c, 1); 
lua_pushinteger(L, c); 
lua_settable(L, -3);
}

#define C(N) lua_pushinteger(L, wx##N);; lua_setfield(L, -2, #N);
C(MOD_CONTROL) C(MOD_SHIFT) C(MOD_ALT) C(MOD_META)
#undef C

return 1;
}

export int luaopen_MouseEvent (lua_State* L) {
lua_pushglobaltable(L);
//T Class for mouse events
Binding::LuaClass<wxMouseEvent>(L, "MouseEvent")
.parent<wxEvent>()
//G integer: X coordinate of the mouse
.getter("x", &wxMouseEvent::GetX)
//G integer: Y coordinate of the mouse
.getter("y", &wxMouseEvent::GetY)
//M get the position of the mouse
//R integer: X coordinate
//R integer: Y coordinate
.method("getPosition", &MouseEventGetPosition)
//G integer: flags indicating which buttons where down
.getter("button", &wxMouseEvent::GetButton)
//G integer: number of consecutive clicks (1=single click, 2=double click, 3=triple click)
.getter("clickCount", wxMouseEvent::GetClickCount)
//G integer: rotation of the mouse wheel
.getter("wheelRotation", &wxMouseEvent::GetWheelRotation)
//G boolean: is the left button down?
.boolGetter("leftDown", &wxMouseEvent::LeftIsDown)
//G boolean: is the right button down?
.boolGetter("rightDown", &wxMouseEvent::RightIsDown)
//G boolean: is the middle button down?
.boolGetter("middleDown", &wxMouseEvent::MiddleIsDown)
//G integer: modifier keys (see MOD_XXX constants)
.getter("modifiers", &wxKeyEvent::GetModifiers)
//G boolean: is Alt key down?
.boolGetter("altDown", &wxKeyEvent::AltDown)
//G boolean: is Shift key down?
.boolGetter("shiftDown", &wxKeyEvent::ShiftDown)
//G boolean: is Ctrl key down?
.boolGetter("controlDown", &wxKeyEvent::ControlDown)
//G boolean: is Ctrl key down?
.boolGetter("ctrlDown", &wxKeyEvent::ControlDown)
//G boolean: is command key down?
.boolGetter("cmdDown", &wxKeyEvent::CmdDown)
//G boolean: is Ctrl key down?
.boolGetter("rawControlDown", &wxKeyEvent::RawControlDown)
//G boolean: is meta key down?
.boolGetter("metaDown", &wxKeyEvent::MetaDown)
.pop();

lua_pop(L, 1);
lua_getglobal(L, "MouseEvent");

#define C(N) lua_pushinteger(L, wxEVT_##N);; lua_setfield(L, -2, #N);
//K integer: event type identifier for left mouse down event
C(LEFT_DOWN) 
//K integer: event type identifier for left mouse up event
C(LEFT_UP) 
//K integer: event type identifier for left double click event
C(LEFT_DCLICK)
//K integer: event type identifier for right mouse down event
C(RIGHT_DOWN) 
//K integer: event type identifier for right mouse up event
C(RIGHT_UP) 
//K integer: event type identifier for right double click event
C(RIGHT_DCLICK)
//K integer: event type identifier for middle mouse down event
C(MIDDLE_DOWN) 
//K integer: event type identifier for middle mouse up event
C(MIDDLE_UP) 
//K integer: event type identifier for middle double click event
C(MIDDLE_DCLICK)
//K integer: event type identifier for mouse motion event
C(MOTION) 
//K integer: event type identifier for mouse wheel rotation event
C(MOUSEWHEEL)
//K integer: event type identifier for mouse entering event
C(ENTER_WINDOW) 
//K integer: event type identifier for mouse leaving event
C(LEAVE_WINDOW)
#undef C

return 1;
}

