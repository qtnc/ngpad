#ifndef _____STRING_CLIENT_DATA_HPP
#define _____STRING_CLIENT_DATA_HPP
#include "../common/wxUtils.hpp"
#include <wx/treectrl.h>

template<class T>
class ObjectClientData: public wxClientData {
public:
T value;

ObjectClientData (): value() {}
ObjectClientData (const T& x): value(x) {}
inline T& GetValue () { return value; }
const inline T& GetValue () const { return value; }
inline void SetValue (const T& x) { value=x; }
};

template <class T>
class ObjectTreeItemData: public wxTreeItemData {
public:
T value;

ObjectTreeItemData (): value() {}
ObjectTreeItemData (const T& x): value(x) {}
inline T& GetValue () { return value; }
const inline T& GetValue () const { return value; }
inline void SetValue (const T& x) { value=x; }
};


typedef ObjectClientData<wxString> StringClientData;
typedef ObjectTreeItemData<wxString> StringTreeItemData;

#endif
