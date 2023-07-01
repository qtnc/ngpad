#include "TextEditor.hpp"
#include "../common/println.hpp"
#include "TextView.hpp"
#include "TextDocument.hpp"
#include "TextCommand.hpp"
#include "TextParser.hpp"
#include "../app/App.hpp"
#include "../common/stringUtils.hpp"
#include <wx/regex.h>


struct TextCtrlTextEditor: TextEditorCtrl<wxTextCtrl> {
TextCtrlTextEditor (TextView* view, wxWindow* parent, int id, const wxString& text, const wxPoint& pos, const wxSize& size, long flags);
wxControl* GetControl () override { return this; }
TextEditor* SetAutoWrap (bool newWrap) override;
wxWindow* GetEditableWindow () override { return wxTextCtrlBase::GetEditableWindow(); }
};

TextCtrlTextEditor::TextCtrlTextEditor (TextView* view0, wxWindow* parent, int id, const wxString& text, const wxPoint& pos, const wxSize& size, long flags) {
view = view0;
wxTextCtrl::Create(parent, id, text, pos, size, flags | wxTE_MULTILINE | wxTE_NOHIDESEL | wxVSCROLL | wxTE_PROCESS_TAB | wxTE_PROCESS_ENTER);
Bind(wxEVT_CHAR_HOOK, &TextEditor::OnCharHook, this);
Bind(wxEVT_CHAR, &TextEditor::OnChar, this);
Bind(wxEVT_TEXT_COPY, [&](auto&e){ TextEditor::OnCopy(e); });
Bind(wxEVT_TEXT_CUT, &TextEditor::OnCut, this);
Bind(wxEVT_TEXT_PASTE, &TextEditor::OnPaste, this);
}

TextEditor* TextCtrlTextEditor::SetAutoWrap (bool newWrap) {
auto doc = static_cast<AbstractDocument*>(view->GetDocument());
auto& props = doc->GetProperties();
auto te = TextEditor::Create(*view, GetParent(), props);
TransferCtrlState(*te);
return te;
}

TextDocument* TextEditor::GetDocument () {
return view? static_cast<TextDocument*>(view->GetDocument()) : nullptr;
}

void TextEditor::MarkModified (bool modified) {
auto doc = GetDocument();
if (doc) doc->Modify(modified);
}

std::pair<long, long> TextEditor::GetSelection () const {
long start, end;
GetSelection(&start, &end);
return std::make_pair(start, end);
}

std::pair<long, long> TextEditor::PositionToXY (long pos) const {
long x, y;
PositionToXY(pos, &x, &y);
return std::make_pair(x, y);
}

long TextEditor::GetLineOfPosition (long pos) const {
auto [x, y] = PositionToXY(pos);
return y;
}

bool TextEditor::HasSelection () const {
auto [start, end] = GetSelection();
return start!=end;
}

static inline int GetSelectionDirection (int start, int end, int anchor) {
if (start==end || start==anchor) return TC_SELECT_FORWARD;
else if (end==anchor) return TC_SELECT_BACKWARD;
else return 0;
}

void TextEditor::TransferCtrlState (TextEditor& te) {
auto [start, end] = GetSelection();
bool modified = IsModified();
te.SetValue(GetValue());
te.SetSelection(start, end);
te.SetModified(modified);
}

int TextEditor::GetIndentType () {
auto doc = GetDocument();
return doc? doc->GetIndentType() :4;
}

wxString TextEditor::GetIndentText () {
auto size = GetIndentType();
if (size<=0) return "\t";
else return wxString(' ', size);
}

TextBlockFinder& TextEditor::GetBlockFinder () {
if (!blockFinder) {
auto doc = GetDocument();
if (doc) blockFinder = std::unique_ptr<TextBlockFinder>( TextBlockFinder::Create(doc->properties.get("block_type", "indentation"), doc->properties) );
}
return *blockFinder;
}

