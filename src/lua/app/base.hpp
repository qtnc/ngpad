#ifndef ____LUA_IMPL_0_HPP
#define ____LUA_IMPL_0_HPP
extern "C" {
#include "lua/lua.h"
#include "lua/lualib.h"
#include "lua/lauxlib.h"
}
#include<optional>
#include "../binding/luabinding.hpp"
#include "../../common/wxUtils.hpp"
#include "../../app/App.hpp"

#pragma GCC diagnostic ignored "-Wcast-function-type"
#pragma GCC diagnostic ignored "-Wextra"

#ifdef __WIN32
#define export extern "C" __declspec(dllexport)
#else
#define export extern "C" 
#endif

extern thread_local lua_State* currentLuaState;

#define GetLuaCS(L) **reinterpret_cast<wxCriticalSection**>( lua_getextraspace(L))
#define LockLuaCS(L) currentLuaState=L; wxCriticalSectionLocker csl##__LINE__ (GetLuaCS(L))

template <auto Method>
struct SyncWrapper;

template <class R, class... Args, R (*Method)(Args...)>
struct SyncWrapper<Method> {
    static R wrapper(Args... args) {
        if constexpr (std::is_void_v<R>) {
            RunEDTSync(GetLuaCS(currentLuaState), [&]() -> void {
                Method(std::forward<Args>(args)...);
            });
        } else {
            return RunEDTSync(GetLuaCS(currentLuaState), [&]() -> R {
                return Method(std::forward<Args>(args)...);
            });
        }
    }
};


template <class C, class R, class... Args, R (C::*Method)(Args...)>
struct SyncWrapper<Method> {
    static R wrapper(C& obj, Args... args) {
        if constexpr (std::is_void_v<R>) {
            RunEDTSync(GetLuaCS(currentLuaState), [&]() -> void {
                (obj.*Method)(std::forward<Args>(args)...);
            });
        } else {
            return RunEDTSync(GetLuaCS(currentLuaState), [&]() -> R {
                return (obj.*Method)(std::forward<Args>(args)...);
            });
        }
    }
};

template <class C, class R, class... Args, R (C::*Method)(Args...) const>
struct SyncWrapper<Method> {
    static R wrapper(const C& obj, Args... args) {
        if constexpr (std::is_void_v<R>) {
            RunEDTSync(GetLuaCS(currentLuaState), [&]() -> void {
                (obj.*Method)(std::forward<Args>(args)...);
            });
        } else {
            return RunEDTSync(GetLuaCS(currentLuaState), [&]() -> R {
                return (obj.*Method)(std::forward<Args>(args)...);
            });
        }
    }
};

#define SYNC(F) &SyncWrapper<F>::wrapper

struct LuaEventHandler* BindLuaHandler (lua_State* L, int idx, wxEvtHandler& obj, wxEventType type, int id1=wxID_ANY, int id2=wxID_ANY);

inline int lua_pushwxstring (lua_State* L, const wxString& str) {  return lua_push(L, U(str)); }
inline wxString lua_towxstring (lua_State* L, int idx) { return U(lua_get<std::string>(L, idx)); }

int lua_iswxpoint (lua_State* L, int idx);
wxPoint lua_towxpoint (lua_State* L, int idx);
int lua_pushwxpoint  (lua_State* L, const wxPoint& pt);
int lua_iswxsize (lua_State* L, int idx);
wxSize lua_towxsize (lua_State* L, int idx);
int lua_pushwxsize  (lua_State* L, const wxSize& pt);
wxArrayString lua_towxarstr (lua_State* L, int idx);
int lua_pushwxarstr  (lua_State* L, const wxArrayString& ar);

void tableToProperties (lua_State* L, int idx, struct Properties& props);
void propertiesToTable (lua_State* L, int idx, struct Properties& props);

LuaRegisterSlotAccessors(wxString, lua_isstring, lua_towxstring, lua_pushwxstring);
LuaRegisterSlotAccessors(wxPoint, lua_iswxpoint, lua_towxpoint, lua_pushwxpoint);
LuaRegisterSlotAccessors(wxSize, lua_iswxsize, lua_towxsize, lua_pushwxsize);
LuaRegisterSlotAccessors(wxArrayString, lua_istable, lua_towxarstr, lua_pushwxarstr);

LuaRegisterReferenceType(wxEvent);
LuaRegisterReferenceType(wxEvtHandler);

#endif
