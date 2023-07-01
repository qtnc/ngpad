#include "base.hpp"
#include "../../text/TextEditor.hpp"

LuaRegisterReferenceType(wxTextCtrlIface)
LuaRegisterDynamicTypeAlias(wxTextAreaBase, wxTextCtrlIface);
LuaRegisterDynamicTypeAlias(wxTextEntryBase, wxTextCtrlIface);


static std::pair<long,long> TextCtrlGetSelection (wxTextCtrlIface& ctl) {
long start, end;
ctl.GetSelection(&start, &end);
return std::make_pair(start+1, end+1);
}

static std::pair<long,long> TextCtrlPositionToXY (wxTextCtrlIface& ctl, long pos) {
if (pos==0) pos = ctl.GetInsertionPoint();
long x=0, y=0;
ctl.PositionToXY(pos, &x, &y);
return std::make_pair(x+1, y+1);
}

static inline void AdjustLineColumn (wxTextCtrlIface& te, long& line, long& col) {
if (line==0) te.PositionToXY(te.GetInsertionPoint(), nullptr, &line);
else if (line<0) line += te.GetNumberOfLines();
else line--;
if (col==0) te.PositionToXY(te.GetInsertionPoint(), &col, nullptr);
else if (col<0) col += te.GetLineLength(line);
else col--;
}

static int TextCtrlXYToPosition (wxTextCtrlIface& te, long x, long y) {
AdjustLineColumn(te, y, x);
return te.XYToPosition(x, y) +1;
}

static int TextCtrlGetLineLength (wxTextCtrlIface& te, long line) {
long unused = 1;
AdjustLineColumn(te, line, unused);
return te.GetLineLength(line);
}

static wxString TextCtrlGetLineText (wxTextCtrlIface& te, long line) {
long unused = 1;
AdjustLineColumn(te, line, unused);
return te.GetLineText(line);
}

static inline void AdjustSelection (wxTextCtrlIface& te, long& start, long& end) {
if (start==0) te.GetSelection(&start, nullptr);
else if (start<0) start += te.GetLastPosition();
else start--;
if (end==0) te.GetSelection(nullptr, &end);
else if (end<0) end += te.GetLastPosition();
else end--;
}

static wxString TextCtrlGetRange (wxTextCtrlIface& te, long start, long end) {
AdjustSelection(te, start, end);
return te.GetRange(start, end);
}

static void TextCtrlSetSelection (wxTextCtrlIface& te, long start, long end) {
AdjustSelection(te, start, end);
te.SetSelection(start, end);
}

static long TextCtrlGetInsertionPoint (wxTextCtrlIface& te) {
return te.GetInsertionPoint() +1;
}

static void TextCtrlSetInsertionPoint (wxTextCtrlIface& te, long pos) {
if (pos==0) return;
long unused = 1;
AdjustSelection(te, pos, unused);
te.SetInsertionPoint(pos);
}

static void TextCtrlReplace (wxTextCtrlIface& te, long start, long end, const wxString& text) {
AdjustSelection(te, start, end);
te.Replace(start, end, text);
}

static void TextCtrlRemove (wxTextCtrlIface& te, long start, long end) {
AdjustSelection(te, start, end);
te.Remove(start, end);
}