TextMarkerFinder& TextEditor::GetMarkerFinder () {
if (!markerFinder) {
auto doc = GetDocument();
if (doc) markerFinder = std::unique_ptr<TextMarkerFinder>( TextMarkerFinder::Create( doc->properties.get("marker_type", "none"), doc->properties));
}
return *markerFinder;
}

wxString TextEditor::GetWordSeparators () {
auto doc = GetDocument();
return doc? U(doc->properties.get("word_separators", "")) : wxString();
}

bool TextEditor::IsUsingCamelCaseWords () {
auto doc = GetDocument();
return doc && doc->properties.get("camel_case_words", true);
}

void TextEditor::PushUndoState (wxCommand* cmd, bool tryJoin) {
if (auto i = dynamic_cast<TextInserted*>(cmd)) AdjustSecondCursor(i->pos, i->text.size());
else if (auto r = dynamic_cast<TextReplaced*>(cmd)) AdjustSecondCursor(std::min(r->start, r->end), (int)(r->newText.size()) - (int)(r->oldText.size()) );
else if (auto d = dynamic_cast<TextDeleted*>(cmd)) AdjustSecondCursor(std::min(d->start, d->end), - std::max(d->start, d->end) - std::min(d->start, d->end));
auto doc = GetDocument();
if (doc) doc->PushUndoState(cmd, tryJoin);
else delete cmd;
}

void TextEditor::ReplaceAndPushUndoState (long start, long end, const wxString& newText) {
bool backward = end<start;
if (backward) std::swap(start, end);
wxString oldText = GetRange(start, end);
PushUndoState(new TextReplaced(this, start, end, oldText, newText, backward? TC_SELECT_BACKWARD : TC_SELECT_FORWARD), false);
Replace(start, end, newText);
MarkDirty();
}

void TextEditor::OnTextInserted (const wxString& text, bool tryJoin) {
auto [start, end] = GetSelection();
if (start!=end) PushUndoState(new TextDeleted(this, start, end, GetRange(start, end), GetSelectionDirection(start, end, anchor)), false);
PushUndoState(new TextInserted(this, start, text, false), tryJoin);
}

void TextEditor::OnEnter () {
int pos = GetInsertionPoint();
int line = GetLineOfPosition(pos);
wxString text = GetLineText(line);
int absbol = pos-text.size();
int rbol = realBeginningOfLine(text);
int curIndent=0, newIndent=0;
wxString prefix;
auto& bf = GetBlockFinder();
if (&bf && !bf.OnEnter(text, prefix, curIndent, newIndent)) return;
wxCommandEvent e(wxEVT_TEXT_ENTER);
e.SetEventObject(GetControl());
e.SetString(prefix);
e.SetInt(newIndent);
e.SetExtraLong(curIndent);
if (ProcessEvent(e)) return;
newIndent = e.GetInt();
curIndent = e.GetExtraLong();
prefix = e.GetString();
wxString newText = "\n" + text.substr(0, rbol);
if (curIndent) {
wxString prevLine = line>0? GetLineText(line -1) : wxString();
int prevRbol = realBeginningOfLine(prevLine);
if (rbol==prevRbol) {
wxString indentText = GetIndentText();
if (curIndent>0) for (int i=0; i<curIndent; i++) text = indentText + text;
if (curIndent<0) text = text.substr(-curIndent * indentText.size());
SetSelection(absbol, pos);
OnTextInserted(text, false);
Replace(absbol, pos, text);
}
else newIndent=0;
}
if (newIndent) {
wxString indentText = GetIndentText();
if (newIndent<0) newText = newText.substr(0, std::max((size_t)1, newText.size() + newIndent*indentText.size() ));
if (newIndent>0) for (int i=0; i<newIndent; i++) newText += indentText;
}
newText += prefix;
OnTextInserted(newText, false);
WriteText(newText);
 }

