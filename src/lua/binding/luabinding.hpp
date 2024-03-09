	#ifndef _____LUA_BINDING_HPP_____
#define _____LUA_BINDING_HPP_____
#include "lua/lua.hpp"
#include<memory>
#include<string>
#include<cstring>
#include<functional>
#include<vector>
#include<unordered_map>
#include<stdexcept>

int luaL_call (lua_State* L, int nArgs, int nRets);
void* luaL_checkutype (lua_State* L, int idx, const char* tname);
void* luaL_testutype (lua_State* L, int idx, const char* tname);

template<class T> inline T* luaL_checkutype (lua_State* L, int idx);
template<class T> inline T* luaL_testutype (lua_State* L, int idx);

namespace Binding {

int luaB_index (lua_State* L);
int luaB_newindex (lua_State* L);

template <class T> struct is_std_function: std::false_type {};
template<class R, class... A> struct is_std_function<std::function<R(A...)>>: std::true_type {};
template<class R, class... A> struct is_std_function<const std::function<R(A...)>&>: std::true_type {};
template<class R, class... A> struct is_std_function<const std::function<R(A...)>>: std::true_type {};

template<class T> struct is_pair: std::false_type {};
template<class A, class B> struct is_pair<std::pair<A,B>>: std::true_type {};
template<class A, class B> struct is_pair<const std::pair<A,B>>: std::true_type {};
template<class A, class B> struct is_pair<const std::pair<A,B>&>: std::true_type {};

template<class T> struct is_tuple: std::false_type {};
template<class... A> struct is_tuple<std::tuple<A...>>: std::true_type {};
template<class... A> struct is_tuple<const std::tuple<A...>&>: std::true_type {};

template<class T> struct is_optional: std::false_type {};

#ifdef __cpp_lib_optional
using std::optional;
template<class T> struct is_optional<optional<T>>: std::true_type {};
#endif

template<class T> struct is_variant: std::false_type {};

#ifdef __cpp_lib_variant
template<class... A> struct is_variant<std::variant<A...>>: std::true_type {};
template<class... A> struct is_variant<const std::variant<A...>>: std::true_type {};
template<class... A> struct is_variant<const std::variant<A...>&>: std::true_type {};
#endif

template <class T> struct is_managed_class {
static constexpr const bool value = std::is_class<T>::value && !is_optional<T>::value && !is_std_function<T>::value && !is_variant<T>::value && !is_pair<T>::value && !is_tuple<T>::value;
};

template<int ...> struct sequence {};
template<int N, int ...S> struct sequence_generator: sequence_generator<N-1, N-1, S...> {};
template<int ...S> struct sequence_generator<0, S...>{ typedef sequence<S...> type; };

template<class T>  struct UserObjectTrait {  };

#define LuaRegisterReferenceTypeUV(T,NUVALUES) \
namespace Binding { \
template<>  struct UserObjectTrait<T> { \
static inline constexpr size_t getMemSize () { return sizeof(T*); } \
static inline constexpr size_t getNUserValues () { return NUVALUES; } \
static inline const char* typeTag () { return typeid(T).name(); } \
static inline const char* typeTag (const T& obj) { return typeid(obj).name(); } \
static inline T*& getPointer (void* ptr) { return *static_cast<T**>(ptr); }  \
static inline void store (void* ptr, T* value) {  getPointer(ptr) = value; }  \
static inline void store (void* ptr, T& value) {  getPointer(ptr) = &value; }  \
template<class... A> static inline void construct (void* ptr, A&&... args) {  getPointer(ptr) = new T(args...); }  \
static inline void destruct (void* ptr) { \
T*& x = getPointer(ptr); \
if (x) { \
delete x; \
x = nullptr; \
} }  \
}; }

#define LuaRegisterValueTypeUV(T,NUVALUES) \
namespace Binding { \
template<>  struct UserObjectTrait<T> { \
static inline constexpr size_t getMemSize () { return sizeof(T); } \
static inline constexpr size_t getNUserValues () { return NUVALUES; } \
static inline const char* typeTag () { return typeid(T).name(); } \
static inline const char* typeTag (const T& obj) { return typeid(obj).name(); } \
static inline T* getPointer (void* ptr) { return static_cast<T*>(ptr); } \
static inline void store (void* ptr, const T& value) { new(ptr) T(value); } \
static inline void store (void* ptr, const T* value) { new(ptr) T(*value); } \
static inline void storeMove (void* ptr, T&& value) { new(ptr) T(value); } \
template<class... A> static inline void construct (void* ptr, A&&... args) { new(ptr) T(args...); } \
static inline void destruct (void* ptr) { typedef  T D; getPointer(ptr)->~D(); } \
}; }

#define LuaRegisterReferenceType(T) LuaRegisterReferenceTypeUV(T,0)

#define LuaRegisterValueType(T)  LuaRegisterValueTypeUV(T,0)

#define LuaRegisterTypeAlias(T,U) \
namespace Binding { \
template<>  struct UserObjectTrait<T>: UserObjectTrait<U> { }; \
}

#define LuaRegisterDynamicTypeAlias(T,U) \
namespace Binding { \
template<>  struct UserObjectTrait<T> { \
static inline constexpr size_t getMemSize () { return UserObjectTrait<U>::getMemSize(); } \
static inline constexpr size_t getNUserValues () { return UserObjectTrait<U>::getNUserValues(); } \
static inline const char* typeTag () { return UserObjectTrait<U>::typeTag(); } \
static inline const char* typeTag (const T& obj) { return typeid(obj).name(); } \
static inline U*& getMutablePointer (void* ptr) { return UserObjectTrait<U>::getPointer(ptr); } \
static inline T* getPointer (void* ptr) { return &dynamic_cast<T&>( *getMutablePointer(ptr)); } \
static inline void store (void* ptr, T* value) {  getMutablePointer(ptr)  = &dynamic_cast<U&>(*value); } \
static inline void store (void* ptr, T& value) {  getMutablePointer(ptr) = &dynamic_cast<U&>(value); } \
template<class... A> static inline void construct (void* ptr, A&&... args) {  getMutablePointer(ptr) = &dynamic_cast<U&>( *new T(args...) ); }  \
static inline void destruct (void* ptr) { \
U*& x = getMutablePointer(ptr); \
if (x) { \
delete x; \
x = nullptr; \
} }  \
}; }

#define LuaRegisterSlotAccessors(T,CHECK,GET,PUSH) \
namespace Binding { \
template<> struct LuaGetSlot<T> { \
typedef T returnType; \
static inline bool check (lua_State* L, int idx) { return CHECK(L, idx); } \
static inline T get (lua_State* L, int idx) {  return GET(L, idx); } \
}; \
template<> struct LuaGetSlot<const T&> { \
typedef T returnType; \
static inline bool check (lua_State* L, int idx) { return CHECK(L, idx); } \
static inline T get (lua_State* L, int idx) {  return GET(L, idx); } \
}; \
template<> struct LuaPushSlot<T> { \
static inline int push (lua_State* L, const T& value) { return PUSH(L, value); } \
}; \
template<> struct LuaPushSlot<const T&> { \
static inline int push (lua_State* L, const T& value) { return PUSH(L, value); } \
}; \
template<> struct LuaPushSlot<T&>  { \
static inline int push (lua_State* L, const T& value) { return PUSH(L, value); } \
}; }

struct LuaFuncRef {
lua_State* L;
int ref;
LuaFuncRef (lua_State* L0, int ref0): L(L0), ref(ref0) {}
~LuaFuncRef () { luaL_unref(L, LUA_REGISTRYINDEX, ref); }
};

template<class T> inline void* luaL_newudata (lua_State* L, const T* obj = nullptr) {
void* ptr = lua_newuserdatauv(L, UserObjectTrait<T>::getMemSize(), UserObjectTrait<T>::getNUserValues());
if (obj) {
if (!luaL_getmetatable(L, UserObjectTrait<T>::typeTag(*obj) )) {
lua_pop(L, 1);
goto basetag;
}}
else basetag: luaL_newmetatable(L, UserObjectTrait<T>::typeTag());
lua_setmetatable(L, -2);
return ptr;
}

template<class T, class B = void> struct LuaGetSlot: std::false_type {};

template <class T> struct LuaGetSlot<T*, typename std::enable_if< std::is_class<T>::value>::type> {
typedef T* returnType;
static inline bool check (lua_State* L, int idx) { return !!luaL_testutype(L, idx, UserObjectTrait<T>::typeTag()); }
static inline T* get (lua_State* L, int idx) {
void* ptr = luaL_checkutype(L, idx, UserObjectTrait<T>::typeTag());
return ptr? UserObjectTrait<T>::getPointer(ptr) :nullptr;
}};

template <class T> struct LuaGetSlot<T&, typename std::enable_if<is_managed_class<T>::value>::type> {
typedef T& returnType;
static inline bool check (lua_State* L, int idx) { return !!luaL_testutype(L, idx, UserObjectTrait<T>::typeTag()); }
static inline T& get (lua_State* L, int idx) {
return *UserObjectTrait<T>::getPointer( luaL_checkutype(L, idx, UserObjectTrait<T>::typeTag()) );
}};

template <class T> struct LuaGetSlot<T, typename std::enable_if< is_managed_class<T>::value>::type> {
typedef T& returnType;
static inline bool check (lua_State* L, int idx) { return !!luaL_testutype(L, idx, UserObjectTrait<T>::typeTag()); }
static inline T& get (lua_State* L, int idx) {
return *UserObjectTrait<T>::getPointer( luaL_checkutype(L, idx, UserObjectTrait<T>::typeTag()) );
}};

template<class T> struct LuaGetSlot<T, typename std::enable_if< std::is_integral<T>::value>::type> {
typedef T returnType;
static inline bool check (lua_State* L, int idx) { return lua_isinteger(L, idx); }
static inline T get (lua_State* L, int idx) { 
return static_cast<T>(luaL_optinteger(L, idx, 0));
}};

template<class T> struct LuaGetSlot<T, typename std::enable_if< std::is_enum<T>::value>::type> {
typedef T returnType;
static inline bool check (lua_State* L, int idx) { return lua_isinteger(L, idx); }
static inline T get (lua_State* L, int idx) { 
return static_cast<T>(luaL_optinteger(L, idx, 0));
}};

template<class T> struct LuaGetSlot<T, typename std::enable_if< std::is_floating_point<T>::value>::type> {
typedef T returnType;
static inline bool check (lua_State* L, int idx) { return lua_isnumber(L, idx); }
static inline T get (lua_State* L, int idx) { 
return static_cast<T>(luaL_optnumber(L, idx, 0.0));
}};

template<> struct LuaGetSlot<bool> {
typedef bool returnType;
static inline bool check (lua_State* L, int idx) { return lua_isboolean(L, idx); }
static inline bool get (lua_State* L, int idx) {  return lua_toboolean(L, idx); }
};

template<> struct LuaGetSlot<std::nullptr_t> {
typedef std::nullptr_t returnType;
static inline bool check (lua_State* L, int idx) { return lua_isnoneornil(L, idx); }
static inline std::nullptr_t get (lua_State* L, int idx) {  return nullptr; }
};

template<> struct LuaGetSlot<std::string> {
typedef std::string returnType;
static inline bool check (lua_State* L, int idx) { return lua_isstring(L, idx); }
static inline std::string get (lua_State* L, int idx) {
std::string s; size_t l;
const char* cs = luaL_optlstring(L, idx, nullptr, &l);
if (cs) {
s.resize(l);
memcpy(const_cast<char*>(s.data()), cs, l);
}
return s;
}};

template<> struct LuaGetSlot<const std::string&> {
typedef std::string returnType;
static inline bool check (lua_State* L, int idx) { return lua_isstring(L, idx); }
static inline std::string get (lua_State* L, int idx) {
std::string s; size_t l;
const char* cs = luaL_optlstring(L, idx, nullptr, &l);
if (cs) {
s.resize(l);
memcpy(const_cast<char*>(s.data()), cs, l);
}
return s;
}};

template<> struct LuaGetSlot<const char*> {
typedef const char* returnType;
static inline bool check (lua_State* L, int idx) { return lua_isstring(L, idx); }
static inline const char* get (lua_State* L, int idx) { 
return luaL_optstring(L, idx, nullptr);
}};

#ifdef __cpp_lib_optional
template<class T> struct LuaGetSlot<optional<T>> {
typedef optional<T> returnType;
static inline bool check (lua_State* L, int idx) { return LuaGetSlot<T>::check(L, idx); }
static inline optional<T> get (lua_State* L, int idx) { 
optional<T> re;
if (lua_gettop(L)>=idx && check(L, idx)) re = LuaGetSlot<T>::get(L, idx);
return re;
}};
#endif

#ifdef __cpp_lib_variant
template<int Z, class V, class... A> struct LuaGetSlotVariant {
};

template<int Z, class V, class T, class... A> struct LuaGetSlotVariant<Z, V, T, A...> {
static inline void getv (lua_State* L, int idx, V& var) {
if (LuaGetSlot<T>::check(L, idx)) var = LuaGetSlot<T>::get(L, idx);
else LuaGetSlotVariant<sizeof...(A), V, A...>::getv(L, idx, var);
}};

template<class V> struct LuaGetSlotVariant<0, V>  {
static inline void getv (lua_State* L, int idx, V& var) { }
};

template<class... A> struct LuaGetSlot<std::variant<A...>> {
typedef std::variant<A...> returnType;
static inline returnType get (lua_State* L, int idx) { 
returnType re;
LuaGetSlotVariant<sizeof...(A), returnType, A...>::getv(L, idx, re);
return re;
}};

template<class... A> struct LuaGetSlot<const std::variant<A...>&> {
typedef std::variant<A...> returnType;
static inline returnType get (lua_State* L, int idx) { 
return LuaGetSlot<returnType>::get(L, idx);
}};
#endif

template<class T, class B = void, int ADDITIONAL_ARG = 1> struct LuaPushSlot: std::false_type {};

template<class T> struct LuaPushSlot<T&, typename std::enable_if< is_managed_class<T>::value>::type> {
static inline int push (lua_State* L, const T& value) { 
auto ptr = luaL_newudata<T>(L, &value);
UserObjectTrait<T>::store(ptr, const_cast<T&>(value));
return 1;
}};

template<class T> struct LuaPushSlot<T*, typename std::enable_if< is_managed_class<T>::value>::type> {
static inline int push (lua_State* L, const T*  value) { 
if (!value) lua_pushnil(L);
else {
auto ptr = luaL_newudata<T>(L, value);
UserObjectTrait<T>::store(ptr, const_cast<T*>(value));
}
return 1;
}};

template<class T> struct LuaPushSlot<T, typename std::enable_if< is_managed_class<T>::value>::type> {
static inline int push (lua_State* L, const T& value) { 
auto ptr = luaL_newudata<T>(L, &value);
UserObjectTrait<T>::store(ptr, const_cast<T&>(value));
return 1;
}};

template<class T> struct LuaPushSlot<T&&, typename std::enable_if< is_managed_class<T>::value>::type> {
static inline int push (lua_State* L, T&& value) { 
auto ptr = luaL_newudata<T>(L, &value);
UserObjectTrait<T>::storeMove(ptr, std::move(value));
return 1;
}};

template<class T> struct LuaPushSlot<T, typename std::enable_if< std::is_integral<T>::value || std::is_enum<T>::value>::type> {
static inline int push (lua_State* L, T value) { 
lua_pushinteger(L, value);
return 1;
}};

template<class T> struct LuaPushSlot<T, typename std::enable_if< std::is_floating_point<T>::value>::type> {
static inline int push (lua_State* L, T value) { 
lua_pushnumber(L, value);
return 1;
}};

template<class T> struct LuaPushSlot<const T&, typename std::enable_if<std::is_integral<T>::value || std::is_enum<T>::value>::type> {
static inline int push (lua_State* L, const T& value) { 
lua_pushinteger(L, value);
return 1;
}};

template<class T> struct LuaPushSlot<T&, typename std::enable_if< (std::is_integral<T>::value || std::is_enum<T>::value) && !std::is_const<T>::value >::type> {
static inline int push (lua_State* L, T& value) { 
lua_pushinteger(L, value);
return 1;
}};

template<class T> struct LuaPushSlot<const T&, typename std::enable_if< std::is_floating_point<T>::value>::type> {
static inline int push (lua_State* L, const T& value) { 
lua_pushnumber(L, value);
return 1;
}};

template<class T> struct LuaPushSlot<T&, typename std::enable_if< std::is_floating_point<T>::value>::type> {
static inline int push (lua_State* L, const T&  value) { 
lua_pushnumber(L, value);
return 1;
}};

template<> struct LuaPushSlot<std::nullptr_t> {
static inline int push (lua_State* L, std::nullptr_t value) { 
lua_pushnil(L);
return 1;
}};

template<> struct LuaPushSlot<bool> {
static inline int push (lua_State* L, bool value) { 
lua_pushboolean(L, value);
return 1;
}};

template<> struct LuaPushSlot<const bool&> {
static inline int push (lua_State* L, const bool& value) { 
lua_pushboolean(L, value);
return 1;
}};

template<> struct LuaPushSlot<bool&> {
static inline int push (lua_State* L, bool& value) { 
lua_pushboolean(L, value);
return 1;
}};

template<> struct LuaPushSlot<const char*> {
static inline int push (lua_State* L, const char* value) { 
lua_pushstring(L, value);
return 1;
}};

template<> struct LuaPushSlot<char*> {
static inline int push (lua_State* L, const char* value) { 
lua_pushstring(L, value);
return 1;
}};

template<> struct LuaPushSlot<const std::string&> {
static inline int push (lua_State* L, const std::string& value) { 
lua_pushlstring(L, value.data(), value.size());
return 1;
}};

template<> struct LuaPushSlot<std::string&> {
static inline int push (lua_State* L, const std::string& value) { 
lua_pushlstring(L, value.data(), value.size());
return 1;
}};

template<> struct LuaPushSlot<std::string> {
static inline int push (lua_State* L, const std::string& value) { 
lua_pushlstring(L, value.data(), value.size());
return 1;
}};

template<> struct LuaPushSlot<lua_CFunction> {
static inline int push (lua_State* L, lua_CFunction value) { 
lua_pushcfunction(L, value);
return 1;
}};

template<class A, class B> struct LuaPushSlot<std::pair<A,B>> {
static inline int push (lua_State* L, const std::pair<A,B>& pair) { 
return LuaPushSlot<A>::push(L, pair.first) + LuaPushSlot<B>::push(L, pair.second);
}};

template<class A, class B> struct LuaPushSlot<const std::pair<A,B>&> {
static inline int push (lua_State* L, const std::pair<A,B>& pair) { 
return LuaPushSlot<A>::push(L, pair.first) + LuaPushSlot<B>::push(L, pair.second);
}};

template<class A, class B> struct LuaPushSlot<std::pair<A,B>&> {
static inline int push (lua_State* L, const std::pair<A,B>& pair) { 
return LuaPushSlot<A>::push(L, pair.first) + LuaPushSlot<B>::push(L, pair.second);
}};

#ifdef __cpp_lib_optional
template<class T> struct LuaPushSlot<std::optional<T>> {
static inline int push (lua_State* L, const std::optional<T>& x) { 
if (!x) return LuaPushSlot<std::nullptr_t>::push(L, nullptr);
else return LuaPushSlot<decltype(*x)>::push(L, *x);
}};

template<class T> struct LuaPushSlot<const std::optional<T>&> {
static inline int push (lua_State* L, const std::optional<T>& x) { 
if (!x) return LuaPushSlot<std::nullptr_t>::push(L, nullptr);
else return LuaPushSlot<decltype(*x)>::push(L, *x);
}};
#endif

#ifdef __cpp_lib_variant
template<class... A> struct LuaPushSlot<std::variant<A...>> {
static inline int push (lua_State* L, const std::variant<A...>& value) { 
return std::visit([&](auto&& x){ return LuaPushSlot<decltype(x)>::push(L, x); }, value);
}};

template<class... A> struct LuaPushSlot<const std::variant<A...>&> {
static inline int push (lua_State* L, const std::variant<A...>& value) { 
return std::visit([&](auto&& x){ return LuaPushSlot<decltype(x)>::push(L, x); }, value);
}};
#endif

static inline int pushMultiple (lua_State* L) { return 0; }

template<class T, class... A> static inline int pushMultiple (lua_State* L, T&& arg, A&&... args) {
return LuaPushSlot<T>::push(L, arg) + pushMultiple(L, args...);
}

template<class... A> struct LuaPushSlot<std::tuple<A...>> {
template<int... S> static int push (sequence<S...> unused, lua_State* L, const std::tuple<A...>& tuple) {  
return pushMultiple(L, std::get<S>(tuple)... );  
}
static inline int push (lua_State* L, const std::tuple<A...>& tuple) { 
typename sequence_generator<sizeof...(A)>::type seq;
return push(seq, L, tuple);
}};

template<class... A> struct LuaPushSlot<const std::tuple<A...>&> {
template<int... S> static int push (sequence<S...> unused, lua_State* L, const std::tuple<A...>& tuple) {  
return pushMultiple(L, std::get<S>(tuple)... );  
}
static inline int push (lua_State* L, const std::tuple<A...>& tuple) { 
typename sequence_generator<sizeof...(A)>::type seq;
return push(seq, L, tuple);
}};

template<class... A> struct LuaPushSlot<std::tuple<A...>&> {
template<int... S> static int push (sequence<S...> unused, lua_State* L, const std::tuple<A...>& tuple) {  
return pushMultiple(L, std::get<S>(tuple)... );  
}
static inline int push (lua_State* L, const std::tuple<A...>& tuple) { 
typename sequence_generator<sizeof...(A)>::type seq;
return push(seq, L, tuple);
}};

template<class R, class... A> struct LuaGetSlot<std::function<R(A...)>> {
typedef std::function<R(A...)> returnType;
static inline returnType get (lua_State* L, int idx) { 
if (lua_isnoneornil(L, idx)) return nullptr;
lua_pushvalue(L, idx);
auto refptr = std::make_shared<LuaFuncRef>( L, luaL_ref(L, LUA_REGISTRYINDEX) );
return [=](A&&... args)->R{
lua_rawgeti(L, LUA_REGISTRYINDEX, refptr->ref);
pushMultiple(L, args...);
if (LUA_OK != luaL_call(L, sizeof...(A), 1)) throw std::runtime_error(lua_tostring(L, -1));
R result = LuaGetSlot<R>::get(L, -1);
lua_pop(L, 1);
return result;
};
}};

template<class... A> struct LuaGetSlot<std::function<void(A...)>> {
typedef std::function<void(A...)> returnType;
static inline returnType get (lua_State* L, int idx) { 
if (lua_isnoneornil(L, idx)) return nullptr;
auto refptr = std::make_shared<LuaFuncRef>( L, luaL_ref(L, LUA_REGISTRYINDEX) );
return [=](A&&... args)->void{
lua_rawgeti(L, LUA_REGISTRYINDEX, refptr->ref);
pushMultiple(L, args...);
if (LUA_OK != luaL_call(L, sizeof...(A), 0)) throw std::runtime_error(lua_tostring(L, -1));
};
}};

template<class R, class... A> struct LuaGetSlot<const std::function<R(A...)>&> {
typedef std::function<R(A...)> returnType;
static inline returnType get (lua_State* L, int idx) { 
return LuaGetSlot<returnType>::get(L, idx);
}};

template<int START, class... A> struct LuaParamExtractor {
template<int N, typename... Ts> using NthTypeOf = typename std::tuple_element<N, std::tuple<Ts...>>::type;
template<int... S> static inline std::tuple<typename LuaGetSlot<A>::returnType...> extract (sequence<S...> unused, lua_State* L) {  
if (lua_istable(L, START) && lua_istable(L, lua_upvalueindex(2))) return std::forward_as_tuple( extractArgFromTable<S, NthTypeOf<S,A...>>(L)... );  
else return std::forward_as_tuple( extractArg<S, NthTypeOf<S,A...>>(L)... );  
}
template<int S, class E> static inline typename LuaGetSlot<E>::returnType  extractArg (lua_State* L) { 
return LuaGetSlot<E>::get(L, S+START); 
}
template<int S, class E> static inline typename LuaGetSlot<E>::returnType  extractArgFromTable (lua_State* L) { 
lua_rawgeti(L, lua_upvalueindex(2), 1+S);
lua_gettable(L, START);
E re = LuaGetSlot<E>::get(L, -1); 
lua_pop(L, 1);
return re;
}
};

template<class F> inline int lua_pushfuncptr (lua_State* L, const F& func) {
void* ptr = lua_newuserdata(L, sizeof(F));
memset(ptr, 0, sizeof(F));
*reinterpret_cast<F*>(ptr) = func;
return 1;
}

template<class T, class P> inline int lua_pushmemptr (lua_State* L, P T::*prop) {
typedef P T::*F;
void* ptr = lua_newuserdata(L, sizeof(F));
*reinterpret_cast<F*>(ptr) = prop;
return 1;
}

template <class T> inline void typeerror (lua_State* L, int idx) {
idx = lua_absindex(L, idx);
luaL_newmetatable(L, UserObjectTrait<T>::typeTag());
lua_getfield(L, -1, "__name");
luaL_typeerror(L, idx, lua_tostring(L, -1));
}

template<int START, class T, class R, class... A> struct LuaPushSlot< R (T::*)(A...), void, START> {
typedef R(T::*Func)(A...);
template<int... S> static R callNative (sequence<S...> unused, T* obj, Func func, const std::tuple<typename LuaGetSlot<A>::returnType...>& params) {  
return (obj->*func)( std::get<S>(params)... );  
}
static int wrapper (lua_State* L) {
typename sequence_generator<sizeof...(A)>::type seq;
const Func& func = *reinterpret_cast<const Func*>(lua_topointer(L, lua_upvalueindex(1)));
T* obj = LuaGetSlot<T*>::get(L, 1);
if (!obj) typeerror<T>(L, 1);
std::tuple<typename LuaGetSlot<A>::returnType...> params = LuaParamExtractor<START+1, A...>::extract(seq, L);
R result = callNative(seq, obj, func, params);
return LuaPushSlot<R>::push(L, result);
}
static int push (lua_State* L, const Func& func) {
lua_pushfuncptr(L, func);
lua_pushcclosure(L, &wrapper, 1);
return 1;
}
};

template<int START, class T, class... A> struct LuaPushSlot< void(T::*)(A...), void, START> {
typedef void(T::*Func)(A...);
template<int... S> static void callNative (sequence<S...> unused, T* obj, Func func, const std::tuple<typename LuaGetSlot<A>::returnType...>& params) { 
 (obj->*func)( std::get<S>(params)... );  
}
static int wrapper (lua_State* L) {
typename sequence_generator<sizeof...(A)>::type seq;
const Func& func = *reinterpret_cast<const Func*>(lua_topointer(L, lua_upvalueindex(1)));
T* obj = LuaGetSlot<T*>::get(L, 1);
if (!obj) typeerror<T>(L, 1);
std::tuple<typename LuaGetSlot<A>::returnType...> params = LuaParamExtractor<START+1, A...>::extract(seq, L);
callNative(seq, obj, func, params);
return 0;
}
static int push (lua_State* L, const Func& func) {
lua_pushfuncptr(L, func);
lua_pushcclosure(L, &wrapper, 1);
return 1;
}
};

template<int START, class T, class R, class... A> struct LuaPushSlot< R (T::*)(A...)const, void, START> {
typedef R(T::*Func)(A...)const;
template<int... S> static R callNative (sequence<S...> unused, T* obj, Func func, const std::tuple<typename LuaGetSlot<A>::returnType...>& params) {  
return (obj->*func)( std::get<S>(params)... );  
}
static int wrapper (lua_State* L) {
typename sequence_generator<sizeof...(A)>::type seq;
const Func& func = *reinterpret_cast<const Func*>(lua_topointer(L, lua_upvalueindex(1)));
T* obj = LuaGetSlot<T*>::get(L, 1);
if (!obj) typeerror<T>(L, 1);
std::tuple<typename LuaGetSlot<A>::returnType...> params = LuaParamExtractor<START+1, A...>::extract(seq, L);
R result = callNative(seq, obj, func, params);
return LuaPushSlot<R>::push(L, result);
}
static int push (lua_State* L, const Func& func) {
lua_pushfuncptr(L, func);
lua_pushcclosure(L, &wrapper, 1);
return 1;
}
};

template<int START, class T, class... A> struct LuaPushSlot< void(T::*)(A...)const, void, START> {
typedef void(T::*Func)(A...)const;
template<int... S> static void callNative (sequence<S...> unused, T* obj, Func func, const std::tuple<typename LuaGetSlot<A>::returnType...>& params) {  
(obj->*func)( std::get<S>(params)... );  
}
static int wrapper (lua_State* L) {
typename sequence_generator<sizeof...(A)>::type seq;
const Func& func = *reinterpret_cast<const Func*>(lua_topointer(L, lua_upvalueindex(1)));
T* obj = LuaGetSlot<T*>::get(L, 1);
if (!obj) typeerror<T>(L, 1);
std::tuple<typename LuaGetSlot<A>::returnType...> params = LuaParamExtractor<START+1, A...>::extract(seq, L);
callNative(seq, obj, func, params);
return 0;
}
static int push (lua_State* L, const Func& func) {
lua_pushfuncptr(L, func);
lua_pushcclosure(L, &wrapper, 1);
return 1;
}
};

template<int START, class R, class... A> struct LuaPushSlot< R(*)(A...), void, START> {
typedef R(*Func)(A...);
template<int... S> static R callNative (sequence<S...> unused, Func func, const std::tuple<typename LuaGetSlot<A>::returnType...>& params) { 
 return func( std::get<S>(params)... );  
}
static int wrapper (lua_State* L) {
typename sequence_generator<sizeof...(A)>::type seq;
std::tuple<typename LuaGetSlot<A>::returnType...> params = LuaParamExtractor<START, A...>::extract(seq, L);
const Func& func = *reinterpret_cast<const Func*>(lua_topointer(L, lua_upvalueindex(1)));
R result = callNative(seq, func, params);
return LuaPushSlot<R>::push(L, result);
}
static int push (lua_State* L, const Func& func) {
lua_pushfuncptr(L, func);
lua_pushcclosure(L, &wrapper, 1);
return 1;
}
};

template<int START, class R, class... A> struct LuaPushSlot< R(*const&)(A...), void, START> {
typedef R(*Func)(A...);
template<int... S> static R callNative (sequence<S...> unused, Func func, const std::tuple<typename LuaGetSlot<A>::returnType...>& params) { 
 return func( std::get<S>(params)... );  
}
static int wrapper (lua_State* L) {
typename sequence_generator<sizeof...(A)>::type seq;
std::tuple<typename LuaGetSlot<A>::returnType...> params = LuaParamExtractor<START, A...>::extract(seq, L);
const Func& func = *reinterpret_cast<const Func*>(lua_topointer(L, lua_upvalueindex(1)));
R result = callNative(seq, func, params);
return LuaPushSlot<R>::push(L, result);
}
static int push (lua_State* L, const Func& func) {
lua_pushfuncptr(L, func);
lua_pushcclosure(L, &wrapper, 1);
return 1;
}
};

template<int START, class... A> struct LuaPushSlot< void(*)(A...), void, START> {
typedef void(*Func)(A...);
template<int... S> static void callNative (sequence<S...> unused, Func func, const std::tuple<typename LuaGetSlot<A>::returnType...>& params) {  
func( std::get<S>(params)... );  
}
static int wrapper (lua_State* L) {
typename sequence_generator<sizeof...(A)>::type seq;
std::tuple<typename LuaGetSlot<A>::returnType...> params = LuaParamExtractor<START, A...>::extract(seq, L);
const Func& func = *reinterpret_cast<const Func*>(lua_topointer(L, lua_upvalueindex(1)));
callNative(seq, func, params);
return 0;
}
static int push (lua_State* L, const Func& func) {
lua_pushfuncptr(L, func);
lua_pushcclosure(L, &wrapper, 1);
return 1;
}
};

template<int START, class... A> struct LuaPushSlot< void(*const&)(A...), void, START> {
typedef void(*Func)(A...);
template<int... S> static void callNative (sequence<S...> unused, Func func, const std::tuple<typename LuaGetSlot<A>::returnType...>& params) {  
func( std::get<S>(params)... );  
}
static int wrapper (lua_State* L) {
typename sequence_generator<sizeof...(A)>::type seq;
std::tuple<typename LuaGetSlot<A>::returnType...> params = LuaParamExtractor<START, A...>::extract(seq, L);
const Func& func = *reinterpret_cast<const Func*>(lua_topointer(L, lua_upvalueindex(1)));
callNative(seq, func, params);
return 0;
}
static int push (lua_State* L, const Func& func) {
lua_pushfuncptr(L, func);
lua_pushcclosure(L, &wrapper, 1);
return 1;
}
};

template<int START, class R, class... A> struct LuaPushSlot< std::function<R(A...)>, void, START> {
typedef std::function<R(A...)> Func;
template<int... S> static R callNative (sequence<S...> unused, const Func& func, const std::tuple<typename LuaGetSlot<A>::returnType...>& params) { 
 return func( std::get<S>(params)... );  
}
static int wrapper (lua_State* L) {
typename sequence_generator<sizeof...(A)>::type seq;
std::tuple<typename LuaGetSlot<A>::returnType...> params = LuaParamExtractor<START, A...>::extract(seq, L);
const Func& func = *reinterpret_cast<const Func*>(lua_topointer(L, lua_upvalueindex(1)));
R result = callNative(seq, func, params);
return LuaPushSlot<R>::push(L, result);
}
static int push (lua_State* L, const Func& func) {
lua_pushfuncptr(L, func);
lua_pushcclosure(L, &wrapper, 1);
return 1;
}
};

template<int START, class R, class... A> struct LuaPushSlot< const std::function<R(A...)>&, void, START> {
typedef std::function<R(A...)> Func;
template<int... S> static R callNative (sequence<S...> unused, Func& func, const std::tuple<typename LuaGetSlot<A>::returnType...>& params) { 
 return func( std::get<S>(params)... );  
}
static int wrapper (lua_State* L) {
typename sequence_generator<sizeof...(A)>::type seq;
std::tuple<typename LuaGetSlot<A>::returnType...> params = LuaParamExtractor<START, A...>::extract(seq, L);
const Func& func = *reinterpret_cast<const Func*>(lua_topointer(L, lua_upvalueindex(1)));
R result = callNative(seq, func, params);
return LuaPushSlot<R>::push(L, result);
}
static int push (lua_State* L, const Func& func) {
lua_pushfuncptr(L, func);
lua_pushcclosure(L, &wrapper, 1);
return 1;
}
};

template<int START, class... A> struct LuaPushSlot< std::function<void(A...)>, void, START> {
typedef std::function<void(A...)> Func;
template<int... S> static void callNative (sequence<S...> unused, Func& func, const std::tuple<typename LuaGetSlot<A>::returnType...>& params) { 
func( std::get<S>(params)... );  
}
static int wrapper (lua_State* L) {
typename sequence_generator<sizeof...(A)>::type seq;
std::tuple<typename LuaGetSlot<A>::returnType...> params = LuaParamExtractor<START, A...>::extract(seq, L);
const Func& func = *reinterpret_cast<const Func*>(lua_topointer(L, lua_upvalueindex(1)));
callNative(seq, func, params);
return 0;
}
static int push (lua_State* L, const Func& func) {
lua_pushfuncptr(L, func);
lua_pushcclosure(L, &wrapper, 1);
return 1;
}
};

template<int START, class... A> struct LuaPushSlot< const std::function<void(A...)>&, void, START> {
typedef std::function<void(A...)> Func;
template<int... S> static void callNative (sequence<S...> unused, Func& func, const std::tuple<typename LuaGetSlot<A>::returnType...>& params) { 
func( std::get<S>(params)... );  
}
static int wrapper (lua_State* L) {
typename sequence_generator<sizeof...(A)>::type seq;
std::tuple<typename LuaGetSlot<A>::returnType...> params = LuaParamExtractor<START, A...>::extract(seq, L);
const Func& func = *reinterpret_cast<const Func*>(lua_topointer(L, lua_upvalueindex(1)));
callNative(seq, func, params);
return 0;
}
static int push (lua_State* L, const Func& func) {
lua_pushfuncptr(L, func);
lua_pushcclosure(L, &wrapper, 1);
return 1;
}
};

#ifdef __WIN32
/*
template<int START, class R, class... A> struct LuaPushSlot< R (__stdcall*)(A...), void, START> {
typedef R(*__stdcall Func)(A...);
template<int... S> static R callNative (sequence<S...> unused, Func func, const std::tuple<typename LuaGetSlot<A>::returnType...>& params) {  
return func( std::get<S>(params)... );  
}
static int wrapper (lua_State* L) {
typename sequence_generator<sizeof...(A)>::type seq;
std::tuple<typename LuaGetSlot<A>::returnType...> params = LuaParamExtractor<START, A...>::extract(seq, L);
const Func& func = *reinterpret_cast<const Func*>(lua_topointer(L, lua_upvalueindex(1)));
R result = callNative(seq, func, params);
return LuaPushSlot<R>::push(L, result);
}
static int push (lua_State* L, const Func& func) {
lua_pushfuncptr(L, func);
lua_pushcclosure(L, &wrapper, 1);
return 1;
}
};

template<int START, class... A> struct LuaPushSlot< void (__stdcall*)(A...), void, START> {
typedef void(*__stdcall Func)(A...);
template<int... S> static void callNative (sequence<S...> unused, Func func, const std::tuple<typename LuaGetSlot<A>::returnType...>& params) {  
func( std::get<S>(params)... );  
}
static int wrapper (lua_State* L) {
typename sequence_generator<sizeof...(A)>::type seq;
std::tuple<typename LuaGetSlot<A>::returnType...> params = LuaParamExtractor<START, A...>::extract(seq, L);
const Func& func = *reinterpret_cast<const Func*>(lua_topointer(L, lua_upvalueindex(1)));
callNative(seq, func, params);
return 0;
}
static int push (lua_State* L, const Func& func) {
lua_pushfuncptr(L, func);
lua_pushcclosure(L, &wrapper, 1);
return 1;
}
};
*/
#endif

template<class T, class... A> struct LuaConstructorWrapper {
template<int... S> static void callConstructor (sequence<S...> unused, void* ptr, const std::tuple<typename LuaGetSlot<A>::returnType...>& params) {  
UserObjectTrait<T>::construct(ptr, std::get<S>(params)... );  
}
static int constructor (lua_State* L) {
typename sequence_generator<sizeof...(A)>::type seq;
std::tuple<typename LuaGetSlot<A>::returnType...> params = LuaParamExtractor<2, A...>::extract(seq, L);
void* ptr = luaL_newudata<T>(L) ; 
callConstructor(seq, ptr, params);
return 1;
}};

template<class T> struct LuaDestructorWrapper {
static int destructor (lua_State* L) {
UserObjectTrait<T>::destruct( luaL_checkutype(L, 1, UserObjectTrait<T>::typeTag()) );
return 0;
}};

template <class T> struct LuaReferenceEqualler {
static bool equals (T* a, T* b) { return a==b; }
};

template<class UNUSED> struct LuaPropertyWrapper: std::false_type {};

template<class T, class P> struct LuaPropertyWrapper<P T::*> {
typedef P T::*Prop;
static int getter (lua_State* L) {
const Prop& prop = *reinterpret_cast<const Prop*>(lua_topointer(L, lua_upvalueindex(1)));
T* obj = LuaGetSlot<T*>::get(L, 1);
P& value = (obj->*prop);
LuaPushSlot<P>::push(L, value);
return 1;
}
static int setter (lua_State* L) {
const Prop& prop = *reinterpret_cast<const Prop*>(lua_topointer(L, lua_upvalueindex(1)));
T* obj = LuaGetSlot<T*>::get(L, 1);
P value = LuaGetSlot<P>::get(L, -1);
(obj->*prop) = value;
return 1;
}
};

inline std::string computeGetterName (const char* name) {
std::string s;
s += "get";
s += toupper(*name);
s += (name+1);
return s;
}

inline std::string computeSetterName (const char* name) {
std::string s;
s += "set";
s += toupper(*name);
s += (name+1);
return s;
}

inline std::string computeBoolGetterName (const char* name) {
std::string s;
s += "is";
s += toupper(*name);
s += (name+1);
return s;
}

inline void pushNamedParamTable (lua_State* L, const std::vector<std::string>& params) {
lua_newtable(L);
for (int i=0, n=params.size(); i<n; i++) {
auto& param = params[i];
if (!param.empty()) {
lua_pushstring(L, param.c_str());
lua_rawseti(L, -2, i+1);
}}
}

template<class T> struct LuaClass {
private:
lua_State* L;
bool poped;

void loadmetatable () {
if (!lua_getmetatable(L, -1)) {
lua_newtable(L);
lua_pushvalue(L, -1);
lua_setmetatable(L, -3);
}}

public:
LuaClass (lua_State* L0, const char* name = nullptr, bool assoc=true): 
L(L0), poped(false)
{
luaL_newmetatable(L, UserObjectTrait<T>::typeTag() );
lua_pushcclosure(L, luaB_index, 0);
lua_setfield(L, -2, "__index");
lua_pushcclosure(L, luaB_newindex, 0);
lua_setfield(L, -2, "__newindex");
if (name && *name) {
lua_pushstring(L, name);
lua_setfield(L, -2, "__name");
if (assoc) {
lua_pushvalue(L, -1);
lua_setfield(L, -3, name);
}}}

void pop () {
if (!poped) lua_pop(L,1);
poped=true;
}

~LuaClass () {
pop();
}

LuaClass& method (const char* name, const lua_CFunction& func) {
lua_pushcclosure(L, func, 0);
lua_setfield(L, -2, name);
return *this;
}

template <class F> LuaClass& method (const char* name, const F& func) {
LuaPushSlot<F>::push(L, func);
lua_setfield(L, -2, name);
return *this;
}

template<class F> LuaClass& method (const char* name, const F& func, const std::vector<std::string>& namedParams) {
lua_pushfuncptr(L, func);
pushNamedParamTable(L, namedParams);
lua_pushcclosure(L, &LuaPushSlot<F>::wrapper, 2);
lua_setfield(L, -2, name);
return *this;
}

LuaClass& constructor (const lua_CFunction& func) {
loadmetatable();
lua_pushcclosure(L, func, 0);
lua_setfield(L, -2, "__call");
lua_pop(L, 1);
return *this;
}

LuaClass& constructor (const lua_CFunction& func, const std::vector<std::string>& namedParams) {
loadmetatable();
lua_pushnil(L);
pushNamedParamTable(L, namedParams);
lua_pushcclosure(L, func, 2);
lua_setfield(L, -2, "__call");
lua_pop(L, 1);
return *this;
}

template <class... A> inline LuaClass& constructor () {
return constructor(&Binding::LuaConstructorWrapper<T, A...>::constructor);
}

template <class... A> inline LuaClass& constructor (const std::vector<std::string>& namedParams) {
return constructor(&Binding::LuaConstructorWrapper<T, A...>::constructor, namedParams);
}

template<class F> LuaClass& constructor (const F& func) {
loadmetatable();
LuaPushSlot<F, void, 2>::push(L, func);
lua_setfield(L, -2, "__call");
lua_pop(L, 1);
return *this;
}

template <class F> LuaClass& constructor (const F& func, const std::vector<std::string>& namedParams) {
loadmetatable();
lua_pushfuncptr(L, func);
pushNamedParamTable(L, namedParams);
lua_pushcclosure(L, &LuaPushSlot<F, void, 2>::wrapper, 2);
lua_setfield(L, -2, "__call");
lua_pop(L, 1);
return *this;
}

inline LuaClass& destructor (const char* name = "__gc") {
return method(name, &Binding::LuaDestructorWrapper<T>::destructor);
}

inline LuaClass& referenceEquals (const char* name = "__eq") {
return method(name, &Binding::LuaReferenceEqualler<T>::equals);
}

LuaClass& getter (const char* name, const lua_CFunction& func) {
lua_pushcclosure(L, func, 0);
lua_setfield(L, -2, computeGetterName(name).c_str());
return *this;
}

template<class F> LuaClass& getter (const char* name, const F& func) {
LuaPushSlot<F>::push(L, func);
lua_setfield(L, -2, computeGetterName(name).c_str());
return *this;
}

template<class P> LuaClass& propertyGetter (const char* name, P T::*prop) {
lua_pushmemptr(L, prop);
lua_pushcclosure(L, &LuaPropertyWrapper<P T::*>::getter, 1);
lua_setfield(L, -2, computeGetterName(name).c_str());
return *this;
}

LuaClass& boolGetter (const char* name, const lua_CFunction& func) {
lua_pushcclosure(L, func, 0);
lua_setfield(L, -2, computeBoolGetterName(name).c_str());
return *this;
}

template<class F> LuaClass& boolGetter (const char* name, const F& func) {
LuaPushSlot<F>::push(L, func);
lua_setfield(L, -2, computeBoolGetterName(name).c_str());
return *this;
}

LuaClass& boolPropertyGetter (const char* name, bool  T::*prop) {
lua_pushmemptr(L, prop);
lua_pushcclosure(L, &LuaPropertyWrapper<bool T::*>::getter, 1);
lua_setfield(L, -2, computeBoolGetterName(name).c_str());
return *this;
}

LuaClass& setter (const char* name, const lua_CFunction& func) {
lua_pushcclosure(L, func, 0);
lua_setfield(L, -2, computeSetterName(name).c_str());
return *this;
}

template<class F> LuaClass& setter (const char* name, const F& func) {
LuaPushSlot<F>::push(L, func);
lua_setfield(L, -2, computeSetterName(name).c_str());
return *this;
}

template<class P> LuaClass& propertySetter (const char* name, P T::*prop) {
lua_pushmemptr(L, prop);
lua_pushcclosure(L, &LuaPropertyWrapper<P T::*>::setter, 1);
lua_setfield(L, -2, computeSetterName(name).c_str());
return *this;
}

LuaClass& property (const char* name, const lua_CFunction& g, const lua_CFunction& s) {
return getter(name, g) .setter(name, s);
}

template<class G, class S> LuaClass& property (const char* name, const G& g, const S& s) {
return getter(name, g) .setter(name, s);
}

template <class P> LuaClass& property (const char* name, P T::*prop) {
return propertyGetter(name, prop) .propertySetter(name, prop);
}

LuaClass& boolProperty (const char* name, const lua_CFunction& g, const lua_CFunction& s) {
return boolGetter(name, g) .setter(name, s);
}

template<class G, class S> LuaClass& boolProperty (const char* name, const G& g, const S& s) {
return boolGetter(name, g) .setter(name, s);
}

LuaClass& boolProperty (const char* name, bool T::*prop) {
return boolPropertyGetter(name, prop) .propertySetter(name, prop);
}

template <class S> LuaClass& parent () {
loadmetatable();
luaL_newmetatable(L, UserObjectTrait<S>::typeTag());
lua_setfield(L, -2, "__index");
lua_pop(L, 1);
return *this;
}

};//end LuaClass

} // namespace Binding

