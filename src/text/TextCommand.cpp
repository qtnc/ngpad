#include "TextCommand.hpp"

TextDeleted::TextDeleted  (wxTextEntryBase* editor, int start0, int end0, const wxString& text0, int select0):
TextCommand(editor, MSG("DeleteUndoLbl")),
text(text0), start(start0), end(end0), select(select0)
{}

bool TextDeleted::Do () {
editor->Replace(start, end, wxEmptyString);
return true;
}

bool TextDeleted::Undo () {
editor->Replace(start, start, text);
switch(select){
case TC_POS_START: editor->SetInsertionPoint(start); break;
case TC_POS_END: editor->SetInsertionPoint(end); break;
case TC_SELECT_FORWARD: editor->SetSelection(start, end); break;
case TC_SELECT_BACKWARD: editor->SetSelection(end, start); break;
}
return true;
}

bool TextDeleted::Join (wxCommand* cmd) {
auto td = dynamic_cast<TextDeleted*>(cmd);
if (!td) return false;
if (td->end==start) {
text = td->text + text;
start = td->start;
return true;
}
else if (start==td->start) {
text += td->text;
end += (td->end - td->start);
return true;
}
return false;
}

TextReplaced::TextReplaced  (wxTextEntryBase* editor, int start0, int end0, const wxString& oldText0, const wxString& newText0, int select0):
TextCommand(editor, MSG("ReplaceUndoLbl")),
newText(newText0), oldText(oldText0), start(start0), end(end0), select(select0) 
{}

bool TextReplaced::Do () {
editor->Replace(start, start+oldText.size(), newText);
switch(select){
case TC_SELECT_FORWARD: editor->SetSelection(start, start+newText.size()); break;
case TC_SELECT_BACKWARD: editor->SetSelection(start+newText.size(), start); break;
case TC_POS_END: editor->SetInsertionPoint(start+newText.size()); break;
case TC_POS_START: editor->SetInsertionPoint(start); break;
}
return true;
}

bool TextReplaced::Undo () {
editor->Replace(start, start+newText.size(), oldText);
switch(select){
case TC_SELECT_FORWARD: editor->SetSelection(start, end); break;
case TC_SELECT_BACKWARD: editor->SetSelection(end, start); break;
case TC_POS_START: editor->SetInsertionPoint(start); break;
case TC_POS_END: editor->SetInsertionPoint(end); break;
}
return true;
}

bool TextReplaced::Join (wxCommand* cmd) {
return false;
}

TextInserted::TextInserted (wxTextEntryBase* editor, size_t pos0, const wxString& text0, bool select0):
TextCommand(editor, MSG("InsertUndoLbl")),
text(text0), pos(pos0), select(select0)
{}

bool TextInserted::Do () {
editor->Replace(pos, pos, text);
if (select) editor->SetSelection(pos, pos+text.size());
else editor->SetInsertionPoint(pos + text.size());
return true;
}

bool TextInserted::Undo () {
editor->Replace(pos, pos+text.size(), wxEmptyString);
return true;
}

bool TextInserted::Join (wxCommand* cmd) {
auto ti = dynamic_cast<TextInserted*>(cmd);
if (ti && ti->pos == pos+text.size()) {
text += ti->text;
return true;
}
return false;
}