void TextEditor::OnCtrlEnter () {
size_t x, y;
PositionToXY(GetInsertionPoint(), reinterpret_cast<long*>(&x), reinterpret_cast<long*>(&y));
wxString text = GetLineText(y);
size_t start=x, end=x;
while (start>0) {
char c = text[start -1];
if (c==' ' || c=='"' || c=='\'' || c=='<' || c=='(' || c=='[' || c=='{' || c==',' || c==';') break;
else start--;
}
while(++end<text.size()) {
char c = text[end];
if (c==' ' || c=='"' || c=='\'' || c=='>' || c==')' || c==']' || c=='}' || c==',' || c==';') break;
else if (c==':' && end<text.size() -1 && text[end+1]==' ') break;
}
wxString cmd = text.substr(start, end-start);
wxGetApp() .DoQuickJump(cmd);
}

void TextEditor::OnHome () {
auto [x, y] = PositionToXY(GetInsertionPoint());
wxString text = GetLineText(y);
int rbol = realBeginningOfLine(text);
SetInsertionPoint(XYToPosition(x==rbol? 0 : rbol, y));
}

void TextEditor::OnShiftEnd () {
auto [start, end] = GetSelection();
auto line = GetLineOfPosition(end);
auto linelen = GetLineLength(line);
end = XYToPosition(linelen, line);
SetSelection(anchor, end);
}

void TextEditor::OnCtrlShiftEnd () {
auto end = GetLastPosition();
SetSelection(anchor, end);
}

void TextEditor::OnTab () {
auto [start, end] = GetSelection();
int sl = GetLineOfPosition(start), el = GetLineOfPosition(end);
int sp = XYToPosition(0, sl), ep = XYToPosition(0, el) + GetLineLength(el);
wxString text = GetRange(sp, ep), oldText = text;
wxString indent = GetIndentText();
wxRegEx("^", wxRE_NEWLINE) .Replace(&text, indent);
Replace(sp, ep, text);
if (start==end) {
PushUndoState(new TextReplaced(this, sp, start, oldText, text, TC_POS_END), false);
SetInsertionPoint(start + indent.size());
} 
else {
PushUndoState(new TextReplaced(this, sp, ep, oldText, text, GetSelectionDirection(start, end, anchor)), false);
SetSelection(sp, sp+text.size());
}
}

void TextEditor::OnShiftTab () {
auto [start, end] = GetSelection();
auto sl = GetLineOfPosition(start);
auto el = GetLineOfPosition(end);
auto sp = XYToPosition(0, sl);
auto ep = XYToPosition(0, el) + GetLineLength(el);
wxString text = GetRange(sp, ep), oldText = text;
wxString indent = GetIndentText();
wxRegEx("^" + indent, wxRE_NEWLINE) .Replace(&text, "");
Replace(sp, ep, text);
if (start==end) {
PushUndoState(new TextReplaced(this, sp, start, oldText, text, TC_POS_END), false);
SetInsertionPoint(std::max(0UL, start-indent.size()));
} 
else {
PushUndoState(new TextReplaced(this, sp, ep, oldText, text, GetSelectionDirection(start, end, anchor)), false);
SetSelection(sp, sp+text.size());
}
}

bool TextEditor::OnBackspace () {
auto [start, end] = GetSelection();
if (start!=end) {
PushUndoState(new TextDeleted(this, start, end, GetRange(start, end), GetSelectionDirection(start, end, anchor)), false);
return false;
}
auto [x, y] = PositionToXY(start);
wxString text = GetLineText(y);
int rbol = realBeginningOfLine(text);
if (x==rbol) {
auto doc = GetDocument();
if (doc && doc->properties.get("unindent_with_backspace", false)) start -= std::max(1, GetIndentType());
else start = --y<0? 0 : XYToPosition(GetLineLength(y), y);
SetSelection(start, end);
}
else if (x<rbol && x>0) {
wxBell();
return true;
}
else {
end = start;
start = std::max(start -1, 0L);
}
PushUndoState(new TextDeleted(this, start, end, GetRange(start, end), TC_POS_START), true);
return false;
}