export int luaopen_TextEditor (lua_State* L) {
lua_pushglobaltable(L);
//T Type representing a text edition zone. 
Binding::LuaClass<wxTextCtrlIface>(L, "TextEditor")
.parent<wxEvtHandler>()
.referenceEquals()

//A boolean: tells if the zone is editable
.boolProperty("editable", &wxTextCtrlIface::IsEditable, &wxTextCtrlIface::SetEditable)
//G boolean: tells if the zone is empty
.boolGetter("empty", &wxTextCtrlIface::IsEmpty)
//M Gets the total number of lines of the zone
//R integer: number of lines of the zone
.method("getNumberOfLines", &wxTextCtrlIface::GetNumberOfLines)
//M Get the length of a line
//P line: integer: 0: 1-based line number, 0=current line, negatives counts from the end.
//R integer: length of the given line
.method("getLineLength", &TextCtrlGetLineLength)
//M Get a line of text from the zone
//P line: integer: 0: 1-based line number, 0=current line, negatives counts from the end.
//R string: the text of the given line
.method("getLine", &TextCtrlGetLineText)
//M Checks if some text can be copied right now
//R boolean: true if some text can be copied
.method("canCopy", &wxTextCtrlIface::CanCopy)
//M Checks if some text can be cut right now
//R boolean: true if some text can be cut
.method("canCut", &wxTextCtrlIface::CanCut)
//M Checks if some text can be pasted right now
//R boolean: true if some text can be pasted
.method("canPaste", &wxTextCtrlIface::CanPaste)
//M Checks if the last operation can be undone
//R boolean: true if the last operation can be undone
.method("canUndo", &wxTextCtrlIface::CanUndo)
//M Checks if the last operation can be redone
//R boolean: true if the last operation can be redone
.method("canRedo", &wxTextCtrlIface::CanRedo)
//A boolean: Tells if the text content of the zone has been modified since the last save
.boolProperty("modified", &wxTextCtrlIface::IsModified, &wxTextCtrlIface::SetModified)
//M Append some text at the end of the zone
//P text: string: nil: text to append
.method("appendText", &wxTextCtrlIface::AppendText)
//M Write some text at the current insertion point
//P text: string: nil: text to write
.method("writeText", &wxTextCtrlIface::WriteText)
//M Convert a Column/Line based position into a character position
//P x: integer: nil: X coordinate / column (1-based)
//P y: integer: nil: Y coordinate / Line (1-based, 0=current line, negatives counts from the end.
//R integer: 1-based charachter position
.method("columnLineToPosition", &TextCtrlXYToPosition)
//M Convert a 1-based character position into an X/Y or Column/Line based position
//P position: integer: 0: 1-based character position, 0=current position.
//R integer: X coordinate / Column
//R integer: Y coordinate / Line
.method("positionToColumnLine", &TextCtrlPositionToXY)
//G integer: position corresponding to the end of the zone
.getter("lastPosition", &wxTextCtrlIface::GetLastPosition)
//M Select the entire zone
.method("selectAll", &wxTextCtrlIface::SelectAll)
//M Deselect everything
.method("selectNone", &wxTextCtrlIface::SelectNone)
//M Get a range of text from the zone
//P start: integer: nil: start of the range (1-based, negatives counts from the end)
//P end: integer: nil: end of the range (1-based, negatives counts from the end)
//R string: the requested range of text
.method("getRange", &TextCtrlGetRange)
//G string: currently selected text
.getter("selectedText", &wxTextCtrlIface::GetStringSelection)
//M Get the current position of the selection anchor and end points (1-based)
//R integer: anchor point of the current selection
//R integer: end point of the current selection
.method("getSelection", &TextCtrlGetSelection)
//M Change the selection anchor and end points
//P start: integer: nil: selection start / anchor point (1-based, negatives counts from the end)
//P end: integer: nil: selection end point (1-based, negatives counts from the end)
.method("setSelection", &TextCtrlSetSelection)
//A integer: current position of the insertion point (1-based)
.property("insertionPoint", &TextCtrlGetInsertionPoint, &TextCtrlSetInsertionPoint)
//A string: entire text of the zone
.property("value", &wxTextCtrlIface::GetValue, &wxTextCtrlIface::SetValue)
//M Change the content of the zone without triggering events
//P text: string: nil: new text
.method("changeValue", &wxTextCtrlIface::ChangeValue)
//M Remove some text from the zone
//P start: integer: nil: position of the first character to remove (1-based, negatives counts from the end)
//P end: integer: nil: position of the last character to remove (1-based, negatives counts from the end)
.method("remove", &TextCtrlRemove)
//M Remove some text from the zone and replace it by something else
//P start: integer: nil: position of the first character to remove (1-based, negatives counts from the end)
//P end: integer: nil: position of the last character to remove (1-based, negatives counts from the end)
//P replacement: string: '': new text to be inserted at the place of the removed region
.method("replace", &TextCtrlReplace)
//M Clear the entire zone
.method("clear", &wxTextCtrlIface::Clear)
.pop();
lua_getglobal(L, "TextEditor");

return 1;
}

