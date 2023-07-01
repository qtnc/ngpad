#ifndef _____TEXT_EDITOR_HPP
#define _____TEXT_EDITOR_HPP
#include "TextBlockFinder.hpp"
#include "TextMarkerFinder.hpp"
#include "../common/wxUtils.hpp"
#include "wx/docview.h"
#include<memory>

#define TEM_RAW 0
#define TEM_RICH1 1
#define TEM_RICH2 2

class TextEditor: public wxTextCtrlIface  {
protected:
struct TextView* view;
mutable int anchor = 0;
mutable int secondCursor = -1;
unsigned long long lastKeyPress = 0;
std::unique_ptr<TextBlockFinder> blockFinder;
std::unique_ptr<TextMarkerFinder> markerFinder;

public:
TextEditor () = default;

virtual wxControl* GetControl () = 0;
virtual bool ProcessEvent (wxEvent& e) = 0;

inline unsigned long long GetLastKeyPress () { return lastKeyPress; }
void MarkModified (bool modified);

using wxTextCtrlIface::PositionToXY, wxTextCtrlIface::GetSelection;
std::pair<long, long> PositionToXY (long pos) const;
std::pair<long, long> GetSelection () const;
bool HasSelection ()  const;
long GetLineOfPosition (long pos) const;

int GetIndentType ();
wxString GetIndentText ();
wxString GetWordSeparators ();
bool IsUsingCamelCaseWords ();
virtual TextEditor* SetAutoWrap (bool wrap) = 0;
struct TextDocument* GetDocument ();
TextBlockFinder& GetBlockFinder ();
TextMarkerFinder& GetMarkerFinder ();

void PushUndoState (struct wxCommand* cmd, bool tryJoin);
void ReplaceAndPushUndoState (long start, long end, const wxString& text);
int DoReplace (struct wxRegEx& reg, const wxString& repl);
void TransferCtrlState (TextEditor& te);

void OnChar  (wxKeyEvent& e);
void OnCharHook (wxKeyEvent& e);
bool OnCopy (wxClipboardTextEvent& e);
void OnCut (wxClipboardTextEvent& e);
void OnPaste (wxClipboardTextEvent& e);
void OnEnter ();
void OnCtrlEnter ();
void OnTab ();
void OnShiftTab ();
void OnHome ();
void OnShiftEnd ();
void OnCtrlShiftEnd ();
bool OnBackspace ();
bool OnDelete ();

bool MoveToNextWord (bool select=false);
bool MoveToPreviousWord (bool select=false);
bool MoveToNextParagraph (bool select = false);
bool MoveToPreviousParagraph (bool select = false);
bool MoveToNextMarker (const wxString& name = wxEmptyString, bool select = false);
bool MoveToPreviousMarker (const wxString& name = wxEmptyString, bool select = false);
bool MoveToMarker (const wxString& name);
bool MoveToOuterIndent (bool select=false);
bool MoveToInnerIndent (bool select=false);
bool MoveToSameIndent (int direction=1, bool select=false);
bool MoveToIndentEdge (int direction=1, bool select=false);
void MoveToSecondCursor (bool select = false);
void AdjustSecondCursor (int pos, int count);
void OnTextInserted (const wxString& text, bool tryJoin);

typedef std::function< TextEditor* (struct TextView&, wxWindow*, struct Properties&) > Factory;
static TextEditor* Create (const std::string& name, struct TextView& view, wxWindow* parent, struct Properties& props);
static TextEditor* Create (struct TextView& view, wxWindow* parent, struct Properties& props);
static void Register (const std::string& name, const Factory& factory);
protected: static inline std::unordered_map<std::string, Factory> factories;
};

template <class Parent>
class TextEditorCtrl: public Parent, public TextEditor {
public:
#define V0(N) void N () override { Parent::N(); }
#define C0(R,N) R N () const override { return Parent::N(); }
#define M1(R,N,A) R N (A a) override { return Parent::N(a); }
#define C1(R,N,A) R N (A a) const override { return Parent::N(a); }
#define M2(R,N,A,B) R N (A a, B b) override { return Parent::N(a,b); }
#define C2(R,N,A,B) R N (A a, B b) const override { return Parent::N(a,b); }
#define M3(R,N,A,B,C) R N (A a, B b, C c) override { return Parent::N(a,b,c); }
#define C3(R,N,A,B,C) R N (A a, B b, C c) const override { return Parent::N(a,b,c); }
V0(Copy) V0(Cut) V0(Paste) V0(Undo) V0(Redo)
C0(bool, CanUndo) C0(bool, CanRedo)
C0(long, GetInsertionPoint) M1(void, SetInsertionPoint, long) 
C1(int, GetLineLength, long) C1(wxString, GetLineText, long)
C3(bool, PositionToXY, long, long*, long*) C2(long, XYToPosition, long, long) C0(long, GetLastPosition) M1(void, ShowPosition, long)
C0(bool, IsModified) C0(bool, IsEditable) M1(void, SetEditable, bool)
M1(bool, ProcessEvent, wxEvent&) C0(wxString, DoGetValue) 
M1(void, WriteText, const wxString&)  M2(void, Remove, long, long)
M2(bool, GetStyle, long, wxTextAttr&) M1(bool, SetDefaultStyle, const wxTextAttr&) M3(bool, SetStyle, long, long, const wxTextAttr&)

TextEditorCtrl () {}
void SetSelection (long x, long y) override { 
Parent::SetSelection(x, y); 
anchor = x; 
}
void GetSelection (long* from, long* to) const override {
long lStart, lEnd, &start = from? *from : lStart, &end = to? *to : lEnd;
Parent::GetSelection(&start, &end);
if (start==end) anchor=start;
else if (end==anchor) std::swap(start, end);
}
int GetNumberOfLines () const override {
return 1 + GetLineOfPosition(GetLastPosition());
}
void SelectAll () override {
Parent::SelectAll();
anchor=0;
}
void DiscardEdits () override {
Parent::DiscardEdits();
MarkModified(false);
}
void MarkDirty () override {
Parent::MarkDirty();
MarkModified(true);
}


#undef V0
#undef C0
#undef C1
#undef C2
#undef C3
#undef M1
#undef M2
#undef M3
};

#endif