bool TextEditor::OnDelete () {
auto [start, end] = GetSelection();
if (start!=end) {
PushUndoState(new TextDeleted(this, start, end, GetRange(start, end), GetSelectionDirection(start, end, anchor)), false);
return false;
}
auto [x, y] = PositionToXY(start);
wxString text = GetLineText(y);
int rbol = realBeginningOfLine(text);
if (x<rbol) {
wxBell();
return true;
}
wxString deletedText = GetRange(start, start+1);
wxChar deletedChar = deletedText[0];
if (deletedChar!='\n') {
PushUndoState(new TextDeleted(this, start, start+1, deletedText, TC_POS_END), true);
return false;
}
int line = GetLineOfPosition(start);
wxString nextText = GetLineText(line+1);
int nextRbol = realBeginningOfLine(nextText);
if (nextRbol>0) {
end = XYToPosition(nextRbol, line+1);
SetSelection(start, end);
}
PushUndoState(new TextDeleted(this, start, end, GetRange(start, end), TC_POS_END), true);
return false;
}

bool TextEditor::MoveToNextWord (bool select) {
size_t start=0, end=0, x=0, y=0;
GetSelection(reinterpret_cast<long*>(&start), reinterpret_cast<long*>(&end));
PositionToXY(end, reinterpret_cast<long*>(&x), reinterpret_cast<long*>(&y));
size_t lLen = GetLineLength(y);
if (x>=lLen) {
lLen = GetLineLength(++y);
x=0;
}
auto text = GetLineText(y);
auto wordSeparators = GetWordSeparators();
if (wordSeparators.empty() || wordSeparators=="native") return false;
size_t x1 = text.find_first_not_of(wordSeparators, x);
if (x1!=std::string::npos) x1 = text.find_first_of(wordSeparators, x1);
if (x1==std::string::npos) x1 = lLen;
else if (!select) x1++;
if (IsUsingCamelCaseWords()) {
auto it2 = std::find_if_not(text.begin()+x, text.end(), [](auto c){ return isupper(c); });
if (it2!=text.end()) it2 = std::find_if(it2, text.end(), [](auto c){ return isupper(c); });
size_t x2 = it2-text.begin();
x = std::min(x1, x2);
}
else x=x1;
end = XYToPosition(x, y);
if (select) SetSelection(start, end);
else SetInsertionPoint(end);
return true;
}

bool TextEditor::MoveToPreviousWord (bool select) {
size_t start=0, end=0, x=0, y=0;
GetSelection(reinterpret_cast<long*>(&start), reinterpret_cast<long*>(&end));
PositionToXY(end, reinterpret_cast<long*>(&x), reinterpret_cast<long*>(&y));
if (end<=0) return false;
if (x==0) x = GetLineLength(--y);
auto text = GetLineText(y);
auto wordSeparators = GetWordSeparators();
if (wordSeparators.empty() || wordSeparators=="native") return false;
size_t x1 = text.find_last_not_of(wordSeparators, x -1);
if (x1==std::string::npos) x1=0;
x1 = text.find_last_of(wordSeparators, x1);
if (x1==std::string::npos) x1=0;
else x1++;
if (IsUsingCamelCaseWords()) {
auto it2 = find_last_if_not(text.begin(), text.begin()+x -1, text.end(), [](auto c){ return isupper(c); });
if (it2==text.end()) it2 = text.begin();
it2 = find_last_if(text.begin(), it2, text.end(), [](auto c){ return isupper(c); });
size_t x2 = it2==text.end()? 0 : it2-text.begin();
x = std::max(x1, x2);
}
else x=x1;
end = XYToPosition(x, y);
if (select) SetSelection(start, end);
else SetInsertionPoint(end);
return true;
}