template<class T> inline T lua_get (lua_State* L, int idx) {
return Binding::LuaGetSlot<T>::get(L, idx);
}

template<class T> inline int lua_push (lua_State* L, const T& value) {
return Binding::LuaPushSlot<T>::push(L, value);
}

template<class T> inline void lua_pushfield (lua_State* L, const char* name, const T& value) {
Binding::LuaPushSlot<T>::push(L, value);
lua_setfield(L, -2, name);
}

template<class T> inline void lua_pushglobal  (lua_State* L, const char* name, const T& value) {
Binding::LuaPushSlot<T>::push(L, value);
lua_setglobal(L, name);
}

template<class F> inline void lua_pushfield (lua_State* L, const char* name, const F& f, const std::vector<std::string>& a) {
Binding::lua_pushfuncptr(L, f); 
Binding::pushNamedParamTable(L, a); 
lua_pushcclosure(L, &Binding::LuaPushSlot<decltype(f)>::wrapper, 2); 
lua_setfield(L, -2, name);
}

template<class F> inline void lua_pushglobal (lua_State* L, const char* name, const F& f, const std::vector<std::string>& a) {
Binding::lua_pushfuncptr(L, f); 
Binding::pushNamedParamTable(L, a); 
lua_pushcclosure(L, &Binding::LuaPushSlot<decltype(f)>::wrapper, 2); 
lua_setglobal(L, name);
}

template<class T> inline T* luaL_checkutype (lua_State* L, int idx) {
void* ptr = luaL_checkutype(L, idx, Binding::UserObjectTrait<T>::typeTag());
return ptr? Binding::UserObjectTrait<T>::getPointer(ptr) : nullptr;
}

template<class T> inline T* luaL_testutype (lua_State* L, int idx) {
void* ptr = luaL_testutype(L, idx, Binding::UserObjectTrait<T>::typeTag());
return ptr? Binding::UserObjectTrait<T>::getPointer(ptr) : nullptr;
}

#endif
