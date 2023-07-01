#include "base.hpp"
#include "../../app/App.hpp"

LuaRegisterReferenceType(wxTimer);

struct LuaEventHandler* BindLuaHandler (lua_State* L, int idx, wxEvtHandler& obj, wxEventType type, int id1, int id2);

static bool TimerStart (wxTimer& timer, int ms, bool oneShot) {
if (ms<=0) ms=-1;
return timer.Start(ms, oneShot);
}

static bool TimerStartOnce (wxTimer& timer, int ms) {
if (ms<=0) ms=-1;
return timer.StartOnce(ms);
}

static int TimerBind (lua_State* L) {
auto& timer = lua_get<wxTimer&>(L, 1);
auto re = BindLuaHandler(L, 2, wxGetApp(), wxEVT_TIMER, timer.GetId(), wxID_ANY);
lua_pushlightuserdata(L, re);
return 1;
}

static wxTimer* TimerCreate () {
return new wxTimer(&wxGetApp(), wxID_ANY);
}


export int luaopen_Timer (lua_State* L) {
lua_pushglobaltable(L);

//T Timer class
Binding::LuaClass<wxTimer>(L, "Timer")
.destructor()
//C Timer constructor
.constructor(&TimerCreate)
//M Bind an event handler to this timer
//P handler: function: nil: event handler to be triggered when the time is out
//R handler: a event handle that can be passed to app.unbind
.method("bind", &TimerBind)
//M Start the timer and run it periodically or one shot. If the timer is already running, it is reset.
//P interval: integer: -1: interval in miliseconds. If 0 or negative, use the previous interval.
//P oneShot: boolean: false: set time timer to be periodic (false) or one shot (true)
//R boolean: true if the timer has started, false if it was unable to start
.method("start", &TimerStart, {"interval", "oneShot"})
//M Start the timer tand run it only once (one shot). If the timer is already running, it is reset.
//P interval: integer: -1: interval in miliseconds. If 0 or negative, use the previous interval.
//R boolean: true if the timer has started, false if it was unable to start
.method("startOnce", &TimerStartOnce, {"interval"})
//M Stops the timer if it's running.
.method("stop", &wxTimer::Stop)
//G integer: ID of the event generated
.getter("id", &wxTimer::GetId)
//G integer: interval of the timer in miliseconds
.getter("interval", &wxTimer::GetInterval)
//G boolean: tells if the timer is currently running (true) or stopped (false).
.boolGetter("running", &wxTimer::IsRunning)
//G boolean: tells if the timer is periodic (false) or one shot (true).
.boolGetter("oneShot", &wxTimer::IsOneShot)
.pop();
lua_getglobal(L, "Timer");
return 1;
}