bool TextEditor::MoveToPreviousParagraph (bool select) {
auto [start, end] = GetSelection();
if (end<=0) return false;
int line = GetLineOfPosition(end);
bool found = false;
while(line>=0) {
wxString text = GetLineText(--line);
if (isBlank(text)) {
if (found) break;
else found=true;
}}
end = line<0? 0 : XYToPosition(0, line+1);
if (select) SetSelection(start, end);
else SetInsertionPoint(end);
return true;
}

bool TextEditor::MoveToNextParagraph (bool select) {
auto [start, end] = GetSelection();
if (end>=GetLastPosition()) return false;
int line = GetLineOfPosition(end), nLines = GetNumberOfLines();
while(line<nLines) {
wxString text = GetLineText(line++);
if (isBlank(text)) break;
}
end = line>=nLines? GetLastPosition() : XYToPosition(0, line);
if (select) SetSelection(start, end);
else SetInsertionPoint(end);
return true;
}

bool TextEditor::MoveToPreviousMarker  (const wxString& name, bool select) {
long start, end, x, y;
GetSelection(&start, &end);
if (end<=0) return false;
auto& df = GetMarkerFinder();
if (!&df) return false;

TextMarker* marker = nullptr;
wxString text = GetValue();
PositionToXY(end, &x, &y);
end = xyToPosition(text, x, y);
if (!df.Reset(text) || !(marker = df.FindPreviousMarker(end, name))) return false;
start = marker->start; end = marker->end;
positionToXY(text, start, reinterpret_cast<size_t&>(x), reinterpret_cast<size_t&>(y));
start = XYToPosition(x, y);
positionToXY(text, end, reinterpret_cast<size_t&>(x), reinterpret_cast<size_t&>(y));
end = XYToPosition(x, y);
if (select) SetSelection(start, end);
else SetInsertionPoint(end);
return true;
}

bool TextEditor::MoveToNextMarker (const wxString& name, bool select) {
long start, end, x, y;
GetSelection(&start, &end);
if (end>=GetLastPosition()) return false;
auto& df = GetMarkerFinder();
if (!&df) return false;

TextMarker* marker = nullptr;
wxString text = GetValue();
PositionToXY(end, &x, &y);
end = xyToPosition(text, x, y);
if (!df.Reset(text) || !(marker = df.FindNextMarker(end, name))) return false;
start = marker->start; end = marker->end;
positionToXY(text, start, reinterpret_cast<size_t&>(x), reinterpret_cast<size_t&>(y));
start = XYToPosition(x, y);
positionToXY(text, end, reinterpret_cast<size_t&>(x), reinterpret_cast<size_t&>(y));
end = XYToPosition(x, y);
if (select) SetSelection(start, end);
else SetInsertionPoint(end);
return true;
}

bool TextEditor::MoveToMarker (const wxString& name) {
long start, end, x, y;
GetSelection(&start, &end);
auto& df = GetMarkerFinder();
if (!&df) return false;

TextMarker* marker = nullptr;
wxString text = GetValue();
PositionToXY(end, &x, &y);
end = xyToPosition(text, x, y);
if (!df.Reset(text) || !(marker = df.FindMarker(end, name))) return false;
start = marker->start; end = marker->end;
positionToXY(text, start, reinterpret_cast<size_t&>(x), reinterpret_cast<size_t&>(y));
start = XYToPosition(x, y);
positionToXY(text, end, reinterpret_cast<size_t&>(x), reinterpret_cast<size_t&>(y));
end = XYToPosition(x, y);
SetInsertionPoint(end);
return true;
}

bool TextEditor::MoveToOuterIndent (bool select) {
auto [start, end] = GetSelection();
auto line = GetLineOfPosition(end);
if (line<=0) return false;
auto& bf = GetBlockFinder();
bf.Reset(-1);
wxString text = GetLineText(line);
int curLevel = bf.GetBlockLevel(text), level = curLevel;
while(--line>=0) {
text = GetLineText(line);
level = bf.GetBlockLevel(text);
if (level<curLevel && !bf.IsLikeBlankLine(text)) break;
}
if (select) SetSelection(start, XYToPosition(0, line));
else SetInsertionPoint(XYToPosition(realBeginningOfLine(text), line));
return true;
}

