#ifndef _____WXW0_____INCL0
#define _____WXW0_____INCL0
#include <wx/wxprec.h>
#ifndef WX_PRECOMP
 #include <wx/wx.h>
#endif
#include<string>
#include<iosfwd>
#include<sstream>

#define MSG(X) U(GetTranslation(X))

#ifdef __WIN32
#undef CreateFile
#undef MessageBoxEx
#endif

class App;
wxDECLARE_APP(App);

std::string GetTranslation (const std::string& key);

unsigned long long mtime();

void MakeLetterNavigable (wxItemContainerImmutable* ctl);
void MakeCheckTreeCtrl (struct wxTreeCtrl* treeCtrl, const std::function<void(const struct wxTreeItemId&,int)>& onItemCheck = nullptr);


void SetClipboardText (const wxString& s);
wxString GetClipboardText ();

inline std::string U (const wxString& s) {
return s.ToStdString(wxConvUTF8);
}

inline std::string UFN (const wxString& s) {
return s.ToStdString(*wxConvFileName);
}

inline wxString U (const std::string& s) {
return wxString(s.data(), wxConvUTF8, s.size());
}

inline wxString U (const char* s) {
return s? wxString::FromUTF8(s) : wxString(wxEmptyString);
}

template <class T> T* GetParentWindow (wxWindow* win) {
if (!win) return nullptr;
auto tlw = wxDynamicCast(win, T);
if (tlw) return tlw;
else return GetParentWindow<T>(win->GetParent());
}

template <class T> inline void safe_delete (T*& obj) {
if (!obj) return;
T* tmp = obj;
obj = nullptr;
delete tmp;
}

template<class F> struct finally {
F f;
finally (const F& x): f(x) {}
~finally () { f(); }
};

inline bool BellIfFalse (bool b) {
if (!b) wxBell();
return b;
}

#endif
