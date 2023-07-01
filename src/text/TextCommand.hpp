#ifndef _____TEXT_COMMAND_HPP
#define _____TEXT_COMMAND_HPP
#include "../common/wxUtils.hpp"
#include <wx/cmdproc.h>

#define TC_POS_START -1
#define TC_POS_END -2
#define TC_SELECT_FORWARD -3
#define TC_SELECT_BACKWARD -4

class TextCommand: public wxCommand {
protected:
wxTextEntryBase* editor;
public:
TextCommand (wxTextEntryBase* editor0, const wxString& name): wxCommand(true, name), editor(editor0)  {}
virtual bool Join (wxCommand* cmd) = 0;
};

class TextDeleted: public TextCommand {
private:
int start, end, select;
wxString text;
friend class TextEditor;

public:
TextDeleted  (wxTextEntryBase* editor, int start, int end, const wxString& text, int select);
bool Do () override;
bool Undo () override;
bool Join (wxCommand* cmd) override;
};

class TextReplaced: public TextCommand {
private:
int start, end, select;
wxString newText, oldText;
friend class TextEditor;

public:
TextReplaced  (wxTextEntryBase* editor, int start, int end, const wxString& oldText, const wxString& newText, int select = TC_SELECT_FORWARD);
bool Do () override;
bool Undo () override;
bool Join (wxCommand* cmd) override;
};

class TextInserted: public TextCommand {
private:
size_t pos;
bool select;
wxString text;
friend class TextEditor;

public:
TextInserted  (wxTextEntryBase* editor, size_t pos, const wxString& text, bool select);
bool Do () override;
bool Undo () override;
bool Join (wxCommand* cmd) override;
};

#endif