bool TextEditor::MoveToInnerIndent (bool select) {
auto [start, end] = GetSelection();
auto line = GetLineOfPosition(end);
auto nLines = GetLineOfPosition(GetLastPosition());
if (line>=nLines) return false;
auto& bf = GetBlockFinder();
bf.Reset(1);
wxString text = GetLineText(line);
int level = bf.GetBlockLevel(text);
wxString nextText = GetLineText(line+1);
int nextLevel = bf.GetBlockLevel(nextText);
if (nextLevel<=level) return false;
if (select) SetSelection(start, XYToPosition(nextText.size(), line+1));
else SetInsertionPoint(XYToPosition(realBeginningOfLine(nextText), line+1));
return true;
}


bool TextEditor::MoveToSameIndent (int direction, bool select) {
auto [start, end] = GetSelection();
auto line = GetLineOfPosition(end);
auto nLines = GetLineOfPosition(GetLastPosition());
if (line+direction<0 || line+direction>nLines) return false;
auto& bf = GetBlockFinder();
bf.Reset(direction);
wxString text = GetLineText(line);
int level = bf.GetBlockLevel(text), curLevel=level;
while(line>=0 && line<=nLines) {
line += direction;
text = GetLineText(line);
level = bf.GetBlockLevel(text);
if (level<curLevel) return false;
else if (level==curLevel && !bf.IsLikeBlankLine(text)) break;
}
if (select) SetSelection(start, XYToPosition(direction>0? GetLineLength(line) : 0, line));
else SetInsertionPoint(XYToPosition(realBeginningOfLine(text), line));
return true;
}

bool TextEditor::MoveToIndentEdge (int direction, bool select) {
int count = 0;
while (MoveToSameIndent(direction, select)) count++;
return count>0;
}

void TextEditor::MoveToSecondCursor (bool select) {
auto [start, end] = GetSelection();
if (secondCursor<0) secondCursor = end;
else if (start==end) {
if (select) SetSelection(secondCursor, end);
else SetInsertionPoint(secondCursor);
secondCursor = start;
}
else if (select) secondCursor = -1;
}

void TextEditor::AdjustSecondCursor (int pos, int count) {
if (secondCursor>=pos) secondCursor += count;
}

bool TextEditor::OnCopy (wxClipboardTextEvent& e) {
auto [start, end] = GetSelection();
if (start==end) {
SetClipboardText(GetLineText(GetLineOfPosition(GetInsertionPoint())));
return true;
}
else {
if (end<start) std::swap(start, end);
int sl = GetLineOfPosition(start), el = GetLineOfPosition(end);
if (sl==el) { 
e.Skip(); 
return false;
}
int sp = XYToPosition(0, sl);
wxString text = GetLineText(sl);
auto rbol = realBeginningOfLine(text);
wxString textToCopy = text.substr(0, rbol) + GetRange(std::max<long>(start, sp+rbol), end);
SetClipboardText(textToCopy);
return true;
}}

void TextEditor::OnCut (wxClipboardTextEvent& e) {
auto [start, end] = GetSelection();
if (start==end) {
int line = GetLineOfPosition(start);
start = XYToPosition(0, line);
end = start + GetLineLength(line) + 1;
SetSelection(start, end);
PushUndoState(new TextDeleted(this, start, end, GetRange(start, end), TC_POS_END), false);
}
else {
PushUndoState(new TextDeleted(this, start, end, GetRange(start, end), GetSelectionDirection(start, end, anchor)), false);
if (OnCopy(e)) Replace(start, end, "");
}}

void TextEditor::OnPaste (wxClipboardTextEvent& e) {
wxString text = GetClipboardText();
wxString curLine = GetLineText(GetLineOfPosition(GetInsertionPoint()));
size_t minRbol = 1048575, curRbol = realBeginningOfLine(curLine);
std::vector<wxString> lines;
boost::split(lines, text, boost::is_any_of("\n"), boost::token_compress_off);
for (auto& l: lines) minRbol = std::min(minRbol, realBeginningOfLine(l));
wxString newText;
for (int i=0, n=lines.size(); i<n; i++) {
if (i>0) newText += "\n" + curLine.substr(0, curRbol);
newText += lines[i].substr(minRbol);
}
OnTextInserted(newText, false);
WriteText(newText);
}

void TextEditor::OnChar (wxKeyEvent& e) {
auto ch = e.GetUnicodeKey();
bool editable = IsEditable();
if (ch!=WXK_NONE && !e.HasAnyModifiers() && !editable) {
wxBell();
}
else {
if (editable && (ch!=WXK_NONE && ch!=WXK_BACK)) OnTextInserted(wxString(ch,1), true);
e.Skip();
}}

void TextEditor::OnCharHook (wxKeyEvent& e) {
int key = e.GetKeyCode(), mod = e.GetModifiers();
lastKeyPress = mtime();
switch(key){
case WXK_RETURN:
if (mod==wxMOD_NONE) OnEnter();
else if (mod==wxMOD_CONTROL) OnCtrlEnter();
else e.Skip();
break;
case WXK_TAB:
if (mod==wxMOD_NONE) OnTab();
else if (mod==wxMOD_SHIFT) OnShiftTab();
else e.Skip();
break;
case WXK_BACK:
if (mod!=wxMOD_NONE || !OnBackspace()) e.Skip();
break;
case WXK_DELETE:
if (mod!=wxMOD_NONE || !OnDelete()) e.Skip();
break;
case WXK_HOME:
if (mod==wxMOD_NONE) OnHome();
else if (mod==wxMOD_ALT) BellIfFalse(MoveToIndentEdge(-1, false));
else if (mod==(wxMOD_ALT | wxMOD_SHIFT)) BellIfFalse(MoveToIndentEdge(-1, true));
else e.Skip();
break;
case WXK_END:
if (mod==wxMOD_SHIFT) OnShiftEnd();
else if (mod==(wxMOD_SHIFT | wxMOD_CONTROL)) OnCtrlShiftEnd();
else if (mod==wxMOD_ALT) BellIfFalse(MoveToIndentEdge(1, false));
else if (mod==(wxMOD_ALT | wxMOD_SHIFT)) BellIfFalse(MoveToIndentEdge(1, true));
else e.Skip();
break;
case WXK_PAGEUP:
if (mod==wxMOD_ALT) BellIfFalse(MoveToPreviousMarker(wxEmptyString, false));
else if (mod== (wxMOD_ALT | wxMOD_SHIFT)) BellIfFalse(MoveToPreviousMarker(wxEmptyString, true));
else e.Skip();
break;
case WXK_PAGEDOWN:
if (mod==wxMOD_ALT) BellIfFalse(MoveToNextMarker(wxEmptyString, false));
else if (mod== (wxMOD_ALT | wxMOD_SHIFT)) BellIfFalse(MoveToNextMarker(wxEmptyString, true));
else e.Skip();
break;
case WXK_UP:
if (mod==wxMOD_CONTROL) BellIfFalse(MoveToPreviousParagraph(false));
else if (mod== (wxMOD_CONTROL | wxMOD_SHIFT)) BellIfFalse(MoveToPreviousParagraph(true));
else if (mod==wxMOD_ALT) BellIfFalse(MoveToSameIndent(-1, false));
else if (mod==(wxMOD_ALT | wxMOD_SHIFT)) BellIfFalse(MoveToSameIndent(-1, true));
else e.Skip();
break;
case WXK_DOWN:
if (mod==wxMOD_CONTROL) BellIfFalse(MoveToNextParagraph(false));
else if (mod== (wxMOD_CONTROL | wxMOD_SHIFT)) BellIfFalse(MoveToNextParagraph(true));
else if (mod==wxMOD_ALT) BellIfFalse(MoveToSameIndent(1, false));
else if (mod==(wxMOD_ALT | wxMOD_SHIFT)) BellIfFalse(MoveToSameIndent(1, true));
else e.Skip();
break;
case WXK_RIGHT:
if (mod==wxMOD_CONTROL && MoveToNextWord(false)) break;
else if (mod==(wxMOD_CONTROL | wxMOD_SHIFT) && MoveToNextWord(true)) break;
else if (mod==wxMOD_ALT) BellIfFalse(MoveToInnerIndent(false));
else if (mod==(wxMOD_ALT | wxMOD_SHIFT)) BellIfFalse(MoveToInnerIndent(true));
else e.Skip();
break;
case WXK_LEFT:
if (mod==wxMOD_CONTROL && MoveToPreviousWord(false)) break;
else if (mod==(wxMOD_CONTROL | wxMOD_SHIFT) && MoveToPreviousWord(true)) break;
else if (mod==wxMOD_ALT) BellIfFalse(MoveToOuterIndent(false));
else if (mod==(wxMOD_ALT | wxMOD_SHIFT)) BellIfFalse(MoveToOuterIndent(true));
else e.Skip();
break;
case WXK_F2:
if (mod==wxMOD_NONE) BellIfFalse(MoveToNextMarker(wxEmptyString, false));
else if (mod==wxMOD_SHIFT) BellIfFalse(MoveToPreviousMarker(wxEmptyString, false));
else e.Skip();
break;
case WXK_F4:
if (mod==wxMOD_NONE) MoveToSecondCursor(false);
else if (mod==wxMOD_SHIFT) MoveToSecondCursor(true);
else e.Skip();
break;
#ifdef DEBUG
/*case WXK_F1: {
int z = 0;
TextLexer tl;
tl.Reset(GetValue());
while(z++<1000 && tl.Next());
Beep(1200, 120);
}break;
*/
#endif
default:
e.Skip();
break;
}}

void TextEditor::Register (const std::string& name, const TextEditor::Factory& factory) {
factories[name] = factory;
}

static TextEditor* createTextCtrlTextEditor (TextView& view, wxWindow* parent, Properties& props, int flags) {
bool wrap = props.get("line_wrap", false);
flags |= (wrap? wxTE_BESTWRAP : wxHSCROLL | wxTE_DONTWRAP);
auto te = new TextCtrlTextEditor(&view, parent, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, flags);
if ((flags&(wxTE_RICH|wxTE_RICH2)) && props.get("spell_check", false)) {
auto proof = wxTextProofOptions::Default();
proof = proof.Language(props.get("spell_check_language", "fr"));
if (props.get("grammar_check", true)) proof = proof.GrammarCheck();
te->EnableProofCheck(proof);
}
return te;
}

static void initTextEditorFactories () {
TextEditor::Register("raw", [](auto& view, auto parent, auto& props){ return createTextCtrlTextEditor(view, parent, props, 0); });
TextEditor::Register("rich1", [](auto& view, auto parent, auto& props){ return createTextCtrlTextEditor(view, parent, props, wxTE_RICH); });
TextEditor::Register("rich2", [](auto& view, auto parent, auto& props){ return createTextCtrlTextEditor(view, parent, props, wxTE_RICH2); });
}

TextEditor* TextEditor::Create (const std::string& name, TextView& view, wxWindow* parent, Properties& props) {
if (factories.empty()) initTextEditorFactories();
auto it = factories.find(name);
if (it!=factories.end()) return (it->second)(view, parent, props);
else return nullptr;
}

TextEditor* TextEditor::Create (TextView& view, wxWindow* parent, Properties& props) {
std::string name = props.get("editor", "raw");
return Create(name, view, parent, props);
}





